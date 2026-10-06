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
#include <atomic>           // B287: nativni OpenSL zvuk (kruhovy buffer)
#include <SLES/OpenSLES.h>  // B287: CESTA A - zvuk primo z jadra, bez JS/Java
#include <SLES/OpenSLES_Android.h>
// B292: NOVE JADRO presne po cyklech (6502 + ANTIC + GTIA + POKEY + PIA,
// nap_atari_6502.h + nap_atari_machine.h). Stare jadro (nap_atari_cpu.*,
// nap_atari_mem.h, nap_atari_video.h, nap_atari_pokey.h, nap_atari_xex.h)
// je z appky odstranene - viz komentar na zacatku nap_atari_machine.h.
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_selftest.h"
#include "nap_atari_tape.h"
#include "nap_atari_audio_ring.h"  // B287: cista (bez JNI) cast - viz test_b287
// BUILD2SC2: skutecny font (Chakra Petch, presne jako schvaleny navrh) pres
// stb_truetype.h - jediny .cpp v projektu, kde se STB_TRUETYPE_IMPLEMENTATION
// skutecne preklada (standardni vzorec pro "single header" knihovny: vsude
// jinde, vcetne nap_atari_device.h (B291), se jen deklaruje bez teto makro -
// overeno primo ve zdroji stb_truetype.h, ze implementace NENI zavisla na
// poradi/vicenasobnem includu deklaraci, jen na tomhle makru).
#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"
// KRITICKE: nap_atari_device.h nize taky dela #include stb_truetype.h
// (jen pro deklarace). Kdyby STB_TRUETYPE_IMPLEMENTATION zustalo
// definovane, tenhle druhy #include by znovu zkompiloval CELOU
// implementaci -> "redefinition" chyby (chyceno primo v test_b2sc1 -
// standalone g++ build to odhalil driv, nez by to zkazilo NDK/CI build).
#undef STB_TRUETYPE_IMPLEMENTATION
// B291: cele zarizeni Atari 130XE ze schvaleneho navrhu (nahrazuje
// nap_atari_keyboard.h z B289/B290 - ta umela jen klavesnici a konzoli)
#include "nap_atari_device.h"
#include "nap_atari_runtime.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <sys/resource.h>

// BUILD2SB79: presunuto sem (z puvodniho mista dale v souboru) - musi
// byt dostupne uz pro bootNative() vyse, ktera ted take zachytava a
// koduje zvuk do base64 (stejny duvod jako atariZachytitCsaveZvukNative).
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "NAPATARI", __VA_ARGS__)

using namespace nap;

