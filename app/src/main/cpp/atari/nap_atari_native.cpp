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
#include <cmath>
#include <map>
#include <vector>
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

/** Studeny start: reset a nechat OS nabehnout.
 *
 * BUILD2SB59: Rene - "po znovu nabootovani po self-testu skoci zelena
 * obrazovka - atari musi byt stoprocentni, jinak se nam to sesype jako
 * domecek z karet." NALEZENO: puvodni reset() cistil jen CPU a PIA
 * (kvuli PORTB bankovani pameti) - ale ANTIC (rozpracovany display
 * list, dlPc/dlMode/dlKroku...), GTIA (barvy, PRIOR) a POKEY
 * (AUDF/AUDC/AUDCTL, cely zvukovy generator vcetne "kliku
 * reproduktoru") ZUSTAVALY z PREDCHOZI relace - takze druhy boot
 * NEBYL skutecny studeny start, jel dal s cizim, nesouvisejicim
 * stavem po sobe (self-test, zaseknuty CSAVE...). Nejbezpecnejsi
 * oprava neni rucne vyjmenovavat KAZDE pole zvlast (snadno se na
 * neco zapomene a bug se vrati pri pristim rozsireni) - je SMAZAT A
 * ZNOVU POSTAVIT cely stroj, presne jako skutecne vypnuti a zapnuti
 * napajeni. g_view se stavi znovu tak, protoze drzi svuj vlastni
 * stav (rozkreslena obrazovka, DLI fronta).
 */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_bootNative(JNIEnv *env, jclass, jint snimku) {
  delete g_stroj; g_stroj = nullptr;
  delete g_view;  g_view  = nullptr;
  zaloz();
  // BUILD2SB59: ram[] nema v deklaraci vychozi inicializator - `new
  // Machine()` sama o sobe NEZARUCUJE vynulovanou pamet (na rozdil od
  // polí s '={}'), takze rucni vycisteni tu porad musi zustat, i kdyz
  // uz mame cerstvy objekt.
  std::memset(g_stroj->mem.ram, 0, sizeof(g_stroj->mem.ram));
  // BUILD2SB61: KRITICKA CHYBA Z B255 - pri prepisu na "smaz a postav
  // znovu" jsem omylem VYNECHAL tenhle radek. `new Machine()` SAMA
  // O SOBE nenacte reset vektor ($FFFC/$FFFD) a neskoci tam - to dela
  // AZ cpu.reset() uvnitr Machine::reset(). Bez nej CPU zustane na
  // PC=0 (vychozi CpuState()) a 600 snimku bezi uplne naprazdno -
  // presne to Rene videl (PC=0, DMACTL=0, DLIST=0 v logu). Rene mel
  // pravdu, ze "dostatecne" nestacilo - tohle byla regrese, ne oprava.
  g_stroj->reset();
  g_stroj->consol = 7;
  for (int f = 0; f < snimku && !g_stroj->cpu.c.jam; f++) g_stroj->runFrame();
  // BUILD2SB75: viz komentar u srovnatSledovaniZvuku() - po tomhle
  // velkem synchronnim bloku snimku srovnat sledovani zvuku, at
  // dalsi genAudio() nedostane zastarala/namackana data.
  g_stroj->srovnatSledovaniZvuku();
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
  // BUILD2SB75: viz komentar u srovnatSledovaniZvuku() v machine.h.
  // POZOR: tenhle prah (>10) je zamerne NAD strop normalni "dohaneci"
  // smycky (max 10 snimku/tik, a ty se navic posouvaji po JEDNOM
  // snimku pres samostatna volani, ne jednim runNative(10)) - takze
  // normalni beh (1 snimek + genAudio hned po nem) se tenhle blok
  // NIKDY netyka. Zasahuje jen SKUTECNE velke synchronni bloky
  // (self-test=400, napsani textu=60, reset=60), kde genAudio() beha
  // az POTOM.
  if (snimku > 10) g_stroj->srovnatSledovaniZvuku();
}

// BUILD2SB59: Rene - "pridej tlacitko RESET." Skutecne Atari RESET
// tlacitko NEMAZE pamet ani hardwarove registry (na rozdil od
// bootNative(), ktery ted dela poradny STUDENY start - viz komentar
// tam) - jen znovu nahodi procesor na reset vektor, presne jako
// Machine::reset() uz dela. Uzitecne i jako zachranna brzda, kdyz se
// ROM nekde zasekne (napr. cekani na kazetovy port, ktery appka
// zatim neemuluje).
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_resetNative(JNIEnv *, jclass) {
  if (!g_stroj) return;
  g_stroj->reset();
  g_stroj->consol = 7;
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

// BUILD2SB55: Rene - "zrus pomocna tlacitka zvuku a grafiky. Po
// botovani nabehne okamzite realne atari s opravdovym snimkovanim. Po
// BYE self test - a tam v testu zvuku pobezi zvuk presne tak jak na
// realnem atari." Cely jednorazovy "snimek zvuku na pozadani" pristup
// (puvodni audioNative, B249/B250) je pryc - misto neho PRUBEZNY,
// NEUSTALE STREAMOVANY zvuk, volany znovu a znovu z JS smycky (~50x/s,
// stejne tempo jako video), presne 1 snimek zvuku na 1 snimek obrazu.
//
// KLICOVY ROZDIL od stareho audioNative(): TADY se stav generatoru
// (pokeyAudio) NIKDY NEVYNULUJE - prubezne navazuje tam, kde skoncil
// predchozi snimek, presne jak to dela skutecny POKEY. Vynulovani
// davalo smysl jen pro "nezavisly snimek na pozadani" (B253), ne pro
// opravdove prubezne prehravani.
// BUILD2SB55: lehke cteni AUDF/AUDC/AUDCTL jen pro ridke logovani
// (viz atariAudioChunk v Jave) - kompaktni text, zadny JSON navic.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_regsNative(JNIEnv *env, jclass) {
  if (!g_stroj) return env->NewStringUTF("?");
  char buf[144];
  snprintf(buf, sizeof(buf), "%d,%d,%d,%d|%d,%d,%d,%d|%d|spk=%d|kliku=%lld|jam=%d|pc=%d",
           g_stroj->audf[0], g_stroj->audf[1], g_stroj->audf[2], g_stroj->audf[3],
           g_stroj->audc[0], g_stroj->audc[1], g_stroj->audc[2], g_stroj->audc[3],
           g_stroj->audctl, g_stroj->gtiaSpeakerBit, g_stroj->gtiaKlikPocitadlo,
           g_stroj->cpu.c.jam ? 1 : 0, g_stroj->cpu.c.pc);
  return env->NewStringUTF(buf);
}

