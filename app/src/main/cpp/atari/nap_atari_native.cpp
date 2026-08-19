// nap_atari_native.cpp
// BUILD2SA14: most do Javy pro jadro Atari v C++ (vrstvy 1-2).
//
// PROC TENHLE TEST EXISTUJE:
// Vsechna mereni jadra jsem delal na pocitaci (x86_64). Telefon je ARM64 a
// tam se nektere veci chovaji JINAK - napriklad 'char' je na ARM ve vychozim
// stavu BEZ znamenka, na x86 SE znamenkem. Kdyz se tim jadro rozejde, pozna
// se to az na obrazu, a to uz se hleda spatne.
// Proto stejny test bezi tady na telefonu a porovnava se s cislem, ktere
// vyslo na pocitaci. Bud sedi, nebo ne.
//
// Zadny obraz ani zvuk tu zatim neni - ANTIC, GTIA, POKEY a VBXE jeste
// nejsou hotove, takze OS Atari se nema o co oprit a nerozbehne se.

#include <jni.h>
#include <android/log.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <string>
#include "nap_atari_cpu.h"
#include "nap_atari_mem.h"
#include "nap_atari_video.h"
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include <string>

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "NAPATARI", __VA_ARGS__)

using namespace nap;

// ---------------------------------------------------------------
//  Spolecny generator - MUSI byt shodny s tim na pocitaci (dt2.cpp)
// ---------------------------------------------------------------
static uint32_t g_salt;
static std::map<int,int> g_ovl;

static inline uint32_t genb(uint32_t a) {
  uint32_t h = (a * 0x9E3779B1u) ^ (g_salt * 0x85EBCA6Bu);
  h ^= h >> 15; h *= 0xC2B2AE35u; h ^= h >> 13;
  return h & 0xFF;
}
static int  T_RD(int a) { auto it = g_ovl.find(a); return it != g_ovl.end() ? it->second : (int)genb((uint32_t)a); }
static void T_WR(int a, int v) { g_ovl[a] = v; }

static uint32_t g_rs;
static inline uint32_t rnd() { g_rs ^= g_rs << 13; g_rs ^= g_rs >> 17; g_rs ^= g_rs << 5; return g_rs; }

static inline void mix(uint32_t &h, uint32_t v) { h ^= v; h *= 16777619u; }

// ---------------------------------------------------------------
//  TEST 1: procesor 6502
// ---------------------------------------------------------------
static uint32_t cpuSelfTest(int perOp, uint32_t seed, long long *outInstr) {
  g_rs = seed;
  Cpu6502 cpu(T_RD, T_WR);
  uint32_t h = 2166136261u;
  long long n = 0;
  for (int op = 0; op < 256; op++) {
    for (int k = 0; k < perOp; k++) {
      g_salt = rnd(); g_ovl.clear();
      cpu.c.a = rnd() & 0xFF; cpu.c.x = rnd() & 0xFF; cpu.c.y = rnd() & 0xFF;
      cpu.c.sp = rnd() & 0xFF; cpu.c.pc = rnd() & 0xFFFF;
      cpu.c.nf = rnd() & 1; cpu.c.vf = rnd() & 1; cpu.c.df = rnd() & 1;
      cpu.c.if_ = rnd() & 1; cpu.c.zf = rnd() & 1; cpu.c.cf = rnd() & 1;
      cpu.c.jam = false; cpu.c.nmiPending = false; cpu.c.irqLine = 0; cpu.c.cycles = 0;
      g_ovl[cpu.c.pc] = op;
      cpu.step();
      mix(h, (uint32_t)op);      mix(h, (uint32_t)cpu.c.a);
      mix(h, (uint32_t)cpu.c.x); mix(h, (uint32_t)cpu.c.y);
      mix(h, (uint32_t)cpu.c.sp);mix(h, (uint32_t)cpu.c.pc);
      mix(h, (uint32_t)(cpu.c.nf | (cpu.c.vf<<1) | (cpu.c.df<<2) |
                        (cpu.c.if_<<3) | (cpu.c.zf<<4) | (cpu.c.cf<<5)));
      mix(h, (uint32_t)cpu.c.cycles);
      mix(h, (uint32_t)(cpu.c.jam ? 1 : 0));
      for (auto &kv : g_ovl) { mix(h, (uint32_t)kv.first); mix(h, (uint32_t)kv.second); }
      n++;
    }
  }
  *outInstr = n;
  return h;
}

// ---------------------------------------------------------------
//  TEST 2: pamet a bankovani
// ---------------------------------------------------------------
static uint8_t g_os[16384], g_bas[8192];
static inline int patRam(int i) { return (i*7 + 11) & 0xFF; }
static inline int patExt(int i) { return (i*13 + 29) & 0xFF; }
static inline int patOs (int i) { return (i*31 + 5)  & 0xFF; }
static inline int patBas(int i) { return (i*17 + 91) & 0xFF; }

// AtariMem ma pres 380 kB (RAM 64 kB + rozsirena pamet 320 kB). Na zasobniku
// vlakna, ktere obsluhuje @JavascriptInterface, by to byl hazard - proto je
// staticka. Test bezi jen jednou a nikdo jiny ji nesaha.
static AtariMem g_mem;