// ---------------------------------------------------------------
//  SAMOKONTROLA JADRA (B292: nove jadro - viz nap_atari_selftest.h)
//  Telefon musi dat presne stejna cisla jako pocitac.
// ---------------------------------------------------------------
static std::mutex g_mSelfTest;

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(JNIEnv *env, jclass) {
  std::lock_guard<std::mutex> zamekTestu(g_mSelfTest);
  char buf[1024];
  long long instr = 0, reads = 0, sInstr = 0;

  timespec a{}, b{};
  clock_gettime(CLOCK_MONOTONIC, &a);
  uint32_t hCpu = nap::selftest::cpuHash(200, 7u, &instr);
  clock_gettime(CLOCK_MONOTONIC, &b);
  double cpuSec = (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;

  clock_gettime(CLOCK_MONOTONIC, &a);
  uint32_t hMem = nap::selftest::memHash(&reads);
  clock_gettime(CLOCK_MONOTONIC, &b);
  double memSec = (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;

  // rychlost: 150 snimku skutecneho startu OS (cely stroj po cyklech)
  double atariSec = 0;
  double spdSec = nap::selftest::speed(NAP_OS_ROM, NAP_BASIC_ROM, &sInstr, &atariSec);
  double mips = (spdSec > 0) ? (sInstr / spdSec / 1e6) : 0.0;
  // kolikrat rychleji nez skutecne Atari (1 = presne realny cas)
  double realtimeX = (spdSec > 0) ? atariSec / spdSec : 0.0;

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

  ALOG("B292 ATARI_SELFTEST cpu=%08X instr=%lld %.2fs | mem=%08X reads=%lld %.2fs | %.2f MIPS = %.1fx realny cas Atari (cely stroj po cyklech)",
       hCpu, instr, cpuSec, hMem, reads, memSec, mips, realtimeX);
  return env->NewStringUTF(buf);
}



// ---------------------------------------------------------------
//  SKUTECNY STROJ: OS z ROM, BASIC, self-test
// ---------------------------------------------------------------
static Machine   *g_stroj = nullptr;
static AnticView *g_view  = nullptr;
// B291: stroj ted krokuje VLASTNI nativni vlakno (emulace zarizeni v HELP),
// ne JS smycka. Kazdy pristup ke g_stroj/g_view - z JNI i z vlakna - proto
// probiha POD TIMHLE ZAMKEM, aby se dve vlakna nikdy nepotkala uvnitr stroje.
static std::mutex g_mStroj;

static void zaloz() {
  if (!g_stroj) g_stroj = new Machine();
  if (!g_view)  g_view  = new AnticView();
  g_stroj->mem.os  = NAP_OS_ROM;
  g_stroj->mem.bas = NAP_BASIC_ROM;
  g_stroj->view    = g_view;      // obraz vznika radek po radku behem emulace
  g_stroj->tapeCapture = true;    // B292: linka SIO DATA OUT pro CSAVE -> WAV
}

// ===================================================================
//  B287: NATIVNI ZVUK (OpenSL ES) - "CESTA A" I PRO ATARI
//
//  Rene po B286 (log pak jeste jednou dukladne overeno primo v kodu -
//  viz PREDAVACI_PROTOKOL): appka ma dve ODDELENE Atari jadra - stare
//  cisto-JS (emu_vbxe, hlavni tlacitko menu appky) a tohle C++
//  (emu_atari_cpp, pod tlacitkem HELP). Rene: "chci ciste jadro atari emu
//  v c++ - Java odhaduje a to je problem... vyzaduji aby... bylo opravdu
//  emu atari pod tlacitkem HELP ciste v c++." Driv i tohle C++ jadro
//  prehravalo zvuk OKLIKOU pres JS Web Audio API (index.html:
//  dalsiZvukStart + rucni odhad/orezavani fronty, zavedeno B285/B286) -
//  presne ta "odhadovaci" vrstva, co Rene nechtel. Ted hraje PRIMO z
//  jadra pres OpenSL ES - stejnym, uz overenym a v produkci bezicim
//  vzorem jako PS1 (nap_ps1_native.cpp, "CESTA A: zvuk bez Javy").
//  OpenSL ES funguje od API 9 (na rozdil od AAudio, ktere chce API 26+),
//  takze jede i na minSdk 24 teto appky.
//
//  POKEY/genAudio() vraci MONO vzorky <-1,1> - OpenSL tu hraje stereo
//  (L=R duplikovano), aby sel pouzit presne stejny, uz overeny format
//  (44100Hz, 16-bit, 2 kanaly) jako u PS1, beze zmeny formatu.
//
//  Kruhovy buffer + zisk/oriznuti/int16 prevod jsou SCHVALNE vytazene do
//  nap_atari_audio_ring.h (zadna JNI/SLES zavislost) - overeno CLI testem
//  (test_b287_atari_audio_ring.cpp), ne jen odhadnuto. Samotne OpenSL ES
//  volani nize uz CLI test neumi overit - to umi jen realny telefon.
// ===================================================================
#define NAP_ATARI_ARING_SHORTS (1u << 17)  // ~1.49s stereo @44100Hz - stejna kapacita jako overeny PS1 vzor
static AtariAudioRing<NAP_ATARI_ARING_SHORTS> g_atariRing;
static std::atomic<long long> g_atariPodtekani{0};  // kolikrat OpenSL callback nemel dost vzorku k dispozici

// Zesili (4x - presne jako drive JS GainNode), orizne a preda kruhovemu
// bufferu. Vola se ze VSECH mist, kde jadro genAudio() pouziva (bootNative,
// atariZachytitCsaveZvukNative, audioChunkNative) - genAudio() samotne se
// timhle NIJAK nemeni (stejny pocet volani, stejne tempo jako driv), jen
// se navic vzorky, co uz tak jako tak vznikly, posilaji do fronty pro
// nativni prehravani misto do JS.
static void nap_atari_audio_push(const float *mono, int n) {
  if (!mono || n <= 0) return;
  std::vector<int16_t> stereo((size_t)n * 2);
  nap::atariGainClampToStereoInt16(mono, n, 4.0f, stereo.data());
  g_atariRing.write(stereo.data(), (unsigned)stereo.size());
}

#define NAP_ATARI_SL_BLOCK_FRAMES 1024
#define NAP_ATARI_SL_BLOCKS 4  // 4 x ~23ms = ~93ms rezervy - presne jako u PS1

static SLObjectItf s_atariSlEngineObj = nullptr;
static SLEngineItf s_atariSlEngine = nullptr;
static SLObjectItf s_atariSlMixObj = nullptr;
static SLObjectItf s_atariSlPlayerObj = nullptr;
static SLPlayItf   s_atariSlPlay = nullptr;
static SLAndroidSimpleBufferQueueItf s_atariSlQueue = nullptr;
static std::atomic<bool> s_atariSlReady{false};
// B291: otevreni/zavreni zvuku se muze sejit z vice vlaken (UI vlakno pri
// vstupu do HELP + JS most pri nacteni stranky) - jen jedno najednou,
// jinak by vznikly dva prehravace (ozvena).
static std::mutex g_mSl;
static int16_t s_atariSlBlocks[NAP_ATARI_SL_BLOCKS][NAP_ATARI_SL_BLOCK_FRAMES * 2];
static int     s_atariSlNext = 0;

// OpenSL si rekne o dalsi blok - naplnime ho z kruhove fronty.
static void nap_atari_sl_callback(SLAndroidSimpleBufferQueueItf bq, void*) {
  size_t need = NAP_ATARI_SL_BLOCK_FRAMES * 2;  // shortu (stereo)
  int16_t *blk = s_atariSlBlocks[s_atariSlNext];
  s_atariSlNext = (s_atariSlNext + 1) % NAP_ATARI_SL_BLOCKS;  // dalsi blok - nikdy neprepisujeme ten, co prave hraje
  size_t take = g_atariRing.read(blk, (unsigned)need);
  if (take < need) {
    g_atariPodtekani.fetch_add(1);
    // Misto tvrdeho ticha (lupanec) dozniva posledni vzorek - pri kratkem
    // vypadku je to slyset mnohem min (presne stejny trik jako u PS1).
    int16_t lastL = take >= 2 ? blk[take - 2] : 0;
    int16_t lastR = take >= 1 ? blk[take - 1] : 0;
    for (size_t i = take; i + 1 < need; i += 2) {
      lastL = (int16_t)(lastL * 7 / 8);
      lastR = (int16_t)(lastR * 7 / 8);
      blk[i] = lastL; blk[i + 1] = lastR;
    }
  }
  (*bq)->Enqueue(bq, blk, need * sizeof(int16_t));
}

static void nap_atari_sl_open(void) {
  std::lock_guard<std::mutex> slZamek(g_mSl);
  if (s_atariSlReady) return;
  if (slCreateEngine(&s_atariSlEngineObj, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlEngineObj)->Realize(s_atariSlEngineObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlEngineObj)->GetInterface(s_atariSlEngineObj, SL_IID_ENGINE, &s_atariSlEngine) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlEngine)->CreateOutputMix(s_atariSlEngine, &s_atariSlMixObj, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlMixObj)->Realize(s_atariSlMixObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  SLDataLocator_AndroidSimpleBufferQueue locBufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NAP_ATARI_SL_BLOCKS };
  SLDataFormat_PCM fmt = { SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
    SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
  SLDataSource src = { &locBufq, &fmt };
  SLDataLocator_OutputMix locMix = { SL_DATALOCATOR_OUTPUTMIX, s_atariSlMixObj };
  SLDataSink sink = { &locMix, nullptr };
  const SLInterfaceID ids[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
  const SLboolean req[1] = { SL_BOOLEAN_TRUE };
  if ((*s_atariSlEngine)->CreateAudioPlayer(s_atariSlEngine, &s_atariSlPlayerObj, &src, &sink, 1, ids, req) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlPlayerObj)->Realize(s_atariSlPlayerObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlPlayerObj)->GetInterface(s_atariSlPlayerObj, SL_IID_PLAY, &s_atariSlPlay) != SL_RESULT_SUCCESS) return;
  if ((*s_atariSlPlayerObj)->GetInterface(s_atariSlPlayerObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &s_atariSlQueue) != SL_RESULT_SUCCESS) return;
  (*s_atariSlQueue)->RegisterCallback(s_atariSlQueue, nap_atari_sl_callback, nullptr);
  (*s_atariSlPlay)->SetPlayState(s_atariSlPlay, SL_PLAYSTATE_PLAYING);
  memset(s_atariSlBlocks, 0, sizeof(s_atariSlBlocks));
  for (int i = 0; i < NAP_ATARI_SL_BLOCKS; i++)
    (*s_atariSlQueue)->Enqueue(s_atariSlQueue, s_atariSlBlocks[i], sizeof(s_atariSlBlocks[i]));
  s_atariSlNext = 0;
  g_atariRing.clear();
  s_atariSlReady = true;
  ALOG("B287 ATARI_ZVUK_NATIVNI OpenSL ES otevren (44100/stereo/i16, ciste C++, bez JS/Java)");
}

static void nap_atari_sl_close(void) {
  std::lock_guard<std::mutex> slZamek(g_mSl);
  if (s_atariSlPlayerObj) { (*s_atariSlPlayerObj)->Destroy(s_atariSlPlayerObj); s_atariSlPlayerObj = nullptr; }
  if (s_atariSlMixObj)    { (*s_atariSlMixObj)->Destroy(s_atariSlMixObj);       s_atariSlMixObj = nullptr; }
  if (s_atariSlEngineObj) { (*s_atariSlEngineObj)->Destroy(s_atariSlEngineObj); s_atariSlEngineObj = nullptr; }
  s_atariSlEngine = nullptr; s_atariSlPlay = nullptr; s_atariSlQueue = nullptr;
  bool bylOtevreny = s_atariSlReady.load();
  s_atariSlReady = false;
  g_atariRing.clear();
  if (bylOtevreny) ALOG("B287 ATARI_ZVUK_NATIVNI OpenSL ES zavren");
}

/** Start/stop nativniho zvuku - volano ze zivotniho cyklu obrazovky HELP
 *  (MainActivity: jeNactene()/onPageStarted-Finished/onPause/onDestroy/
 *  onResume), presne jako u PS1. idempotentni (bezpecne volat vicekrat). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_atariAudioStartNative(JNIEnv*, jclass) {
  nap_atari_sl_open();
}
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_atariAudioStopNative(JNIEnv*, jclass) {
  nap_atari_sl_close();
}
/** Kratky diagnosticky radek PRIMO ze stavu nativni fronty (zadny JS
 *  odhad) - nahrazuje stary JS vypocet ZVUK_PODTEKANI. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_atariZvukDiagNative(JNIEnv *env, jclass) {
  char buf[128];
  unsigned fillShorts = g_atariRing.avail();
  int fillMs = (int)((fillShorts / 2) * 1000 / 44100);
  snprintf(buf, sizeof(buf), "ZVUK_NATIVNI podtekani=%lld fronta=%dms otevreno=%s",
           (long long)g_atariPodtekani.load(), fillMs, s_atariSlReady ? "ano" : "ne");
  return env->NewStringUTF(buf);
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
  std::lock_guard<std::mutex> zamek(g_mStroj);
  delete g_stroj; g_stroj = nullptr;
  delete g_view;  g_view  = nullptr;
  zaloz();
  // BUILD2SB59: ram[] nema v deklaraci vychozi inicializator - `new
  // Machine()` sama o sobe NEZARUCUJE vynulovanou pamet (na rozdil od
  // polí s '={}'), takze rucni vycisteni tu porad musi zustat, i kdyz
  // uz mame cerstvy objekt.
  // B292: pamet uz po zapnuti obsahuje vzor DRAM skutecneho 130XE
  // (Machine::coldInit - dram130xe), ne nuly.
  // BUILD2SB61: KRITICKA CHYBA Z B255 - pri prepisu na "smaz a postav
  // znovu" jsem omylem VYNECHAL tenhle radek. `new Machine()` SAMA
  // O SOBE nenacte reset vektor ($FFFC/$FFFD) a neskoci tam - to dela
  // AZ cpu.reset() uvnitr Machine::reset(). Bez nej CPU zustane na
  // PC=0 (vychozi CpuState()) a 600 snimku bezi uplne naprazdno -
  // presne to Rene videl (PC=0, DMACTL=0, DLIST=0 v logu). Rene mel
  // pravdu, ze "dostatecne" nestacilo - tohle byla regrese, ne oprava.
  g_stroj->reset();
  g_stroj->consol = 7;
  // BUILD2SB79: stejna oprava jako u atariNapisText/self-testu -
  // misto tiseho behu 600 snimku a nasledneho zahozeni (B268 fix)
  // ZACHYTIT zvuk PO CELOU DOBU bootu, at je "dlouhy boot zvuk"
  // (Rene: "pri startu ma dlouhy zvuk, jen se ctvereckem, pak READY")
  // SKUTECNE slyset, misto zahozeny driv, nez ho appka stihne prehrat.
  const double SR_BOOT = 44100.0;
  const int vzorkuNaSnimekBoot = 882;
  std::vector<float> zvukBoot((size_t)snimku * vzorkuNaSnimekBoot);
  for (int f = 0; f < snimku && !g_stroj->cpu.jam; f++) {
    g_stroj->runFrame();
    float *cil = zvukBoot.data() + (size_t)f * vzorkuNaSnimekBoot;
    if (g_stroj->cpu.jam) std::memset(cil, 0, vzorkuNaSnimekBoot * sizeof(float));
    else g_stroj->genAudio(cil, vzorkuNaSnimekBoot, SR_BOOT);
  }
  // B287: stejne vzorky, co uz vznikly vyse, navic posilame do nativni
  // OpenSL fronty (zesilene+oriznute+int16 - viz nap_atari_audio_push) -
  // genAudio() samotne se timhle NEMENI (stejny pocet volani jako driv).
  nap_atari_audio_push(zvukBoot.data(), (int)zvukBoot.size());
  std::string zvukRaw; zvukRaw.reserve(zvukBoot.size() * 2);
  for (float f : zvukBoot) {
    if (f > 1.0f) f = 1.0f; else if (f < -1.0f) f = -1.0f;
    int16_t v = (int16_t)std::lround(f * 32767.0);
    zvukRaw.push_back((char)(v & 0xFF));
    zvukRaw.push_back((char)((v >> 8) & 0xFF));
  }
  std::string zvukB64; zvukB64.reserve((zvukRaw.size() + 2) / 3 * 4);
  for (size_t i = 0; i < zvukRaw.size(); i += 3) {
    const unsigned a0 = (unsigned char)zvukRaw[i];
    const unsigned a1 = (i + 1 < zvukRaw.size()) ? (unsigned char)zvukRaw[i + 1] : 0;
    const unsigned a2 = (i + 2 < zvukRaw.size()) ? (unsigned char)zvukRaw[i + 2] : 0;
    const unsigned t = (a0 << 16) | (a1 << 8) | a2;
    zvukB64.push_back(B64[(t >> 18) & 63]); zvukB64.push_back(B64[(t >> 12) & 63]);
    zvukB64.push_back(i + 1 < zvukRaw.size() ? B64[(t >> 6) & 63] : '=');
    zvukB64.push_back(i + 2 < zvukRaw.size() ? B64[t & 63] : '=');
  }
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"pc\":%d,\"jam\":%s,\"dmactl\":%d,\"dlist\":%d,\"portb\":%d,\"snimku\":%lld,\"zvuk_b64\":\"",
    g_stroj->cpu.pc, g_stroj->cpu.jam ? "true" : "false",
    g_stroj->dmactl(), g_stroj->dlistAddr(), g_stroj->mem.portB(), g_stroj->frame);
  ALOG("BUILD2SA18 ATARI_BOOT PC=$%04X DMACTL=$%02X DLIST=$%04X PORTB=$%02X snimku=%lld",
       g_stroj->cpu.pc, g_stroj->dmactl(), g_stroj->dlistAddr(),
       g_stroj->mem.portB(), g_stroj->frame);
  // BUILD2SB79: char buf[256] je PRILIS MALY pro zvukova data (mohou
  // byt megabajty) - slozit finalni JSON jako std::string, ne
  // snprintf do pevneho bufferu.
  std::string vysledek = std::string(buf) + zvukB64 + "\"}";
  return env->NewStringUTF(vysledek.c_str());
}

/** Stisk klavesy (kod KBCODE) a nekolik snimku, aby ji OS prevzal. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_keyNative(JNIEnv *, jclass, jint kod, jint snimku) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return;
  g_stroj->klavesa(kod);
  for (int f = 0; f < snimku && !g_stroj->cpu.jam; f++) g_stroj->runFrame();
}

// B283: Rene - "v self testu ti ujizdi grafika od zvuku...mozna furt
// lagy??" PRICINA: atariKonzole()/atariHelp() (Java) drzely tlacitko
// pres consolNative(maska,N)/keyNative(kod,N), ktere N snimku odbehnou
// SYNCHRONNE BEZ volani genAudio() (presne ta stejna nemoc, co
// BUILD2SB79/80 uz opravily u psani textu/self-testu-pres-BYE/bootu -
// viz srovnatSledovaniZvuku() vyse) - zvuk z tech snimku se proste
// ztratil, misto aby se poslal do JS, coz zpusobovalo ZVUK_PODTEKANI
// presne v okamziku stisku SELECT/START/OPTION/HELP (primo potvrzeno
// Reneho logem - pocet vyskocil o 165 prave pri KONZOLE:start). Tenhle
// setter ODDELUJE "drzet tlacitko" od "odbehnout snimky" (puvodni
// consolNative dela obojí najednou a na konci VZDY sama pusti tlacitko
// zpet na 7 - nejde tak z Javy "drzet pres vice volani"), aby Java
// strana (atariKonzole) mohla mezitim pouzit JIZ existujici
// zachytitCsaveZvukSafe() - presne stejny, uz overeny vzorec jako u
// atariDoSelfTestu/atariNapisText - a zvuk z celeho useku tak dojde do
// JS stejne poctive jako cokoli jineho.
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_consolSetNative(JNIEnv *, jclass, jint maska) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return;
  g_stroj->consol = maska & 7;
}

/** Konzolove klavesy: bit0 START, bit1 SELECT, bit2 OPTION. 0 = stisknuto. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_consolNative(JNIEnv *, jclass, jint maska, jint snimku) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return;
  g_stroj->consol = maska & 7;
  for (int f = 0; f < snimku && !g_stroj->cpu.jam; f++) g_stroj->runFrame();
  g_stroj->consol = 7;
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runNative(JNIEnv *, jclass, jint snimku) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return;
  for (int f = 0; f < snimku && !g_stroj->cpu.jam; f++) g_stroj->runFrame();
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
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return;
  g_stroj->reset();
  g_stroj->consol = 7;
}

/** Vykresli aktualni obraz a vrati ho jako base64 RGB. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_screenNative(JNIEnv *env, jclass) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj || !g_view) return env->NewStringUTF("{\"chyba\":\"stroj nebezi\"}");
  // Obraz uz je hotovy - vznikl radek po radku behem emulace snimku,
  // takze v nem sedi i zmeny barev z DLI. Tady se jen zabali.
  const int kroku = g_stroj->dlKroku;
  const int W = AnticView::W, H = AnticView::H, PX = W * H;
  std::string raw; raw.reserve(PX * 3);
  for (int i = 0; i < PX; i++) {
    // fb ma 4 body na barevny takt (sirka 768) - sem kazdy druhy (384)
    const uint32_t v = g_view->fb[(i / W) * AnticView::FW + (i % W) * 2];
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
    + ",\"pc\":" + std::to_string(g_stroj->cpu.pc)
    + ",\"dmactl\":" + std::to_string(g_stroj->dmactl())
    + ",\"dlist\":" + std::to_string(g_stroj->dlistAddr())
    + ",\"portb\":" + std::to_string(g_stroj->mem.portB())
    + ",\"snimku\":" + std::to_string(g_stroj->frame)
    + ",\"jam\":" + std::string(g_stroj->cpu.jam ? "true" : "false")
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
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return env->NewStringUTF("?");
  char buf[144];
  snprintf(buf, sizeof(buf), "%d,%d,%d,%d|%d,%d,%d,%d|%d|spk=%d|kliku=%lld|jam=%d|pc=%d",
           g_stroj->audf[0], g_stroj->audf[1], g_stroj->audf[2], g_stroj->audf[3],
           g_stroj->audc[0], g_stroj->audc[1], g_stroj->audc[2], g_stroj->audc[3],
           g_stroj->audctl, g_stroj->gtiaSpeakerBit, g_stroj->gtiaKlikPocitadlo,
           g_stroj->cpu.jam ? 1 : 0, g_stroj->cpu.pc);
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
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return env->NewStringUTF("");
  const double SR = 44100.0;
  const int vzorkuNaSnimek = 882; // 44100/50 - presne 1 PAL snimek
  const int n = celkemSnimku > 0 ? celkemSnimku : 1;

  std::vector<float> buf((size_t)n * vzorkuNaSnimek);
  for (int f = 0; f < n; f++) {
    if (!g_stroj->cpu.jam) g_stroj->runFrame();
    float *cil = buf.data() + (size_t)f * vzorkuNaSnimek;
    if (g_stroj->cpu.jam) {
      std::memset(cil, 0, vzorkuNaSnimek * sizeof(float));
    } else {
      g_stroj->genAudio(cil, vzorkuNaSnimek, SR);
    }
  }
  // B287: viz komentar u bootNative() vyse - stejny princip.
  nap_atari_audio_push(buf.data(), (int)buf.size());

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

// BUILD2SB92: Rene - "cim vic csave tim se lag zvetsuje...doporuceni
// vyhazuj nepotrebne veci z pameti za chodu." SKUTECNA PRICINA nebyla
// primo v C++ jadru, ale v JS strance (index.html): kazde CSAVE
// spustilo nahravani WAV na PEVNYCH 50 vterin
// (setTimeout(ukoncitNahravani,50000)) BEZ OHLEDU na to, jak dlouho
// SKUTECNE CSAVE trva (~24s, primo zmereno v logu) - PRIMO POTVRZENO
// v logu Reneho appky: ulozeny WAV soubor mel VZDY presne 4418820
// bajtu = presne 50,1s zvuku, i kdyz skutecny prenos skoncil davno
// pred tim. Kazde dalsi CSAVE tak drzelo v pameti o desitky vterin
// zvuku NAVIC nez bylo potreba (~26s x 44100 x 2 bajty = pres 2MB
// navic KAZDE spusteni) - u opakovaneho pouzivani appky bez restartu
// tohle POSTUPNE zaplnovalo dostupnou pamet presne jak Rene tusil
// (pozdejsi log primo ukazal "pametVolna=0MB pametMax=256MB").
// OPRAVA: pridana tahle lehounka funkce, aby JS strana mohla primo
// zjistit, jestli kazetovy motor PRAVE BEZI - podle OFICIALNE
// zdokumentovaneho PACTL bitu (De Re Atari / PIA registry: $D302
// bit3 = "Motor Control" - 1=motor VYPNUTY, 0=motor ZAPNUTY,
// overeno na nezavislem hardwarovem referencnim zdroji, ne jen
// odhadem). POZOR: NENI to totez jako PORTB bit3 (ten uz je v tomhle
// jadru pouzity pro 130XE bankovani rozsirene pameti - viz
// xe130Bank()/rambo320Bank() v nap_atari_mem.h) - motor control je
// VYHRADNE v PACTL (pia.ctlA), ne v PORTB. JS pak sleduje PRECHOD
// motoru ze zapnuteho na vypnuty (skutecny konec CSAVE prenosu) a
// ukonci nahravani OKAMZITE misto cekani na pevny 50s limit - ten
// zustava jen jako zalozni pojistka, kdyby motor z nejakeho duvodu
// nikdy nezhasl.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_motorZapnutyNative(JNIEnv *, jclass) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return 0;
  // B292: motor bezi jen kdyz je CA2 vystup (PACTL bit5=1, bit4=1) a v nule
  // (bit3=0) - presne jako Machine::motorOn() (po zapnuti je PACTL=0 a motor
  // na skutecnem Atari stoji)
  return g_stroj->motorOn() ? 1 : 0;
}

// runFrame() (spravne), ALE audioChunkNative() na to NEBRALA OHLED -
// dal cetla posledni, ZAMRZLE registry a poctive je porad dokola
// prehravala jako "spravny" tón. Realny hardware, kdyz spadne, prestane
// hrat - nezacykli se na poslednim tonu donekonecna. Dokud neni
// kazetovy port hotovy (a CSAVE tim padem nezaseknuty), tohle aspon
// zajisti, ze vysledek zaseknuti je TICHO, ne nesmyslny bzukot.
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_audioChunkNative(JNIEnv *env, jclass, jint pocetVzorku) {
  std::lock_guard<std::mutex> zamek(g_mStroj);
  if (!g_stroj) return env->NewStringUTF("");
  const double SR = 44100.0;
  const int n = pocetVzorku > 0 ? pocetVzorku : 1;
  std::vector<float> tmp(n);
  if (g_stroj->cpu.jam) {
    std::memset(tmp.data(), 0, tmp.size() * sizeof(float));
  } else {
    g_stroj->genAudio(tmp.data(), n, SR);
  }
  // B287: hlavni, prubezny zvuk (~50x/s) - tohle je JEDINE misto, odkud
  // nativni fronta dostava zvuk BEHEM normalniho behu (self-test, hrani,
  // cokoli). JS uz tenhle vracena b64 NEPREHRAVA (viz index.html) - jen
  // ho porad vola (musi, kvuli genAudio() tempu) a vraceny base64 pouziva
  // VYHRADNE pro CSAVE->WAV nahravku, kdyz bezi.
  nap_atari_audio_push(tmp.data(), n);

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


// ===================================================================
//  B291: ZARIZENI ATARI 130XE V HELP - CELE V C++
//
//  Rene po B290: "tlacitka se nezamackavaji nemas kazetak - nemas nic !!!
//  ... to preved v helpu presne tak vcetne kazetaku a obrazovky - podle
//  toho navrhu na kterem jsme se dohodli".
//
//  PROC SE V B290 TLACITKA NEZAMACKAVALA (zjisteno v kodu, ne odhadem):
//   1) kazdy dotek posilal do JavaScriptu cely obrazek klavesnice
//      (~3,6 MB base64) dvakrat (prst dolu + nahoru), synchronne - prohlizec
//      nestihl "zmacknuty" obrazek mezi tim vubec vykreslit;
//   2) kbdTouchNative koncil hned na zacatku "if (!g_stroj) return;" -
//      dokud nebylo stisknuto NABOOTOVAT OS, dotek nedelal VUBEC NIC.
//
//  TED: zadny JavaScript ani base64. Java jen vytvori plochu (SurfaceView)
//  a posila sem souradnice prstu. Tady bezi dve vlakna:
//   - EMULACE: 50 snimku/s (PAL), jemne dorovnavane podle zaplneni
//     zvukove fronty OpenSL; klavesy/konzole/POWER z dotyku, psani textu,
//     zavadeni XEX, CSAVE -> WAV.
//   - KRESLENI: nap_atari_device.h kresli pristroj PRIMO do okna displeje
//     (ANativeWindow), jen zmenene oblasti. Stisk je videt do jednoho
//     snimku (~16 ms) a vzdy aspon 110 ms.
// ===================================================================
static long long nap_ted_ns() {
  timespec t{};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (long long)t.tv_sec * 1000000000LL + t.tv_nsec;
}
static long long nap_ted_ms() { return nap_ted_ns() / 1000000LL; }

static nap::dev::Device &devGet() {
  static nap::dev::Device *d = new nap::dev::Device();
  return *d;
}
static std::mutex g_mDev;                          // stav pristroje (dotyk x kresleni)
static std::mutex g_mEv;                           // fronta udalosti pro stroj
static std::vector<nap::dev::Ev> g_evQ;
static nap::TypeQueue g_typeQ;                     // pod g_mStroj
static nap::CsaveRecorder g_rec;                   // pod g_mStroj
static nap::XexLoader g_xex;                       // pod g_mStroj
static nap::KazetaBoot g_kazBoot;                  // B298: boot z kazety (START+OPTION, RETURN po pipnuti) - pod g_mStroj
static bool g_atrVymena = false;                   // B298: dalsi ATR jen vymenit v D1: (bez restartu) - pod g_mStroj
static nap::PsaniProgramu g_psani;                 // B299: program z TXT souboru -> BASIC / TBXL - pod g_mStroj
static std::string g_klavLog;                      // B299: co uzivatel napsal na klavesnici pristroje (do logu po RETURN) - pod g_mStroj
static int g_klavPrazdnych = 0;                    // B299: prazdne RETURN po sobe (do logu jen prvni 3)
static std::mutex g_mFrame;
static std::vector<uint32_t> g_sdilenySnimek;      // posledni snimek Atari pro kresleni
static std::atomic<unsigned long long> g_snimekSeq{0};
static std::atomic<bool> g_motorOn{false};
static std::atomic<bool> g_strojBezi{false};       // POWER (z pohledu emulace)
static std::atomic<bool> g_run{false};
static std::thread g_emuVlakno, g_kresliVlakno;
static std::mutex g_mRun;
static std::mutex g_mWake;
static std::condition_variable g_cvWake;
static bool g_wake = false;
// okno displeje (timed_mutex: zanik plochy nesmi nikdy zablokovat UI vlakno)
static std::timed_mutex g_mWin;
static ANativeWindow *g_win = nullptr;
static bool g_uvolnitOkno = false;      // zanik plochy behem kresleni - uvolni kreslici vlakno
static std::atomic<int> g_viewW{0}, g_viewH{0}, g_bufW{0}, g_bufH{0};
static bool g_needLayout = false, g_needGeometry = false, g_forceFull = false;
static bool g_formatHlaseno = false;
// vystupy pro Javu (log, hotovy WAV)
static std::mutex g_mOut;
static std::vector<std::string> g_logOut;
static std::vector<int16_t> g_wavOut;
static bool g_wavReady = false;
static std::atomic<long long> g_snimkuCelkem{0};
// START/SELECT/OPTION: program je cte jako UROVEN (ne preruseni) - i hodne
// kratke tuknuti musi stroj videt drzene aspon 5 snimku, jinak by se mohlo
// ztratit mezi dvema snimky. (Klavesy se ztratit nemuzou - jdou pres IRQ.)
static int g_consolChtene = 7;
static long long g_consolDrzetDo[3] = {0, 0, 0};
// B296: joystick 1 - z doteku na obrazovce a z herniho ovladace (bity:
// 0 nahoru, 1 dolu, 2 vlevo, 3 vpravo, 4 FIRE); START/SELECT/OPTION z
// ovladace (1/2/4 = stisknuto). Vse pod g_mStroj.
static int g_joyDotyk = 0, g_joyPad = 0, g_padConsol = 0, g_padKlav = 0;
static bool g_joyHlasenoDotyk = false, g_joyHlasenoPad = false;
static int consolEfektivni() {
  int m = g_consolChtene & 7;
  long long f = g_snimkuCelkem.load();
  for (int b = 0; b < 3; b++) if (f < g_consolDrzetDo[b]) m &= ~(1 << b);
  m &= ~g_padConsol;
  return m;
}
static void aplikujJoyLocked() {
  if (!g_stroj) return;
  const int m = g_joyDotyk | g_joyPad;
  g_stroj->porta = (0xF0 | (~m & 15)) & 0xFF;      // PORTA: joystick 1 = dolni 4 bity, 0 = stisknuto
  g_stroj->trig[0] = (m & 16) ? 0 : 1;            // TRIG0 = FIRE joysticku 1
}
// B296: co dela VBXE - do logu jen zmeny (zadny spam po snimcich)
static long long g_vbxeZapisuPred = 0, g_vbxeBlituPred = 0;
static bool g_vbxeXdlPred = false;
static int g_vbxeRezimPred = -1;

static void devLog(const std::string &s) {
  ALOG("%s", s.c_str());
  std::lock_guard<std::mutex> l(g_mOut);
  if (g_logOut.size() < 400) g_logOut.push_back(s);
}
// Priorita vlakna (nice): emulace vyrabi i zvuk, nesmi zaostavat (-8 =
// Android THREAD_PRIORITY_URGENT_DISPLAY), kresleni -4 (DISPLAY). Kdyby to
// system nedovolil, bezi vlakno dal s normalni prioritou.
static void nastavPriorituVlakna(int nice, const char *jmeno) {
  int r = setpriority(PRIO_PROCESS, 0, nice);
  char b[120];
  snprintf(b, sizeof(b), "B291 %s priorita nice=%d %s", jmeno, nice, r == 0 ? "nastavena" : "nepovolena (bezi normalne)");
  devLog(b);
}
static void pokeRender() {
  { std::lock_guard<std::mutex> l(g_mWake); g_wake = true; }
  g_cvWake.notify_one();
}
// Studeny start - smazat a postavit cely stroj znovu (jako bootNative).
// POZOR: volat jen pod g_mStroj.
static bool g_shiftZamek = false;                  // SHIFT zamceny na pristroji (pod g_mStroj)
static std::string g_statusText;                   // B295: zprava pod pristroj z emulacniho vlakna (pod g_mStroj)
static int g_statusMs = 0;
static void studenyStartLocked(int consol) {
  // B292: kazeta v magnetofonu zustava i pri vypnuti/zapnuti pocitace
  // B295: stejne tak disketa v mechanice D1:
  nap::TapeDeck paska;
  nap::AtrDisk disketa;
  if (g_stroj) { paska = std::move(g_stroj->tape); disketa = std::move(g_stroj->disk); }
  delete g_stroj; g_stroj = nullptr;
  delete g_view;  g_view  = nullptr;
  zaloz();                       // novy stroj: pamet = vzor DRAM 130XE po zapnuti
  g_stroj->reset();
  g_stroj->consol = consol & 7;
  g_stroj->tape = std::move(paska);
  g_stroj->tape.line = 1; g_stroj->tape.rxPhase = 0;
  g_stroj->disk = std::move(disketa);
  g_stroj->shiftDrzen(g_shiftZamek);
  aplikujJoyLocked();
  g_vbxeZapisuPred = g_vbxeBlituPred = 0; g_vbxeXdlPred = false; g_vbxeRezimPred = -1;
  g_typeQ.clear();
  g_rec.reset();
  g_kazBoot.zrus();
  g_psani.zrus();                // B299: novy stroj = psani programu z TXT konci
  g_klavLog.clear();
}
static void vyzvednoutVystupyLocked() {
  for (auto &s : g_rec.log) devLog(s);
  g_rec.log.clear();
  for (auto &s : g_xex.log) devLog(s);
  g_xex.log.clear();
  for (auto &s : g_kazBoot.log) devLog(s);
  g_kazBoot.log.clear();
  for (auto &s : g_psani.log) devLog(s);
  g_psani.log.clear();
  if (g_rec.maHotovo) {
    std::lock_guard<std::mutex> o(g_mOut);
    g_wavOut.swap(g_rec.hotovo);
    g_rec.hotovo.clear();
    g_rec.maHotovo = false;
    g_wavReady = true;
  }
}

static void emuVlaknoMain() {
  std::vector<float> zvuk(882), pasek(882);
  std::string statusText; int statusMs = 0;
  long long dalsi = nap_ted_ns();
  devLog("B291 EMULACE_VLAKNO start (50 snimku/s, zvuk OpenSL, vse v C++)");
  nastavPriorituVlakna(-8, "EMULACE_VLAKNO");
  while (g_run.load()) {
    // 1) udalosti z doteku na pristroji
    std::vector<nap::dev::Ev> ev;
    { std::lock_guard<std::mutex> l(g_mEv); ev.swap(g_evQ); }
    if (!ev.empty()) {
      std::lock_guard<std::mutex> l(g_mStroj);
      for (const auto &e : ev) {
        switch (e.t) {
          case nap::dev::Ev::POWER_ON: {
            g_xex.zrus();
            // B298: START/SELECT/OPTION, ktere uzivatel PRAVE DRZI na pristroji,
            // plati i pri zapnuti - jako na skutecnem 130XE (START = boot
            // z kazety, OPTION = bez BASICu). Drive se pri zapnuti pustily.
            const int drzene = (g_consolChtene & ~g_padConsol) & 7;   // prst na pristroji nebo herni ovladac
            studenyStartLocked(drzene);
            if (g_stroj) g_stroj->sioRychlyTimeout = false;
            g_strojBezi = true; g_atariRing.clear();
            g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
            devLog("B291 POWER ZAPNUTO - studeny start (pamet i hardware od nuly, jako vypinac na skrini)");
            if (g_stroj && !(drzene & 1) && g_stroj->tape.loaded) {
              // START drzeny + kazeta v magnetofonu = boot z kazety; po pipnuti RETURN sam
              g_stroj->tape.pos = 0; g_stroj->tape.rxPhase = 0; g_stroj->tape.play = true;
              g_kazBoot.start(*g_stroj);
              devLog("B298 POWER se START (+OPTION) a kazetou " + g_stroj->tape.name + " - boot z kazety (pasek pretocen na zacatek, po pipnuti RETURN)");
              g_statusText = "BOOT Z KAZETY - RETURN STISKNU SÁM"; g_statusMs = 8000;
            } else if (g_stroj && g_stroj->disk.mounted) {
              // B295: s disketou v D1: se startuje jako s hrou - OPTION drzene (BASIC vypnuty)
              g_consolDrzetDo[2] = g_snimkuCelkem.load() + 150;
              devLog("B295 POWER s disketou v D1: (" + g_stroj->disk.name + ") - OPTION drzeno pri startu, BASIC vypnuty, bootuje disketa");
              // B298: at je videt, proc se zase nahrava hra (Rene: "W3D nepomuze ani vypnout a zapnout")
              {   // nazev zkratit, at se zprava vejde na radek pod pristrojem
                std::string jm = g_stroj->disk.name; if (jm.size() > 18) jm = jm.substr(0, 16) + "..";
                g_statusText = "V D1: JE " + jm + " - VYSUNOUT: ATR/DISK"; g_statusMs = 7000;
              }
            } else if (drzene != 7) {
              devLog(std::string("B298 POWER s drzenym") + ((drzene & 1) ? "" : " START") + ((drzene & 2) ? "" : " SELECT") + ((drzene & 4) ? "" : " OPTION"));
            }
            break;
          }
          case nap::dev::Ev::POWER_OFF:
            g_strojBezi = false; g_typeQ.clear(); g_rec.reset(); g_xex.zrus(); g_atariRing.clear(); g_psani.zrus();
            devLog("B291 POWER VYPNUTO");
            break;
          case nap::dev::Ev::KEY:
            // B292: klavesa drzena, dokud je na ni prst (OS ji pak opakuje)
            if (g_stroj && g_strojBezi) {
              g_stroj->klavesa(e.v, true);
              // B299: co presne uzivatel napsal (radek do logu po RETURN) - kvuli chybam, ktere
              // na PC nejdou zopakovat (TBXL LIST u Reneho)
              const bool ret = (e.v & 0x3F) == 12;
              if (!ret) g_klavLog += nap::napScanText(e.v);
              if (ret || g_klavLog.size() > 160) {
                // prazdny RETURN (hry, potvrzovani) jen 3x po sobe, at log nezahlti
                if (!g_klavLog.empty()) g_klavPrazdnych = 0;
                if (!g_klavLog.empty() || ++g_klavPrazdnych <= 3)
                  devLog("B299 KLAVESY: " + g_klavLog + (ret ? (g_klavLog.empty() ? "<RETURN>" : " <RETURN>") : " ..."));
                g_klavLog.clear();
              }
            }
            break;
          case nap::dev::Ev::KEYUP:
            if (g_stroj) g_stroj->klavesaPustena();
            break;
          case nap::dev::Ev::SHIFT:
            g_shiftZamek = e.v != 0;
            if (g_stroj) g_stroj->shiftDrzen(g_shiftZamek);
            break;
          case nap::dev::Ev::TAPE:
            if (g_stroj) {
              g_stroj->tape.play = (e.v & 1) != 0;
              char b[200];
              snprintf(b, sizeof(b), "B292 KAZETAK %s%s - kazeta: %s, pozice %.1f s",
                       (e.v & 1) ? "PLAY" : "STOP", (e.v & 2) ? "+REC" : "",
                       g_stroj->tape.loaded ? g_stroj->tape.name.c_str() : "ZADNA",
                       g_stroj->tape.loaded ? g_stroj->tape.pos / g_stroj->tape.img.rate : 0.0);
              devLog(b);
            }
            break;
          case nap::dev::Ev::REWIND:
            if (g_stroj && g_stroj->tape.loaded) {
              g_stroj->tape.pos = 0; g_stroj->tape.rxPhase = 0;
              devLog("B292 KAZETAK REW - pasek pretocen na zacatek");
            }
            break;
          case nap::dev::Ev::FFWD:
            if (g_stroj && g_stroj->tape.loaded) {
              nap::TapeDeck &t = g_stroj->tape;
              t.pos = std::min((double)t.img.n, t.pos + 10.0 * t.img.rate); t.rxPhase = 0;
              char b[120];
              snprintf(b, sizeof(b), "B292 KAZETAK FWD - pasek +10 s (pozice %.1f / %.1f s)", t.pos / t.img.rate, t.img.seconds());
              devLog(b);
            }
            break;
          case nap::dev::Ev::CONSOL: {
            int nove = e.v & 7;
            for (int b = 0; b < 3; b++)
              if (!(nove & (1 << b)) && (g_consolChtene & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
            g_consolChtene = nove;
            if (g_stroj) g_stroj->consol = consolEfektivni();
            break;
          }
          case nap::dev::Ev::RESET:
            if (g_stroj && g_strojBezi) {
              g_stroj->reset(); g_stroj->consol = 7;
              g_kazBoot.zrus();          // B298: teply start boot z kazety nedela
              g_psani.zrus();
              devLog("B291 RESET (tlacitko RESET na pristroji - pamet zustava)");
            }
            break;
          case nap::dev::Ev::BREAK:
            if (g_stroj && g_strojBezi) {
              g_stroj->breakKey();
              devLog("B299 KLAVESY: " + g_klavLog + "<BREAK>");
              g_klavLog.clear();
            }
            break;
          case nap::dev::Ev::JOY:
            g_joyDotyk = e.v & 31;
            aplikujJoyLocked();
            if (!g_joyHlasenoDotyk) { g_joyHlasenoDotyk = true; devLog("B296 JOYSTICK 1 dotykem na obrazovce (leva pulka = smer, prava = FIRE)"); }
            break;
          case nap::dev::Ev::PAD: {
            g_joyPad = e.v & 31;
            aplikujJoyLocked();
            const int pc = (e.v >> 5) & 7;
            if (pc != g_padConsol) {
              // kratky stisk START/SELECT/OPTION musi stroj videt aspon 5 snimku
              for (int b = 0; b < 3; b++)
                if ((pc & (1 << b)) && !(g_padConsol & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
              g_padConsol = pc;
              if (g_stroj && !g_xex.aktivni()) g_stroj->consol = consolEfektivni();
            }
            // tlacitka X / Y = klavesy MEZERA / RETURN (drzene jako prstem)
            const int kl = (e.v >> 8) & 3;
            if (kl != g_padKlav && g_stroj && g_strojBezi) {
              const int nove = kl & ~g_padKlav;
              if (nove & 1) g_stroj->klavesa(33, true);
              else if (nove & 2) g_stroj->klavesa(12, true);
              else if (!kl) g_stroj->klavesaPustena();
            }
            g_padKlav = kl;
            if (!g_joyHlasenoPad) { g_joyHlasenoPad = true; devLog("B296 JOYSTICK 1 z herniho ovladace / klavesnice telefonu (smer, FIRE, START, SELECT, OPTION)"); }
            break;
          }
          default: break;
        }
      }
      vyzvednoutVystupyLocked();
    }
    if (!g_strojBezi.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(15));
      dalsi = nap_ted_ns();
      continue;
    }
    // 2) tempo: PAL 50 snimku/s; kdyz je ve zvukove fronte vic/min nez
    //    ~60 ms, snimek se protahne/zkrati (max o 8 %) - zvuk tak nikdy
    //    neuteka ani nenarusta zpozdeni, a kdyz zvuk nebezi, plati hodiny.
    long long perioda = 20000000LL;
    if (s_atariSlReady) {
      long long fillMs = (long long)(g_atariRing.avail() / 2) * 1000LL / 44100LL;
      long long korekce = (fillMs - 60) * 40000LL;
      if (korekce > 1600000LL) korekce = 1600000LL;
      if (korekce < -1600000LL) korekce = -1600000LL;
      perioda += korekce;
    }
    dalsi += perioda;
    long long ted = nap_ted_ns();
    if (dalsi > ted) std::this_thread::sleep_for(std::chrono::nanoseconds(dalsi - ted));
    else if (ted - dalsi > 200000000LL) dalsi = ted;   // zpozdeni (pozadi): nedohanet naraz
    // 3) jeden snimek stroje
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      if (!g_stroj) studenyStartLocked(7);
      if (!g_xex.aktivni()) g_stroj->consol = consolEfektivni();   // pri zavadeni XEX drzi OPTION zavadec
      if (g_kazBoot.aktivni()) g_stroj->consol = nap::KazetaBoot::konzole(g_stroj->consol);   // B298: boot z kazety
      g_typeQ.step(*g_stroj);
      // B299: pri psani programu z TXT jde dalsi klavesa hned, jak ji OS prevezme
      if (!g_stroj->cpu.jam) {
        if (g_psani.pise()) g_stroj->runFrameRadky([](Machine &m) { g_psani.radek(m); });
        else g_stroj->runFrame();
      }
      if (g_stroj->cpu.jam) std::fill(zvuk.begin(), zvuk.end(), 0.f);
      else g_stroj->genAudio(zvuk.data(), 882, 44100.0);
      nap_atari_audio_push(zvuk.data(), 882);
      // B292: CSAVE nahrava linku SIO DATA OUT (signal pro magnetofon), ne
      // reproduktor - presne to, co by zapsal skutecny Atari 410/1010
      g_stroj->genTape(pasek.data(), 882);
      for (float &v : pasek) v *= 0.8f;
      g_rec.snimek(*g_stroj, pasek.data(), 882);
      g_xex.poSnimku(*g_stroj);
      if (g_kazBoot.poSnimku(*g_stroj)) { g_statusText = "NAHRÁVÁ SE Z KAZETY..."; g_statusMs = 15000; }
      g_psani.poSnimku(*g_stroj);
      if (!g_psani.status.empty()) { g_statusText = g_psani.status; g_statusMs = g_psani.statusMs; g_psani.status.clear(); }
      // B296: VBXE - do logu, kdyz ho program najde / zapne XDL / zmeni rezim
      {
        static int hlaseni = 0;
        const nap::Vbxe &v = g_stroj->vbx;
        if (v.nWrites > 0 && g_vbxeZapisuPred == 0)
          devLog("B296 VBXE: program nasel kartu VBXE (registry $D640, FX 1.26) a pouziva ji");
        g_vbxeZapisuPred = v.nWrites;
        const bool xdl = v.xdlEnabled;
        const int rez = (v.lastOvMode >= 0 && v.lastOvMode <= 4) ? v.lastOvMode : 0;
        if ((xdl != g_vbxeXdlPred || (xdl && rez != g_vbxeRezimPred)) && hlaseni < 30) {
          static const char *kRez[5] = {"zadny", "LR 160 bodu", "SR 320 bodu", "HR 640 bodu/16 barev", "TEXT 80 sloupcu"};
          char b[300];
          snprintf(b, sizeof(b), "B296 VBXE: XDL %s, overlay %s, blitru %lld (seznamu %lld), zapisu palety %lld, MEMAC A $%02X/$%02X B $%02X, snimek %lld",
                   xdl ? "ZAPNUTO" : "VYPNUTO", kRez[rez], v.nBlits, v.nBlitLists, v.nPalWrites, v.memacCtl, v.memacBankA, v.memacBankB,
                   g_snimkuCelkem.load());
          devLog(b);
          hlaseni++;
        }
        g_vbxeXdlPred = xdl; g_vbxeRezimPred = rez;
      }
      g_motorOn = nap::CsaveRecorder::motor(*g_stroj);
      // B295: co se deje s kazetou - do logu a pod pristroj
      {
        static bool motorPred = false;
        static long long bajtuPred = 0, ramcePred = 0;
        const bool mot = g_motorOn.load();
        const nap::TapeDeck &t = g_stroj->tape;
        if (mot != motorPred) {
          motorPred = mot;
          char b[320];
          if (mot) {
            bajtuPred = t.bytesRx; ramcePred = t.framingRx;
            if (t.loaded)
              snprintf(b, sizeof(b), "B295 KAZETA motor ZAPNUT - kazeta %s, PLAY=%s, pozice %.1f / %.1f s",
                       t.name.c_str(), t.play ? "ano (pasek bezi)" : "NE - pasek STOJI, zmackni PLAY", t.pos / t.img.rate, t.img.seconds());
            else
              snprintf(b, sizeof(b), "B295 KAZETA motor ZAPNUT - v magnetofonu neni kazeta (CSAVE nahrava do WAV, CLOAD nema co cist - EJECT a vyber kazetu)");
            devLog(b);
            g_statusText = t.loaded ? (t.play ? "MOTOR BEZI - CTU PASKU" : "MOTOR BEZI - ZMACKNI PLAY!") : "MOTOR BEZI";
            g_statusMs = t.loaded && !t.play ? 30000 : 6000;
          } else {
            if (t.loaded) {
              // B298: pocita se jen to, co OS opravdu cetl (sum v mezerach mezi zaznamy OS necte)
              snprintf(b, sizeof(b), "B295 KAZETA motor VYPNUT - OS z pasky precetl %lld bajtu (chyb ramce pri cteni %lld), pozice %.1f / %.1f s",
                       t.bytesRx - bajtuPred, t.framingRx - ramcePred, t.pos / t.img.rate, t.img.seconds());
              devLog(b);
            }
          }
        }
      }
      {
        std::lock_guard<std::mutex> f(g_mFrame);
        if (g_sdilenySnimek.size() != (size_t)AnticView::FW * AnticView::H)
          g_sdilenySnimek.assign((size_t)AnticView::FW * AnticView::H, 0xFF000000u);
        std::memcpy(g_sdilenySnimek.data(), g_view->fb, g_sdilenySnimek.size() * 4);
      }
      g_snimekSeq++;
      g_snimkuCelkem++;
      vyzvednoutVystupyLocked();
      statusText.swap(g_statusText); statusMs = g_statusMs; g_statusText.clear();
    }
    if (!statusText.empty()) {               // mimo g_mStroj (poradi zamku)
      std::lock_guard<std::mutex> dl(g_mDev);
      devGet().setStatusMessage(statusText.c_str(), nap_ted_ms(), statusMs);
      statusText.clear();
    }
    pokeRender();
  }
  devLog("B291 EMULACE_VLAKNO stop");
}

static void kresliVlaknoMain() {
  unsigned long long posledniSeq = ~0ULL;
  devLog("B291 KRESLICI_VLAKNO start (pristroj kresli C++ primo na displej)");
  nastavPriorituVlakna(-4, "KRESLICI_VLAKNO");
  while (g_run.load()) {
    {
      std::unique_lock<std::mutex> lk(g_mWake);
      g_cvWake.wait_for(lk, std::chrono::milliseconds(16), [] { return g_wake; });
      g_wake = false;
    }
    if (!g_run.load()) break;
    std::lock_guard<std::timed_mutex> wl(g_mWin);
    if (g_uvolnitOkno) {
      if (g_win) ANativeWindow_release(g_win);
      g_win = nullptr; g_uvolnitOkno = false;
      devLog("B291 PRISTROJ plocha uvolnena kreslicim vlaknem (zanikla behem kresleni)");
    }
    if (!g_win || g_bufW.load() <= 0 || g_bufH.load() <= 0) continue;
    nap::dev::Device &d = devGet();
    nap::dev::IRect pr{0, 0, 0, 0};
    bool full = false;
    const long long ted = nap_ted_ms();
    {
      std::lock_guard<std::mutex> dl(g_mDev);
      if (g_needGeometry) {
        ANativeWindow_setBuffersGeometry(g_win, g_bufW.load(), g_bufH.load(), WINDOW_FORMAT_RGBA_8888);
        g_needGeometry = false; full = true;
        // prvni rozlozeni pristroje trva na telefonu chvili - mezitim plocha
        // hned tmava (barva stranky navrhu #0b0b0d), aby neprobliklo nic jineho
        if (d.W != g_bufW.load() || d.H != g_bufH.load()) {
          ANativeWindow_Buffer b0;
          if (ANativeWindow_lock(g_win, &b0, nullptr) == 0) {
            if (b0.bits && (b0.format == WINDOW_FORMAT_RGBA_8888 || b0.format == WINDOW_FORMAT_RGBX_8888)) {
              for (int y = 0; y < b0.height; y++) {
                uint32_t *row = (uint32_t *)b0.bits + (size_t)y * b0.stride;
                for (int x = 0; x < b0.width; x++) row[x] = 0xFF0D0B0Bu;
              }
            }
            ANativeWindow_unlockAndPost(g_win);
          }
        }
      }
      if (g_needLayout || d.W != g_bufW.load() || d.H != g_bufH.load()) {
        long long t0 = nap_ted_ms();
        d.layout(g_bufW.load(), g_bufH.load());
        char b[160];
        snprintf(b, sizeof(b), "B291 PRISTROJ rozlozen %dx%d (displej %dx%d) za %lld ms", d.W, d.H, g_viewW.load(), g_viewH.load(), nap_ted_ms() - t0);
        devLog(b);
        g_needLayout = false; full = true;
      }
      unsigned long long seq = g_snimekSeq.load();
      if (seq != posledniSeq) {
        posledniSeq = seq;
        if (g_strojBezi.load()) {
          std::lock_guard<std::mutex> f(g_mFrame);
          if (g_sdilenySnimek.size() == (size_t)AnticView::FW * AnticView::H) d.setAtariFrame(g_sdilenySnimek.data());
        }
      }
      d.motorOn = g_motorOn.load();
      d.update(ted);
      d.render(ted);
      pr = d.takePresentRect();
      if (g_forceFull) { full = true; g_forceFull = false; }
      if (full) pr = nap::dev::IRect{0, 0, d.W, d.H};
    }
    if (pr.x1 <= pr.x0 || pr.y1 <= pr.y0) continue;
    ARect ar;
    ar.left = pr.x0; ar.top = pr.y0; ar.right = pr.x1; ar.bottom = pr.y1;
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(g_win, &buf, &ar) != 0) { g_forceFull = true; continue; }
    // bezpecnost: zapisovat jen do 32bit bufferu presne ocekavane velikosti
    bool ok = buf.bits && buf.width == d.W && buf.height == d.H && buf.stride >= buf.width &&
              (buf.format == WINDOW_FORMAT_RGBA_8888 || buf.format == WINDOW_FORMAT_RGBX_8888);
    if (ok) {
      d.copyOut((uint32_t *)buf.bits, buf.stride, nap::dev::IRect{ar.left, ar.top, ar.right, ar.bottom});
    } else {
      if (!g_formatHlaseno) {
        char b[160];
        snprintf(b, sizeof(b), "B291 PRISTROJ buffer displeje nesedi (%dx%d fmt=%d, cekano %dx%d RGBA) - nastavuji znovu",
                 buf.width, buf.height, buf.format, d.W, d.H);
        devLog(b); g_formatHlaseno = true;
      }
      g_needGeometry = true;
    }
    ANativeWindow_unlockAndPost(g_win);
  }
  devLog("B291 KRESLICI_VLAKNO stop");
}

/** Start pristroje (vstup do HELP / navrat z pozadi). studeny=1: jako
 *  zapnuti vypinace - novy stroj a vychozi stav pristroje. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(JNIEnv *, jclass, jboolean studeny) {
  std::lock_guard<std::mutex> r(g_mRun);
  bool zapnuto;
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    if (studeny) devGet().resetUi();
    zapnuto = devGet().power;
  }
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (studeny) {
      g_joyDotyk = g_joyPad = g_padConsol = g_padKlav = 0;   // B296: joystick pusteny
      // B298: START/OPTION drzene pri POWER plati (boot z kazety) - novy vstup do
      // HELP ale zacina s pustenou konzoli
      g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
    }
    if (studeny || !g_stroj) {
      g_xex.zrus();
      studenyStartLocked(7);
      devLog(studeny ? "B291 HELP otevren - pristroj ZAPNUT, studeny start Atari"
                     : "B291 HELP - stroj neexistoval, studeny start");
      // B293: co presne se emuluje - at je to v logu cerne na bilem
      char b[400];
      const unsigned osSum = (unsigned)NAP_OS_ROM[0] | ((unsigned)NAP_OS_ROM[1] << 8);
      snprintf(b, sizeof(b),
               "B293 JADRO = ATARI 130XE PAL: RAM 64 kB + rozsirena 4x16 kB (PORTB bity 2-3, procesor bit 4, ANTIC bit 5), "
               "OS XL/XE rev.%u (soucet $%04X), BASIC rev.C vestaveny, self-test ROM $5000, TRIG3=0 (bez cartridge), "
               "RESET = reset CPU+ANTIC+PIA/MMU, plovouci datova sbernice, ANTIC/GTIA PAL 312 radku, POKEY 1,773 MHz",
               (unsigned)NAP_OS_ROM[0x3FF7], osSum);
      devLog(b);
      devLog("B296 VBXE FX 1.26 v jadre: registry $D640, 512 kB VRAM, MEMAC A/B, XDL (overlay LR/SR/HR/text 80 sloupcu), "
             "atributova mapa, palety 4x256, blitter (rezimy 0-6, zoom, vzor, kolize) + IRQ; obraz 4 body na barevny takt. "
             "Joystick 1: leva pulka obrazovky = smer, prava = FIRE; herni ovladac taky.");
      devLog("B298 ZVUK s pasmem jako TV (FIR 16 kHz, bez prekladu ultrazvuku do slysitelna), asynchronni prijem POKEY "
             "restartuje casovace 3+4 start bitem; KAZETA: hra (boot) = START+OPTION a RETURN samo, stereo WAV = zvukova "
             "stopa do TV, CLOAD i s disketou v D1:; DISKETA: ATR/DISK = vysunout / vymenit bez restartu.");
      devLog("B299 BASIC/TBXL TXT: TXT soubor s programem napise appka sama do ATARI BASICu / Turbo-BASICu XL; "
             "klavesy z pristroje do logu (B299 KLAVESY); LOG/CHYBA = stav a cela pamet Atari v logu; "
             "SIO zkratka jen se zapnutou ROM OS (Turbo-BASIC XL ma na $E459 vlastni kod).");
    }
    g_strojBezi = zapnuto;
  }
  nap_atari_sl_open();
  if (g_run.load()) { pokeRender(); return; }
  g_run = true;
  g_emuVlakno = std::thread(emuVlaknoMain);
  g_kresliVlakno = std::thread(kresliVlaknoMain);
}

/** Stop pristroje (odchod z HELP, appka na pozadi). Ceka na obe vlakna. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(JNIEnv *, jclass) {
  std::lock_guard<std::mutex> r(g_mRun);
  if (!g_run.load()) return;
  g_run = false;
  pokeRender();
  if (g_emuVlakno.joinable()) g_emuVlakno.join();
  if (g_kresliVlakno.joinable()) g_kresliVlakno.join();
}

/** Plocha displeje (SurfaceView). surface=null -> plocha zanika: po navratu
 *  z teto funkce uz se do ni nekresli (drzime g_mWin jako kreslici vlakno). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(JNIEnv *env, jclass, jobject surface, jint w, jint h) {
  if (!surface) {
    // Plocha zanika: pockat, az kreslici vlakno dokresli snimek (max 1,5 s).
    // Kdyby se zaseklo (napr. v ANativeWindow_lock), UI vlakno NEBLOKOVAT -
    // okno uvolni kreslici vlakno samo hned, jak se vrati (drzime vlastni
    // referenci, takze objekt okna do te doby plati).
    std::unique_lock<std::timed_mutex> wl(g_mWin, std::defer_lock);
    if (!wl.try_lock_for(std::chrono::milliseconds(1500))) {
      g_uvolnitOkno = true;
      devLog("B291 PRISTROJ plocha zanikla behem kresleni - uvolni ji kreslici vlakno");
      return;
    }
    if (g_win) { ANativeWindow_release(g_win); g_win = nullptr; }
    g_uvolnitOkno = false;
    devLog("B291 PRISTROJ plocha zanikla");
    return;
  }
  std::lock_guard<std::timed_mutex> wl(g_mWin);
  if (g_uvolnitOkno) { if (g_win) ANativeWindow_release(g_win); g_win = nullptr; g_uvolnitOkno = false; }
  ANativeWindow *nw = ANativeWindow_fromSurface(env, surface);
  if (!nw) { devLog("B291 PRISTROJ CHYBA: ANativeWindow_fromSurface vratilo null"); return; }
  bool nova = (nw != g_win);
  if (!nova) ANativeWindow_release(nw);                 // stejna plocha: jen zmena velikosti
  else { if (g_win) ANativeWindow_release(g_win); g_win = nw; }
  int vw = w > 0 ? w : ANativeWindow_getWidth(g_win);
  int vh = h > 0 ? h : ANativeWindow_getHeight(g_win);
  if (vw <= 0 || vh <= 0) return;
  // vnitrni rozliseni: kratsi strana max 1080 px (vetsi displej dopocita
  // kompozitor). B297: na sirku tedy vyska max 1080 (driv sirka 1080 ->
  // na sirku jen ~486 radku a rozmazany obraz).
  int bw = vw, bh = vh;
  const int kratsi = std::min(bw, bh);
  if (kratsi > 1080) { bw = (int)((long long)bw * 1080 / kratsi); bh = (int)((long long)bh * 1080 / kratsi); }
  g_viewW = vw; g_viewH = vh;
  if (bw != g_bufW.load() || bh != g_bufH.load()) {
    // B297: otoceni displeje - pustit vse, co drzi prsty (D-pad, FIRE, klavesy)
    nap::dev::Ev ev[16]; int n = 0;
    {
      std::lock_guard<std::mutex> dl(g_mDev);
      nap::dev::Device &d = devGet();
      if (d.W > 1) n = d.pointerCancelAll(nap_ted_ms(), ev, 16);
    }
    if (n > 0) { std::lock_guard<std::mutex> l(g_mEv); for (int i = 0; i < n; i++) g_evQ.push_back(ev[i]); }
    g_bufW = bw; g_bufH = bh; g_needLayout = true;
  }
  if (nova) g_formatHlaseno = false;
  g_needGeometry = true;
  g_forceFull = true;
  char b[160];
  snprintf(b, sizeof(b), "B291 PRISTROJ plocha %s %dx%d -> kresleni %dx%d", nova ? "nova" : "zmena", vw, vh, bw, bh);
  devLog(b);
  pokeRender();
}

/** Dotek prstu. akce: 0=dolu, 1=nahoru, 2=zruseno (vsechny prsty).
 *  x,y v pixelech plochy. Vraci servisni akci pro Javu (0 = zadna). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(JNIEnv *, jclass, jint pid, jint akce, jfloat x, jfloat y) {
  const int vw = g_viewW.load(), bw = g_bufW.load();
  float k = (vw > 0 && bw > 0) ? (float)bw / (float)vw : 1.f;
  float bx = x * k, by = y * k;
  nap::dev::Ev ev[8];
  int n = 0, svc = 0;
  const long long ted = nap_ted_ms();
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    if (d.W <= 1) return 0;
    if (akce == 0) n = d.pointerDown(pid & 15, bx, by, ted, ev, 8);
    else if (akce == 1) n = d.pointerUp(pid & 15, bx, by, ted, ev, 8, &svc);
    else if (akce == 3) n = d.pointerMove(pid & 15, bx, by, ted, ev, 8);   // B296: posun prstu (joystick)
    else n = d.pointerCancelAll(ted, ev, 8);
  }
  if (n > 0) {
    std::lock_guard<std::mutex> l(g_mEv);
    for (int i = 0; i < n; i++) g_evQ.push_back(ev[i]);
  }
  bool vibrace = false;
  { std::lock_guard<std::mutex> dl(g_mDev); nap::dev::Device &d = devGet(); vibrace = d.hapticReq; d.hapticReq = false; }
  if (akce != 3 || n > 0) pokeRender();
  return svc | (vibrace ? 0x1000 : 0);              // B297: bit 0x1000 = Java kratce zavibruje
}

/** B297: ovladani na sirku (D-pad a tlacitka jako u Segy).
 *  "get" -> nastaveni, "set:<nastaveni>" -> pouzit, "edit" / "mirror" / "reset" /
 *  "done" -> prikazy z menu D-PAD A OVLADANI, "stav" -> "land=0/1;edit=0/1".
 *  Vraci aktualni nastaveni (Java ho uklada do SharedPreferences). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(JNIEnv *env, jclass, jstring jcmd) {
  std::string cmd;
  if (jcmd) { const char *c = env->GetStringUTFChars(jcmd, nullptr); if (c) { cmd = c; env->ReleaseStringUTFChars(jcmd, c); } }
  // B298: disketova mechanika D1: - "d1" = co je v mechanice ("" = prazdna),
  // "disk_eject" = vysunout a zapnout znovu (BASIC), "atr_vymena" = dalsi ATR
  // jen vymenit bez restartu (druha strana hry), "atr_normal" = zrusit
  if (cmd == "d1" || cmd == "disk_eject" || cmd == "atr_vymena" || cmd == "atr_normal") {
    std::string out, jm;
    bool vysunuto = false;
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      if (g_stroj && g_stroj->disk.mounted) jm = g_stroj->disk.name;
      if (cmd == "atr_vymena") g_atrVymena = true;
      else if (cmd == "atr_normal") g_atrVymena = false;
      else if (cmd == "disk_eject" && g_stroj) {
        vysunuto = g_stroj->disk.mounted;
        g_stroj->disk = nap::AtrDisk();
        g_xex.zrus();
        studenyStartLocked(7);                 // jako vysunout disketu a vypnout/zapnout: BASIC
        g_stroj->sioRychlyTimeout = false;
        g_strojBezi = true;
        g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
        vyzvednoutVystupyLocked();
      }
      out = (cmd == "disk_eject") ? std::string() : jm;
    }
    if (cmd == "disk_eject") {
      devLog(vysunuto ? "B298 DISKETA " + jm + " vysunuta z D1: - studeny start (BASIC)" : std::string("B298 DISKETA: D1: byla prazdna - studeny start (BASIC)"));
      std::lock_guard<std::mutex> dl(g_mDev);
      nap::dev::Device &dv = devGet();
      if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
      dv.atariValid = false; dv.markScreen(); dv.markLegend();
      dv.setStatusMessage("DISKETA VYSUNUTA - D1: PRÁZDNÁ", nap_ted_ms(), 4000);
    } else if (cmd == "atr_vymena") devLog("B298 DISKETA: dalsi ATR se jen vymeni v D1: (bez restartu, napr. druha strana hry)");
    pokeRender();
    return env->NewStringUTF(out.c_str());
  }
  std::string out;
  nap::dev::Ev ev[16]; int n = 0;
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    if (cmd.rfind("set:", 0) == 0) d.ctlApplyConfig(cmd.c_str() + 4);
    else if (cmd == "stav") { out = std::string("land=") + (d.land ? "1" : "0") + ";edit=" + (d.editMode ? "1" : "0"); }
    else if (cmd == "atari_reset") { ev[0].t = nap::dev::Ev::RESET; ev[0].v = 0; n = 1; }   // RESET z menu na sirku
    else if (cmd != "get") n = d.ctlCommand(cmd.c_str(), ev, 16);
    if (out.empty()) out = d.ctlConfig();
  }
  if (n > 0) { std::lock_guard<std::mutex> l(g_mEv); for (int i = 0; i < n; i++) g_evQ.push_back(ev[i]); }
  if (cmd != "get" && cmd != "stav") {
    devLog("B297 OVLADANI " + (cmd.size() > 80 ? cmd.substr(0, 80) : cmd) + " -> " + out);
    pokeRender();
  }
  return env->NewStringUTF(out.c_str());
}

/** B296: herni ovladac / klavesnice telefonu -> joystick 1 a konzole.
 *  maska: bit0 nahoru, 1 dolu, 2 vlevo, 3 vpravo, 4 FIRE, 5 START, 6 SELECT, 7 OPTION,
 *  8 klavesa MEZERA, 9 klavesa RETURN. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(JNIEnv *, jclass, jint maska) {
  nap::dev::Ev e; e.t = nap::dev::Ev::PAD; e.v = maska & 0x3FF;
  { std::lock_guard<std::mutex> l(g_mEv); g_evQ.push_back(e); }
}

/** Napsat text do Atari (tlacitko BASIC/TBXL TXT). runPotom: na konec RUN. */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(JNIEnv *env, jclass, jstring text, jboolean runPotom) {
  if (!text) return 0;
  const char *c = env->GetStringUTFChars(text, nullptr);
  std::string t = c ? c : "";
  if (c) env->ReleaseStringUTFChars(text, c);
  if (runPotom) { if (!t.empty() && t.back() != '\n') t.push_back('\n'); t += "RUN"; }
  int n, preskoceno;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    int p0 = g_typeQ.preskoceno;
    n = g_typeQ.addText(t);
    preskoceno = g_typeQ.preskoceno - p0;
  }
  char b[160];
  snprintf(b, sizeof(b), "B291 TXT do Atari: %d stisku klaves pripraveno%s (%d znaku na Atari klavesnici neni)",
           n, runPotom ? " + RUN" : "", preskoceno);
  devLog(b);
  return n;
}

/** B299: program z TXT souboru (tlacitko BASIC/TBXL TXT -> TXT SOUBOR).
 *  rezim 0 = ATARI BASIC (studeny start s BASICem, disketa z D1: ven),
 *  rezim 1 = Turbo-BASIC XL (ten uz zavedla Java pres devLoadXexNative).
 *  Psani zacne, az Atari ukaze READY (nap::PsaniProgramu). Vraci text pro log. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devProgramNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno, jint rezim) {
  if (!data) return env->NewStringUTF("CHYBA zadna data");
  const jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  std::string nm = "program.txt";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  for (auto &ch : nm) if ((unsigned char)ch >= 0x80) ch = '?';
  std::string vysl;
  bool odpojena = false;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) zaloz();
    if (rezim == 0) {
      g_xex.zrus();
      if (g_stroj->disk.mounted) { odpojena = true; g_stroj->disk = nap::AtrDisk(); }
      studenyStartLocked(7);                       // ATARI BASIC
      g_stroj->sioRychlyTimeout = true;            // D1: je prazdna - OS nema na co cekat
      g_strojBezi = true;
      g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
    }
    g_psani.start(buf.data(), buf.size(), rezim == 1, nm, rezim == 0);
    vysl = g_psani.log.empty() ? std::string("OK") : g_psani.log.back();
    vyzvednoutVystupyLocked();
  }
  if (odpojena) devLog("B299 TXT PROGRAM: disketa vyjmuta z D1: (Atari se zapina s ATARI BASICem)");
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    if (!d.power) { d.power = true; d.powerAt = nap_ted_ms(); }
    d.atariValid = false; d.markScreen(); d.markLegend();
  }
  pokeRender();
  return env->NewStringUTF(vysl.c_str());
}

/** B299: diagnostika pameti pro LOG/CHYBA - stav procesoru a MMU, ukazatele
 *  BASICu a kontrolni soucty oblasti pameti (porovnam se stejnym postupem na PC). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devDiagNative(JNIEnv *env, jclass) {
  std::string out;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) return env->NewStringUTF("B299 PAMET: stroj neexistuje");
    const Machine &m = *g_stroj;
    const uint8_t *r = m.mem.ram;
    auto w = [&](int a) { return r[a] | (r[a + 1] << 8); };
    const int pb = m.mem.portB();
    char b[900];
    snprintf(b, sizeof(b), "B299 PAMET STAV: PC=$%04X A=$%02X X=$%02X Y=$%02X S=$%02X P=$%02X%s, PORTB=$%02X (ROM OS %s, BASIC %s, "
             "rozsirena pamet pro CPU %s banka %d), NMIEN=$%02X, DLIST=$%04X, SAVMSC=$%04X, DDEVIC=$%02X, D1: %s, rychly timeout SIO %s, "
             "XEX stav %d, psani TXT stav %d, snimek %lld",
             m.cpu.pc, m.cpu.a, m.cpu.x, m.cpu.y, m.cpu.s, m.cpu.p, m.cpu.jam ? " JAM" : "", pb, (pb & 1) ? "zap" : "VYP",
             (pb & 2) ? "vyp" : "ZAP", (pb & 0x10) ? "vyp" : "ZAP", (pb >> 2) & 3, m.nmien, w(0x230), w(0x58), r[0x300],
             m.disk.mounted ? m.disk.name.c_str() : "prazdna", m.sioRychlyTimeout ? "ANO" : "ne", (int)g_xex.stav, (int)g_psani.stav,
             (long long)m.frame);
    out += b; out += "\n";
    out += nap::napDiagPameti(m);
  }
  return env->NewStringUTF(out.c_str());
}

/** B299: cela pamet Atari (64 kB + rozsirenych 64 kB + registry) pro LOG/CHYBA -
 *  Java ji prilozi k logu. Hlavicka 16 B "NAP130XE-B299", 64 B registru. */
extern "C" JNIEXPORT jbyteArray JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devRamDumpNative(JNIEnv *env, jclass) {
  std::vector<uint8_t> d(16 + 64 + 65536 + 65536, 0);
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) return nullptr;
    const Machine &m = *g_stroj;
    std::memcpy(d.data(), "NAP130XE-B299", 13);
    uint8_t *g = d.data() + 16;
    g[0] = m.cpu.pc & 0xFF; g[1] = m.cpu.pc >> 8; g[2] = m.cpu.a; g[3] = m.cpu.x; g[4] = m.cpu.y; g[5] = m.cpu.s; g[6] = m.cpu.p;
    g[7] = (uint8_t)m.mem.portB(); g[8] = (uint8_t)m.mem.pia.ctlB; g[9] = (uint8_t)m.mem.pia.ddrB; g[10] = (uint8_t)m.mem.pia.orB;
    g[11] = m.cpu.jam ? 1 : 0;
    for (int i = 0; i < 8; i++) g[12 + i] = (uint8_t)((unsigned long long)m.frame >> (8 * i));
    g[20] = (uint8_t)m.consol; g[21] = m.disk.mounted ? 1 : 0; g[22] = m.sioRychlyTimeout ? 1 : 0; g[23] = (uint8_t)g_xex.stav;
    g[24] = m.vbx.memacCtl; g[25] = m.vbx.memacBankA; g[26] = m.vbx.memacBankB; g[27] = m.nmien;
    std::memcpy(d.data() + 80, m.mem.ram, 65536);
    std::memcpy(d.data() + 80 + 65536, m.mem.ext, 65536);
  }
  jbyteArray a = env->NewByteArray((jsize)d.size());
  if (a) env->SetByteArrayRegion(a, 0, (jsize)d.size(), (const jbyte *)d.data());
  return a;
}