// BUILD2SB62: Rene - "po CSAVE a RETURN je zvuk ve smycce nesmyslene."
// NALEZENO: kdyz procesor ZASEKNE (JAM - narazi na neplatnou/
// neimplementovanou instrukci, coz je presne to, co se stava behem
// CSAVE, protoze appka jeste neemuluje kazetovy port a OS rutina se s
// tim neumi vyporadat), runNative()/bootNative() prestanou volat
// BUILD2SB76: Rene - "udelej realny WAV, budu ho testovat na skutecnem
// Atari, ne na Altirre - te neverim." Cely smysl: pokud jadro
// SPRAVNE emuluje CPU+ROM+POKEY (a to uz je overeno testy vyse -
// beep pocty, motor timing, ERROR 138 vse sedi presne), pak zvuk,
// ktery genAudio() BEHEM CSAVE vyrobi, JE TA SPRAVNA KAZETOVA
// NAHRAVKA - zadny zvlastni "FSK enkoder" psat nemusim, ROM sam
// pise spravne hodnoty do AUDF/AUDC/GTIA behem cele operace presne
// jako na realnem hardwaru, staci to jen ZACHYTIT PO CELOU DOBU,
// misto jen v kratkych kouscich jako normalni beh.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_atariZachytitCsaveZvukNative(JNIEnv *env, jclass, jint celkemSnimku) {
  if (!g_stroj) return env->NewStringUTF("");
  const double SR = 44100.0;
  const int vzorkuNaSnimek = 882; // 44100/50 - presne 1 PAL snimek
  const int n = celkemSnimku > 0 ? celkemSnimku : 1;

  std::vector<float> buf((size_t)n * vzorkuNaSnimek);
  for (int f = 0; f < n; f++) {
    if (!g_stroj->cpu.c.jam) g_stroj->runFrame();
    float *cil = buf.data() + (size_t)f * vzorkuNaSnimek;
    if (g_stroj->cpu.c.jam) {
      std::memset(cil, 0, vzorkuNaSnimek * sizeof(float));
    } else {
      g_stroj->genAudio(cil, vzorkuNaSnimek, SR);
    }
  }

  std::string raw; raw.reserve(buf.size() * 2);
  for (float f : buf) {
    if (f > 1.0f) f = 1.0f; else if (f < -1.0f) f = -1.0f;
    int16_t v = (int16_t)std::lround(f * 32767.0);
    raw.push_back((char)(v & 0xFF));
    raw.push_back((char)((v >> 8) & 0xFF));
  }
  std::string b64; b64.reserve((raw.size() + 2) / 3 * 4);
  for (size_t i = 0; i < raw.size(); i += 3) {
    const unsigned a0 = (unsigned char)raw[i];
    const unsigned a1 = (i + 1 < raw.size()) ? (unsigned char)raw[i + 1] : 0;
    const unsigned a2 = (i + 2 < raw.size()) ? (unsigned char)raw[i + 2] : 0;
    const unsigned t = (a0 << 16) | (a1 << 8) | a2;
    b64.push_back(B64[(t >> 18) & 63]); b64.push_back(B64[(t >> 12) & 63]);
    b64.push_back(i + 1 < raw.size() ? B64[(t >> 6) & 63] : '=');
    b64.push_back(i + 2 < raw.size() ? B64[t & 63] : '=');
  }
  return env->NewStringUTF(b64.c_str());
}

// runFrame() (spravne), ALE audioChunkNative() na to NEBRALA OHLED -
// dal cetla posledni, ZAMRZLE registry a poctive je porad dokola
// prehravala jako "spravny" tón. Realny hardware, kdyz spadne, prestane
// hrat - nezacykli se na poslednim tonu donekonecna. Dokud neni
// kazetovy port hotovy (a CSAVE tim padem nezaseknuty), tohle aspon
// zajisti, ze vysledek zaseknuti je TICHO, ne nesmyslny bzukot.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_audioChunkNative(JNIEnv *env, jclass, jint pocetVzorku) {
  if (!g_stroj) return env->NewStringUTF("");
  const double SR = 44100.0;
  const int n = pocetVzorku > 0 ? pocetVzorku : 1;
  std::vector<float> tmp(n);
  if (g_stroj->cpu.c.jam) {
    std::memset(tmp.data(), 0, tmp.size() * sizeof(float));
  } else {
    g_stroj->genAudio(tmp.data(), n, SR);
  }

  std::string raw; raw.reserve((size_t)n * 2);
  for (int i = 0; i < n; i++) {
    float f = tmp[i];
    if (f > 1.0f) f = 1.0f; else if (f < -1.0f) f = -1.0f;
    int16_t v = (int16_t)std::lround(f * 32767.0);
    raw.push_back((char)(v & 0xFF));
    raw.push_back((char)((v >> 8) & 0xFF));
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
  return env->NewStringUTF(b64.c_str());
}
