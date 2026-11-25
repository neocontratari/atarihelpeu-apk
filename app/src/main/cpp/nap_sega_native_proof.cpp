#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <deque>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <csignal>
#include <csetjmp>
#include <atomic>
#include <chrono>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <cerrno>
#include <thread>
#include <malloc.h> // BUILD2RW: passive native heap audit only (mallinfo)
#include <memory>   // B362: kruh stavu pro opravu online odhadu

#if NAP_SEGA_VENDOR_CORE_PRESENT
#include "clownmdemu.h"
#endif
// B305: Sega na TV / PC cela v C++ (jako Atari 130XE v B303)
#include "tvproud/nap_tv_proud.h"
#include "tvproud/nap_tv_proud_index.h"
#include "nap_ovladac.h"   // B320: herni ovladac (Retroid Pocket 6, gamepad)
#include "online/nap_online_klient.h"   // B357: online hrani pres engine N&P
#include <zlib.h>                       // B360: zabaleny stav jadra pro synchronizaci online hry

#define NAP_LOG_TAG "NAP_SEGA_NATIVE_PROOF"
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, NAP_LOG_TAG, __VA_ARGS__)

static std::atomic<bool> g_input[8] = {false,false,false,false,false,false,false,false};   // B357: atomic - pise Java (dotyk), cte vlakno emulace (ThreadSanitizer)
// B320: herni ovladac (Retroid Pocket 6, Bluetooth / USB) - Java ho jen pozna; co tlacitko udela,
// je v nap_ovladac.h: leve = A, spodni = B, prave = C (A B C v rade zleva, jako Genesis Plus GX
// v RetroArchu), START; krizek a leva packa = smer. Bity jako g_input (0 U, 1 D, 2 L, 3 R,
// 4 A, 5 B, 6 C, 7 START) - jadro dostane dotyk NEBO ovladac (nap_vstup).
static std::mutex g_ovl_m;
static nap::ovladac::Stav g_ovl_stav;              // pod g_ovl_m (vlakno UI Javy)
static std::atomic<uint32_t> g_ovl_bity{0};        // bity 0-7 jako g_input, B355: 8-11 X Y Z MODE
// B354: druhy herni ovladac = ovladac 2 Segy (hrac 2, player_id 1); dotyk hraje jen za hrace 1
static nap::ovladac::Stav g_ovl_stav2;             // pod g_ovl_m
static std::atomic<uint32_t> g_ovl_bity2{0};
static std::atomic<int> g_ovl_udalosti{0};
// B355: mapa tlacitek (TLACITKA OVLADACE v appce, pod g_ovl_m); bity 8-11 = X Y Z MODE ovladace se 6 tlacitky
static nap::ovladac::Mapa g_ovl_mapa = nap::ovladac::vychoziMapa(nap::ovladac::J_SEGA);
static inline bool nap_vstup(int i) {
    return g_input[i] || ((g_ovl_bity.load(std::memory_order_relaxed) >> i) & 1u);
}
static uint32_t g_lastRomHash = 0;
static uint16_t g_lastChecksumStored = 0;
static uint16_t g_lastChecksumCalc = 0;
static std::string g_lastTitle = "";

// BUILD2RV: balanced audio-clock path built on the first clean zeroed FM/PSG frontend.
// BUILD2QM FM-only was cleaner but Sonic jump/ring PSG effects were missing and delay stayed ~1s.
// BUILD2QL full FM+PSG+PCM mixer lowered delay but made Nox audio crackle badly.
// This stage removes the WebView Java wrapper path and trims FIFO/AudioTrack latency while keeping attenuated FM+PSG and PCM/CDDA diagnostic only.
static ClownMDEmu_Region g_real_cfg_region = CLOWNMDEMU_REGION_OVERSEAS;
static ClownMDEmu_TVStandard g_real_cfg_tv = CLOWNMDEMU_TV_STANDARD_NTSC;
static std::string g_real_cfg_region_label = "DEFAULT_US_NTSC";
static std::mutex g_audio_mutex;
// BUILD2RV: balanced audio-clock frontend for ClownMDEmu.
// Keep the first proven fix: zero FM/PSG scratch buffers before ClownMDEmu += mixing.
// Do not use the RP tiny FIFO hard-drop path; use medium FIFO and audio-clock backpressure.
// Core low-pass stays ON; no fake rumble filter and no stale-buffer bass/gong artefact.
static std::deque<jshort> g_audio_fm_l_fifo;
static std::deque<jshort> g_audio_fm_r_fifo;
static std::deque<jshort> g_audio_psg_fifo;
static double g_audio_fm_phase = 0.0;
static double g_audio_psg_phase = 0.0;
static int64_t g_audio_fm_sum_l = 0;
static int64_t g_audio_fm_sum_r = 0;
static int64_t g_audio_psg_sum = 0;
static int g_audio_fm_sum_count = 0;
static int g_audio_psg_sum_count = 0;
static double g_audio_pcm_acc = 0.0;
static uint64_t g_audio_fm_pushed = 0;
static uint64_t g_audio_psg_pushed = 0;
static uint64_t g_audio_pcm_seen = 0;
static uint64_t g_audio_pull_count = 0;
static uint64_t g_audio_drop_count = 0;
static uint64_t g_audio_desync_drop_count = 0;
static uint64_t g_audio_underrun_count = 0;
static int32_t g_audio_rumble_l = 0;
static int32_t g_audio_rumble_r = 0;
static int g_audio_startup_mute_remaining = 0;
static uint64_t g_audio_trim_events = 0;
static uint64_t g_audio_clock_sleep_events = 0;
static std::atomic<int> g_worker_priority_nice_result{999};
static const int NAP_AUDIO_OUT_RATE = 48000;
// ===================================================================
//  B305: SEGA NA TV / PC CELA V C++ (jako Atari 130XE v B303)
//  Rene: "udelas ten tv prenos u segi a ps1 na tv a pc jako si udelal u
//  atari emu c++, aby to bylo taky v c++ a tak super jak v emu atari c++?"
//  Snimek si prenos bere hned, jak ho jadro dokresli (konec posledniho
//  radku), zvuk primo z toho, co jde do reproduktoru. Kazdy prohlizec
//  (/sega.stream) ma vlastni vlakno, ktere ceka v proudDalsiNative a dostane
//  paket: obraz bez ztraty (paleta + LZ, nap_tv_proud_index.h) + zvuk.
//  Java jen posila bajty. Zamerne "new" bez delete - vlakna prohlizecu
//  nesmi pri ukonceni procesu narazit na zniceny objekt.
// ===================================================================
static nap::proud::Hub<nap::proud::KodekIndex> &g_segaProud =
        *new nap::proud::Hub<nap::proud::KodekIndex>("SEGA", NAP_AUDIO_OUT_RATE);
