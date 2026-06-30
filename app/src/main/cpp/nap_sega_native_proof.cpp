#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <android/log.h>

#define NAP_LOG_TAG "NAP_SEGA_NATIVE_PROOF"
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, NAP_LOG_TAG, __VA_ARGS__)

static bool g_input[8] = {false,false,false,false,false,false,false,false};
static uint32_t g_lastRomHash = 0;
static uint16_t g_lastChecksumStored = 0;
static uint16_t g_lastChecksumCalc = 0;
static std::string g_lastTitle = "";

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
    std::string s = "BUILD2PU NATIVE C++ IN-PLACE NORMAL SEGA UI PROOF OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ native log export: OK\n"
                    "C++ 60Hz timing proof target: OK\n"
                    "Status: proof only, no fake Sega gameplay, no ROM in APK";
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

    out << "\nBUILD2PU DULEZITE:\n";
    out << "ROM je ted realne prectena v Jave a analyzovana v C++.\n";
    out << "Native proof pattern ma v BUILD2PU cil 60 FPS, aby se overila nativni cesta pred Sega core.\n";
    out << "Dalsi krok je vymena proof patternu za skutecny Sega C++ core.\n";
    std::string s = out.str();
    NAPLOG("BUILD2PU ROM info generated, bytes=%d fnv=0x%08x", len, fnv);
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


// BUILD2PU: same native core exposed to MainActivity/WebView in-place bridge.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_buildString(JNIEnv* env, jclass) {
    std::string s = "BUILD2PU NATIVE C++ IN-PLACE NORMAL SEGA UI PROOF OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "ROM header parser: OK\n"
                    "C++ input state: OK\n"
                    "C++ PCM audio generator: OK\n"
                    "C++ render pattern: OK\n"
                    "Status: integrated into normal Sega UI, proof only, no fake Sega gameplay, no ROM in APK";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_romInfo(JNIEnv* env, jclass, jbyteArray romBytes) {
    return Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRomInfo(env, nullptr, romBytes);
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaCoreBridge_renderPattern(JNIEnv* env, jclass, jint width, jint height, jint frame, jintArray argbOut) {
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
