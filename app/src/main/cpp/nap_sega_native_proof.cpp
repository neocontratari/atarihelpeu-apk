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
#include <mutex>
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

#if NAP_SEGA_VENDOR_CORE_PRESENT
#include "clownmdemu.h"
#endif

#define NAP_LOG_TAG "NAP_SEGA_NATIVE_PROOF"
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, NAP_LOG_TAG, __VA_ARGS__)

static bool g_input[8] = {false,false,false,false,false,false,false,false};
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
static void nap_audio_push_fm_batch(const std::vector<jshort>& left, const std::vector<jshort>& right) {
    if (left.empty() && right.empty()) return;
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    const size_t n = std::min(left.size(), right.size());
    g_audio_fm_l_fifo.insert(g_audio_fm_l_fifo.end(), left.begin(), left.begin() + n);
    g_audio_fm_r_fifo.insert(g_audio_fm_r_fifo.end(), right.begin(), right.begin() + n);
    g_audio_fm_pushed += n;
    nap_audio_balance_locked();
}
static void nap_audio_push_psg_batch(const std::vector<jshort>& samples) {
    if (samples.empty()) return;
    std::lock_guard<std::mutex> lock(g_audio_mutex);
    g_audio_psg_fifo.insert(g_audio_psg_fifo.end(), samples.begin(), samples.end());
    g_audio_psg_pushed += samples.size();
    nap_audio_balance_locked();
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
    out << "audio_mode=FM_PSG_ZEROED_RW_AUDIT_PCM_CDDA_SKIP_RX pcmCddaGen=SKIPPED_NO_CD sonic_main fixedAudioClock=YES stereo=YES zeroInputBuffers=YES coreLPF=ON noRumbleFilter=YES noSamplePick=YES singleAudioPath=YES audioMasterClock=YES noStarve=YES noHardTrim=YES fmGain=100 psgGain=100 masterGain=90 target=" << nap_audio_target_fifo()
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
}

static cc_bool nap_real_input_requested(void *user_data, cc_u8f player_id, ClownMDEmu_Button button_id) {
    (void)user_data;
    if (player_id != 0) return cc_false;
    switch (button_id) {
        case CLOWNMDEMU_BUTTON_UP: return g_input[0] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_DOWN: return g_input[1] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_LEFT: return g_input[2] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_RIGHT: return g_input[3] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_A: return g_input[4] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_B: return g_input[5] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_C: return g_input[6] ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_START: return g_input[7] ? cc_true : cc_false;
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
    const std::string regionMode = nap_real_detect_and_set_region(bytes, size);
    nap_audio_clear();

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
    NAPLOG("BUILD2RX real core worker start gen=%d bigstack=8MB passiveAudit=YES pcmCddaSkip=YES", generation);
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
                // BUILD2RV: LOW/HIGH is video-only. HIGH captures every frame; LOW presents ~30 fps.
                // Audio frontend timing and FIFO sizes stay identical in both modes.
                uint32_t preIt = g_real_thread_iterations.load();
                int divisor = g_native_perf_low.load() ? 2 : 1;
                // BUILD2RV: S8 no-starve mode reduces native scanline capture when audio FIFO is low.
                // The WebView/TextureView can keep presenting the cached frame, while the core spends
                // less time copying video and more time producing YM/PSG audio.
                if (nap_mobile_no_starve()) {
                    size_t aq = nap_audio_min_fifo();
                    if (aq < nap_audio_low_water() / 2u) divisor = 4;
                    else divisor = std::max(divisor, 2);
                }
                bool captureFrame = ((preIt % (uint32_t)divisor) == 0u);
                g_render_capture_current.store(captureFrame ? 1 : 0);
                if (!captureFrame) g_render_skipped_frames.fetch_add(1);
                auto iter_start = std::chrono::steady_clock::now();
                ClownMDEmu_Iterate(&g_real.emu);
                g_render_capture_current.store(1);
                auto iter_end = std::chrono::steady_clock::now();
                uint64_t iter_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(iter_end - iter_start).count();
                nap_scene_stress_record(iter_ns);
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
    NAPLOG("BUILD2RX real core worker stop gen=%d", generation);
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

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeCoreBuildString(JNIEnv* env, jclass) {
    std::string s = "BUILD2RX NATIVE C++ RW AUDIT + PCM/CDDA CPU SKIP OK\n"
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
    std::string s = "BUILD2RX NATIVE C++ RW AUDIT + PCM/CDDA CPU SKIP OK\n"
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
    nap_real_stop_worker();
    for (int i = 0; i < 8; ++i) g_input[i] = false;
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
    return env->NewStringUTF("NATIVE_SHUTDOWN_OK_RV_QT_CPP_ONLY noVendor input=RELEASED");
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

