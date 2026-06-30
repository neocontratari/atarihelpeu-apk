#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <android/log.h>

#define NAP_LOG_TAG "NAP_SEGA_NATIVE_PROOF"
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, NAP_LOG_TAG, __VA_ARGS__)

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
    std::string s = "BUILD2PP NATIVE C++ PROOF OK\n"
                    "JNI bridge: OK\n"
                    "C++ library: napsega_native_proof\n"
                    "Status: proof only, no fake Sega gameplay, no ROM in APK";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRomInfo(JNIEnv* env, jclass, jbyteArray romBytes) {
    if (!romBytes) return env->NewStringUTF("ROM NULL");
    jsize len = env->GetArrayLength(romBytes);
    std::vector<uint8_t> data((size_t)len);
    env->GetByteArrayRegion(romBytes, 0, len, reinterpret_cast<jbyte*>(data.data()));

    std::ostringstream out;
    out << "NATIVE C++ ROM ANALYZA:\n";
    out << "- bytes: " << len << "\n";
    out << "- FNV1A32: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << fnv1a32(data.data(), data.size()) << std::dec << "\n";

    if (data.size() >= 0x200) {
        std::string console = ascii_field(data.data(), data.size(), 0x100, 16);
        std::string titleDomestic = ascii_field(data.data(), data.size(), 0x120, 48);
        std::string titleOverseas = ascii_field(data.data(), data.size(), 0x150, 48);
        std::string serial = ascii_field(data.data(), data.size(), 0x180, 14);
        uint16_t stored = (uint16_t(data[0x18e]) << 8) | uint16_t(data[0x18f]);
        uint16_t calc = md_checksum(data.data(), data.size());
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
        out << "- Mega Drive header: NE / soubor je mensi nez 0x200\n";
    }

    out << "\nDULEZITE:\n";
    out << "Tohle jeste neemuluje hru. Jen dokazuje, ze Android appka umi poslat ROM do C++ nativni vrstvy.\n";
    std::string s = out.str();
    NAPLOG("ROM info generated, bytes=%d", len);
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeSegaProofActivity_nativeRenderPattern(JNIEnv* env, jclass, jint width, jint height, jint frame, jintArray argbOut) {
    if (!argbOut || width <= 0 || height <= 0) return;
    jsize len = env->GetArrayLength(argbOut);
    int needed = width * height;
    if (len < needed) return;
    std::vector<jint> px((size_t)needed);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int band = ((x + frame * 2) / 16 + y / 28) & 7;
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
            // tmavy ramecek a sit, aby bylo videt ze jde o native render test pattern, ne fake hra
            if (x < 6 || y < 6 || x >= width - 6 || y >= height - 6 || (x % 32 == 0) || (y % 28 == 0)) {
                r = r / 4; g = g / 4; b = b / 4;
            }
            px[(size_t)y * width + x] = (jint)(0xff000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b));
        }
    }
    env->SetIntArrayRegion(argbOut, 0, needed, px.data());
}