/** Spustit XEX (XEX/MOBIL, TURBO/BASIC, NET HRY). */
extern "C" JNIEXPORT jboolean JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno) {
  if (!data) return JNI_FALSE;
  jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  std::string nm = "program.xex";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  bool ok;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    ok = g_xex.priprav(buf.data(), buf.size(), nm);
    if (ok) {
      studenyStartLocked(3);                 // OPTION drzene = BASIC vypnuty
      // B295: disketa z mechaniky ven - jinak by OS nabootoval ji misto XEX
      if (g_stroj->disk.mounted) { devLog("B295 DISKETA vyjmuta z D1: (spousti se XEX)"); g_stroj->disk = nap::AtrDisk(); }
      g_stroj->sioRychlyTimeout = true;
      g_strojBezi = true;
    }
    vyzvednoutVystupyLocked();
  }
  if (ok) {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    if (!d.power) { d.power = true; d.powerAt = nap_ted_ms(); }
    d.atariValid = false; d.markScreen(); d.markLegend();
    d.setStatusMessage(("SPOUSTIM " + nm).c_str(), nap_ted_ms(), 4000);
  }
  pokeRender();
  return ok ? JNI_TRUE : JNI_FALSE;
}

/** B292: vlozit kazetu (WAV) do magnetofonu - CLOAD. Vraci text pro log
 *  (zacina "OK " nebo "CHYBA "). Demodulace FSK probehne hned (nap_atari_tape.h),
 *  stroj pak pasku cte v realnem case pri zapnutem motoru a PLAY. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadTapeNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno) {
  std::string nm = "kazeta.wav";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  for (auto &ch : nm) if ((unsigned char)ch >= 0x80) ch = '?';
  if (!data) return env->NewStringUTF("CHYBA zadna data");
  jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  nap::TapeImage img; std::string err;
  const long long t0 = nap_ted_ms();
  const bool ok = nap::napTapeFromWav(buf.data(), buf.size(), img, err);
  const long long ms = nap_ted_ms() - t0;
  char b[400];
  if (!ok) {
    snprintf(b, sizeof(b), "CHYBA %s: %s", nm.c_str(), err.c_str());
    devLog(std::string("B292 KAZETA ") + b);
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage(("KAZETA NEJDE NACIST: " + err).c_str(), nap_ted_ms(), 7000);
    pokeRender();
    return env->NewStringUTF(b);
  }
  // B298: zaznamy s platnym kontrolnim souctem, chyby ramce jen UVNITR zaznamu
  // (ty by vadily; sum v mezerach mezi zaznamy OS necte), druh kazety
  snprintf(b, sizeof(b), "OK %s: %.1f s, %d Hz, %d kanal(y) (FSK v kanalu %d), %d bit; pri 600 Bd %lld bajtu v %lld zaznamech "
                         "(kontrolni soucet OK %lld, spatne %lld), chyb ramce v zaznamech %lld (sum v mezerach %lld); druh: %s%s; demodulace %lld ms",
           nm.c_str(), img.seconds(), img.srcRate, img.channels, img.usedChannel + 1, img.bitsPerSample,
           img.bytes600, img.records600, img.recordsOk600, img.recordsBad600, img.framingRec600, img.framing600 - img.framingRec600,
           nap::napTapeDruh(img), img.audio.empty() ? "" : ", zvukova stopa (druhy kanal) hraje do TV jako u skutecneho magnetofonu", ms);
  const int druh = img.druh;
  bool odpojenaDisketa = false;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) zaloz();
    nap::TapeDeck &t = g_stroj->tape;
    t.eject();
    // B295: Rene "cload nechapu" - vlozena kazeta ma rovnou zmacknute PLAY
    // (jako bys kazetu vlozil a zmackl PLAY). Pasek se stejne hne az kdyz
    // OS zapne motor (CLOAD + RETURN + po pipnuti RETURN).
    t.img = std::move(img); t.loaded = true; t.name = nm; t.play = true; t.pos = 0;
    if (druh == nap::TapeImage::BOOT) {
      // B298: Rene "pro wav jsou hry, kde je potreba podrzet start+option" -
      // presne to se ted stane samo: studeny start s drzenym START+OPTION,
      // OS pipne, RETURN, hra se nahraje a spusti.
      g_xex.zrus();
      if (g_stroj->disk.mounted) { odpojenaDisketa = true; g_stroj->disk = nap::AtrDisk(); }
      studenyStartLocked(nap::KazetaBoot::konzole(7));
      g_stroj->sioRychlyTimeout = false;
      g_kazBoot.start(*g_stroj);
      g_strojBezi = true;
      g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
      vyzvednoutVystupyLocked();
    }
  }
  devLog(std::string("B292 KAZETA VLOZENA ") + b);
  const char *stav;
  if (druh == nap::TapeImage::BOOT) {
    if (odpojenaDisketa) devLog("B298 DISKETA vyjmuta z D1: (bootuje kazeta)");
    devLog("B298 KAZETA S BOOTEM - studeny start s drzenym START+OPTION (jako pri zapnuti na skutecnem 130XE), po pipnuti RETURN sam");
    stav = "HRA Z KAZETY - NAHRAJE SE SAMA";
  } else if (druh == nap::TapeImage::TEXT) {
    devLog("B298 KAZETA s vypisem BASICu (text) - napis ENTER \"C:\", RETURN, po pipnuti RETURN");
    stav = "NAPIŠ ENTER \"C:\" A 2x RETURN";
  } else {
    devLog("B295 KAZETA PLAY zmacknuto automaticky - ted napis CLOAD, RETURN, a po pipnuti jeste jednou RETURN");
    stav = druh == nap::TapeImage::BASIC ? "NAPIŠ CLOAD + RETURN, PO PÍPNUTÍ RETURN"
                                         : "NAPIŠ CLOAD, HRA: START+OPTION+POWER";
  }
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    d.setDoor(false, nap_ted_ms());
    d.play = true; d.rec = false; d.markTape(); d.markWindow(); d.markLegend();
    d.counter = 0; d.counterAcc = 0; d.markCounter();
    if (druh == nap::TapeImage::BOOT) {
      if (!d.power) { d.power = true; d.powerAt = nap_ted_ms(); }
      d.atariValid = false; d.markScreen();
    }
    d.setStatusMessage(stav, nap_ted_ms(), druh == nap::TapeImage::BOOT ? 20000 : 120000);
  }
  pokeRender();
  return env->NewStringUTF(b);
}

/** B295: disketa (ATR) do mechaniky D1: a studeny start (OPTION drzene =
 *  BASIC vypnuty, jako u her). Mechanika se obsluhuje na urovni prikazu SIO
 *  (cteni/zapis/stav/format sektoru) - Machine::sioPatch. Vraci "OK ..."/"CHYBA ...". */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno) {
  std::string nm = "disketa.atr";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  for (auto &ch : nm) if ((unsigned char)ch >= 0x80) ch = '?';
  if (!data) return env->NewStringUTF("CHYBA zadna data");
  jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  nap::AtrDisk d;
  char b[300];
  if (!d.load(buf.data(), buf.size(), nm)) {
    snprintf(b, sizeof(b), "CHYBA %s neni platny ATR (chybi hlavicka $96 $02 nebo je prazdny)", nm.c_str());
    devLog(std::string("B295 DISKETA ") + b);
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage(("NENI ATR: " + nm).c_str(), nap_ted_ms(), 6000);
    pokeRender();
    return env->NewStringUTF(b);
  }
  bool vymena = false;
  {
    // B298: vymena diskety bez restartu (hra chce "vloz disketu 2 / otoc disketu")
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_atrVymena && g_stroj && g_strojBezi) {
      vymena = true;
      g_atrVymena = false;
      g_stroj->disk = std::move(d);
    }
    g_atrVymena = false;
  }
  if (vymena) {
    snprintf(b, sizeof(b), "OK %s -> mechanika D1: VYMENENA bez restartu (program bezi dal)", nm.c_str());
    devLog(std::string("B298 DISKETA ") + b);
    {
      std::lock_guard<std::mutex> dl(g_mDev);
      devGet().setStatusMessage(("V D1: JE " + (nm.size() > 26 ? nm.substr(0, 24) + ".." : nm)).c_str(), nap_ted_ms(), 5000);
    }
    pokeRender();
    return env->NewStringUTF(b);
  }
  snprintf(b, sizeof(b), "OK %s: %d sektoru po %d B (%u kB) -> mechanika D1:, studeny start s OPTION (BASIC vypnuty)",
           nm.c_str(), d.sectors, d.sectorSize, (unsigned)(d.data.size() / 1024));
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    g_xex.zrus();
    studenyStartLocked(3);
    g_stroj->disk = std::move(d);
    g_stroj->sioRychlyTimeout = false;
    g_strojBezi = true;
    // OPTION drzet pri startu (~3 s), pak pustit - hry ho pak ctou samy
    g_consolChtene = 7;
    g_consolDrzetDo[2] = g_snimkuCelkem.load() + 150;
    g_consolDrzetDo[0] = g_consolDrzetDo[1] = 0;
    vyzvednoutVystupyLocked();
  }
  devLog(std::string("B295 DISKETA VLOZENA ") + b);
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &dv = devGet();
    if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
    dv.atariValid = false; dv.markScreen(); dv.markLegend();
    dv.setStatusMessage(("DISKETA D1: " + nm).c_str(), nap_ted_ms(), 5000);
  }
  pokeRender();
  return env->NewStringUTF(b);
}