static uint32_t memSelfTest(long long *outReads) {
  for (int i = 0; i < 16384; i++) g_os[i]  = (uint8_t)patOs(i);
  for (int i = 0; i < 8192;  i++) g_bas[i] = (uint8_t)patBas(i);
  AtariMem &m = g_mem;
  std::memset(m.ram, 0, sizeof(m.ram));
  std::memset(m.ext, 0, sizeof(m.ext));
  m.pia = Pia();
  m.os = g_os; m.bas = g_bas;
  for (int i = 0; i < 65536; i++) m.ram[i] = (uint8_t)patRam(i);
  auto setPortB = [&](int v) {
    m.piaWrite(3, 0x00); m.piaWrite(1, 0xFF);
    m.piaWrite(3, 0x04); m.piaWrite(1, v & 0xFF);
  };
  for (int b = 0; b < 16; b++) {
    int pb = ((b & 1) << 2) | (((b >> 1) & 1) << 3) | (((b >> 2) & 1) << 5) | (((b >> 3) & 1) << 6);
    setPortB(pb);
    int bank = ((pb & 0x40) == 0) ? AtariMem::rambo320Bank(pb) : AtariMem::xe130Bank(pb);
    int base = (bank << 14) & (AtariMem::EXT_SIZE - 1);
    for (int o = 0; o < 16384; o++) m.cpuWrite(0x4000 + o, patExt(base + o));
  }
  for (int i = 0; i < 65536; i++) m.ram[i] = (uint8_t)patRam(i);

  uint32_t h = 2166136261u; long long n = 0;
  for (int pb = 0; pb < 256; pb++) {
    setPortB(pb);
    for (int a = 0; a < 65536; a++) {
      if (a >= 0xD000 && a < 0xD800) continue;
      mix(h, (uint32_t)m.cpuRead(a));
      mix(h, (uint32_t)m.anticRead(a));
      n++;
    }
  }
  *outReads = n;
  return h;
}