// ===================================================================
//  B306: ZVUK PRO TV / PC HNED Z JADRA, SNIMEK AZ PO KROKU JADRA
//  V B305 se zvuk pro TV bral az v pullAudioStereo, tedy az PROJDE frontou
//  pro reproduktor telefonu. Ta na S8 drzi ~6000 vzorku (~130 ms), aby
//  reproduktor nikdy nehladovel - na TV tak zvuk zaostaval za obrazem o celou
//  tu frontu. Ted se FM a PSG pro TV smichaji hned, jak je jadro vyrobi
//  (stejne zesileni a omezovac jako pro reproduktor), a snimek jde do prenosu
//  az po celem kroku jadra (FM / PSG se dopocitavaji na konci kroku) - v paketu
//  je tak obraz i zvuk tehoz okamziku hry. Kdyz se nikdo nediva, nic se nedeje.
// ===================================================================
static std::vector<jshort> g_tv_fm_l, g_tv_fm_r, g_tv_psg;   // jen pod g_audio_mutex
static std::atomic<bool> g_tv_snimek_hotov{false};           // posledni radek snimku je venku
static std::atomic<int> g_render_display_current{1};         // B306: tento snimek i na displej telefonu
static const int NAP_AUDIO_STARTUP_MUTE_FRAMES = 0;
static std::atomic<int> g_native_perf_low{0};
// BUILD2RV: Android 9/S8 needs a no-starve audio reservoir and less native video capture work.
// Nox/A12 keeps the RQ balanced profile because it was clean there.
static std::atomic<int> g_android_sdk{0};
static std::atomic<int> g_android_cores{0};
static std::atomic<int> g_mobile_no_starve{0};
static std::atomic<uint64_t> g_audio_starve_events{0};
static std::atomic<uint64_t> g_audio_catchup_iterations{0};
static inline bool nap_mobile_no_starve() { return g_mobile_no_starve.load() != 0; }
static inline size_t nap_audio_target_fifo() { return nap_mobile_no_starve() ? 6144u : 3072u; }
static inline size_t nap_audio_max_fifo() { return nap_mobile_no_starve() ? 32768u : 16384u; }
static inline size_t nap_audio_desync_limit() { return 0u; }
static inline size_t nap_audio_low_water() { return nap_mobile_no_starve() ? 3072u : 1536u; }
// BUILD2RV values use the core output directly: the real fix is zeroing ClownMDEmu += buffers,
// then using normal final headroom instead of fake bass filters.
static const int NAP_FM_GAIN_PERCENT = 100;
static const int NAP_PSG_GAIN_PERCENT = 100;
static const int NAP_MASTER_GAIN_PERCENT = 90;
// ============================================================================
// BUILD2RW PASSIVE AUDIT ONLY. Nothing below changes emulation, region, FIFO
// sizes, clocks, gain or render. It only measures and reports.
// ============================================================================
// 10s rolling FIFO window (guarded by g_audio_mutex)
static uint64_t g_audit_win_start_ms = 0;
static uint64_t g_audit_win_fifo_min = 0;
static uint64_t g_audit_win_fifo_max = 0;
static uint64_t g_audit_win_fifo_sum = 0;
static uint64_t g_audit_win_fifo_samples = 0;
static uint64_t g_audit_win_underruns_start = 0;
static uint64_t g_audit_win_pull_ns_max = 0;
static uint64_t g_audit_win_pull_ns_sum = 0;
static uint64_t g_audit_win_pull_count = 0;
// last completed 10s window snapshot (guarded by g_audio_mutex)
static uint64_t g_audit_last_fifo_min = 0;
static uint64_t g_audit_last_fifo_max = 0;
static uint64_t g_audit_last_fifo_avg = 0;
static uint64_t g_audit_last_underruns_delta = 0;
static uint64_t g_audit_last_pull_us_max = 0;
static uint64_t g_audit_last_pull_us_avg = 0;
static uint64_t g_audit_last_pull_count = 0;
static uint64_t g_audit_windows_done = 0;
static inline uint64_t nap_audit_now_ms() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void nap_audit_fifo_sample_locked(size_t backlogMin, uint64_t pull_ns) {
    const uint64_t now = nap_audit_now_ms();
    if (g_audit_win_start_ms == 0) {
        g_audit_win_start_ms = now;
        g_audit_win_fifo_min = backlogMin;
        g_audit_win_fifo_max = backlogMin;
        g_audit_win_underruns_start = g_audio_underrun_count;
    }
    if (backlogMin < g_audit_win_fifo_min || g_audit_win_fifo_samples == 0) g_audit_win_fifo_min = backlogMin;
    if (backlogMin > g_audit_win_fifo_max) g_audit_win_fifo_max = backlogMin;
    g_audit_win_fifo_sum += backlogMin;
    g_audit_win_fifo_samples++;
    if (pull_ns > g_audit_win_pull_ns_max) g_audit_win_pull_ns_max = pull_ns;
    g_audit_win_pull_ns_sum += pull_ns;
    g_audit_win_pull_count++;
    if (now - g_audit_win_start_ms >= 10000) {
        g_audit_last_fifo_min = g_audit_win_fifo_min;
        g_audit_last_fifo_max = g_audit_win_fifo_max;
        g_audit_last_fifo_avg = g_audit_win_fifo_samples ? (g_audit_win_fifo_sum / g_audit_win_fifo_samples) : 0;
        g_audit_last_underruns_delta = g_audio_underrun_count - g_audit_win_underruns_start;
        g_audit_last_pull_us_max = g_audit_win_pull_ns_max / 1000u;
        g_audit_last_pull_us_avg = g_audit_win_pull_count ? (g_audit_win_pull_ns_sum / g_audit_win_pull_count / 1000u) : 0;
        g_audit_last_pull_count = g_audit_win_pull_count;
        g_audit_windows_done++;
        g_audit_win_start_ms = now;
        g_audit_win_fifo_sum = 0; g_audit_win_fifo_samples = 0;
        g_audit_win_pull_ns_max = 0; g_audit_win_pull_ns_sum = 0; g_audit_win_pull_count = 0;
        g_audit_win_underruns_start = g_audio_underrun_count;
    }
}
static void nap_audit_reset_locked() {
    g_audit_win_start_ms = 0;
    g_audit_win_fifo_min = g_audit_win_fifo_max = g_audit_win_fifo_sum = 0;
    g_audit_win_fifo_samples = 0;
    g_audit_win_underruns_start = 0;
    g_audit_win_pull_ns_max = g_audit_win_pull_ns_sum = g_audit_win_pull_count = 0;
    g_audit_last_fifo_min = g_audit_last_fifo_max = g_audit_last_fifo_avg = 0;
    g_audit_last_underruns_delta = 0;
    g_audit_last_pull_us_max = g_audit_last_pull_us_avg = g_audit_last_pull_count = 0;
    g_audit_windows_done = 0;
}
// left-edge / stride passive audit (atomics, written from scanline callback)
static std::atomic<int> g_audit_left_boundary{-1};
static std::atomic<int> g_audit_right_boundary{-1};
static std::atomic<int> g_audit_screen_w{0};
static std::atomic<int> g_audit_screen_h{0};
static std::atomic<uint64_t> g_audit_left_nonzero_frames{0};
static std::atomic<uint64_t> g_audit_boundary_change_count{0};
static std::atomic<uint64_t> g_audit_leftcol_black_frames{0};
static std::atomic<uint64_t> g_audit_leftcol_nonblack_frames{0};
static std::atomic<uint32_t> g_audit_midrow_left16_fnv{0};
static std::string nap_audit_status_locked() {
    std::ostringstream out;
    out << "auditRW=PASSIVE_ONLY windowsDone=" << g_audit_windows_done
        << " fifo10sMin=" << g_audit_last_fifo_min
        << " fifo10sMax=" << g_audit_last_fifo_max
        << " fifo10sAvg=" << g_audit_last_fifo_avg
        << " underruns10s=" << g_audit_last_underruns_delta
        << " pull10sUsAvg=" << g_audit_last_pull_us_avg
        << " pull10sUsMax=" << g_audit_last_pull_us_max
        << " pulls10s=" << g_audit_last_pull_count;
    return out.str();
}
static std::string nap_audit_frame_status() {
    std::ostringstream out;
    out << "leftAuditRW leftBoundary=" << g_audit_left_boundary.load()
        << " rightBoundary=" << g_audit_right_boundary.load()
        << " coreScreen=" << g_audit_screen_w.load() << "x" << g_audit_screen_h.load()
        << " leftNonzeroFrames=" << g_audit_left_nonzero_frames.load()
        << " boundaryChanges=" << g_audit_boundary_change_count.load()
        << " leftColBlackFrames=" << g_audit_leftcol_black_frames.load()
        << " leftColNonBlackFrames=" << g_audit_leftcol_nonblack_frames.load()
        << " midRowLeft16Fnv=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
        << g_audit_midrow_left16_fnv.load() << std::dec;
    return out.str();
}
static std::string nap_audit_native_heap_status() {
    // Passive: bionic mallinfo. Values are approximate but their GROWTH over time is what matters on S8.
    std::ostringstream out;
#if defined(__ANDROID__)
    struct mallinfo mi = mallinfo();
    out << "nativeMallocOrdblksKB=" << ((uint64_t)mi.uordblks / 1024u)
        << " nativeMallocArenaKB=" << ((uint64_t)mi.arena / 1024u)
        << " nativeMallocFreeKB=" << ((uint64_t)mi.fordblks / 1024u);
#else
    out << "nativeMalloc=NA";
#endif
    return out.str();
}
static std::string nap_region_clock_status() {
    // Passive summary: what the core was ACTUALLY configured with (not just the label).
    const bool pal = (g_real_cfg_tv == CLOWNMDEMU_TV_STANDARD_PAL);
    std::ostringstream out;
    out << "regionClockRW label=" << g_real_cfg_region_label
        << " tv=" << (pal ? "PAL" : "NTSC")
        << " framePeriodNs=" << (pal ? 20000000LL : 16666667LL)
        << " frameHz=" << (pal ? "50.00" : "59.94")
#if NAP_SEGA_VENDOR_CORE_PRESENT
        << " fmSrcRate=" << (pal ? (double)CLOWNMDEMU_FM_SAMPLE_RATE_PAL : (double)CLOWNMDEMU_FM_SAMPLE_RATE_NTSC)
        << " psgSrcRate=" << (pal ? (double)CLOWNMDEMU_PSG_SAMPLE_RATE_PAL : (double)CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC)
#endif
        << " outRate=" << NAP_AUDIO_OUT_RATE;
    return out.str();
}
static std::atomic<int> g_render_capture_current{1};
static std::atomic<uint64_t> g_render_skipped_frames{0};
static std::atomic<uint64_t> g_render_captured_frames{0};
static std::atomic<uint64_t> g_core_last_iter_ns{0};
static std::atomic<uint64_t> g_core_sum_iter_ns{0};
static std::atomic<uint64_t> g_core_max_iter_ns{0};
static std::atomic<uint32_t> g_core_iter_window_count{0};
static inline void nap_scene_stress_record(uint64_t ns) {
    g_core_last_iter_ns.store(ns);
    g_core_sum_iter_ns.fetch_add(ns);
    g_core_iter_window_count.fetch_add(1);
    uint64_t old = g_core_max_iter_ns.load();
    while (ns > old && !g_core_max_iter_ns.compare_exchange_weak(old, ns)) {}
}
static inline jshort nap_clip16(int32_t v) {
    if (v > 32767) return (jshort)32767;
    if (v < -32768) return (jshort)-32768;
    return (jshort)v;
}
static inline int32_t nap_audio_rumble_block(int32_t x, int32_t &low) {
    // One-pole low estimate around the sub/bass rumble area, then subtract most of it.
    // This is not a volume fader: it removes the false low-frequency "gong/rumble" component
    // while leaving the FM lead/SEGA voice present.
    low += (x - low) >> 6;
    int32_t y = x - (low * 7) / 8;
    if (y > 32767) y = 32767;
    if (y < -32768) y = -32768;
    return y;
}
static inline int32_t nap_master_limiter(int32_t v) {
    v = (v * NAP_MASTER_GAIN_PERCENT) / 100;
    if (v > 28600) return 28600 + (v - 28600) / 12;
    if (v < -28600) return -28600 + (v + 28600) / 12;
    return v;
}
static void nap_audio_trim_one_locked(std::deque<jshort>& q) {
    if (q.size() > nap_audio_max_fifo()) {
        size_t drop = q.size() - nap_audio_max_fifo();
        while (drop-- && !q.empty()) { q.pop_front(); g_audio_drop_count++; }
        g_audio_trim_events++;
    }
}
static void nap_audio_balance_locked() {
    // Keep all streams independent. Do not delete FM music because PSG has fewer samples.
    nap_audio_trim_one_locked(g_audio_fm_l_fifo);
    nap_audio_trim_one_locked(g_audio_fm_r_fifo);
    nap_audio_trim_one_locked(g_audio_psg_fifo);
}
static size_t nap_audio_min_fifo_locked() {
    return std::min(std::min(g_audio_fm_l_fifo.size(), g_audio_fm_r_fifo.size()), g_audio_psg_fifo.size());
}
static size_t nap_audio_max_backlog_locked() {
    return std::max(std::max(g_audio_fm_l_fifo.size(), g_audio_fm_r_fifo.size()), g_audio_psg_fifo.size());
}
static size_t nap_audio_min_fifo() {
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    return nap_audio_min_fifo_locked();
}
static size_t nap_audio_max_backlog() {
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    return nap_audio_max_backlog_locked();
}
static void nap_audio_clock_governor_sleep() {
    // BUILD2RV: RR recovery safe-audit guard.
    // RQ slept when one FIFO was already empty on S8, so AudioTrack starved while video still looked 60fps.
    // Never apply backpressure unless ALL streams are safely above low-water.
    const size_t minq = nap_audio_min_fifo();
    if (minq < nap_audio_low_water()) return;
    size_t backlog = nap_audio_max_backlog();
    const size_t target = nap_audio_target_fifo();
    const size_t guard = nap_mobile_no_starve() ? 2048u : 768u;
    if (backlog <= target + guard) return;
    size_t extra = backlog - target;
    useconds_t us = (useconds_t)std::min<size_t>(nap_mobile_no_starve() ? 1800u : 4000u,
                                                std::max<size_t>(400u, (extra * 1000000u) / (NAP_AUDIO_OUT_RATE * 8u)));
    usleep(us);
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    g_audio_clock_sleep_events++;
}
// B306: FM + PSG pro TV hned po vyrobe (stejne jako v nap_audio_pull_stereo pro
// reproduktor: soucet, omezovac, oriznuti). Mixuje se, co je od obou zdroju;
// kdyz jeden zdroj chybi dele nez 50 ms (hra ho nepouziva), jde ten druhy sam.
static void nap_tv_zvuk_mix_locked(std::vector<int16_t> &ven) {
    ven.clear();
    if (g_segaProud.klientu() <= 0) {
        g_tv_fm_l.clear(); g_tv_fm_r.clear(); g_tv_psg.clear();
        return;
    }
    const size_t BEZ = NAP_AUDIO_OUT_RATE / 20;
    size_t nf = std::min(g_tv_fm_l.size(), g_tv_fm_r.size());
    size_t np = g_tv_psg.size();
    if (nf >= np + BEZ) { g_tv_psg.resize(nf, 0); np = nf; }
    else if (np >= nf + BEZ) { g_tv_fm_l.resize(np, 0); g_tv_fm_r.resize(np, 0); nf = np; }
    const size_t n = std::min(nf, np);
    if (n == 0) return;
    ven.resize(n * 2);
    for (size_t i = 0; i < n; i++) {
        ven[i * 2 + 0] = nap_clip16(nap_master_limiter((int32_t)g_tv_fm_l[i] + (int32_t)g_tv_psg[i]));
        ven[i * 2 + 1] = nap_clip16(nap_master_limiter((int32_t)g_tv_fm_r[i] + (int32_t)g_tv_psg[i]));
    }
    g_tv_fm_l.erase(g_tv_fm_l.begin(), g_tv_fm_l.begin() + (std::ptrdiff_t)n);
    g_tv_fm_r.erase(g_tv_fm_r.begin(), g_tv_fm_r.begin() + (std::ptrdiff_t)n);
    g_tv_psg.erase(g_tv_psg.begin(), g_tv_psg.begin() + (std::ptrdiff_t)n);
}
static void nap_audio_push_fm_batch(const std::vector<jshort>& left, const std::vector<jshort>& right) {
    if (left.empty() && right.empty()) return;
    thread_local std::vector<int16_t> tv;
    {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        const size_t n = std::min(left.size(), right.size());
        g_audio_fm_l_fifo.insert(g_audio_fm_l_fifo.end(), left.begin(), left.begin() + n);
        g_audio_fm_r_fifo.insert(g_audio_fm_r_fifo.end(), right.begin(), right.begin() + n);
        g_audio_fm_pushed += n;
        nap_audio_balance_locked();
        if (g_segaProud.klientu() > 0) {
            g_tv_fm_l.insert(g_tv_fm_l.end(), left.begin(), left.begin() + n);
            g_tv_fm_r.insert(g_tv_fm_r.end(), right.begin(), right.begin() + n);
        }
        nap_tv_zvuk_mix_locked(tv);
    }
    if (!tv.empty()) g_segaProud.zvuk(tv.data(), tv.size());   // B306: na TV hned (mimo zamek zvuku)
}
static void nap_audio_push_psg_batch(const std::vector<jshort>& samples) {
    if (samples.empty()) return;
    thread_local std::vector<int16_t> tv;
    {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        g_audio_psg_fifo.insert(g_audio_psg_fifo.end(), samples.begin(), samples.end());
        g_audio_psg_pushed += samples.size();
        nap_audio_balance_locked();
        if (g_segaProud.klientu() > 0) g_tv_psg.insert(g_tv_psg.end(), samples.begin(), samples.end());
        nap_tv_zvuk_mix_locked(tv);
    }
    if (!tv.empty()) g_segaProud.zvuk(tv.data(), tv.size());
}
static bool nap_audio_pop_locked(std::deque<jshort>& q, jshort &v) {
    if (q.empty()) { v = 0; return false; }
    v = q.front(); q.pop_front(); return true;
}
static int nap_audio_pull_stereo(jshort *out, int stereoFrames) {
    if (!out || stereoFrames <= 0) return 0;
    int got = 0;
    const auto audit_pull_start = std::chrono::steady_clock::now(); // BUILD2RW passive audit
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    nap_audio_balance_locked();
    for (int i = 0; i < stereoFrames; ++i) {
        jshort fl=0, fr=0, p=0;
        bool hfl = nap_audio_pop_locked(g_audio_fm_l_fifo, fl);
        bool hfr = nap_audio_pop_locked(g_audio_fm_r_fifo, fr);
        bool hp  = nap_audio_pop_locked(g_audio_psg_fifo, p);
        int32_t l = 0, r = 0;
        if (hfl || hfr || hp) {
            got++;
            l = (int32_t)fl + (int32_t)p;
            r = (int32_t)fr + (int32_t)p;
            // BUILD2RV: do not invent a bass/rumble filter here. The previous RN/RM/RL filters
            // created the user's "gong / two musics" artefact. Use the zeroed core samples
            // and only apply the final safety limiter.
            l = nap_master_limiter(l);
            r = nap_master_limiter(r);
            if (g_audio_startup_mute_remaining > 0) {
                l = r = 0;
                --g_audio_startup_mute_remaining;
            }
        } else {
            // RN: no decaying low-frequency tail. True silence on real underrun is cleaner than rumble.
            g_audio_underrun_count++;
            l = r = 0;
            g_audio_rumble_l = g_audio_rumble_r = 0;
        }
        out[i * 2 + 0] = nap_clip16(l);
        out[i * 2 + 1] = nap_clip16(r);
    }
    g_audio_pull_count++;
    {
        // BUILD2RW passive audit: record backlog + pull cost into the 10s window. Measure only.
        const uint64_t pull_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - audit_pull_start).count();
        nap_audit_fifo_sample_locked(nap_audio_min_fifo_locked(), pull_ns);
    }
    return got;
}
static int nap_audio_pull_mono(jshort *out, int frames) {
    if (!out || frames <= 0) return 0;
    static thread_local std::vector<jshort> stereo;
    stereo.assign((size_t)frames * 2u, 0);
    int got = nap_audio_pull_stereo(stereo.data(), frames);
    for (int i = 0; i < frames; ++i) {
        out[i] = nap_clip16(((int32_t)stereo[i * 2] + (int32_t)stereo[i * 2 + 1]) / 2);
    }
    return got;
}
static void nap_audio_clear() {
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    g_audio_fm_l_fifo.clear();
    g_audio_fm_r_fifo.clear();
    g_audio_psg_fifo.clear();
    g_tv_fm_l.clear(); g_tv_fm_r.clear(); g_tv_psg.clear();   // B306
    g_audio_fm_phase = 0.0;
    g_audio_psg_phase = 0.0;
    g_audio_fm_sum_l = 0;
    g_audio_fm_sum_r = 0;
    g_audio_psg_sum = 0;
    g_audio_fm_sum_count = 0;
    g_audio_psg_sum_count = 0;
    g_audio_pcm_acc = 0.0;
    g_audio_fm_pushed = 0;
    g_audio_psg_pushed = 0;
    g_audio_pcm_seen = 0;
    g_audio_pull_count = 0;
    g_audio_drop_count = 0;
    g_audio_desync_drop_count = 0;
    g_audio_trim_events = 0;
    g_audio_clock_sleep_events = 0;
    g_audio_starve_events.store(0);
    g_audio_catchup_iterations.store(0);
    g_audio_underrun_count = 0;
    g_audio_rumble_l = g_audio_rumble_r = 0;
    g_audio_startup_mute_remaining = NAP_AUDIO_STARTUP_MUTE_FRAMES;
    g_render_skipped_frames.store(0);
    g_render_captured_frames.store(0);
    // BUILD2RW: fresh audit window per ROM session.
    nap_audit_reset_locked();
    g_audit_left_boundary.store(-1);
    g_audit_right_boundary.store(-1);
    g_audit_screen_w.store(0);
    g_audit_screen_h.store(0);
    g_audit_left_nonzero_frames.store(0);
    g_audit_boundary_change_count.store(0);
    g_audit_leftcol_black_frames.store(0);
    g_audit_leftcol_nonblack_frames.store(0);
    g_audit_midrow_left16_fnv.store(0);
}
static std::string nap_audio_status_locked() {
    std::ostringstream out;
    out << "audio_mode=FM_PSG_ZEROED_RX_UNCHANGED_CORE_O3_RY pcmCddaGen=SKIPPED_NO_CD coreOptO3=YES vdpLeftBlank=IMPLEMENTED sonic_main fixedAudioClock=YES stereo=YES zeroInputBuffers=YES coreLPF=ON noRumbleFilter=YES noSamplePick=YES singleAudioPath=YES audioMasterClock=YES noStarve=YES noHardTrim=YES fmGain=100 psgGain=100 masterGain=90 target=" << nap_audio_target_fifo()
        << " max=" << nap_audio_max_fifo()
        << " desyncLimit=" << nap_audio_desync_limit()
        << " fm_l_fifo=" << g_audio_fm_l_fifo.size()
        << " fm_r_fifo=" << g_audio_fm_r_fifo.size()
        << " psg_fifo=" << g_audio_psg_fifo.size()
        << " backlogMin=" << nap_audio_min_fifo_locked()
        << " latencyFrames=" << nap_audio_max_backlog_locked()
        << " latencyMs=" << ((nap_audio_max_backlog_locked() * 1000) / NAP_AUDIO_OUT_RATE)
        << " trimEvents=" << g_audio_trim_events
        << " clockSleeps=" << g_audio_clock_sleep_events
        << " starveEvents=" << g_audio_starve_events.load()
        << " catchupIterations=" << g_audio_catchup_iterations.load()
        << " androidSdk=" << g_android_sdk.load()
        << " androidCores=" << g_android_cores.load()
        << " s8NoStarve=" << g_mobile_no_starve.load()
        << " workerPrioNice=" << g_worker_priority_nice_result.load()
        << " perfLow=" << g_native_perf_low.load()
        << " fm_pushed=" << g_audio_fm_pushed
        << " psg_pushed=" << g_audio_psg_pushed
        << " pcm_seen=" << g_audio_pcm_seen
        << " pulls=" << g_audio_pull_count
        << " drops=" << g_audio_drop_count
        << " desyncDrops=" << g_audio_desync_drop_count
        << " underruns=" << g_audio_underrun_count
        << " videoCaptured=" << g_render_captured_frames.load()
        << " videoSkipped=" << g_render_skipped_frames.load()
        << " | " << nap_audit_status_locked();
    return out.str();
}

// BUILD2RV: native signal guard for real-core bring-up.
// User reported full app process crash after ROM selection in BUILD2RR/QC. Java try/catch cannot catch SIGSEGV/SIGABRT.
// This guard is debug-stage only: it catches a native signal inside the C++ core-load call and returns a log marker instead of killing the app.
static sigjmp_buf g_nap_sega_sig_jmp;
static std::atomic<int> g_nap_sega_guard_active{0};
static volatile sig_atomic_t g_nap_sega_last_signal = 0;
static void nap_sega_signal_handler(int sig) {
    g_nap_sega_last_signal = sig;
    if (g_nap_sega_guard_active.load() != 0) siglongjmp(g_nap_sega_sig_jmp, 1);
}
struct NapSignalGuard {
    struct sigaction old_segv{}, old_bus{}, old_abrt{}, handler{};
    NapSignalGuard() {
        handler.sa_handler = nap_sega_signal_handler;
        sigemptyset(&handler.sa_mask);
        handler.sa_flags = 0;
        sigaction(SIGSEGV, &handler, &old_segv);
        sigaction(SIGBUS,  &handler, &old_bus);
        sigaction(SIGABRT, &handler, &old_abrt);
        g_nap_sega_last_signal = 0;
        g_nap_sega_guard_active.store(1);
    }
    ~NapSignalGuard() {
        g_nap_sega_guard_active.store(0);
        sigaction(SIGSEGV, &old_segv, nullptr);
        sigaction(SIGBUS,  &old_bus,  nullptr);
        sigaction(SIGABRT, &old_abrt, nullptr);
    }
};
static const char* nap_signal_name(int sig) {
    switch(sig) { case SIGSEGV: return "SIGSEGV"; case SIGBUS: return "SIGBUS"; case SIGABRT: return "SIGABRT"; default: return "SIGNAL"; }
}

#if NAP_SEGA_VENDOR_CORE_PRESENT
// BUILD2RV: tiny Android frontend for the real ClownMDEmu-core.
// It is intentionally small: ROM load -> hard reset -> iterate -> scanline framebuffer -> existing in-place view.
// Audio mixing is not wired yet; this stage is the first real core import/visual boot attempt, no fake gameplay.
struct NapRealCoreState {
    bool loaded = false;
    bool frame_ready = false;
    int frame_w = 320;
    int frame_h = 224;
    int display_w = 320;
    int display_h = 224;
    uint32_t frame_counter = 0;
    uint32_t display_frame_counter = 0;
    uint32_t iterations_last = 0;
    std::vector<cc_u16l> cart_words;
    std::vector<uint32_t> frame_argb;        // worker write buffer
    std::vector<uint32_t> display_argb;      // render snapshot, swapped only after full frame
    uint32_t palette[0x10000];
    ClownMDEmu emu;
    ClownMDEmu_InitialConfiguration cfg;
    ClownMDEmu_Callbacks cb;
    std::string status = "REAL_CORE_NOT_LOADED";
};

static NapRealCoreState g_real;
static std::mutex g_real_mutex;
static std::mutex g_frame_mutex; // BUILD2RV: render must not wait on full ClownMDEmu_Iterate()
static bool g_real_core_loaded_but_render_guarded = false;
static uint32_t g_guard_frame_counter = 0;
static int g_real_step_stage = 0; // BUILD2RV: explicit core step: 0 constant, 1 init, 2 setcart, 3 hardreset, 4 iterate
static size_t g_real_staged_bytes = 0;
static pthread_t g_real_thread{};
static std::atomic<int> g_real_thread_created{0};
static std::atomic<int> g_real_thread_run{0};
static std::atomic<int> g_real_thread_alive{0};
static std::atomic<int> g_real_thread_generation{0};
static std::atomic<uint32_t> g_real_thread_iterations{0};
static void* nap_real_worker_thread_entry(void *arg);
static void nap_real_worker_thread(int generation);
static void nap_real_stop_worker() {
    g_real_thread_run.store(0);
    if (g_real_thread_created.load() != 0) {
        pthread_join(g_real_thread, nullptr);
        g_real_thread_created.store(0);
    }
    g_real_thread_alive.store(0);
}

// BUILD2RV: reset the huge core struct in-place. Do NOT use `g_real = NapRealCoreState()`: \n// ClownMDEmu is >1 MB and a temporary can overflow Android/WebView thread stack.
static void nap_real_reset_state_locked() {
    g_real.loaded = false;
    g_real.frame_ready = false;
    g_real.frame_w = 320;
    g_real.frame_h = 224;
    g_real.display_w = 320;
    g_real.display_h = 224;
    g_real.frame_counter = 0;
    g_real.display_frame_counter = 0;
    g_real.iterations_last = 0;
    g_real.cart_words.clear();
    g_real.frame_argb.clear();
    g_real.display_argb.clear();
    for (int i = 0; i < 0x10000; ++i) g_real.palette[i] = 0xff000000u;
    std::memset(&g_real.emu, 0, sizeof(g_real.emu));
    std::memset(&g_real.cfg, 0, sizeof(g_real.cfg));
    std::memset(&g_real.cb, 0, sizeof(g_real.cb));
    g_real.status = "REAL_CORE_NOT_LOADED";
}

// BUILD2RV: forward declaration required by C++ before nap_real_load_rom_bytes().
static void nap_real_setup_cfg_callbacks();