/** B292: vyjmout kazetu (EJECT bez vyberu). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devEjectTapeNative(JNIEnv *, jclass) {
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) { const bool play = g_stroj->tape.play; g_stroj->tape.eject(); g_stroj->tape.play = play; }
  }
  devLog("B292 KAZETA vyjmuta");
}

/** Kratka zprava ve stavovem radku pod pristrojem (napr. "CSAVE ULOZENO"). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStatusNative(JNIEnv *env, jclass, jstring msg, jint ms) {
  std::string m;
  if (msg) { const char *c = env->GetStringUTFChars(msg, nullptr); if (c) { m = c; env->ReleaseStringUTFChars(msg, c); } }
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage(m.c_str(), nap_ted_ms(), ms > 0 ? ms : 4000);
  }
  pokeRender();
}

/** Radky do logu appky (Java si je vyzvedava a pripisuje). null = nic. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(JNIEnv *env, jclass) {
  std::vector<std::string> v;
  { std::lock_guard<std::mutex> o(g_mOut); v.swap(g_logOut); }
  if (v.empty()) return nullptr;
  std::string s;
  for (auto &l : v) { s += l; s.push_back('\n'); }
  // NewStringUTF chce "modified UTF-8": nase radky jsou ciste ASCII
  for (auto &ch : s) if ((unsigned char)ch >= 0x80) ch = '?';
  return env->NewStringUTF(s.c_str());
}

/** Hotovy CSAVE zaznam (16-bit PCM mono 44100 Hz, little endian) nebo null. */
extern "C" JNIEXPORT jbyteArray JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTakeWavNative(JNIEnv *env, jclass) {
  std::vector<int16_t> pcm;
  {
    std::lock_guard<std::mutex> o(g_mOut);
    if (!g_wavReady) return nullptr;
    pcm.swap(g_wavOut);
    g_wavReady = false;
  }
  std::vector<uint8_t> b(pcm.size() * 2);
  for (size_t i = 0; i < pcm.size(); i++) {
    b[i * 2] = (uint8_t)(pcm[i] & 0xFF);
    b[i * 2 + 1] = (uint8_t)((pcm[i] >> 8) & 0xFF);
  }
  jbyteArray arr = env->NewByteArray((jsize)b.size());
  if (!arr) return nullptr;
  env->SetByteArrayRegion(arr, 0, (jsize)b.size(), (const jbyte *)b.data());
  return arr;
}

/** Kratky stav pro log (POWER, snimky, motor, frontu klaves). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devInfoNative(JNIEnv *env, jclass) {
  char b[256];
  int pc = -1; bool jam = false; size_t fronta;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) { pc = g_stroj->cpu.pc; jam = g_stroj->cpu.jam; }
    fronta = g_typeQ.q.size();
  }
  snprintf(b, sizeof(b), "B291 PRISTROJ stav: bezi=%s power=%s snimku=%lld pc=$%04X%s motor=%s klaves_ve_fronte=%u zvuk_fronta=%ums",
           g_run.load() ? "ano" : "ne", g_strojBezi.load() ? "ZAP" : "VYP", g_snimkuCelkem.load(), pc & 0xFFFF,
           jam ? " JAM" : "", g_motorOn.load() ? "ZAP" : "VYP", (unsigned)fronta,
           (unsigned)((g_atariRing.avail() / 2) * 1000 / 44100));
  return env->NewStringUTF(b);
}