// ---------------------------------------------------------------
//  TEST 3: jak rychle to na tomhle telefonu bezi
// ---------------------------------------------------------------
static double speedTest(long long *outInstr) {
  g_salt = 12345; g_ovl.clear();
  Cpu6502 cpu(T_RD, T_WR);
  cpu.c.pc = 0x2000; cpu.c.sp = 0xFF;
  // maly program v pameti: smycka INX / BNE
  g_ovl[0x2000] = 0xE8;                       // INX
  g_ovl[0x2001] = 0xD0; g_ovl[0x2002] = 0xFD; // BNE -3
  timespec t0{}, t1{};
  clock_gettime(CLOCK_MONOTONIC, &t0);
  const long long N = 20000000;
  for (long long i = 0; i < N; i++) cpu.step();
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
  *outInstr = N;
  return sec;
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(JNIEnv *env, jclass) {
  char buf[1024];
  long long instr = 0, reads = 0, sInstr = 0;

  timespec a{}, b{};
  clock_gettime(CLOCK_MONOTONIC, &a);
  uint32_t hCpu = cpuSelfTest(200, 7u, &instr);
  clock_gettime(CLOCK_MONOTONIC, &b);
  double cpuSec = (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;

  clock_gettime(CLOCK_MONOTONIC, &a);
  uint32_t hMem = memSelfTest(&reads);
  clock_gettime(CLOCK_MONOTONIC, &b);
  double memSec = (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;

  double spdSec = speedTest(&sInstr);
  double mips = (spdSec > 0) ? (sInstr / spdSec / 1e6) : 0.0;

  // 6502 v Atari bezi na 1,77 MHz -> kolik "Atari" zvladne tenhle telefon
  double realtimeX = mips * 1e6 / 1773447.0;

  snprintf(buf, sizeof(buf),
    "{\"cpuHash\":\"%08X\",\"cpuInstr\":%lld,\"cpuSec\":%.2f,"
    "\"memHash\":\"%08X\",\"memReads\":%lld,\"memSec\":%.2f,"
    "\"mips\":%.2f,\"realtimeX\":%.1f,"
    "\"abi\":\"%s\",\"charSigned\":%d}",
    hCpu, instr, cpuSec, hMem, reads, memSec, mips, realtimeX,
#if defined(__aarch64__)
    "arm64-v8a",
#elif defined(__arm__)
    "armeabi-v7a",
#else
    "jine",
#endif
    ((char)-1 < 0) ? 1 : 0);

  ALOG("BUILD2SA14 ATARI_SELFTEST cpu=%08X instr=%lld %.2fs | mem=%08X reads=%lld %.2fs | %.2f MIPS = %.1fx Atari",
       hCpu, instr, cpuSec, hMem, reads, memSec, mips, realtimeX);
  return env->NewStringUTF(buf);
}



// ---------------------------------------------------------------
//  SKUTECNY STROJ: OS z ROM, BASIC, self-test
// ---------------------------------------------------------------
static Machine   *g_stroj = nullptr;
static AnticView *g_view  = nullptr;

static void zaloz() {
  if (!g_stroj) g_stroj = new Machine();
  if (!g_view)  g_view  = new AnticView();
  g_stroj->mem.os  = NAP_OS_ROM;
  g_stroj->mem.bas = NAP_BASIC_ROM;
  g_stroj->view    = g_view;      // obraz vznika radek po radku behem emulace
}

/** Studeny start: reset a nechat OS nabehnout. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_bootNative(JNIEnv *env, jclass, jint snimku) {
  zaloz();
  std::memset(g_stroj->mem.ram, 0, sizeof(g_stroj->mem.ram));
  g_stroj->reset();
  g_stroj->consol = 7;
  g_stroj->line = 0; g_stroj->frame = 0;
  for (int f = 0; f < snimku && !g_stroj->cpu.c.jam; f++) g_stroj->runFrame();
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"pc\":%d,\"jam\":%s,\"dmactl\":%d,\"dlist\":%d,\"portb\":%d,\"snimku\":%lld}",
    g_stroj->cpu.c.pc, g_stroj->cpu.c.jam ? "true" : "false",
    g_stroj->dmactl, g_stroj->dlistAddr(), g_stroj->mem.portB(), g_stroj->frame);
  ALOG("BUILD2SA18 ATARI_BOOT PC=$%04X DMACTL=$%02X DLIST=$%04X PORTB=$%02X snimku=%lld",
       g_stroj->cpu.c.pc, g_stroj->dmactl, g_stroj->dlistAddr(),
       g_stroj->mem.portB(), g_stroj->frame);
  return env->NewStringUTF(buf);
}

/** Stisk klavesy (kod KBCODE) a nekolik snimku, aby ji OS prevzal. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_keyNative(JNIEnv *, jclass, jint kod, jint snimku) {
  if (!g_stroj) return;
  g_stroj->klavesa(kod);
  for (int f = 0; f < snimku && !g_stroj->cpu.c.jam; f++) g_stroj->runFrame();
}

/** Konzolove klavesy: bit0 START, bit1 SELECT, bit2 OPTION. 0 = stisknuto. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_consolNative(JNIEnv *, jclass, jint maska, jint snimku) {
  if (!g_stroj) return;
  g_stroj->consol = maska & 7;
  for (int f = 0; f < snimku && !g_stroj->cpu.c.jam; f++) g_stroj->runFrame();
  g_stroj->consol = 7;
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runNative(JNIEnv *, jclass, jint snimku) {
  if (!g_stroj) return;
  for (int f = 0; f < snimku && !g_stroj->cpu.c.jam; f++) g_stroj->runFrame();
}

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/** Vykresli aktualni obraz a vrati ho jako base64 RGB. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_screenNative(JNIEnv *env, jclass) {
  if (!g_stroj || !g_view) return env->NewStringUTF("{\"chyba\":\"stroj nebezi\"}");
  // Obraz uz je hotovy - vznikl radek po radku behem emulace snimku,
  // takze v nem sedi i zmeny barev z DLI. Tady se jen zabali.
  const int kroku = g_stroj->dlKroku;
  const int W = AnticView::W, H = AnticView::H, PX = W * H;
  std::string raw; raw.reserve(PX * 3);
  for (int i = 0; i < PX; i++) {
    const uint32_t v = g_view->fb[i];
    raw.push_back((char)(v & 0xFF));
    raw.push_back((char)((v >> 8) & 0xFF));
    raw.push_back((char)((v >> 16) & 0xFF));
  }
  std::string b64; b64.reserve((raw.size() + 2) / 3 * 4);
  for (size_t i = 0; i < raw.size(); i += 3) {
    const unsigned a0 = (unsigned char)raw[i];
    const unsigned a1 = (i + 1 < raw.size()) ? (unsigned char)raw[i + 1] : 0;
    const unsigned a2 = (i + 2 < raw.size()) ? (unsigned char)raw[i + 2] : 0;
    const unsigned t = (a0 << 16) | (a1 << 8) | a2;
    b64.push_back(B64[(t >> 18) & 63]); b64.push_back(B64[(t >> 12) & 63]);
    b64.push_back((i + 1 < raw.size()) ? B64[(t >> 6) & 63] : '=');
    b64.push_back((i + 2 < raw.size()) ? B64[t & 63] : '=');
  }
  std::string out = "{\"w\":" + std::to_string(W) + ",\"h\":" + std::to_string(H)
    + ",\"dlKroku\":" + std::to_string(kroku)
    + ",\"pc\":" + std::to_string(g_stroj->cpu.c.pc)
    + ",\"dmactl\":" + std::to_string(g_stroj->dmactl)
    + ",\"dlist\":" + std::to_string(g_stroj->dlistAddr())
    + ",\"portb\":" + std::to_string(g_stroj->mem.portB())
    + ",\"snimku\":" + std::to_string(g_stroj->frame)
    + ",\"jam\":" + std::string(g_stroj->cpu.c.jam ? "true" : "false")
    + ",\"rgb\":\"" + b64 + "\"}";
  return env->NewStringUTF(out.c_str());
}