static uint32_t nap_md_colour_to_argb(cc_u16f colour) {
    // Mega Drive CRAM is commonly 0x0BGR with 3 meaningful bits per channel.
    int r3 = (int)((colour >> 1) & 7);
    int g3 = (int)((colour >> 5) & 7);
    int b3 = (int)((colour >> 9) & 7);
    int r = (r3 << 5) | (r3 << 2) | (r3 >> 1);
    int g = (g3 << 5) | (g3 << 2) | (g3 >> 1);
    int b = (b3 << 5) | (b3 << 2) | (b3 >> 1);
    return 0xff000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

static void nap_real_colour_updated(void *user_data, cc_u16f index, cc_u16f colour) {
    NapRealCoreState *st = static_cast<NapRealCoreState*>(user_data);
    if (!st) return;
    st->palette[index & 0xffffu] = nap_md_colour_to_argb(colour);
}

static void nap_real_scanline_rendered(void *user_data, cc_u16f scanline, const cc_u8l *pixels,
                                       cc_u16f left_boundary, cc_u16f right_boundary,
                                       cc_u16f screen_width, cc_u16f screen_height) {
    NapRealCoreState *st = static_cast<NapRealCoreState*>(user_data);
    if (!st || !pixels || screen_width == 0 || screen_height == 0) return;
    if (g_render_capture_current.load() == 0) return;
    int sw = (int)screen_width;
    int sh = (int)screen_height;
    int y = (int)scanline;
    if (sw <= 0 || sh <= 0 || y < 0 || y >= sh) return;
    if (st->frame_w != sw || st->frame_h != sh || st->frame_argb.size() != (size_t)(sw * sh)) {
        st->frame_w = sw;
        st->frame_h = sh;
        st->frame_argb.assign((size_t)sw * sh, 0xff000000u);
    }
    int left = std::max(0, (int)left_boundary);
    int right = std::min(sw, (int)right_boundary);
    if (right <= left) { left = 0; right = sw; }
    const int activeW = std::max(0, right - left);
    if (y == 0) {
        // BUILD2RW passive left-edge audit: record what the core REALLY reports, do not change rendering.
        int prevL = g_audit_left_boundary.exchange(left);
        int prevR = g_audit_right_boundary.exchange(right);
        g_audit_screen_w.store(sw);
        g_audit_screen_h.store(sh);
        if (prevL != -1 && (prevL != left || prevR != right)) g_audit_boundary_change_count.fetch_add(1);
        if (left != 0) g_audit_left_nonzero_frames.fetch_add(1);
    }
    for (int x = 0; x < sw; ++x) {
        // BUILD2RV: real left-edge fix. Do not duplicate the first active pixels into the border
        // (that produced the coloured shifted strip). Shift the active MD scanline to x=0 and
        // fill the unused tail with black. Java draws the full 320x224 source again.
        uint32_t c = 0xff000000u;
        if (activeW > 0 && x < activeW) {
            int sx = left + x;
            if (sx < 0) sx = 0;
            if (sx >= sw) sx = sw - 1;
            cc_u8l pix = pixels[sx];
            c = st->palette[pix];
            if (c == 0) c = 0xff000000u;
        }
        st->frame_argb[(size_t)y * sw + x] = c;
    }
    if (y == sh / 2) {
        // BUILD2RW passive: checksum of the 16 leftmost OUTPUT pixels of the middle row.
        // If a coloured left strip exists in the produced framebuffer, this FNV changes and
        // leftColNonBlackFrames grows; if the strip appears only on screen, the bug is in
        // Java TextureView scaling/stride, not here.
        uint32_t h32 = 2166136261u;
        int limit = std::min(sw, 16);
        for (int x = 0; x < limit; ++x) {
            uint32_t c = st->frame_argb[(size_t)y * sw + x];
            h32 ^= c; h32 *= 16777619u;
        }
        g_audit_midrow_left16_fnv.store(h32);
        if (sw > 0 && st->frame_argb[(size_t)y * sw + 0] == 0xff000000u) g_audit_leftcol_black_frames.fetch_add(1);
        else g_audit_leftcol_nonblack_frames.fetch_add(1);
    }
    if (y >= sh - 1) {
        // B306: snimek, ktery se snima jen kvuli TV (telefon v rezimu S8 kresli
        // kazdy druhy), na displej telefonu nejde - telefon ma stejnou praci jako dosud
        if (g_render_display_current.load() != 0) {
            g_render_captured_frames.fetch_add(1);
            // BUILD2RV: worker writes into frame_argb, renderer reads only display_argb.
            // This avoids blocking mobile video on the full emulator iterate mutex and prevents torn partial frames.
            std::lock_guard<std::mutex> flock(g_frame_mutex);
            st->display_w = st->frame_w;
            st->display_h = st->frame_h;
            st->display_argb = st->frame_argb;
            st->display_frame_counter++;
            st->frame_counter = st->display_frame_counter;
            st->frame_ready = true;
        }
        // B305/B306: hotovy snimek do prenosu na TV / PC - preda se az po kroku
        // jadra (nap_tv_po_kroku), kdy je vyrobeny i zvuk celeho snimku
        g_tv_snimek_hotov.store(true);
    }
}

// B306: po kazdem kroku jadra (vlakno emulace): hotovy snimek do prenosu na TV / PC
// (kopie jen kdyz se nekdo diva; frame_argb pise jen tohle vlakno, zamek netreba).
// Zvuk tehoz kroku uz je v prenosu (nap_audio_push_*_batch), takze paket nese
// obraz i zvuk stejneho okamziku hry.
static void nap_tv_po_kroku(NapRealCoreState *st) {
    if (!g_tv_snimek_hotov.exchange(false)) return;
    g_segaProud.snimek(g_segaProud.klientu() > 0 ? st->frame_argb.data() : nullptr, st->frame_w, st->frame_h);
}

// bit tlacitka ve vstupu Segy: 0 U 1 D 2 L 3 R 4 A 5 B 6 C 7 START, B355: 8 X 9 Y 10 Z 11 MODE
static int nap_sega_tlacitko_bit(ClownMDEmu_Button button_id) {
    switch (button_id) {
        case CLOWNMDEMU_BUTTON_UP: return 0;
        case CLOWNMDEMU_BUTTON_DOWN: return 1;
        case CLOWNMDEMU_BUTTON_LEFT: return 2;
        case CLOWNMDEMU_BUTTON_RIGHT: return 3;
        case CLOWNMDEMU_BUTTON_A: return 4;
        case CLOWNMDEMU_BUTTON_B: return 5;
        case CLOWNMDEMU_BUTTON_C: return 6;
        case CLOWNMDEMU_BUTTON_START: return 7;
        case CLOWNMDEMU_BUTTON_X: return 8;      // B355: ovladac se 6 tlacitky
        case CLOWNMDEMU_BUTTON_Y: return 9;
        case CLOWNMDEMU_BUTTON_Z: return 10;
        case CLOWNMDEMU_BUTTON_MODE: return 11;
        default: return -1;
    }
}

// =====================================================================================================
//  B357: ONLINE HRANI (Rene: "chci hrat hry online pomoci meho enginu") - dva telefony, kazdy emuluje sam,
//  po siti jdou jen stisky (online/nap_online.h, pres engine N&P). Hrac 1 = kdo hru zalozil (ovladac 1 Segy),
//  hrac 2 = kdo se pripojil (ovladac 2). Oba zacnou od zapnuti (snimek 0) ve stejny okamzik; snimek N se
//  emuluje, az jsou vstupy obou pro N (g_online_vstup, cte je nap_real_input_requested). Kazdou sekundu se
//  porovna otisk stavu (RAM 68000 a Z80, VRAM, CRAM, VSRAM) - jiny = hra se rozjela, online konci.
//  Bez online beze zmeny (g_online_aktivni 0).
// =====================================================================================================
static nap::online::OnlineKlient g_online;
static std::atomic<int> g_online_aktivni{0};          // online zapnuto (ceka na hrace / hraje)
static std::atomic<int> g_online_hraje{0};            // hra bezi online (vstupy z relace)
static std::atomic<uint32_t> g_online_vstup[2];       // vstupy hrace 1 / 2 pro prave emulovany snimek
static uint32_t g_online_snimek = 0;                  // jen vlakno emulace
static std::mutex g_online_m;                         // otisk hry / text konce
static uint8_t g_sega_otisk_hry[20] = {};             // SHA-1 nahrane ROM (pod g_online_m)
static bool g_sega_otisk_je = false;
static std::string g_online_konec;                    // proc online skoncilo (pro Javu)
static std::atomic<uint32_t> g_online_snimku_celkem{0};
static std::atomic<int> g_online_startu{0};          // B358: kolikate spojeni (Java pozna novy pokus i se stejnym koncem)
// Rizeni po jednom: onlineStart / onlineStop / nova hra / odchod ze Segy a konec zjisteny vlaknem emulace. Konec stare
// online hry (vlakno emulace) tak nikdy nevypne novou, kterou Java mezitim zalozila. Vlakno site ho nebere nikdy.
static std::mutex g_online_rizeni;
// B362: oprava odhadu - jadro pocita snimky znovu: zvuk a obraz techto snimku nikam nejdou (uz jednou hraly)
static std::atomic<int> g_online_prepocet{0};

/** Vstup tohoto telefonu (hrac sam za sebe): dotyk + 1. herni ovladac, 12 bitu jako nap_sega_tlacitko_bit. */
static uint32_t nap_sega_lokalni_bity() {
    uint32_t b = 0;
    for (int i = 0; i < 8; i++) if (nap_vstup(i)) b |= 1u << i;
    b |= g_ovl_bity.load(std::memory_order_relaxed) & 0xF00u;
    return b;
}

static cc_bool nap_real_input_requested(void *user_data, cc_u8f player_id, ClownMDEmu_Button button_id) {
    (void)user_data;
    if (g_online_hraje.load(std::memory_order_relaxed)) {   // B357: online - vstupy obou hracu pro tento snimek
        if (player_id > 1) return cc_false;
        const int i = nap_sega_tlacitko_bit(button_id);
        return (i >= 0 && ((g_online_vstup[player_id].load(std::memory_order_relaxed) >> i) & 1u)) ? cc_true : cc_false;
    }
    if (player_id == 1) {                       // B354: hrac 2 = druhy herni ovladac
        const uint32_t b2 = g_ovl_bity2.load(std::memory_order_relaxed);
        const int i = nap_sega_tlacitko_bit(button_id);
        return (i >= 0 && ((b2 >> i) & 1u)) ? cc_true : cc_false;
    }
    if (player_id != 0) return cc_false;
    switch (button_id) {
        // B320: dotyk (g_input) + herni ovladac (g_ovl_bity)
        case CLOWNMDEMU_BUTTON_UP: return nap_vstup(0) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_DOWN: return nap_vstup(1) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_LEFT: return nap_vstup(2) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_RIGHT: return nap_vstup(3) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_A: return nap_vstup(4) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_B: return nap_vstup(5) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_C: return nap_vstup(6) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_START: return nap_vstup(7) ? cc_true : cc_false;
        // B355: X Y Z MODE ovladace se 6 tlacitky - jen herni ovladac (dotyk je nema)
        case CLOWNMDEMU_BUTTON_X: return ((g_ovl_bity.load(std::memory_order_relaxed) >> 8) & 1u) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_Y: return ((g_ovl_bity.load(std::memory_order_relaxed) >> 9) & 1u) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_Z: return ((g_ovl_bity.load(std::memory_order_relaxed) >> 10) & 1u) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_MODE: return ((g_ovl_bity.load(std::memory_order_relaxed) >> 11) & 1u) ? cc_true : cc_false;
        default: return cc_false;
    }
}

static inline void nap_audio_resample_fm_box(int32_t l, int32_t r, double srcRate, std::vector<jshort>& outL, std::vector<jshort>& outR) {
    g_audio_fm_sum_l += l;
    g_audio_fm_sum_r += r;
    g_audio_fm_sum_count++;
    g_audio_fm_phase += (double)NAP_AUDIO_OUT_RATE / srcRate;
    if (g_audio_fm_phase >= 1.0 && g_audio_fm_sum_count > 0) {
        int32_t avgL = (int32_t)(g_audio_fm_sum_l / g_audio_fm_sum_count);
        int32_t avgR = (int32_t)(g_audio_fm_sum_r / g_audio_fm_sum_count);
        outL.push_back(nap_clip16((avgL * NAP_FM_GAIN_PERCENT) / 100));
        outR.push_back(nap_clip16((avgR * NAP_FM_GAIN_PERCENT) / 100));
        g_audio_fm_phase -= 1.0;
        g_audio_fm_sum_l = 0;
        g_audio_fm_sum_r = 0;
        g_audio_fm_sum_count = 0;
        if (g_audio_fm_phase >= 1.0) g_audio_fm_phase = std::fmod(g_audio_fm_phase, 1.0);
    }
}

static inline void nap_audio_resample_psg_box(int32_t mono, double srcRate, std::vector<jshort>& out) {
    g_audio_psg_sum += mono;
    g_audio_psg_sum_count++;
    g_audio_psg_phase += (double)NAP_AUDIO_OUT_RATE / srcRate;
    if (g_audio_psg_phase >= 1.0 && g_audio_psg_sum_count > 0) {
        int32_t avg = (int32_t)(g_audio_psg_sum / g_audio_psg_sum_count);
        out.push_back(nap_clip16((avg * NAP_PSG_GAIN_PERCENT) / 100));
        g_audio_psg_phase -= 1.0;
        g_audio_psg_sum = 0;
        g_audio_psg_sum_count = 0;
        if (g_audio_psg_phase >= 1.0) g_audio_psg_phase = std::fmod(g_audio_psg_phase, 1.0);
    }
}

static void nap_audio_fm(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    static thread_local std::vector<cc_s16l> tmp;
    static thread_local std::vector<jshort> outL;
    static thread_local std::vector<jshort> outR;
    tmp.resize(n * CLOWNMDEMU_FM_CHANNEL_COUNT);
    // BUILD2RV CRITICAL: ClownMDEmu FM_OutputSamples uses += into sample_buffer.
    // Reusing thread_local buffers without clearing them mixed old audio into new audio,
    // heard as loud fake bass/gong/"two musics". Always zero before calling the core generator.
    std::fill(tmp.begin(), tmp.end(), 0);
    outL.clear();
    outR.clear();
    outL.reserve((n * (size_t)NAP_AUDIO_OUT_RATE) / 53000u + 16u);
    outR.reserve((n * (size_t)NAP_AUDIO_OUT_RATE) / 53000u + 16u);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
    if (tmp.empty()) return;
    // B362: oprava online odhadu - stav FM se pocita (gen vyse), zvuk snimku uz jednou hral -> ne do reproduktoru
    if (g_online_prepocet.load(std::memory_order_relaxed)) return;
    const double srcRate = (g_real_cfg_tv == CLOWNMDEMU_TV_STANDARD_PAL) ? (double)CLOWNMDEMU_FM_SAMPLE_RATE_PAL : (double)CLOWNMDEMU_FM_SAMPLE_RATE_NTSC;
    for (size_t i = 0; i < n; ++i) {
        int32_t l = tmp[i * CLOWNMDEMU_FM_CHANNEL_COUNT + 0] / CLOWNMDEMU_FM_VOLUME_DIVISOR;
        int32_t r = (CLOWNMDEMU_FM_CHANNEL_COUNT > 1) ? (tmp[i * CLOWNMDEMU_FM_CHANNEL_COUNT + 1] / CLOWNMDEMU_FM_VOLUME_DIVISOR) : l;
        // BUILD2RV: zeroed native YM/FM buffer, then average-resample to 48 kHz. No stale samples.
        nap_audio_resample_fm_box(l, r, srcRate, outL, outR);
    }
    nap_audio_push_fm_batch(outL, outR);
}

static void nap_audio_psg(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    static thread_local std::vector<cc_s16l> tmp;
    static thread_local std::vector<jshort> out;
    tmp.resize(n * CLOWNMDEMU_PSG_CHANNEL_COUNT);
    // BUILD2RV CRITICAL: PSG_Update also uses +=. Zero the scratch buffer before generation.
    std::fill(tmp.begin(), tmp.end(), 0);
    out.clear();
    out.reserve((n * (size_t)NAP_AUDIO_OUT_RATE) / 224000u + 16u);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
    if (tmp.empty()) return;
    if (g_online_prepocet.load(std::memory_order_relaxed)) return;   // B362: viz FM
    const double srcRate = (g_real_cfg_tv == CLOWNMDEMU_TV_STANDARD_PAL) ? (double)CLOWNMDEMU_PSG_SAMPLE_RATE_PAL : (double)CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC;
    for (size_t i = 0; i < n; ++i) {
        int32_t mono = tmp[i * CLOWNMDEMU_PSG_CHANNEL_COUNT] / CLOWNMDEMU_PSG_VOLUME_DIVISOR;
        // BUILD2RV: zeroed PSG buffer first; PSG_Update uses +=, so clearing is mandatory. Then box-average downsample.
        nap_audio_resample_psg_box(mono, srcRate, out);
    }
    nap_audio_push_psg_batch(out);
}
static void nap_audio_pcm(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    // BUILD2RX: RW audit proved the S8 problem is raw CPU (coreAvgMs=23.7 > 16.67 budget on Aladdin,
    // progressive starvation on Ayrton) while heap/GC stayed flat. PCM was generated into a scratch
    // buffer and thrown away (cd_add_on_enabled=false, Sonic DAC lives in FM). Stop paying CPU for
    // discarded silence: do not call the generator at all. Counter stays for the audit.
    (void)c; (void)gen;
    if (n) { std::lock_guard<std::mutex> lock(g_audio_mutex); g_audio_pcm_seen += n; }
}
static void nap_audio_cdda(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    // BUILD2RX: same as PCM above - CDDA has no CD attached and its output was discarded.
    (void)c; (void)gen;
    if (n) { std::lock_guard<std::mutex> lock(g_audio_mutex); g_audio_pcm_seen += n; }
}
static void nap_cd_seeked(void*, cc_u32f) {}
static void nap_cd_sector_read(void*, cc_u16l *buffer) { if (buffer) std::memset(buffer, 0, 2352); }
static cc_bool nap_cd_track_seeked(void*, cc_u16f, ClownMDEmu_CDDAMode) { return cc_false; }
static size_t nap_cd_audio_read(void*, cc_s16l*, size_t) { return 0; }
static cc_bool nap_save_open_read(void*, const char*) { return cc_false; }
static cc_s16f nap_save_read(void*) { return -1; }
static cc_bool nap_save_open_write(void*, const char*) { return cc_false; }
static void nap_save_written(void*, cc_u8f) {}
static void nap_save_closed(void*) {}
static cc_bool nap_save_removed(void*, const char*) { return cc_false; }
static cc_bool nap_save_size(void*, const char*, size_t*) { return cc_false; }

static std::string nap_real_detect_and_set_region(const uint8_t* bytes, size_t size) {
    std::string region;
    if (bytes && size >= 0x200) {
        for (int i = 0; i < 16; ++i) {
            unsigned char c = bytes[0x1F0 + i];
            if (c >= 32 && c < 127) region.push_back((char)c);
        }
    }
    std::string upper;
    for (char c : region) upper.push_back((char)std::toupper((unsigned char)c));
    bool hasU = upper.find('U') != std::string::npos;
    bool hasE = upper.find('E') != std::string::npos;
    bool hasJ = upper.find('J') != std::string::npos;
    if (hasE && !hasU) {
        g_real_cfg_region = CLOWNMDEMU_REGION_OVERSEAS;
        g_real_cfg_tv = CLOWNMDEMU_TV_STANDARD_PAL;
        g_real_cfg_region_label = "EU_PAL_OVERSEAS header=" + upper;
    } else if (hasJ && !hasU && !hasE) {
        g_real_cfg_region = CLOWNMDEMU_REGION_DOMESTIC;
        g_real_cfg_tv = CLOWNMDEMU_TV_STANDARD_NTSC;
        g_real_cfg_region_label = "JP_NTSC_DOMESTIC header=" + upper;
    } else {
        g_real_cfg_region = CLOWNMDEMU_REGION_OVERSEAS;
        g_real_cfg_tv = CLOWNMDEMU_TV_STANDARD_NTSC;
        g_real_cfg_region_label = "US_NTSC_OVERSEAS header=" + upper;
    }
    return g_real_cfg_region_label;
}

static std::string nap_real_load_rom_bytes(const uint8_t* bytes, size_t size) {
    if (!bytes || size < 0x200) return "REAL_CORE_LOAD_ERROR bad rom";
    {   // B357: jina hra uprostred online = konec online (druhy telefon dostane KONEC)
        std::lock_guard<std::mutex> r(g_online_rizeni);
        if (g_online_aktivni.load()) {
            g_online_aktivni.store(0);
            g_online_hraje.store(0);
            g_online.stop();
            std::lock_guard<std::mutex> l(g_online_m);
            g_online_konec = "nahrana jina hra - online skoncilo";
            NAPLOG("B357 ONLINE konec - nahrana jina hra");
        }
    }
    const std::string regionMode = nap_real_detect_and_set_region(bytes, size);
    nap_audio_clear();
    {   // B357: otisk hry pro online (oba telefony musi mit tutez ROM)
        nap::online::Sha1 h;
        h.pridej(bytes, size);
        std::lock_guard<std::mutex> l(g_online_m);
        h.konec(g_sega_otisk_hry);
        g_sega_otisk_je = true;
    }

    // BUILD2RV: do not run ClownMDEmu from the WebView JavaBridge thread or from View.onDraw.
    // The previous builds could crash the whole app after ROM selection. Host-side core test passed,
    // so this build moves real init/reset/iterate to one dedicated native worker thread.
    nap_real_stop_worker();

    {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        nap_real_reset_state_locked();
        g_real.frame_argb.assign(320 * 224, 0xff000000u);
        {
            std::lock_guard<std::mutex> flock(g_frame_mutex);
            g_real.display_w = 320;
            g_real.display_h = 224;
            g_real.display_frame_counter = 0;
            g_real.display_argb.assign(320 * 224, 0xff000000u);
        }
        g_real_core_loaded_but_render_guarded = false;
        g_guard_frame_counter = 0;
        g_real_step_stage = 0;
        g_real_staged_bytes = size;
        g_real_thread_iterations.store(0);

        g_real.cart_words.resize((size + 1) / 2);
        for (size_t i = 0, j = 0; i < size; i += 2, ++j) {
            uint16_t hi = bytes[i];
            uint16_t lo = (i + 1 < size) ? bytes[i + 1] : 0;
            g_real.cart_words[j] = (cc_u16l)((hi << 8) | lo);
        }
        g_real.status = "REAL_CORE_ROM_STAGED_FOR_WORKER_THREAD bytes=" + std::to_string(size);
    }

    int generation = g_real_thread_generation.fetch_add(1) + 1;
    g_real_thread_run.store(1);
    g_real_thread_alive.store(1);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // BUILD2RV: ClownMDEmu core state is large and the emulator core can use more native stack than Android's default.
    // Give the worker 8 MB so ROM selection does not kill the process with stack overflow.
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);
    int rc = pthread_create(&g_real_thread, &attr, nap_real_worker_thread_entry, (void*)(intptr_t)generation);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        g_real_thread_run.store(0);
        g_real_thread_alive.store(0);
        g_real_thread_created.store(0);
        return std::string("REAL_CORE_THREAD_START_ERROR pthread_create rc=") + std::to_string(rc);
    }
    g_real_thread_created.store(1);

    std::ostringstream out;
    out << "REAL_CORE_THREAD_START_OK\n";
    out << "core=ClownMDEmu-core offline/vendor\n";
    out << "bytes=" << size << " words=" << ((size + 1) / 2) << "\n";
    out << "nativeRegionAuto=" << regionMode << "\n";
    out << "threading=DEDICATED_NATIVE_WORKER_THREAD_BIGSTACK_8MB; no core calls from WebView bridge; no core calls from UI onDraw; reset=no_stack_temporary\n";
    out << "pattern=OFF; fake/proof moving cubes removed\n";
    out << "render=CACHED_FRAME_COPY_ONLY_FROM_NATIVE_MONITOR_DRAW\n";
    out << "audio=FM_PSG_ZEROED_RR_RECOVERY_SAFE_AUDIT_RV stereo AudioTrack; ZEROED callback buffers before ClownMDEmu += mixing; S8 no-starve reservoir/catchup; video capture throttles under audio starvation; core low-pass ON; no fake rumble filter; no FM/PSG desync drops";
    return out.str();
}

