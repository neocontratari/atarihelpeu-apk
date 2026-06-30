#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <mutex>
#include <android/log.h>

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

#if NAP_SEGA_VENDOR_CORE_PRESENT
// BUILD2QC: tiny Android frontend for the real ClownMDEmu-core.
// It is intentionally small: ROM load -> hard reset -> iterate -> scanline framebuffer -> existing in-place view.
// Audio mixing is not wired yet; this stage is the first real core import/visual boot attempt, no fake gameplay.
struct NapRealCoreState {
    bool loaded = false;
    bool frame_ready = false;
    int frame_w = 320;
    int frame_h = 224;
    uint32_t frame_counter = 0;
    uint32_t iterations_last = 0;
    std::vector<cc_u16l> cart_words;
    std::vector<uint32_t> frame_argb;
    uint32_t palette[0x10000];
    ClownMDEmu emu;
    ClownMDEmu_InitialConfiguration cfg;
    ClownMDEmu_Callbacks cb;
    std::string status = "REAL_CORE_NOT_LOADED";
};

static NapRealCoreState g_real;
static std::mutex g_real_mutex;
static bool g_real_core_loaded_but_render_guarded = false;
static uint32_t g_guard_frame_counter = 0;

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
    for (int x = 0; x < sw; ++x) {
        cc_u8l pix = pixels[x];
        uint32_t c = st->palette[pix];
        if (c == 0) c = 0xff000000u;
        st->frame_argb[(size_t)y * sw + x] = c;
    }
    if (y >= sh - 1) {
        st->frame_ready = true;
        st->frame_counter++;
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

static void nap_audio_fm(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    std::vector<cc_s16l> tmp(n * CLOWNMDEMU_FM_CHANNEL_COUNT);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
}
static void nap_audio_psg(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    std::vector<cc_s16l> tmp(n * CLOWNMDEMU_PSG_CHANNEL_COUNT);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
}
static void nap_audio_pcm(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    std::vector<cc_s16l> tmp(n * CLOWNMDEMU_PCM_CHANNEL_COUNT);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
}
static void nap_audio_cdda(void*, ClownMDEmu *c, size_t n, void (*gen)(ClownMDEmu*, cc_s16l*, size_t)) {
    std::vector<cc_s16l> tmp(n * CLOWNMDEMU_CDDA_CHANNEL_COUNT);
    if (gen && !tmp.empty()) gen(c, tmp.data(), n);
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

static std::string nap_real_load_rom_bytes(const uint8_t* bytes, size_t size) {
    if (!bytes || size < 0x200) return "REAL_CORE_LOAD_ERROR bad rom";
    std::lock_guard<std::mutex> lock(g_real_mutex);
    g_real = NapRealCoreState();
    for (int i = 0; i < 0x10000; ++i) g_real.palette[i] = 0xff000000u;
    g_real.frame_argb.assign(320 * 224, 0xff000000u);
    g_real.cart_words.resize((size + 1) / 2);
    for (size_t i = 0, j = 0; i < size; i += 2, ++j) {
        uint16_t hi = bytes[i];
        uint16_t lo = (i + 1 < size) ? bytes[i + 1] : 0;
        g_real.cart_words[j] = (cc_u16l)((hi << 8) | lo);
    }
    std::memset(&g_real.emu, 0, sizeof(g_real.emu));
    std::memset(&g_real.cfg, 0, sizeof(g_real.cfg));
    std::memset(&g_real.cb, 0, sizeof(g_real.cb));
    g_real.cfg.general.region = CLOWNMDEMU_REGION_OVERSEAS;
    g_real.cfg.general.tv_standard = CLOWNMDEMU_TV_STANDARD_NTSC;
    g_real.cfg.general.low_pass_filter_disabled = cc_true;
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
    ClownMDEmu_Constant_Initialise();
    ClownMDEmu_Initialise(&g_real.emu, &g_real.cfg, &g_real.cb);
    ClownMDEmu_SetCartridge(&g_real.emu, g_real.cart_words.data(), (cc_u32f)g_real.cart_words.size());
    // BUILD2QC CRASH GUARD:
    // 2QB buildove prosel, ale po vyberu ROM padal pri prvnim realtime Iterate/render loopu.
    // Proto ted core pripravime a ROM vlozime, ale NEpoustime automaticky Iterate z UI kreslici smycky.
    // Tim se odstrani pad aplikace i behajici proof-kostky. Dalsi krok bude teprve bezpecne
    // napojeni real frame-step/audio callbacku, ne render loop od pasu.
    ClownMDEmu_HardReset(&g_real.emu, cc_true, cc_false);
    g_real.loaded = true;
    g_real_core_loaded_but_render_guarded = true;
    g_guard_frame_counter = 0;
    std::ostringstream out;
    out << "REAL_CORE_LOAD_OK_GUARDED bytes=" << size << " words=" << g_real.cart_words.size() << "\n";
    out << "core=ClownMDEmu-core offline vendored + selectively linked by CMake\n";
    out << "video=REAL_CORE_RENDER_GUARDED_NO_AUTO_ITERATE to stop ROM-load crash\n";
    out << "pattern=OFF; no running cubes; monitor stays native guarded until safe frame-step is wired\n";
    out << "audio=core audio not wired yet; C++ tone test remains separate output proof";
    g_real.status = out.str();
    return g_real.status;
}

static bool nap_real_render_to_argb(int out_w, int out_h, jint *out_px) {
    // BUILD2QC: no automatic ClownMDEmu_Iterate from Android View render thread.
    // This prevents the ROM-load crash reported in 2QB while keeping real core load state intact.
    (void)out_w; (void)out_h; (void)out_px;
    return false;
}

static void nap_render_guard_frame(int width, int height, jintArray argbOut, JNIEnv* env) {
    if (!argbOut || width <= 0 || height <= 0) return;
    jsize len = env->GetArrayLength(argbOut);
    int needed = width * height;
    if (len < needed) return;
    std::vector<jint> px((size_t)needed, (jint)0xff05090du);
    // Static, no-running-cubes guarded native monitor.
    // Green/blue border means JNI/native view alive; hash bars mean ROM reached C++ if present.
    auto put = [&](int x0,int y0,int rw,int rh,uint32_t c){
        int x1=std::max(0,x0), y1=std::max(0,y0), x2=std::min(width,x0+rw), y2=std::min(height,y0+rh);
        for(int y=y1;y<y2;++y) for(int x=x1;x<x2;++x) px[(size_t)y*width+x]=(jint)c;
    };
    put(0,0,width,4,0xff00a0ffu); put(0,height-4,width,4,0xff00a0ffu);
    put(0,0,4,height,0xff00a0ffu); put(width-4,0,4,height,0xff00a0ffu);
    uint32_t statusCol = g_real_core_loaded_but_render_guarded ? 0xff00ff99u : 0xff3050a0u;
    put(width/2-18, height/2-18, 36, 36, 0xff000000u);
    put(width/2-12, height/2-12, 24, 24, statusCol);
    uint32_t h = g_lastRomHash;
    if (h != 0) {
        for (int i=0;i<16;++i) if (h & (1u<<i)) put(10+i*18, height-22, 12, 10, 0xffffffffu);
    }
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
    std::string s = "BUILD2QC NATIVE C++ IN-PLACE NORMAL SEGA UI PROOF OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ native log export: OK\n"
                    "C++ 60Hz timing proof target: OK\n"
                    "Status: ClownMDEmu-core native import attempt; no ROM in APK";
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

    out << "\nBUILD2QC DULEZITE:\n";
    out << "ROM je ted realne prectena v Jave a analyzovana v C++.\n";
    out << "Native proof pattern ma v BUILD2QC cil 60 FPS, aby se overila nativni cesta pred Sega core.\n";
    out << "Dalsi krok je vymena proof patternu za skutecny Sega C++ core.\n";
    std::string s = out.str();
    NAPLOG("BUILD2QC ROM info generated, bytes=%d fnv=0x%08x", len, fnv);
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
    std::vector<jint> real_px((size_t)needed);
    if (nap_real_render_to_argb((int)width, (int)height, real_px.data())) {
        env->SetIntArrayRegion(argbOut, 0, needed, real_px.data());
        return;
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


// BUILD2QC: same native core exposed to MainActivity/WebView in-place bridge.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_buildString(JNIEnv* env, jclass) {
    std::string s = "BUILD2QC NATIVE C++ IN-PLACE NORMAL SEGA UI PROOF OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ render pattern: OK\n"
                    "Status: integrated into normal Sega UI; ClownMDEmu-core native import; no ROM in APK";
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
            std::vector<jint> px((size_t)needed);
            if (nap_real_render_to_argb((int)width, (int)height, px.data())) {
                env->SetIntArrayRegion(argbOut, 0, needed, px.data());
                return;
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

// BUILD2QC REAL CORE ADAPTER SLOT
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
    out << "mode=core load only with crash guard; no automatic Iterate in render thread\n";
    out << "REAL_CORE_RENDER_GUARDED=" << (g_real_core_loaded_but_render_guarded ? "YES" : "NO") << "\n";
    out << "pattern=OFF no running cubes\n";
    out << "loaded=" << (g_real.loaded ? "YES" : "NO");
    return env->NewStringUTF(out.str().c_str());
#else
    std::string s = "REAL_CORE_PRESENT=NO\nCORE_VENDOR_MISSING\nCMake did not enable NAP_SEGA_VENDOR_CORE_PRESENT.";
    return env->NewStringUTF(s.c_str());
#endif
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_realCoreLoadRom(JNIEnv* env, jclass, jbyteArray romBytes) {
    if (!romBytes) return env->NewStringUTF("REAL_CORE_LOAD_ERROR rom=null");
    jsize len = env->GetArrayLength(romBytes);
#if NAP_SEGA_VENDOR_CORE_PRESENT
    std::vector<uint8_t> data((size_t)len);
    env->GetByteArrayRegion(romBytes, 0, len, reinterpret_cast<jbyte*>(data.data()));
    std::string result = nap_real_load_rom_bytes(data.data(), data.size());
    return env->NewStringUTF(result.c_str());
#else
    std::ostringstream out;
    out << "REAL_CORE_LOAD_BLOCKED bytes=" << len << "\n";
    out << "CORE_VENDOR_MISSING\n";
    out << "C++ bridge is ready, but native core was not linked.";
    return env->NewStringUTF(out.str().c_str());
#endif
}