static void nap_real_setup_cfg_callbacks() {
    std::memset(&g_real.cfg, 0, sizeof(g_real.cfg));
    std::memset(&g_real.cb, 0, sizeof(g_real.cb));
    g_real.cfg.general.region = g_real_cfg_region;
    g_real.cfg.general.tv_standard = g_real_cfg_tv;
    g_real.cfg.general.low_pass_filter_disabled = cc_false;
    g_real.cfg.general.cd_add_on_enabled = cc_false;
    g_real.cb.user_data = &g_real;
    g_real.cb.colour_updated = nap_real_colour_updated;
    g_real.cb.scanline_rendered = nap_real_scanline_rendered;
    g_real.cb.input_requested = nap_real_input_requested;
    g_real.cb.fm_audio_to_be_generated = nap_audio_fm;
    g_real.cb.psg_audio_to_be_generated = nap_audio_psg;
    g_real.cb.pcm_audio_to_be_generated = nap_audio_pcm;
    g_real.cb.cdda_audio_to_be_generated = nap_audio_cdda;
    g_real.cb.cd_seeked = nap_cd_seeked;
    g_real.cb.cd_sector_read = nap_cd_sector_read;
    g_real.cb.cd_track_seeked = nap_cd_track_seeked;
    g_real.cb.cd_audio_read = nap_cd_audio_read;
    g_real.cb.save_file_opened_for_reading = nap_save_open_read;
    g_real.cb.save_file_read = nap_save_read;
    g_real.cb.save_file_opened_for_writing = nap_save_open_write;
    g_real.cb.save_file_written = nap_save_written;
    g_real.cb.save_file_closed = nap_save_closed;
    g_real.cb.save_file_removed = nap_save_removed;
    g_real.cb.save_file_size_obtained = nap_save_size;
}

static std::string nap_real_step_once() {
    std::lock_guard<std::mutex> lock(g_real_mutex);
    std::ostringstream out;
    out << "REAL_CORE_STEP_DISABLED_IN_BUILD2RV\n";
    out << "reason=core now runs from native monitor render under single mutex after ROM load\n";
    out << "loaded=" << (g_real.loaded ? "YES" : "NO") << "\n";
    out << "status=" << g_real.status;
    return out.str();
}

static bool nap_real_render_to_argb(int out_w, int out_h, jint *out_px) {
    if (out_w <= 0 || out_h <= 0 || !out_px) return false;

    // BUILD2RV: renderer asks for 320x224. Copy directly under the frame mutex and avoid
    // allocating/copying a temporary vector every TextureView tick on S8/A12.
    std::lock_guard<std::mutex> flock(g_frame_mutex);
    if (!g_real.frame_ready || g_real.display_frame_counter == 0 || g_real.display_argb.empty() || g_real.display_w <= 0 || g_real.display_h <= 0) return false;
    const int sw = g_real.display_w;
    const int sh = g_real.display_h;
    const std::vector<uint32_t>& src = g_real.display_argb;

    if (out_w == sw && out_h == sh) {
        const int n = out_w * out_h;
        for (int i = 0; i < n; ++i) out_px[i] = (jint)(src[(size_t)i] ? src[(size_t)i] : 0xff000000u);
        return true;
    }

    for (int y = 0; y < out_h; ++y) {
        int sy = (int)((int64_t)y * sh / out_h);
        if (sy < 0) sy = 0; if (sy >= sh) sy = sh - 1;
        for (int x = 0; x < out_w; ++x) {
            int sx = (int)((int64_t)x * sw / out_w);
            if (sx < 0) sx = 0; if (sx >= sw) sx = sw - 1;
            uint32_t c = src[(size_t)sy * sw + sx];
            out_px[(size_t)y * out_w + x] = (jint)(c ? c : 0xff000000u);
        }
    }
    return true;
}


// =====================================================================================================
//  B357: ONLINE - studeny start, otisk stavu, krok s vstupy obou hracu (vlakno emulace)
// =====================================================================================================
// Studeny start jako pri startu vlakna emulace: pamet od nuly (jinak by se po predchozi hre lisila od druheho telefonu).
static void nap_real_studeny_start_locked() {
    std::memset(&g_real.emu, 0, sizeof(g_real.emu));
    nap_real_setup_cfg_callbacks();
    ClownMDEmu_Initialise(&g_real.emu, &g_real.cfg, &g_real.cb);
    ClownMDEmu_SetCartridge(&g_real.emu, g_real.cart_words.data(), (cc_u32f)g_real.cart_words.size());
    ClownMDEmu_HardReset(&g_real.emu, cc_true, cc_false);
}

// Otisk stavu pro porovnani telefonu: RAM 68000 a Z80, VRAM, CRAM, VSRAM (pole bez vyplne - stejne vsude).
// B360: kazda cast zvlast (jdou s otiskem - pri rozjeti je v logu videt, co se lisi), otisk = otisk casti.
static const int NAP_SEGA_CASTI = 5;
static const char *const NAP_SEGA_NAZVY_CASTI[NAP_SEGA_CASTI] = {"RAM 68000", "RAM Z80", "VRAM", "CRAM (barvy)", "VSRAM"};
static uint64_t nap_sega_otisk_stavu_casti(uint64_t c[NAP_SEGA_CASTI]) {
    const ClownMDEmu &e = g_real.emu;
    c[0] = nap::online::otisk(e.state.m68k.ram, sizeof(e.state.m68k.ram));
    c[1] = nap::online::otisk(e.state.z80.ram, sizeof(e.state.z80.ram));
    c[2] = nap::online::otisk(e.vdp.state.vram, sizeof(e.vdp.state.vram));
    c[3] = nap::online::otisk(e.vdp.state.cram, sizeof(e.vdp.state.cram));
    c[4] = nap::online::otisk(e.vdp.state.vsram, sizeof(e.vdp.state.vsram));
    return nap::online::otisk(c, sizeof(uint64_t) * NAP_SEGA_CASTI);
}
static uint64_t nap_sega_otisk_stavu() {
    uint64_t c[NAP_SEGA_CASTI];
    return nap_sega_otisk_stavu_casti(c);
}

// B360: barvy pro obraz si appka drzi sama (jadro je hlasi jen pri zapisu do CRAM) - po nahrani stavu je
// prepocitat z CRAM presne jako vdp.c WriteAndIncrement (normalni / stin / zvyrazneni), jinak by hra mela az do
// pristiho zapisu barev barvy z minula.
static void nap_real_obnov_paletu_locked() {
    for (cc_u16f i = 0; i < 64; i++) {
        const cc_u16f c = g_real.emu.vdp.state.cram[i] & 0xEEE;
        nap_real_colour_updated(&g_real, 0 + i, c | ((c & 0x888) >> 3));
        nap_real_colour_updated(&g_real, 64 + i, c >> 1);
        nap_real_colour_updated(&g_real, 128 + i, 0x888 + (c >> 1));
    }
}

// =====================================================================================================
//  B360: SYNCHRONIZACE STAVU PRI ROZJETI (online/nap_online.h). Rene 23. 11.: pokus 1 skoncil po 13 s "hra se
//  rozjela". Ted hrac 1 zabali CELY stav jadra (ClownMDEmu_SaveState + stav ovladacu, ktery v zaloze neni),
//  zlib, a hrac 2 ho pred stejnym snimkem nahraje - hraje se dal. Data: "NAPS", velikost zalohy, velikost
//  ovladacu (u32 LE - jina velikost = jine jadro, nenahrat), zaloha, ovladace; ven: u32 delka + zlib.
//  Velke buffery staticky (zaloha ma 1,1 MB - ne na zasobnik). Jen vlakno emulace.
// =====================================================================================================
static ClownMDEmu_StateBackup g_online_zaloha;
static std::vector<uint8_t> g_online_surova;
static std::atomic<uint32_t> g_online_stav_kb{0};

static void nap_sega_le32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }

static bool nap_sega_stav_zabal(std::vector<uint8_t> &ven) {
    const size_t nz = sizeof(ClownMDEmu_StateBackup), nc = sizeof(ControllerManager);
    g_online_surova.resize(12 + nz + nc);
    uint8_t *p = g_online_surova.data();
    {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        ClownMDEmu_SaveState(&g_real.emu, &g_online_zaloha);
        std::memcpy(p + 12 + nz, &g_real.emu.controller_manager, nc);
    }
    std::memcpy(p, "NAPS", 4);
    nap_sega_le32(p + 4, (uint32_t)nz);
    nap_sega_le32(p + 8, (uint32_t)nc);
    std::memcpy(p + 12, &g_online_zaloha, nz);
    uLongf mez = compressBound((uLong)g_online_surova.size());
    ven.resize(4 + (size_t)mez);
    nap_sega_le32(ven.data(), (uint32_t)g_online_surova.size());
    if (compress2(ven.data() + 4, &mez, g_online_surova.data(), (uLong)g_online_surova.size(), 1) != Z_OK) return false;
    ven.resize(4 + (size_t)mez);
    g_online_stav_kb.store((uint32_t)((ven.size() + 1023) / 1024));
    return true;
}

static bool nap_sega_stav_nahraj(const std::vector<uint8_t> &d) {
    const size_t nz = sizeof(ClownMDEmu_StateBackup), nc = sizeof(ControllerManager);
    if (d.size() < 5) return false;
    const uint32_t surova = nap::online::r32(d.data());
    if (surova != 12 + nz + nc) return false;   // jina velikost = jine jadro (jina verze appky)
    g_online_surova.resize(surova);
    uLongf n = surova;
    if (uncompress(g_online_surova.data(), &n, d.data() + 4, (uLong)(d.size() - 4)) != Z_OK || n != surova) return false;
    const uint8_t *p = g_online_surova.data();
    if (std::memcmp(p, "NAPS", 4) != 0 || nap::online::r32(p + 4) != nz || nap::online::r32(p + 8) != nc) return false;
    std::memcpy(&g_online_zaloha, p + 12, nz);
    {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        ClownMDEmu_LoadState(&g_real.emu, &g_online_zaloha);
        std::memcpy(&g_real.emu.controller_manager, p + 12 + nz, nc);
        nap_real_obnov_paletu_locked();
    }
    g_online_stav_kb.store((uint32_t)((d.size() + 1023) / 1024));
    return true;
}

// B360: test (test_overeni/b360) si sem dosadi "poruchu" jadra hrace 2 - v appce je to prazdne (nic se neprelozi).
// B362: vola se i pri oprave odhadu (snimek spocitany znovu = stejna "porucha" jako poprve)
#ifndef NAP_SEGA_ONLINE_TEST_PO_KROKU
#define NAP_SEGA_ONLINE_TEST_PO_KROKU(snimek)
#endif

// =====================================================================================================
//  B362: ODHAD STISKU DRUHEHO + OPRAVA (online/nap_online.h). Rene 24. 11.: "furt je ten online spatny ... testuju
//  na mistni LAN" - oba telefony cekaly na sebe (spicky site obema smery), zpozdeni vyrostlo na 11 snimku. Ted kdyz
//  stisk druheho jeste neprisel, snimek se spocita s odhadem (druhy drzi, co drzel) a PRED nim se ulozi stav jadra
//  (ClownMDEmu_SaveState + ovladace, ~1,1 MB, kruh ODHAD_MAX + 2 - jen kdyz se odhaduje, jinak nic). Kdyz skutecny
//  stisk prijde jiny, stav se vrati a snimky az do ted se spocitaji znovu - bez zvuku a obrazu (g_online_prepocet),
//  na displeji je hned spravny dalsi snimek. Limit odhadu podle rychlosti jadra (pomaly telefon = mene snimku znovu).
//  Jen vlakno emulace.
// =====================================================================================================
struct NapSegaZaloha {
    uint32_t snimek = UINT32_MAX;
    ClownMDEmu_StateBackup stav;
    ControllerManager ovladace;
};
static const uint32_t NAP_SEGA_KRUH = nap::online::ODHAD_MAX + 2;   // oprava je nejvys ODHAD_MAX + 1 snimku
static std::unique_ptr<NapSegaZaloha> g_online_kruh[NAP_SEGA_KRUH];
static uint64_t g_online_jadro_ema_us = 8000;                      // prumerna doba snimku jadra (vlakno emulace)
static std::atomic<uint32_t> g_online_odhad_max{nap::online::ODHAD_MAX};
static std::atomic<uint32_t> g_online_chyb_opravy{0};
static std::atomic<uint32_t> g_online_navrat_ze{0}, g_online_navrat_na{0};   // hrac 2: synchronizace ho vratila zpet
static uint64_t g_online_vykon_od_us = 0, g_online_vykon_jadro_us = 0, g_online_vykon_jadro_max_us = 0;
static uint32_t g_online_vykon_snimku = 0;
// B363: SETRNE KRESLENI (Rene 25. 11.: "proc treba na slabsi telefony nepridat optimalizaci? Pri hrani online ...
// stacilo by jen ponizit grafiku"). Jadro musi na obou telefonech pocitat KAZDY snimek (jinak by se hra rozjela),
// ale obraz se kreslit nemusi kazdy: kdyz jadro bere vic nez ~9 ms z 16,7 ms, jde na displej jen kazdy 2. snimek
// (30/s - stejne jako rezim LOW / S8 bez hladoveni) - prumer za 2 s (kratka spicka ho nezapne); vypne se po 6 s pod
// 6 ms. Java ho zapise do logu (onlineStav).
static std::atomic<int> g_online_setrne{0};
static std::atomic<uint32_t> g_online_setrne_c{0};       // kolikrat se zaplo
static std::atomic<uint32_t> g_online_setrne_ms10{0};    // jadro (0,1 ms) pri zapnuti - do logu
static int g_online_klidnych_oken = 0;                  // 2s oken po sobe pod 8 ms (vypnout po 3)

static void nap_sega_online_uloz(uint32_t snimek) {
    std::unique_ptr<NapSegaZaloha> &z = g_online_kruh[snimek % NAP_SEGA_KRUH];
    if (!z) z.reset(new (std::nothrow) NapSegaZaloha());
    if (!z) return;   // bez pameti: oprava tohoto snimku nepujde -> otisky to poznaji, synchronizace to spravi
    std::lock_guard<std::mutex> lock(g_real_mutex);
    ClownMDEmu_SaveState(&g_real.emu, &z->stav);
    std::memcpy(&z->ovladace, &g_real.emu.controller_manager, sizeof(ControllerManager));
    z->snimek = snimek;
}

static bool nap_sega_online_vrat(uint32_t snimek) {
    std::unique_ptr<NapSegaZaloha> &z = g_online_kruh[snimek % NAP_SEGA_KRUH];
    if (!z || z->snimek != snimek) return false;
    std::lock_guard<std::mutex> lock(g_real_mutex);
    ClownMDEmu_LoadState(&g_real.emu, &z->stav);
    std::memcpy(&g_real.emu.controller_manager, &z->ovladace, sizeof(ControllerManager));
    nap_real_obnov_paletu_locked();
    return true;
}

/** Kruh stavu: zapomenout (novy start / synchronizace), uvolnit = konec online hry (az 11 MB zpet systemu). */
static void nap_sega_online_kruh_zahod(bool uvolnit) {
    for (auto &z : g_online_kruh) {
        if (uvolnit) z.reset();
        else if (z) z->snimek = UINT32_MAX;
    }
}

/** Otisk stavu po snimku (kazdych 60) - i pri oprave (relace ho odesle, az je snimek jisty). */
static void nap_sega_online_otisk(const std::shared_ptr<nap::online::Relace> &r, uint32_t snimek) {
    if (!r || snimek % nap::online::HASH_KAZDYCH != 0) return;
    uint64_t c[NAP_SEGA_CASTI];
    const uint64_t h = nap_sega_otisk_stavu_casti(c);
    r->otiskStavu(snimek, h, c, (uint8_t)NAP_SEGA_CASTI);
}

/** Oprava: stav pred snimkem 'od' zpet, snimky od .. g_online_snimek-1 znovu se skutecnymi (a novymi odhadnutymi) stisky. */
static void nap_sega_online_oprav(const std::shared_ptr<nap::online::Relace> &r, uint32_t od) {
    const uint32_t ted = g_online_snimek;
    if (od >= ted) return;
    if (!nap_sega_online_vrat(od)) {
        // nemelo by nastat (stav se uklada pred kazdym odhadnutym snimkem) - otisky rozdil poznaji, synchronizace spravi
        if (g_online_chyb_opravy.fetch_add(1) < 5) NAPLOG("B362 ONLINE oprava od snimku %u: ulozeny stav chybi", od);
        return;
    }
    const auto t0 = std::chrono::steady_clock::now();
    const int zachyt = g_render_capture_current.load(), displej = g_render_display_current.load();
    g_render_capture_current.store(0);   // snimky opravy se nekresli (nap_real_scanline_rendered hned konci)
    g_render_display_current.store(0);
    g_online_prepocet.store(1);
    uint32_t f = od;
    for (; f < ted; f++) {
        uint32_t p1 = 0, p2 = 0;
        bool odhad = false;
        if (r->vstupyOdhad(f, p1, p2, odhad, 0, nap::online::ODHAD_OPRAVA) != 1) break;   // konec relace
        if (odhad) nap_sega_online_uloz(f);
        g_online_vstup[0].store(p1);
        g_online_vstup[1].store(p2);
        ClownMDEmu_Iterate(&g_real.emu);
        NAP_SEGA_ONLINE_TEST_PO_KROKU(f);
        nap_sega_online_otisk(r, f);
    }
    g_online_prepocet.store(0);
    g_render_capture_current.store(zachyt);
    g_render_display_current.store(displej);
    if (f < ted) g_online_snimek = f;   // relace skoncila uprostred - dal se stejne nehraje
    r->opraveno(od, f, (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count());
}

/**
 * Vykon jadra za online hry (po kazdem snimku, vlakno emulace): limit odhadu (oprava nejvys ~66 ms pocitani) a
 * kazde 2 s text pro druhy telefon (STATISTIKA) - "59.9 snimku/s, jadro 7.6 ms (nejvys 25.3), odhad nejvys 8".
 */
static void nap_sega_online_vykon(uint64_t iterNs) {
    const uint64_t us = iterNs / 1000;
    g_online_jadro_ema_us = (g_online_jadro_ema_us * 15 + us) / 16;
    uint32_t m = (uint32_t)(66000 / std::max<uint64_t>(g_online_jadro_ema_us, 1000));
    m = std::max<uint32_t>(2, std::min<uint32_t>(nap::online::ODHAD_MAX, m));
    g_online_odhad_max.store(m);
    const uint64_t ted = nap::online::tedUs();

    if (!g_online_vykon_od_us) g_online_vykon_od_us = ted;
    g_online_vykon_snimku++;
    g_online_vykon_jadro_us += us;
    if (us > g_online_vykon_jadro_max_us) g_online_vykon_jadro_max_us = us;
    if (ted - g_online_vykon_od_us >= 2000000) {
        // B363: setrne kresleni (obraz 30/s) - prumer jadra za 2 s nad 12 ms (z 16,7 ms, B364: do B363 od 9 ms);
        // vypnout po 3 oknech pod 8 ms
        const uint64_t prumer = g_online_vykon_jadro_us / std::max<uint32_t>(g_online_vykon_snimku, 1);
        if (!g_online_setrne.load()) {
            if (prumer > 12000 && g_online_snimek > 120) {
                g_online_setrne.store(1);
                g_online_setrne_c.fetch_add(1);
                g_online_setrne_ms10.store((uint32_t)(prumer / 100));
                g_online_klidnych_oken = 0;
                NAPLOG("B363 ONLINE SETRNE KRESLENI zapnuto: jadro %.1f ms na snimek - obraz jen kazdy 2. snimek", prumer / 1000.0);
            }
        } else if (prumer < 8000) {
            if (++g_online_klidnych_oken >= 3) {
                g_online_setrne.store(0);
                NAPLOG("B363 ONLINE SETRNE KRESLENI vypnuto: jadro %.1f ms", prumer / 1000.0);
            }
        } else {
            g_online_klidnych_oken = 0;
        }
        char b[160];
        std::snprintf(b, sizeof(b), "%.1f snimku/s%s, jadro %.1f ms (nejvys %.1f), odhad nejvys %u",
                      g_online_vykon_snimku * 1e6 / (double)(ted - g_online_vykon_od_us), g_online_setrne.load() ? " (obraz 30/s setrne)" : "",
                      g_online_vykon_jadro_us / 1000.0 / std::max<uint32_t>(g_online_vykon_snimku, 1), g_online_vykon_jadro_max_us / 1000.0, m);
        std::shared_ptr<nap::online::Relace> r = g_online.relace();
        if (r) r->vykon(b);
        g_online_vykon_od_us = ted;
        g_online_vykon_snimku = 0;
        g_online_vykon_jadro_us = g_online_vykon_jadro_max_us = 0;
    }
}

// Vlakno emulace zjistilo konec online hry (relace r skoncila, nebo bez relace spojeni skoncilo). Pod g_online_rizeni
// se overi, ze je to porad TATAZ hra - Java mohla mezitim zalozit novou (pak nic).
static void nap_sega_online_konec(const std::shared_ptr<nap::online::Relace> &r) {
    std::lock_guard<std::mutex> rz(g_online_rizeni);
    if (r ? g_online.relace() != r : g_online.faze() != nap::online::OnlineKlient::Faze::KONEC) return;
    std::string proc = g_online.chyba();
    // B360: vlakno emulace konec casto pozna DRIV, nez vlakno site zapise text (Rene 23. 11., pokus 2: v logu jen
    // "online skoncilo", ve skutecnosti 15 s nic od druheho) - text konce primo z relace, stejny jako zapise sit
    if (proc.empty() && r) proc = r->textKonce();
    {
        std::lock_guard<std::mutex> l(g_online_m);
        g_online_konec = proc.empty() ? "online skoncilo" : proc;
    }
    g_online_hraje.store(0);
    g_online_aktivni.store(0);
    g_online_setrne.store(0);           // B363
    nap_sega_online_kruh_zahod(true);   // B362: stavy pro opravu uz netreba
    NAPLOG("B357 ONLINE konec po %u snimcich: %s", g_online_snimek, proc.c_str());
}

// Pred krokem jadra: 1 = emulovat s vstupy z relace, 0 = ted ne (vstup druheho hrace jeste neprisel), -1 = bez online.
static int nap_sega_online_pred_krokem() {
    if (!g_online_aktivni.load()) return -1;
    if (g_online.vezmiStart()) {
        {
            std::lock_guard<std::mutex> lock(g_real_mutex);
            nap_real_studeny_start_locked();
        }
        g_online_snimek = 0;
        g_online_vstup[0].store(0);
        g_online_vstup[1].store(0);
        nap_sega_online_kruh_zahod(false);   // B362
        g_online_vykon_od_us = 0;
        g_online_vykon_snimku = 0;
        g_online_vykon_jadro_us = g_online_vykon_jadro_max_us = 0;
        g_online_navrat_ze.store(0);
        g_online_navrat_na.store(0);
        g_online_setrne.store(0);   // B363
        g_online_klidnych_oken = 0;
        g_online_hraje.store(1);
        NAPLOG("B357 ONLINE start - studeny start, snimek 0 (B362: odhad stisku druheho nejvys %u snimku)", g_online_odhad_max.load());
    }
    if (!g_online_hraje.load()) {
        if (g_online.faze() == nap::online::OnlineKlient::Faze::KONEC) { nap_sega_online_konec(nullptr); return -1; }
        // ceka se na druheho hrace: hra zatim bezi normalne jen na tomto telefonu (obraz zije - hlidac obrazu
        // v Jave by jinak hru po nahrani restartoval); po domluve obou telefonu studeny start od snimku 0
        return -1;
    }
    std::shared_ptr<nap::online::Relace> r = g_online.relace();
    if (!r) return 0;
    r->lokalni(g_online_snimek, nap_sega_lokalni_bity());
    // B362: prisly skutecne stisky druheho pro snimky spocitane s odhadem? Nesedi-li, oprava (stav zpet, znovu)
    const uint32_t opravitOd = r->zkontrolujOdhady(g_online_snimek);
    if (opravitOd != UINT32_MAX) nap_sega_online_oprav(r, opravitOd);
    // B360: synchronizace stavu po rozjeti - hrac 1 posle stav pred snimkem G, hrac 2 ho pred G nahraje
    if (r->hrac1()) {
        if (r->chciPoslatStav(g_online_snimek)) {
            std::vector<uint8_t> st;
            if (nap_sega_stav_zabal(st)) {
                r->posliStav(g_online_snimek, st);
                NAPLOG("B360 ONLINE synchronizace: stav jadra pred snimkem %u odeslan (%zu B)", g_online_snimek, st.size());
            } else {
                r->ukonci(nap::online::D_ROZJETO);   // stav nejde zabalit - hra zustane rozjeta, konec (oba vedi proc)
            }
        }
    } else {
        std::vector<uint8_t> st;
        uint32_t g = 0;
        const int k = r->stavPro(g_online_snimek, st, g, 30);
        if (k < 0) return 0;   // stav od hrace 1 jeste neprisel - emulator stoji
        if (k > 0) {
            if (nap_sega_stav_nahraj(st)) {
                // B362: s odhadem mohl hrac 2 dojit ZA snimek G, nez RESYNC prisel - stav G ho vrati zpet na G
                const uint32_t bylo = g_online_snimek;
                g_online_snimek = g;
                nap_sega_online_kruh_zahod(false);
                r->stavNahran(g);
                if (bylo > g) { g_online_navrat_ze.store(bylo); g_online_navrat_na.store(g); }
                NAPLOG("B360 ONLINE synchronizace: stav jadra pred snimkem %u nahran (%zu B)%s%s", g, st.size(),
                       bylo > g ? " - B362: vracen ze snimku " : "", bylo > g ? std::to_string(bylo).c_str() : "");
            } else {
                r->ukonci(nap::online::D_ROZJETO);   // poskozeny / cizi stav (jine jadro) - nenahran, konec (oba vedi proc)
            }
        }
    }
    uint32_t p1 = 0, p2 = 0;
    bool odhad = false;
    // B362: stisk druheho jeste neprisel -> snimek s odhadem (nejvys g_online_odhad_max napred), stav pred nim ulozit
    const int v = r->vstupyOdhad(g_online_snimek, p1, p2, odhad, 30, g_online_odhad_max.load());
    if (v == 0) return 0;
    if (v < 0) { nap_sega_online_konec(r); return -1; }
    if (odhad) nap_sega_online_uloz(g_online_snimek);
    g_online_vstup[0].store(p1);
    g_online_vstup[1].store(p2);
    return 1;
}

static void nap_sega_online_po_kroku() {
    NAP_SEGA_ONLINE_TEST_PO_KROKU(g_online_snimek);
    nap_sega_online_otisk(g_online.relace(), g_online_snimek);   // B362: snimek s odhadem - relace otisk podrzi
    g_online_snimek++;
    g_online_snimku_celkem.store(g_online_snimek);
}

static void nap_native_worker_set_priority_rp() {
    int niceResult = 999;
    errno = 0;
    // Best effort only: Android may deny negative nice for normal apps. Log result, never fail gameplay.
    if (setpriority(PRIO_PROCESS, 0, -8) == 0) niceResult = -8;
    else niceResult = errno ? errno : -1;
    g_worker_priority_nice_result.store(niceResult);
}

static void* nap_real_worker_thread_entry(void *arg) {
    int generation = (int)(intptr_t)arg;
    nap_real_worker_thread(generation);
    return nullptr;
}

static void nap_real_worker_thread(int generation) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    NAPLOG("BUILD2RY real core worker start gen=%d bigstack=8MB passiveAudit=YES pcmCddaSkip=YES", generation);
    nap_native_worker_set_priority_rp();
    try {
        static bool constants_ready = false;
        {
            std::lock_guard<std::mutex> lock(g_real_mutex);
            if (!constants_ready) {
                ClownMDEmu_Constant_Initialise();
                constants_ready = true;
            }
            std::memset(&g_real.emu, 0, sizeof(g_real.emu));
            nap_real_setup_cfg_callbacks();
            ClownMDEmu_Initialise(&g_real.emu, &g_real.cfg, &g_real.cb);
            ClownMDEmu_SetCartridge(&g_real.emu, g_real.cart_words.data(), (cc_u32f)g_real.cart_words.size());
            ClownMDEmu_HardReset(&g_real.emu, cc_true, cc_false);
            g_real.loaded = true;
            g_real.frame_ready = false;
            g_real.frame_counter = 0;
            g_real.iterations_last = 0;
            g_real.status = "REAL_CORE_LOAD_OK_WORKER_THREAD generation=" + std::to_string(generation) + " region=" + g_real_cfg_region_label;
        }

        auto next_tick = std::chrono::steady_clock::now();
        const std::chrono::nanoseconds frame_period =
                (g_real_cfg_tv == CLOWNMDEMU_TV_STANDARD_PAL)
                ? std::chrono::nanoseconds(20000000LL)      // 50.000 Hz PAL
                : std::chrono::nanoseconds(16666667LL);     // 59.94/60 Hz NTSC practical Android clock
        while (g_real_thread_run.load() != 0 && generation == g_real_thread_generation.load()) {
            next_tick += frame_period;
            if (g_real.loaded) {
                // B357: online - snimek az se vstupy obou hracu (ceka se v relaci); start online = studeny start
                const int online = nap_sega_online_pred_krokem();
                if (online == 0) { next_tick = std::chrono::steady_clock::now(); continue; }
                // BUILD2RV: LOW/HIGH is video-only. HIGH captures every frame; LOW presents ~30 fps.
                // Audio frontend timing and FIFO sizes stay identical in both modes.
                uint32_t preIt = g_real_thread_iterations.load();
                int divisor = g_native_perf_low.load() ? 2 : 1;
                if (online == 1 && g_online_setrne.load()) divisor = std::max(divisor, 2);   // B363: setrne kresleni za online
                bool audioHungry = false;
                // BUILD2RV: S8 no-starve mode reduces native scanline capture when audio FIFO is low.
                // The WebView/TextureView can keep presenting the cached frame, while the core spends
                // less time copying video and more time producing YM/PSG audio.
                if (nap_mobile_no_starve()) {
                    size_t aq = nap_audio_min_fifo();
                    audioHungry = aq < nap_audio_low_water() / 2u;
                    if (audioHungry) divisor = 4;
                    else divisor = std::max(divisor, 2);
                }
                bool displayFrame = ((preIt % (uint32_t)divisor) == 0u);
                // B306: kdyz se nekdo diva na TV / PC, snima se KAZDY snimek (60/s) - jen pro
                // prenos; displej telefonu dal kazdy druhy (stejna prace jako dosud). Snimani
                // radku je ~0,2 ms z 16,7 ms (jadro na S8 ~8 ms, viz log). Kdyz zvuk hladovi
                // nebo je zvoleny rezim LOW, plati puvodni deleni i pro TV.
                bool tvFrame = !audioHungry && g_native_perf_low.load() == 0 && g_segaProud.klientu() > 0;
                bool captureFrame = displayFrame || tvFrame;
                g_render_capture_current.store(captureFrame ? 1 : 0);
                g_render_display_current.store(displayFrame ? 1 : 0);
                if (!displayFrame) g_render_skipped_frames.fetch_add(1);
                auto iter_start = std::chrono::steady_clock::now();
                ClownMDEmu_Iterate(&g_real.emu);
                if (online == 1) nap_sega_online_po_kroku();   // B357: otisk stavu, dalsi snimek
                g_render_capture_current.store(1);
                g_render_display_current.store(1);
                nap_tv_po_kroku(&g_real);   // B306: snimek + zvuk tehoz kroku do prenosu na TV / PC
                auto iter_end = std::chrono::steady_clock::now();
                uint64_t iter_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(iter_end - iter_start).count();
                nap_scene_stress_record(iter_ns);
                if (online == 1) nap_sega_online_vykon(iter_ns);   // B362: limit odhadu + vykon pro druhy telefon
                nap_audio_clock_governor_sleep();
                if (nap_mobile_no_starve() && nap_audio_min_fifo() < nap_audio_low_water()) {
                    g_audio_starve_events.fetch_add(1);
                }
                g_real.iterations_last++;
                uint32_t it = g_real_thread_iterations.fetch_add(1) + 1;
                if ((it & 31u) == 0u) {
                    uint32_t wc = g_core_iter_window_count.load();
                    uint64_t avg = wc ? (g_core_sum_iter_ns.load() / wc) : 0;
                    uint64_t mx = g_core_max_iter_ns.load();
                    bool stress = (avg > 12000000ULL || mx > 16000000ULL);
                    // BUILD2RV: keep lock order clean. Do not take audio mutex while holding real mutex.
                    size_t audioBacklogForStatus = nap_audio_min_fifo();
                    std::lock_guard<std::mutex> lock(g_real_mutex);
                    g_real.status = "REAL_CORE_RENDER_OK_WORKER_THREAD_RV frameCounter=" + std::to_string(g_real.frame_counter) +
                                    " iterateCount=" + std::to_string(it) +
                                    " src=" + std::to_string(g_real.frame_w) + "x" + std::to_string(g_real.frame_h) +
                                    " frameClockNs=" + std::to_string(frame_period.count()) +
                                    " audioBacklog=" + std::to_string(nap_audio_min_fifo()) +
                                    " coreLastMs=" + std::to_string(iter_ns / 1000000.0) +
                                    " coreAvgMs=" + std::to_string(avg / 1000000.0) +
                                    " coreMaxMs=" + std::to_string(mx / 1000000.0) +
                                    " sceneStress=" + std::string(stress ? "YES" : "NO") +
                                    " perfLow=" + std::to_string(g_native_perf_low.load()) +
                                    " videoCaptured=" + std::to_string(g_render_captured_frames.load()) +
                                    " videoSkipped=" + std::to_string(g_render_skipped_frames.load());
                    if ((it % 300u) == 0u) {
                        g_core_sum_iter_ns.store(0);
                        g_core_iter_window_count.store(0);
                        g_core_max_iter_ns.store(0);
                    }
                }
            }
            auto now = std::chrono::steady_clock::now();
            if (nap_mobile_no_starve() && nap_audio_min_fifo() < nap_audio_low_water() && g_real.loaded) {
                // BUILD2RV: no-starve catchup. If S8 AudioTrack is hungry, do not wait for the next
                // video tick; run the next core iteration immediately. This is capped naturally by
                // the core cost and avoids the RQ pattern: pretty video, empty FM FIFO, chirping sound.
                g_audio_catchup_iterations.fetch_add(1);
                next_tick = now;
                sched_yield();
                continue;
            }
            if (now + frame_period < next_tick) {
                // system clock jump / suspend guard
                next_tick = now;
            } else if (now < next_tick) {
                std::this_thread::sleep_until(next_tick);
            } else {
                // BUILD2RV: RF-style clock guard. If phone is late, resync; do not run extra frames that make Sonic rhythm drift.
                if (now - next_tick > frame_period) next_tick = now;
                sched_yield();
            }
        }
    } catch (const std::exception &e) {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        g_real.status = std::string("REAL_CORE_WORKER_CPP_EXCEPTION ") + e.what();
    } catch (...) {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        g_real.status = "REAL_CORE_WORKER_UNKNOWN_EXCEPTION";
    }
    g_real_thread_alive.store(0);
    NAPLOG("BUILD2RY real core worker stop gen=%d", generation);
#endif
}

static void nap_render_guard_frame(int width, int height, jintArray argbOut, JNIEnv* env) {
    if (!argbOut || width <= 0 || height <= 0) return;
    jsize len = env->GetArrayLength(argbOut);
    int needed = width * height;
    if (len < needed) return;
    std::vector<jint> px((size_t)needed, (jint)0xff06142au);
    // BUILD2RV: blank guarded monitor. No running cubes, no center square, no hash bars.
    // Subtle blue border only shows the native view is alive and placed correctly.
    auto put = [&](int x0,int y0,int rw,int rh,uint32_t c){
        int x1=std::max(0,x0), y1=std::max(0,y0), x2=std::min(width,x0+rw), y2=std::min(height,y0+rh);
        for(int y=y1;y<y2;++y) for(int x=x1;x<x2;++x) px[(size_t)y*width+x]=(jint)c;
    };
    put(0,0,width,5,0xff2bb6ffu); put(0,height-5,width,5,0xff2bb6ffu);
    put(0,0,5,height,0xff2bb6ffu); put(width-5,0,5,height,0xff2bb6ffu);
    for(int y=8;y<height-8;y+=18){ for(int x=8;x<width-8;x+=18){ if(((x+y)/18)&1) put(x,y,8,8,0xff0b2d55u); } }
    env->SetIntArrayRegion(argbOut, 0, needed, px.data());
}

#endif


static std::string ascii_field(const uint8_t* data, size_t size, size_t off, size_t len) {
    std::string s;
    if (off >= size) return "";
    size_t end = std::min(size, off + len);
    for (size_t i = off; i < end; ++i) {
        unsigned char c = data[i];
        s.push_back((c >= 32 && c < 127) ? (char)c : ' ');
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

static uint16_t md_checksum(const uint8_t* data, size_t size) {
    uint32_t sum = 0;
    for (size_t i = 0x200; i + 1 < size; i += 2) {
        sum = (sum + ((uint16_t(data[i]) << 8) | uint16_t(data[i + 1]))) & 0xffff;
    }
    return uint16_t(sum & 0xffff);
}

static uint32_t fnv1a32(const uint8_t* data, size_t size) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}


// ===================================================================
//  SEGA: OBRAZ PRIMO NA PLOCHU  (stejna cesta jako PS1)
//
//  Jadro clownmdemu uz snimek vyrabi - kresli po radcich do
//  g_real.frame_argb (ARGB, hotovy). Doted si ho ale nikdo nebral
//  a obraz se na TV snimal z okna aplikace, coz je pomale.
//
//  Tady je stejna kostra jako u PS1:
//    jadro -> frame_argb -> GL textura -> obrazovka
//  Vlastni EGL kontext, vlastni vlakno, zadne sdileni s jadrem.
//  Snimek se pod zamkem zkopiruje, aby ho vlakno emulace nemohlo
//  prepsat uprostred kresleni (na tomhle spadla PS1 v B94).
// ===================================================================
static std::mutex          g_sega_frame_mutex;
static ANativeWindow      *g_sega_win = nullptr;
static std::thread         g_sega_thread;
static std::atomic<bool>   g_sega_run{false};
static std::atomic<long>   g_sega_gen{0};

static const char *SEGA_VS =
    "attribute vec2 aPos;\n attribute vec2 aUV;\n varying vec2 vUV;\n"
    "void main(){ vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
static const char *SEGA_FS =
    "precision mediump float;\n varying vec2 vUV;\n uniform sampler2D uTex;\n"
    /* snimek je ARGB (v pameti B,G,R,A), GL ho cte jako RGBA -> prohodit */
    "void main(){ gl_FragColor = vec4(texture2D(uTex, vUV).bgr, 1.0); }\n";

static GLuint nap_sega_shader(GLenum typ, const char *src) {
    GLuint sh = glCreateShader(typ);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { glDeleteShader(sh); return 0; }
    return sh;
}

/* Bezpecne prevzeti snimku - kopie pod zamkem do pameti volajiciho. */
static int nap_sega_kopiruj(std::vector<uint32_t> &kam, int *w, int *h) {
    std::lock_guard<std::mutex> lk(g_sega_frame_mutex);
    int vw = g_real.frame_w, vh = g_real.frame_h;
    if (vw <= 0 || vh <= 0) return 0;
    if (g_real.frame_argb.size() < (size_t)(vw * vh)) return 0;
    if (kam.size() < (size_t)(vw * vh)) kam.resize((size_t)vw * vh);
    memcpy(kam.data(), g_real.frame_argb.data(), (size_t)vw * vh * 4);
    if (w) *w = vw;
    if (h) *h = vh;
    return 1;
}

static void nap_sega_thread_fn(ANativeWindow *win, long gen) {
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint ma = 0, mi = 0;
    if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &ma, &mi)) {
        ANativeWindow_release(win); return;
    }
    const EGLint cfga[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                            EGL_NONE };
    EGLConfig cfg; EGLint n = 0;
    if (!eglChooseConfig(dpy, cfga, &cfg, 1, &n) || n < 1) {
        ANativeWindow_release(win); return;
    }
    /* format okna musi odpovidat konfiguraci - bez toho to pada (viz PS1 B96) */
    EGLint fmt = 0;
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &fmt);
    ANativeWindow_setBuffersGeometry(win, 0, 0, fmt);

    EGLSurface surf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)win, nullptr);
    const EGLint ctxa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa);
    if (surf == EGL_NO_SURFACE || ctx == EGL_NO_CONTEXT ||
        !eglMakeCurrent(dpy, surf, surf, ctx)) {
        ANativeWindow_release(win); return;
    }

    GLuint vs = nap_sega_shader(GL_VERTEX_SHADER, SEGA_VS);
    GLuint fs = nap_sega_shader(GL_FRAGMENT_SHADER, SEGA_FS);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint aPos = glGetAttribLocation(prog, "aPos");
    GLint aUV  = glGetAttribLocation(prog, "aUV");
    GLint uTex = glGetUniformLocation(prog, "uTex");

    GLuint tex = 0; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint32_t> muj;
    int lastW = 0, lastH = 0; long snimku = 0;
    bool cerna = false;
    while (g_sega_run.load() && g_sega_gen.load() == gen) {
        /* B338: VYKON LOW (slabe zarizeni - TV TCL) - obraz kresli TextureView (320x224, zvetsuje grafika).
           Tahle druha cesta byla pod nim neviditelna a stala 60x za sekundu kopii snimku, nahrani textury
           a kresleni cele obrazovky. V LOW jednou zcerna (pruhy vedle obrazu) a stoji. */
        if (g_native_perf_low.load()) {
            if (!cerna) {
                int cw = ANativeWindow_getWidth(win), ch = ANativeWindow_getHeight(win);
                if (cw > 0 && ch > 0) {
                    glViewport(0, 0, cw, ch);
                    glClearColor(0.f, 0.f, 0.f, 1.f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    if (!eglSwapBuffers(dpy, surf)) break;
                    cerna = true;
                }
            }
            usleep(50000);
            continue;
        }
        cerna = false;
        int sw = 0, sh = 0;
        if (!nap_sega_kopiruj(muj, &sw, &sh)) { usleep(8000); continue; }

        int w = ANativeWindow_getWidth(win), h = ANativeWindow_getHeight(win);
        if (w <= 0 || h <= 0) { usleep(8000); continue; }

        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        if (sw != lastW || sh != lastH) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, muj.data());
            lastW = sw; lastH = sh;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, muj.data());
        }

        /* na sirku vyplnit plochu, na vysku zachovat pomer (jako PS1) */
        int vw, vh;
        if (w > h) { vw = w; vh = h; }
        else {
            vw = w; vh = (int)((long)w * sh / sw);
            if (vh > h) { vh = h; vw = (int)((long)h * sw / sh); }
        }
        glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        {
            /* snimek chodi shora dolu -> V obracene */
            const GLfloat quad[] = { -1.f,-1.f, 0.f,1.f,   1.f,-1.f, 1.f,1.f,
                                     -1.f, 1.f, 0.f,0.f,   1.f, 1.f, 1.f,0.f };
            glUseProgram(prog);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(uTex, 0);
            glVertexAttribPointer((GLuint)aPos, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), quad);
            glVertexAttribPointer((GLuint)aUV,  2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), quad+2);
            glEnableVertexAttribArray((GLuint)aPos);
            glEnableVertexAttribArray((GLuint)aUV);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        if (!g_sega_run.load()) break;
        if (!eglSwapBuffers(dpy, surf)) break;
        snimku++;
        usleep(2000);
    }

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surf);
    ANativeWindow_release(win);
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setDisplaySurface(JNIEnv *env, jclass, jobject jsurf) {
    long moje = g_sega_gen.fetch_add(1) + 1;
    bool bezelo = g_sega_run.exchange(false);
    if (bezelo && g_sega_thread.joinable()) g_sega_thread.detach();
    g_sega_win = nullptr;
    if (jsurf == nullptr) return;
    ANativeWindow *win = ANativeWindow_fromSurface(env, jsurf);
    if (!win) return;
    g_sega_win = win;
    g_sega_run.store(true);
    g_sega_thread = std::thread(nap_sega_thread_fn, win, moje);
}

/* Snimek pro TV - stejne rozhrani jako u PS1, aby ho mohl pouzit
   existujici H.264 enkoder. Vraci sirku<<16|vyska, 0 = neni. */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_grabFrame(JNIEnv *env, jclass, jintArray out) {
    std::lock_guard<std::mutex> lk(g_sega_frame_mutex);
    int w = g_real.frame_w, h = g_real.frame_h;
    if (w <= 0 || h <= 0) return 0;
    size_t need = (size_t)w * h;
    if (g_real.frame_argb.size() < need) return 0;
    if (!out || (size_t)env->GetArrayLength(out) < need) return -((w << 16) | h);
    env->SetIntArrayRegion(out, 0, (jsize)need, (const jint*)g_real.frame_argb.data());
    return (w << 16) | h;
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeCoreBuildString(JNIEnv* env, jclass) {
    std::string s = "BUILD2RY NATIVE C++ CORE O3 + VDP LEFT BLANK OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ native log export: OK\n"
                    "C++ 60Hz timing proof target: OK\n"
                    "Status: C++ only Sega runtime; Java wrapper disabled; QT audio profile + RR zeroed-buffer balanced audio-clock governor; no ROM in APK";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRomInfo(JNIEnv* env, jclass, jbyteArray romBytes) {
    if (!romBytes) return env->NewStringUTF("ROM NULL");
    jsize len = env->GetArrayLength(romBytes);
    std::vector<uint8_t> data((size_t)len);
    env->GetByteArrayRegion(romBytes, 0, len, reinterpret_cast<jbyte*>(data.data()));

    uint32_t fnv = fnv1a32(data.data(), data.size());
    g_lastRomHash = fnv;

    std::ostringstream out;
    out << "NATIVE C++ ROM ANALYZA:\n";
    out << "- bytes: " << len << "\n";
    out << "- FNV1A32: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << fnv << std::dec << "\n";

    if (data.size() >= 0x200) {
        std::string console = ascii_field(data.data(), data.size(), 0x100, 16);
        std::string titleDomestic = ascii_field(data.data(), data.size(), 0x120, 48);
        std::string titleOverseas = ascii_field(data.data(), data.size(), 0x150, 48);
        std::string serial = ascii_field(data.data(), data.size(), 0x180, 14);
        uint16_t stored = (uint16_t(data[0x18e]) << 8) | uint16_t(data[0x18f]);
        uint16_t calc = md_checksum(data.data(), data.size());
        g_lastChecksumStored = stored;
        g_lastChecksumCalc = calc;
        g_lastTitle = titleOverseas.empty() ? titleDomestic : titleOverseas;
        std::string io = ascii_field(data.data(), data.size(), 0x190, 16);
        std::string region = ascii_field(data.data(), data.size(), 0x1f0, 16);
        out << "- Mega Drive header: ANO\n";
        out << "- console: " << console << "\n";
        out << "- title domestic: " << titleDomestic << "\n";
        out << "- title overseas: " << titleOverseas << "\n";
        out << "- serial: " << serial << "\n";
        out << "- IO: " << io << "\n";
        out << "- region: " << region << "\n";
        out << "- checksum stored: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << stored;
        out << " calc: 0x" << std::setw(4) << calc << std::dec << "\n";
        out << "- checksum match: " << (stored == calc ? "YES" : "NO / patched copy or bad dump") << "\n";
    } else {
        g_lastTitle = "NO_HEADER";
        g_lastChecksumStored = 0;
        g_lastChecksumCalc = 0;
        out << "- Mega Drive header: NE / soubor je mensi nez 0x200\n";
    }

    out << "\nBUILD2RV DULEZITE:\n";
    out << "ROM je ted realne prectena v Jave a analyzovana v C++.\n";
    out << "BUILD2RV ma vypnuty proof pattern a chrani real-core load proti padu aplikace.\n";
    out << "Dalsi krok je podle logu opravit konkretni core init/reset/iterate misto dalsich fake patternu.\n";
    std::string s = out.str();
    NAPLOG("BUILD2RV ROM info generated, bytes=%d fnv=0x%08x", len, fnv);
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeSetInput(JNIEnv*, jclass, jint key, jboolean pressed) {
    if (key >= 0 && key < 8) {
        g_input[key] = pressed == JNI_TRUE;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeGetInputStatus(JNIEnv* env, jclass) {
    std::ostringstream out;
    out << "C++ INPUT U" << (g_input[0] ? 1 : 0)
        << " D" << (g_input[1] ? 1 : 0)
        << " L" << (g_input[2] ? 1 : 0)
        << " R" << (g_input[3] ? 1 : 0)
        << " A" << (g_input[4] ? 1 : 0)
        << " B" << (g_input[5] ? 1 : 0)
        << " C" << (g_input[6] ? 1 : 0)
        << " S" << (g_input[7] ? 1 : 0)
        << " hash=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << g_lastRomHash;
    std::string s = out.str();
    return env->NewStringUTF(s.c_str());
}

static void put_rect(std::vector<jint>& px, int w, int h, int x0, int y0, int rw, int rh, uint32_t color) {
    int x1 = std::max(0, x0), y1 = std::max(0, y0);
    int x2 = std::min(w, x0 + rw), y2 = std::min(h, y0 + rh);
    for (int y = y1; y < y2; ++y) {
        for (int x = x1; x < x2; ++x) px[(size_t)y * w + x] = (jint)color;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRenderPattern(JNIEnv* env, jclass, jint width, jint height, jint frame, jintArray argbOut) {
    if (!argbOut || width <= 0 || height <= 0) return;
    jsize len = env->GetArrayLength(argbOut);
    int needed = width * height;
    if (len < needed) return;
#if NAP_SEGA_VENDOR_CORE_PRESENT
    jint* real_px = env->GetIntArrayElements(argbOut, nullptr);
    if (real_px) {
        bool ok = nap_real_render_to_argb((int)width, (int)height, real_px);
        env->ReleaseIntArrayElements(argbOut, real_px, 0);
        if (ok) return;
    }
    nap_render_guard_frame((int)width, (int)height, argbOut, env);
    return;
#endif
    std::vector<jint> px((size_t)needed);

    int joyX = (g_input[3] ? 1 : 0) - (g_input[2] ? 1 : 0);
    int joyY = (g_input[1] ? 1 : 0) - (g_input[0] ? 1 : 0);
    int speed = g_input[7] ? 5 : 2;
    int offset = frame * speed + int(g_lastRomHash & 31u);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int band = ((x + offset) / 16 + y / 28) & 7;
            int r = 0, g = 0, b = 0;
            switch (band) {
                case 0: r = 255; g = 210; b = 60; break;
                case 1: r = 20; g = 160; b = 255; break;
                case 2: r = 30; g = 255; b = 120; break;
                case 3: r = 255; g = 80; b = 70; break;
                case 4: r = 170; g = 120; b = 255; break;
                case 5: r = 255; g = 255; b = 255; break;
                case 6: r = 255; g = 150; b = 30; break;
                default: r = 10; g = 30; b = 80; break;
            }
            if (x < 6 || y < 6 || x >= width - 6 || y >= height - 6 || (x % 32 == 0) || (y % 28 == 0)) {
                r = r / 4; g = g / 4; b = b / 4;
            }
            px[(size_t)y * width + x] = (jint)(0xff000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b));
        }
    }

    int cx = width / 2 - 12 + joyX * 52;
    int cy = height / 2 - 12 + joyY * 38;
    uint32_t col = 0xff00ff99u;
    if (g_input[4]) col = 0xffffff00u;
    if (g_input[5]) col = 0xffff4040u;
    if (g_input[6]) col = 0xff40a0ffu;
    put_rect(px, width, height, cx - 4, cy - 4, 32, 32, 0xff000000u);
    put_rect(px, width, height, cx, cy, 24, 24, col);

    // checksum indicator bars; real ROM hash mění native obraz, aby bylo videt, ze C++ ma data ROM
    int hashBars = int((g_lastRomHash ^ (g_lastRomHash >> 16)) & 0xffu);
    for (int i = 0; i < 8; ++i) {
        if (hashBars & (1 << i)) put_rect(px, width, height, 10 + i * 18, height - 18, 12, 8, 0xffffffffu);
    }

    env->SetIntArrayRegion(argbOut, 0, needed, px.data());
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeMakeAudioTone(JNIEnv* env, jclass, jshortArray pcmOut, jint sampleRate, jdouble hz) {
    if (!pcmOut || sampleRate <= 0 || hz <= 0) return;
    jsize n = env->GetArrayLength(pcmOut);
    std::vector<jshort> pcm((size_t)n);
    const double pi = 3.14159265358979323846;
    double phase = 0.0;
    double step = 2.0 * pi * double(hz) / double(sampleRate);
    for (int i = 0; i < n; ++i) {
        double envAmp = 1.0;
        int fade = std::min<int>(1200, n / 8);
        if (i < fade) envAmp = double(i) / double(fade);
        if (i > n - fade) envAmp = double(n - i) / double(fade);
        double v = std::sin(phase) * 0.22 * envAmp;
        pcm[(size_t)i] = (jshort)std::max(-32767, std::min(32767, int(v * 32767.0)));
        phase += step;
    }
    env->SetShortArrayRegion(pcmOut, 0, n, pcm.data());
}



extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_configureRuntime(JNIEnv* env, jclass, jint sdk, jint cores, jstring model) {
    std::string smodel;
    const char* m = model ? env->GetStringUTFChars(model, nullptr) : nullptr;
    if (m) smodel = m;
    if (model && m) env->ReleaseStringUTFChars(model, m);
    std::string upper;
    for (char c : smodel) upper.push_back((char)std::toupper((unsigned char)c));
    bool s8 = (sdk > 0 && sdk <= 28) || (upper.find("SM-G950") != std::string::npos) || (upper.find("S8") != std::string::npos);
    g_android_sdk.store((int)sdk);
    g_android_cores.store((int)cores);
    g_mobile_no_starve.store(s8 ? 1 : 0);
    std::ostringstream out;
    out << "NATIVE_RUNTIME_OK_RV sdk=" << sdk << " cores=" << cores << " model=" << smodel
        << " s8NoStarve=" << (s8 ? "YES" : "NO")
        << " target=" << nap_audio_target_fifo() << " max=" << nap_audio_max_fifo() << " lowWater=" << nap_audio_low_water();
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setPerformanceMode(JNIEnv* env, jclass, jstring mode) {
    const char* m = mode ? env->GetStringUTFChars(mode, nullptr) : nullptr;
    std::string sm = m ? m : "HIGH";
    if (mode && m) env->ReleaseStringUTFChars(mode, m);
    std::string upper;
    for (char c : sm) upper.push_back((char)std::toupper((unsigned char)c));
    int low = (upper.find("LOW") != std::string::npos) ? 1 : 0;
    g_native_perf_low.store(low);
    std::ostringstream out;
    out << "NATIVE_PERF_MODE_OK_RV mode=" << (low ? "LOW" : "HIGH")
        << " target=" << nap_audio_target_fifo()
        << " max=" << nap_audio_max_fifo()
        << " lowWater=" << nap_audio_low_water()
        << " s8NoStarve=" << g_mobile_no_starve.load();
    return env->NewStringUTF(out.str().c_str());
}

// BUILD2RV: same native core exposed to MainActivity/WebView in-place bridge.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_buildString(JNIEnv* env, jclass) {
    std::string s = "BUILD2RY NATIVE C++ CORE O3 + VDP LEFT BLANK OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ guarded render: OK\n"
                    "Status: normal Sega UI; Java wrapper disabled; RR zeroed-buffer balanced audio-clock governor + native visible-frame guard; no ROM in APK";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_romInfo(JNIEnv* env, jclass, jbyteArray romBytes) {
    return Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRomInfo(env, nullptr, romBytes);
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_renderPattern(JNIEnv* env, jclass, jint width, jint height, jint frame, jintArray argbOut) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    if (argbOut && width > 0 && height > 0) {
        jsize len = env->GetArrayLength(argbOut);
        int needed = width * height;
        if (len >= needed) {
            // BUILD2RV: mobile video fast path. Do not allocate a std::vector every frame and do not copy twice.
            // Java TextureView owns one 320x224 int[]; native writes directly into it.
            jint* out = env->GetIntArrayElements(argbOut, nullptr);
            if (out) {
                bool ok = nap_real_render_to_argb((int)width, (int)height, out);
                env->ReleaseIntArrayElements(argbOut, out, 0);
                if (ok) return;
            }
            nap_render_guard_frame((int)width, (int)height, argbOut, env);
            return;
        }
    }
#endif
    Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRenderPattern(env, nullptr, width, height, frame, argbOut);
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setInput(JNIEnv* env, jclass, jint key, jboolean pressed) {
    Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeSetInput(env, nullptr, key, pressed);
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_inputStatus(JNIEnv* env, jclass) {
    return Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeGetInputStatus(env, nullptr);
}

// ===== B320: HERNI OVLADAC - JNI (Java NapOvladac / MainActivity) =====
/** Klavesa z herniho ovladace (KeyEvent.KEYCODE_*). Vraci logicka tlacitka
 *  (nap_ovladac.h T_*), 0 = neni klavesa ovladace (Java ji pusti dal). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesaHrac(JNIEnv*, jclass, jint hrac, jint keyCode, jboolean dolu) {
    std::lock_guard<std::mutex> l(g_ovl_m);
    if (hrac == 1) {                            // B354: hrac 2
        const uint32_t t2 = nap::ovladac::klavesa(g_ovl_stav2, keyCode, dolu == JNI_TRUE);
        if (t2) g_ovl_bity2.store(nap::ovladac::segaMapuj(g_ovl_stav2.vse(), g_ovl_mapa), std::memory_order_relaxed);
        return (jint)t2;
    }
    const uint32_t t = nap::ovladac::klavesa(g_ovl_stav, keyCode, dolu == JNI_TRUE);
    if (t) {
        g_ovl_bity.store(nap::ovladac::segaMapuj(g_ovl_stav.vse(), g_ovl_mapa), std::memory_order_relaxed);
        if (g_ovl_udalosti.fetch_add(1, std::memory_order_relaxed) < 10)
            NAPLOG("B320 OVLADAC klavesa %d %s -> %s -> Sega bity 0x%03x", (int)keyCode, dolu ? "dolu" : "nahoru",
                   nap::ovladac::jmena(t).c_str(), (unsigned)nap::ovladac::segaMapuj(t, g_ovl_mapa));
    }
    return (jint)t;
}
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesa(JNIEnv* env, jclass c, jint keyCode, jboolean dolu) {
    return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesaHrac(env, c, 0, keyCode, dolu);
}
/** Osy: leva packa x y, prava x y, krizek HAT x y, spoust L2 R2 + maska (nap_ovladac.h). B354: hrac 0 / 1. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsyHrac(JNIEnv* env, jclass, jint hrac, jfloatArray josy, jint maska) {
    float o[nap::ovladac::O_POCET] = {0};
    if (josy) {
        const jsize n = env->GetArrayLength(josy);
        env->GetFloatArrayRegion(josy, 0, n < nap::ovladac::O_POCET ? n : nap::ovladac::O_POCET, o);
    }
    std::lock_guard<std::mutex> l(g_ovl_m);
    if (hrac == 1) {
        nap::ovladac::osy(g_ovl_stav2, o, maska);
        g_ovl_bity2.store(nap::ovladac::segaMapuj(g_ovl_stav2.vse(), g_ovl_mapa), std::memory_order_relaxed);
        return;
    }
    nap::ovladac::osy(g_ovl_stav, o, maska);
    g_ovl_bity.store(nap::ovladac::segaMapuj(g_ovl_stav.vse(), g_ovl_mapa), std::memory_order_relaxed);
}
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsy(JNIEnv* env, jclass c, jfloatArray josy, jint maska) {
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsyHrac(env, c, 0, josy, maska);
}
/** Vse pustit (ovladac odpojen, odchod ze Segy). B354: oba hraci. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacPust(JNIEnv*, jclass) {
    std::lock_guard<std::mutex> l(g_ovl_m);
    nap::ovladac::pustVse(g_ovl_stav);
    g_ovl_bity.store(0, std::memory_order_relaxed);
    nap::ovladac::pustVse(g_ovl_stav2);
    g_ovl_bity2.store(0, std::memory_order_relaxed);
}
/** B355: mapa tlacitek z Javy (NapOvladacMapa, "A=B;B=C;X=A;..."; "" = vychozi). Vraci mapu tak, jak ji
 *  jadro pouziva (nezname dvojice zustanou vychozi) - Java ji da do logu. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacMapa(JNIEnv* env, jclass, jstring jmapa) {
    std::string s;
    if (jmapa) {
        const char* c = env->GetStringUTFChars(jmapa, nullptr);
        if (c) { s.assign(c, strnlen(c, 4096)); env->ReleaseStringUTFChars(jmapa, c); }
    }
    std::string norm;
    {
        std::lock_guard<std::mutex> l(g_ovl_m);
        g_ovl_mapa = nap::ovladac::mapaZRetezce(nap::ovladac::J_SEGA, s);
        g_ovl_bity.store(nap::ovladac::segaMapuj(g_ovl_stav.vse(), g_ovl_mapa), std::memory_order_relaxed);
        g_ovl_bity2.store(nap::ovladac::segaMapuj(g_ovl_stav2.vse(), g_ovl_mapa), std::memory_order_relaxed);
        norm = nap::ovladac::mapaNaRetezec(g_ovl_mapa);
    }
    return env->NewStringUTF(norm.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_makeAudioTone(JNIEnv* env, jclass, jshortArray pcmOut, jint sampleRate, jdouble hz) {
    Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeMakeAudioTone(env, nullptr, pcmOut, sampleRate, hz);
}

// BUILD2RV REAL CORE ADAPTER SLOT
// This stage imports local vendored ClownMDEmu-core sources and links only interpreter/core libraries into the native .so.
#ifndef NAP_SEGA_VENDOR_CORE_PRESENT
#define NAP_SEGA_VENDOR_CORE_PRESENT 0
#endif

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_realCoreStatus(JNIEnv* env, jclass) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    std::ostringstream out;
    out << "REAL_CORE_PRESENT=YES\n";
    out << "core=ClownMDEmu-core\n";
    out << "vendor_offline=local ZIP sources; no FetchContent; no tools/tests\n";
    out << "mode=real core runs on dedicated native worker pthread with 8MB stack; Android TextureView copies cached 320x224 frame off WebView UI thread\n";
    out << "REAL_CORE_WORKER_ALIVE=" << (g_real_thread_alive.load() ? "YES" : "NO") << "\n";
    out << "nativeRegionAuto=" << g_real_cfg_region_label << "\n";
    out << nap_region_clock_status() << "\n";       // BUILD2RW passive: real clock the core is configured with
    out << nap_audit_frame_status() << "\n";        // BUILD2RW passive: left-edge/stride audit
    out << nap_audit_native_heap_status() << "\n";  // BUILD2RW passive: native heap growth watch
    { std::lock_guard<std::mutex> alock(g_audio_mutex); out << nap_audio_status_locked() << "\n"; }
    out << "pattern=OFF no running cubes/no squares; cached real frame path active\n";
    {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        out << "loaded=" << (g_real.loaded ? "YES" : "NO") << "\n";
        out << "frameReady=" << (g_real.frame_ready ? "YES" : "NO") << "\n";
        out << "frameCounter=" << g_real.frame_counter << "\n";
        out << "displayFrameCounter=" << g_real.display_frame_counter << "\n";
        out << "frameSize=" << g_real.frame_w << "x" << g_real.frame_h << " display=" << g_real.display_w << "x" << g_real.display_h << "\n";
        out << "iterationsLast=" << g_real.iterations_last << "\n";
        out << "workerIterations=" << g_real_thread_iterations.load() << "\n";
        out << "status=" << g_real.status;
    }
    return env->NewStringUTF(out.str().c_str());
#else
    std::string s = "REAL_CORE_PRESENT=NO\nCORE_VENDOR_MISSING\nCMake did not enable NAP_SEGA_VENDOR_CORE_PRESENT.";
    return env->NewStringUTF(s.c_str());
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_shutdown(JNIEnv* env, jclass) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    {   // B357: odchod ze Segy = konec online (zamek jen tady - vlakno emulace na nej muze cekat, nize se na nej ceka)
        std::lock_guard<std::mutex> r(g_online_rizeni);
        if (g_online_aktivni.load() || g_online.faze() != nap::online::OnlineKlient::Faze::NIC) {
            g_online_aktivni.store(0);
            g_online_hraje.store(0);
            g_online.stop();
        }
    }
    nap_real_stop_worker();
    for (int i = 0; i < 8; ++i) g_input[i] = false;
    { std::lock_guard<std::mutex> l(g_ovl_m); nap::ovladac::pustVse(g_ovl_stav); g_ovl_bity.store(0); nap::ovladac::pustVse(g_ovl_stav2); g_ovl_bity2.store(0); }   // B320, B354
    nap_audio_clear();
    {
        std::lock_guard<std::mutex> lock(g_real_mutex);
        g_real.loaded = false;
        g_real.frame_ready = false;
        g_real.status = "REAL_CORE_STOPPED_BY_LIFECYCLE_QT_CPP_ONLY";
    }
    return env->NewStringUTF("NATIVE_SHUTDOWN_OK_RV_QT_CPP_ONLY worker=STOPPED audio=CLEARED input=RELEASED");
#else
    for (int i = 0; i < 8; ++i) g_input[i] = false;
    { std::lock_guard<std::mutex> l(g_ovl_m); nap::ovladac::pustVse(g_ovl_stav); g_ovl_bity.store(0); nap::ovladac::pustVse(g_ovl_stav2); g_ovl_bity2.store(0); }   // B320, B354
    return env->NewStringUTF("NATIVE_SHUTDOWN_OK_RV_QT_CPP_ONLY noVendor input=RELEASED");
#endif
}

// BUILD2SB37: Rene - "dalo by se vymyslet ukladani her, jako treba
// Sonic - ulozit pozici a pak z ni pokracovat?" ANO - ClownMDEmu jadro
// uz ma vestavenou ClownMDEmu_SaveState/LoadState (viz clownmdemu.h),
// jen ji appka dosud vubec nepouzivala. Neni to totez jako oficialni
// baterii-zalohovana SRAM ulozena pozice (tu ma jen par her jako
// Sonic 3, a jen na mistech, kde to hra sama dovoli) - je to univerzalni
// "snimek celeho stavu jadra" (CPU, VDP, zvukove cipy...), funguje na
// KTERYKOLI hre, KDEKOLI ve hre, presne jako "save state" u emulatoru.
//
// KRITICKE PRO BEZPECNOST: ClownMDEmu_StateBackup je STEJNE VELKY jako
// hlavni ClownMDEmu struktura (>1 MB - viz komentar u NapRealCoreState
// vyse: "can overflow Android/WebView thread stack"). Proto je tenhle
// buffer STATICKY/GLOBALNI, NIKDY ne lokalni promenna uvnitr funkce -
// presne stejna opatrnost, jakou uz tenhle soubor ma pro g_real samotne.
static ClownMDEmu_StateBackup g_sega_state_backup;

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_saveState(JNIEnv* env, jclass, jstring jpath) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    if (!jpath) return env->NewStringUTF("SEGA_STATE_SAVE_FAIL path=null");
    const char *pathChars = env->GetStringUTFChars(jpath, nullptr);
    std::string path(pathChars ? pathChars : "");
    if (pathChars) env->ReleaseStringUTFChars(jpath, pathChars);

    std::lock_guard<std::mutex> lock(g_real_mutex);
    if (!g_real.loaded) return env->NewStringUTF("SEGA_STATE_SAVE_FAIL zadna_hra_nebezi");

    ClownMDEmu_SaveState(&g_real.emu, &g_sega_state_backup);

    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return env->NewStringUTF(("SEGA_STATE_SAVE_FAIL nejde_otevrit_soubor " + path).c_str());
    size_t wrote = fwrite(&g_sega_state_backup, 1, sizeof(g_sega_state_backup), f);
    fclose(f);
    if (wrote != sizeof(g_sega_state_backup)) {
        return env->NewStringUTF("SEGA_STATE_SAVE_FAIL nekompletni_zapis");
    }
    char out[256];
    snprintf(out, sizeof(out), "SEGA_STATE_SAVE_OK bytes=%zu path=%s", wrote, path.c_str());
    return env->NewStringUTF(out);
#else
    return env->NewStringUTF("SEGA_STATE_SAVE_FAIL zadne_jadro_v_tomhle_buildu");
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_loadState(JNIEnv* env, jclass, jstring jpath) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    if (!jpath) return env->NewStringUTF("SEGA_STATE_LOAD_FAIL path=null");
    const char *pathChars = env->GetStringUTFChars(jpath, nullptr);
    std::string path(pathChars ? pathChars : "");
    if (pathChars) env->ReleaseStringUTFChars(jpath, pathChars);

    // B357: v online hre by nahrana pozice rozjela telefony (druhy ji nema) - nejde
    if (g_online_aktivni.load()) return env->NewStringUTF("SEGA_STATE_LOAD_FAIL online_hra");
    std::lock_guard<std::mutex> lock(g_real_mutex);
    if (!g_real.loaded) return env->NewStringUTF("SEGA_STATE_LOAD_FAIL zadna_hra_nebezi");

    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return env->NewStringUTF(("SEGA_STATE_LOAD_FAIL soubor_neexistuje " + path).c_str());
    size_t got = fread(&g_sega_state_backup, 1, sizeof(g_sega_state_backup), f);
    fclose(f);
    if (got != sizeof(g_sega_state_backup)) {
        return env->NewStringUTF("SEGA_STATE_LOAD_FAIL nekompletni_cteni_spatny_soubor");
    }

    ClownMDEmu_LoadState(&g_real.emu, &g_sega_state_backup);
    nap_real_obnov_paletu_locked();   // B360: barvy obrazu hned podle nahrane pozice (ne az po zapisu do CRAM)

    char out[256];
    snprintf(out, sizeof(out), "SEGA_STATE_LOAD_OK bytes=%zu path=%s", got, path.c_str());
    return env->NewStringUTF(out);
#else
    return env->NewStringUTF("SEGA_STATE_LOAD_FAIL zadne_jadro_v_tomhle_buildu");
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_realCoreLoadRom(JNIEnv* env, jclass, jbyteArray romBytes) {
    if (!romBytes) return env->NewStringUTF("REAL_CORE_STAGE_ERROR rom=null");
    jsize len = env->GetArrayLength(romBytes);
#if NAP_SEGA_VENDOR_CORE_PRESENT
    std::vector<uint8_t> data((size_t)len);
    env->GetByteArrayRegion(romBytes, 0, len, reinterpret_cast<jbyte*>(data.data()));
    std::string res = nap_real_load_rom_bytes(data.data(), data.size());
    return env->NewStringUTF(res.c_str());
#else
    std::ostringstream out;
    out << "REAL_CORE_PRESENT=NO\nCORE_VENDOR_MISSING\nbytes=" << len;
    return env->NewStringUTF(out.str().c_str());
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_realCoreStep(JNIEnv* env, jclass) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    std::string res = nap_real_step_once();
    return env->NewStringUTF(res.c_str());
#else
    return env->NewStringUTF("REAL_CORE_STEP_ERROR CORE_VENDOR_MISSING");
#endif
}


extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_pullAudioStereo(JNIEnv* env, jclass, jshortArray pcmOut, jint stereoFrames) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    if (!pcmOut || stereoFrames <= 0) return 0;
    jsize len = env->GetArrayLength(pcmOut);
    int n = std::min<int>((int)len / 2, (int)stereoFrames);
    if (n <= 0) return 0;
    jshort* out = env->GetShortArrayElements(pcmOut, nullptr);
    if (!out) return 0;
    int got = nap_audio_pull_stereo(out, n);
    // (B306: zvuk na TV / PC uz jde hned z jadra - nap_audio_push_*_batch - ne az odsud,
    //  za frontou reproduktoru telefonu, ktera na S8 drzi ~130 ms)
    env->ReleaseShortArrayElements(pcmOut, out, 0);
    return got;
#else
    (void)stereoFrames;
    if (pcmOut) {
        jsize len = env->GetArrayLength(pcmOut);
        std::vector<jshort> zero((size_t)len, 0);
        env->SetShortArrayRegion(pcmOut, 0, len, zero.data());
    }
    return 0;
#endif
}

extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_pullAudio(JNIEnv* env, jclass, jshortArray pcmOut, jint frames) {
#if NAP_SEGA_VENDOR_CORE_PRESENT
    if (!pcmOut || frames <= 0) return 0;
    jsize len = env->GetArrayLength(pcmOut);
    int n = std::min<int>((int)len, (int)frames);
    if (n <= 0) return 0;
    jshort* out = env->GetShortArrayElements(pcmOut, nullptr);
    if (!out) return 0;
    int got = nap_audio_pull_mono(out, n);
    env->ReleaseShortArrayElements(pcmOut, out, 0);
    return got;
#else
    (void)frames;
    if (pcmOut) {
        jsize len = env->GetArrayLength(pcmOut);
        std::vector<jshort> zero((size_t)len, 0);
        env->SetShortArrayRegion(pcmOut, 0, len, zero.data());
    }
    return 0;
#endif
}

// ===================================================================
//  B305: PRENOS SEGY NA TV / PC (Java /sega.stream jen posila bajty)
// ===================================================================
/** Novy prohlizec. Vraci cislo klienta (> 0). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudOtevriNative(JNIEnv *, jclass) {
    return (jint)g_segaProud.otevri();
}

/** Ceka (nejvys cekejMs) na novy snimek a zabali ho do out: 4 B delka + paket.
 *  zapisMs = jak dlouho trval zapis minuleho paketu do site.
 *  Vraci pocet bajtu, 0 = nic noveho, zaporne = out je male (potreba -n bajtu,
 *  paket pocka na dalsi volani), INT32_MIN = neznamy klient. */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudDalsiNative(JNIEnv *env, jclass, jint id, jbyteArray out, jint cekejMs, jint zapisMs) {
    if (!out) return INT32_MIN;
    thread_local std::vector<uint8_t> pk;
    const int r = g_segaProud.dalsi((int)id, (int)cekejMs, (int)zapisMs, pk);
    if (r < 0) return INT32_MIN;
    if (r == 0) return 0;
    if ((jsize)pk.size() > env->GetArrayLength(out)) {
        const jint potreba = (jint)pk.size();
        g_segaProud.odloz((int)id, pk);
        return -potreba;
    }
    env->SetByteArrayRegion(out, 0, (jsize)pk.size(), (const jbyte *)pk.data());
    return (jint)pk.size();
}

/** Prohlizec odpojen. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudZavriNative(JNIEnv *, jclass, jint id) {
    g_segaProud.zavri((int)id);
}

/** bit0 = jadro posila snimky (posledni pred < 2 s), bit1 = nekdo se diva */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudStavNative(JNIEnv *, jclass) {
    return (g_segaProud.bezi(2000) ? 1 : 0) | (g_segaProud.klientu() > 0 ? 2 : 0);
}

/** Radky pro log appky (pripojen / odpojen / kazdych 30 s), null = nic. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudLogNative(JNIEnv *env, jclass) {
    const std::string s = g_segaProud.vezmiLog();
    if (s.empty()) return nullptr;
    return env->NewStringUTF(s.c_str());
}

// =====================================================================================================
//  B357: ONLINE HRANI - JNI (NativeSegaCoreBridge.online*). Java da adresu enginu, cestu (zaloz / pripoj)
//  a hlavicky s podpisem telefonu; vse ostatni (spojeni, domluva, vstupy, otisky) je v C++.
// =====================================================================================================
#if NAP_SEGA_VENDOR_CORE_PRESENT
static std::string nap_sega_jstr(JNIEnv *env, jstring js, size_t max = 4096) {
    std::string s;
    if (!js) return s;
    const char *c = env->GetStringUTFChars(js, nullptr);
    if (c) { s.assign(c, strnlen(c, max)); env->ReleaseStringUTFChars(js, c); }
    return s;
}

static std::string nap_sega_hex(const uint8_t *d, size_t n) {
    static const char *h = "0123456789abcdef";
    std::string r;
    for (size_t i = 0; i < n; i++) { r += h[d[i] >> 4]; r += h[d[i] & 15]; }
    return r;
}

/** Otisk nahrane hry (SHA-1 hex) a nastaveni jadra pro zalozeni hry ("otisk|nastaveni"), "" = zadna hra. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineOtisk(JNIEnv *env, jclass) {
    std::string r;
    {
        std::lock_guard<std::mutex> l(g_online_m);
        if (g_sega_otisk_je) r = nap_sega_hex(g_sega_otisk_hry, 20) + "|" + std::to_string((unsigned)g_real_cfg_region * 16u + (unsigned)g_real_cfg_tv);
    }
    return env->NewStringUTF(r.c_str());
}

/** Zacne online hru (hrac1 = zaklada). "OK" nebo text chyby. Hra bezi dal, dokud se telefony nedomluvi, pak studeny start. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStart(JNIEnv *env, jclass, jstring jadresa, jstring jcesta, jobjectArray jhlavicky,
                                                       jboolean hrac1, jstring jjmeno) {
    nap::online::OnlineKlient::Nastaveni n;
    n.adresa = nap_sega_jstr(env, jadresa);
    n.cesta = nap_sega_jstr(env, jcesta);
    if (jhlavicky) {
        const jsize k = env->GetArrayLength(jhlavicky);
        for (jsize i = 0; i < k; i++) {
            jstring h = (jstring)env->GetObjectArrayElement(jhlavicky, i);
            n.hlavicky.push_back(nap_sega_jstr(env, h, 1024));
            if (h) env->DeleteLocalRef(h);
        }
    }
    n.hrac1 = hrac1 == JNI_TRUE;
    n.ja.jadro = nap::online::J_SEGA;
    n.ja.jmeno = nap_sega_jstr(env, jjmeno, 60);
    n.ja.nastaveni = (uint32_t)g_real_cfg_region * 16u + (uint32_t)g_real_cfg_tv;
    n.snimekMs = g_real_cfg_tv == CLOWNMDEMU_TV_STANDARD_PAL ? 20.0 : 1000.0 / 60.0;
    // B362: jadro Segy umi odhad + opravu -> zpozdeni na startu bez rezervy na kolisani; za hry roste jen po malych
    // krocich (spicky do ~8 snimku spolkne odhad - vetsi zpozdeni by hrac citil porad, ne jen pri spicce)
    n.ladeni.odhad = true;
    n.ladeni.zvysitNejvys = 2;
    n.ladeni.zpozdeniNavic = 4;
    {
        std::lock_guard<std::mutex> l(g_online_m);
        if (!g_sega_otisk_je) return env->NewStringUTF("CHYBA: neni nahrana zadna hra");
        std::memcpy(n.ja.hra, g_sega_otisk_hry, 20);
        g_online_konec.clear();
    }
    if (n.adresa.empty() || n.cesta.empty()) return env->NewStringUTF("CHYBA: chybi adresa enginu");
    std::lock_guard<std::mutex> rz(g_online_rizeni);
    g_online_hraje.store(0);
    g_online_aktivni.store(0);
    g_online.start(n);
    g_online_startu.fetch_add(1);
    g_online_snimku_celkem.store(0);
    g_online_aktivni.store(1);
    NAPLOG("B357 ONLINE %s %s", n.hrac1 ? "zaklada" : "pripojuje", n.cesta.c_str());
    return env->NewStringUTF("OK");
}

/** Konec online hry (hrac odchazi). Hra bezi dal sama (ovladani zase jen tento telefon). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(JNIEnv *, jclass) {
    std::lock_guard<std::mutex> rz(g_online_rizeni);
    g_online_aktivni.store(0);
    g_online_hraje.store(0);
    g_online.stop();
}

/** Stav pro Javu: radky klic=hodnota (faze, kod, druhy, popis, chyba, odezva, zpozdeni, overeno, snimek, aktivni, hraje, konec). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStav(JNIEnv *env, jclass) {
    using F = nap::online::OnlineKlient::Faze;
    const F f = g_online.faze();
    const char *fs = f == F::PRIPOJUJE ? "PRIPOJUJE" : f == F::CEKA_NA_HRACE ? "CEKA_NA_HRACE" : f == F::HRAJE ? "HRAJE" : f == F::KONEC ? "KONEC" : "NIC";
    std::shared_ptr<nap::online::Relace> r = g_online.relace();
    std::string konec;
    {
        std::lock_guard<std::mutex> l(g_online_m);
        konec = g_online_konec;
    }
    std::ostringstream o;
    o << "faze=" << fs << "\nkod=" << g_online.kodMistnosti() << "\ndruhy=" << g_online.jmenoDruheho()
      << "\npopis=" << g_online.popis() << "\nchyba=" << g_online.chyba()
      << "\nodezva=" << (r ? r->odezvaMs() : 0) << "\nzpozdeni=" << (r ? r->zpozdeni() : 0)
      << "\novereno=" << (r ? r->overenychOtisku() : 0) << "\ncekani=" << (r ? r->cekani() : 0)
      << "\nsnimek=" << g_online_snimku_celkem.load() << "\naktivni=" << g_online_aktivni.load()
      << "\nhraje=" << g_online_hraje.load() << "\nhrac1=" << (r ? (r->hrac1() ? 1 : 0) : 0) << "\nkonec=" << konec
      << "\nstart=" << g_online_startu.load();
    if (r) {   // B360: prubeh hry (cekani na stisky druheho, zpozdeni, synchronizace) - Java z toho pise log
        o << "\nzpozdeni_ted=" << r->zpozdeniTed() << "\ncekani_ms=" << r->cekaniMs() << "\ncekani_max_ms=" << r->cekaniMaxMs()
          << "\ndlouhych=" << r->dlouhychCekani() << "\nzadosti=" << r->zadosti() << "\nzvyseni=" << r->zvyseni()
          << "\nsnizeni=" << r->snizeni() << "\nsync=" << (r->synchronizuje() ? 1 : 0) << "\nsynchronizaci=" << r->synchronizaci()
          << "\nsynchronizovano=" << r->synchronizovano() << "\nrozjeti=" << r->rozjeti() << "\nrozjeti_snimek=" << r->rozjetoVeSnimku()
          << "\nrozjeti_casti=" << nap::online::nazvyCasti(r->rozdilCasti(), NAP_SEGA_NAZVY_CASTI, NAP_SEGA_CASTI)
          << "\nstav_kb=" << g_online_stav_kb.load() << "\nticho_ms=" << r->tichoMs() << "\nstoji_ms=" << r->stojiMs();
        // B362: odhad + oprava, statistika druheho telefonu, posledni zprava od nej (Java z toho pise log)
        o << "\nodhad=" << r->odhadnuto() << "\nodhad_sedelo=" << r->odhaduSedelo() << "\noprav=" << r->oprav()
          << "\noprava_max=" << r->opravaMax() << "\noprava_max_ms=" << r->opravaMaxMs() << "\nopravy_ms=" << r->opravyMs()
          << "\nprepocteno=" << r->prepocteno() << "\nodhad_max=" << g_online_odhad_max.load()
          << "\nchyb_opravy=" << g_online_chyb_opravy.load() << "\nnavrat_ze=" << g_online_navrat_ze.load()
          << "\nnavrat_na=" << g_online_navrat_na.load() << "\nposledni_zprava=" << r->posledniZprava()
          << "\njeho_do=" << r->jehoDo() << "\ndruhy_info_c=" << r->statistikDruheho()
          << "\ndruhy_info_snimek=" << r->mujSnimekPriStatistice() << "\ndruhy_info=" << r->statistikaDruheho();
    }
    // B363: setrne kresleni (Java ho zapise do logu)
    o << "\nsetrne=" << g_online_setrne.load() << "\nsetrne_c=" << g_online_setrne_c.load()
      << "\nsetrne_proc=jadro " << (g_online_setrne_ms10.load() / 10) << "." << (g_online_setrne_ms10.load() % 10) << " ms na snimek (z 16.7 ms)";
    return env->NewStringUTF(o.str().c_str());
}

/** B362: telefon o sobe (model, Android, Wi-Fi, baterie - Java kazdych par s) - jde druhemu telefonu ve STATISTIKA. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineTelefon(JNIEnv *env, jclass, jstring jinfo) {
    std::string s = nap_sega_jstr(env, jinfo, 200);
    for (char &c : s) if ((unsigned char)c < 32 || (unsigned char)c > 126) c = '?';   // jen ASCII (log druheho)
    g_online.infoTelefonu(s);
}
#else
extern "C" JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineOtisk(JNIEnv *env, jclass) { return env->NewStringUTF(""); }
extern "C" JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStart(JNIEnv *env, jclass, jstring, jstring, jobjectArray, jboolean, jstring) {
    return env->NewStringUTF("CHYBA: jadro Segy chybi");
}
extern "C" JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(JNIEnv *, jclass) {}
extern "C" JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStav(JNIEnv *env, jclass) { return env->NewStringUTF("faze=NIC"); }
extern "C" JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineTelefon(JNIEnv *, jclass, jstring) {}
#endif
