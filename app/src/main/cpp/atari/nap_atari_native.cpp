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
#include <set>
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
#include "nap_atari_pc_klavesnice.h"   // B302: klavesnice pocitace (prohlizec na TV/PC) -> Atari
#include "nap_atari_tv_proud.h"         // B303: obraz a zvuk na TV/PC primo z C++ (male zpozdeni)
#include "nap_atari_archiv.h"            // B349: hry v ZIP / RAR (rozbaleni a vyber disket v C++)
#include "../nap_ovladac.h"              // B320: herni ovladac (Retroid Pocket 6, gamepad)
#include "nap_atari_online.h"             // B363: online Atari - stav stroje, stisky hracu, kruh pro opravu odhadu
#include "../online/nap_online_klient.h"  // B363: online hra pres engine N&P (stejne jako Sega)
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <sys/resource.h>
#include <sstream>                // B363: stav online pro Javu

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

// B361: druhy POKEY (stereo) - nastaveni z Javy (OPTIONS / kolecko: POKEY STEREO), plati pro kazdy novy stroj
static std::atomic<bool> g_pokeyStereo{true};
// B363: nova hra / POWER / odchod uprostred online hry = konec online (definice u online hry nize, pred vlaknem emulace)
static void onlineKonecProNovouHru(const char *proc);

static void zaloz() {
  if (!g_stroj) { g_stroj = new Machine(); g_stroj->stereoPokey = g_pokeyStereo.load(); }
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
// B301: odbocka zvuku pro TV / PC (web prohlizec v appce) - presne ta data,
// ktera jdou do OpenSL. Kruhova fronta ~0,19 s; kdyz ji nikdo necte (TV
// nebezi), nejstarsi data se prepisuji. Cte devPullTvAudioNative.
static std::mutex g_mTvZvuk;
static std::vector<int16_t> g_tvZvuk;
static unsigned long long g_tvZvukZapis = 0, g_tvZvukCteni = 0;   // pocty shortu (stereo)
static const size_t TV_ZVUK_KAP = 65536;   // B303: ~0,74 s (prohlizec s pomalejsi Wi-Fi nic neztrati)
static void tvZvukZapis(const int16_t *s, size_t n) {
  std::lock_guard<std::mutex> l(g_mTvZvuk);
  if (g_tvZvuk.size() != TV_ZVUK_KAP) g_tvZvuk.assign(TV_ZVUK_KAP, 0);
  for (size_t i = 0; i < n; i++) g_tvZvuk[(size_t)((g_tvZvukZapis + i) % TV_ZVUK_KAP)] = s[i];
  g_tvZvukZapis += n;
  if (g_tvZvukZapis - g_tvZvukCteni > TV_ZVUK_KAP) g_tvZvukCteni = g_tvZvukZapis - TV_ZVUK_KAP;
}

static void nap_atari_audio_push(const float *mono, int n) {
  if (!mono || n <= 0) return;
  std::vector<int16_t> stereo((size_t)n * 2);
  nap::atariGainClampToStereoInt16(mono, n, 4.0f, stereo.data());
  g_atariRing.write(stereo.data(), (unsigned)stereo.size());
  tvZvukZapis(stereo.data(), stereo.size());
}
// B361: POKEY stereo - levy a pravy kanal zvlast (pravy = druhy POKEY; dokud ho program nepouziva, je jako levy)
static void nap_atari_audio_push_stereo(const float *l, const float *r, int n) {
  if (!l || !r || n <= 0) return;
  std::vector<int16_t> stereo((size_t)n * 2);
  nap::atariGainClampStereoToInt16(l, r, n, 4.0f, stereo.data());
  g_atariRing.write(stereo.data(), (unsigned)stereo.size());
  tvZvukZapis(stereo.data(), stereo.size());
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
  onlineKonecProNovouHru("novy stroj (boot)");   // B363
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
static bool g_klavLogPc = false;                   // B302: v radku je klavesa z klavesnice pocitace
// B310: POSTUP HRACE - Sega se odemyka po 50 napsanych radcich programu v Atari.
// Pocitalo to jen stare Atari v JavaScriptu (emu_vbxe, smazane v B310) - ted C++.
// Stejne pravidlo: radek zacinajici cislem (radek programu, ne prikaz), davka se
// odesle pri RUN nebo LIST (i zkratka L.) a Java ji uzna od 5 radku (NapPostup).
// Pocita se psani na pristroji i z klavesnice pocitace (TV/PC); text vlozeny z PC
// (Ctrl+V) a program z TXT souboru (pise ho appka sama) se nepocita.
static std::string g_postupRadek;                  // napsane znaky aktualniho radku - pod g_mStroj
static bool g_postupRadekVlozeny = false;          // v radku je vlozeny text - pod g_mStroj
static int g_postupDavka = 0;                      // radku programu od posledniho RUN / LIST - pod g_mStroj
static std::vector<int> g_postupHotove;            // davky pro Javu (devCtl "postup") - pod g_mOut
static std::mutex g_mFrame;
static std::vector<uint32_t> g_sdilenySnimek;      // posledni snimek Atari pro kresleni
static std::atomic<unsigned long long> g_snimekSeq{0};
// B303: prohlizece na TV/PC (web viewer appky) - obraz a zvuk primo z C++.
// Kazdy prohlizec ma vlastni vlakno v Jave (/atari.stream), ktere ceka v
// devTvProudDalsiNative na novy snimek. Poradi zamku: g_mStroj -> g_mTvProud.
struct TvKlientNat {
  nap::TvProudKlient k;
  std::vector<uint32_t> rgb;
  std::vector<int16_t> zvuk;
  std::vector<uint8_t> pk;
  int id = 0;
  long long startMs = 0, logMs = 0, logPaketu = 0, logBajtu = 0, paketu = 0, bajtu = 0;
};
static std::mutex g_mTvProud;
static std::condition_variable g_cvTvProud;
static std::map<int, std::unique_ptr<TvKlientNat>> g_tvKlienti;   // pod g_mTvProud
static int g_tvDalsiId = 1;
static std::atomic<int> g_tvKlientu{0};
static void tvProudSnimek() {
  if (g_tvKlientu.load() <= 0) return;
  { std::lock_guard<std::mutex> l(g_mTvProud); }   // kdo uz overil, ze snimek neni, ted opravdu ceka - zadne ztracene probuzeni
  g_cvTvProud.notify_all();
}
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
// B349: posledni archiv ZIP / RAR s hrou (devArchivNative, ATR / DISK)
static std::mutex g_mArchiv;
static nap::AtariArchiv g_archiv;          // posledni otevreny archiv (jeho diskety a programy)
static int g_archivAktualni = -1;          // ktery soubor archivu se naposled spustil / vlozil
static std::string g_archivChyba;          // proc z posledniho archivu nic nebylo
static std::vector<std::string> g_logOut;
static std::vector<int16_t> g_wavOut;
static bool g_wavReady = false;
static std::atomic<long long> g_snimkuCelkem{0};
// START/SELECT/OPTION: program je cte jako UROVEN (ne preruseni) - i hodne
// kratke tuknuti musi stroj videt drzene aspon 5 snimku, jinak by se mohlo
// ztratit mezi dvema snimky. (Klavesy se ztratit nemuzou - jdou pres IRQ.)
static int g_consolChtene = 7;
static long long g_consolDrzetDo[3] = {0, 0, 0};
// B348: jak dlouho drzet OPTION po zapnuti s disketou (BASIC vypnuty). OS 130XE OPTION
// precte uz ~8. snimek po zapnuti (zmereno v jadru: drzeni do 8. snimku = BASIC vypnuty).
// Drive 150 snimku (3 s) - to uz bezel zavadec hry a OPTION videl taky: xBIOS (Prince of
// Persia a dalsi nove hry) s drzenym OPTION ukaze svou nabidku souboru misto hry.
static const long long B348_OPTION_PRI_BOOTU = 50;
// B296: joystick 1 - z doteku na obrazovce a z herniho ovladace (bity:
// 0 nahoru, 1 dolu, 2 vlevo, 3 vpravo, 4 FIRE); START/SELECT/OPTION z
// ovladace (1/2/4 = stisknuto). Vse pod g_mStroj.
static int g_joyDotyk = 0, g_joyPad = 0, g_padConsol = 0, g_padKlav = 0;
static bool g_padBreak = false;                    // B355: BREAK z herniho ovladace (drzeny)
// B355: udalost PAD hrace 2 ma bit 30 (bity 8-17 hrace 1 jsou ted klavesa a BREAK - nap_ovladac.h atariMapuj)
static constexpr int PAD_HRAC2 = 1 << 30;
static int g_joyPad2 = 0, g_padConsol2 = 0;        // B354: druhy herni ovladac = joystick 2 (hrac 2)
static bool g_joyHlasenoDotyk = false, g_joyHlasenoPad = false, g_joyHlasenoPad2 = false;
// B302: klavesnice pocitace (prohlizec na TV/PC): joystick 1 a START/SELECT/OPTION
// (pod g_mStroj), klavesy v tempu pro OS (g_pcHrac, pod g_mStroj). Mapovani,
// rezim PSANI/HRANI a drzene klavesy (g_pcMapa) + fronta akci z vlakna
// webserveru (g_pcVstup) pod g_mPc. Poradi zamku: g_mStroj -> g_mPc.
static int g_joyPc = 0, g_pcConsol = 0;
static bool g_joyHlasenoPc = false;
static nap::PcPrehravac g_pcHrac;
static std::mutex g_mPc;
static nap::PcKlavesnice g_pcMapa;
static std::vector<nap::PcAkce> g_pcVstup;
static bool g_pcPrvni = false;
static int g_pcNeznamych = 0;
static std::deque<nap::PcAkce> g_pcText;            // B303: text vlozeny z PC (Ctrl+V) - pod g_mPc, do g_pcHrac po malych davkach
// B363: online hra - stroj jede jen ze stisku relace (obou hracu, atariOnlineVstupy pred snimkem). Dotyk, herni
// ovladac a klavesnice PC za online hry jen skladaji stisky tohoto telefonu (onlineLokalniBityLocked), do stroje nesahaji.
static std::atomic<int> g_onlineHraje{0};
static inline bool onlineVstupyZRelace() { return g_onlineHraje.load(std::memory_order_relaxed) != 0; }
static int consolEfektivni() {
  int m = g_consolChtene & 7;
  long long f = g_snimkuCelkem.load();
  for (int b = 0; b < 3; b++) if (f < g_consolDrzetDo[b]) m &= ~(1 << b);
  m &= ~(g_padConsol | g_padConsol2 | g_pcConsol);
  return m;
}
static void aplikujJoyLocked() {
  if (!g_stroj || onlineVstupyZRelace()) return;   // B363: za online hry jen stisky z relace
  const int m = g_joyDotyk | g_joyPad | g_joyPc;
  const int m2 = g_joyPad2;                         // B354: hrac 2 = druhy herni ovladac
  g_stroj->porta = (((~m2 & 15) << 4) | (~m & 15)) & 0xFF;   // PORTA: joystick 1 = dolni 4 bity, 2 = horni, 0 = stisknuto
  g_stroj->trig[0] = (m & 16) ? 0 : 1;            // TRIG0 = FIRE joysticku 1
  g_stroj->trig[1] = (m2 & 16) ? 0 : 1;           // TRIG1 = FIRE joysticku 2
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
// B352: rozsirena pamet 1088K (64 bank = 1 MB jako Ultimate 1MB). Vychozi je 320K (130XE + Compy Shop, B323);
// program, ktery zapisuje do okna $4000-$7FFF pod vic nez 16 ruznymi PORTB (= hleda vic bank, nez 320K ma),
// appka spusti znovu od zacatku s 1088K a jeho jmeno si pamatuje, dalsi start uz je rovnou s 1088K (pod g_mStroj).
static bool g_pamet1088 = false;                   // dalsi studeny start zalozi stroj s 1088K
static std::set<std::string> g_jmena1088;          // programy (XEX / disketa / cartridge / kazeta), ktere 1088K chtely
static std::vector<uint8_t> g_xexData;             // posledni spusteny XEX - kvuli spusteni znovu s 1088K
static std::string g_xexJmeno;
static bool g_xexPosledni = false;                 // posledni start byl XEX (ne POWER, disketa, cartridge, kazeta)
static unsigned g_strojGen = 0;                    // B353: kolikaty stroj (studeny start = novy program)
static bool chce1088(const std::string &jm) { return !jm.empty() && g_jmena1088.count(jm) > 0; }
// co nabootuje studeny start se strojem, jak je (disketa v D1: > cartridge > nic = BASIC)
static std::string programVeStrojiLocked() {
  if (!g_stroj) return "";
  if (g_stroj->disk.mounted) return g_stroj->disk.name;
  if (g_stroj->cart.vlozena()) return g_stroj->cart.jmeno;
  return "";
}
static std::string g_statusText;                   // B295: zprava pod pristroj z emulacniho vlakna (pod g_mStroj)
static int g_statusMs = 0;
static void studenyStartLocked(int consol) {
  // B292: kazeta v magnetofonu zustava i pri vypnuti/zapnuti pocitace
  // B295: stejne tak disketa v mechanice D1:
  // B316: a cartridge ve slotu (banky se vrati do stavu po zapnuti)
  nap::TapeDeck paska;
  nap::AtrDisk disketa;
  nap::Cartridge cartridge;
  if (g_stroj) { paska = std::move(g_stroj->tape); disketa = std::move(g_stroj->disk); cartridge = std::move(g_stroj->cart); }
  delete g_stroj; g_stroj = nullptr;
  delete g_view;  g_view  = nullptr;
  zaloz();                       // novy stroj: pamet = vzor DRAM 130XE po zapnuti
  g_stroj->mem.rezim1088 = g_pamet1088;            // B352
  g_strojGen++;                                    // B353
  if (g_pamet1088) devLog("B352 PAMET: studeny start s 1088K (64 bank = 1 MB) - program uz jednou hledal vic bank nez 320K");
  g_stroj->cart = std::move(cartridge);
  if (g_stroj->cart.vlozena()) g_stroj->cart.studenyStart();
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
  g_klavLog.clear(); g_klavLogPc = false;
  g_postupRadek.clear(); g_postupRadekVlozeny = false;   // B310: rozepsany radek zmizel se strojem
  g_pcHrac.zrus();               // B302: novy stroj nema zadnou klavesu stisknutou
}

// B316: jiny program (XEX, ATR, TXT do BASICu, kazeta s bootem, VYSUNOUT DISKETU)
// -> cartridge ze slotu ven, jinak by ji OS po zapnuti spustil misto nej.
static void vyjmoutCartridgeLocked(const char *proc) {
  if (!g_stroj || !g_stroj->cart.vlozena()) return;
  devLog(std::string("B316 CARTRIDGE ") + g_stroj->cart.jmeno + " vyjmuta ze slotu (" + proc + ")");
  g_stroj->cart.vyjmout();
}

// B310: postup hrace - je to radek programu ("10 PRINT A"), ne jen cislo (smazani radku)?
static bool postupJeRadekProgramu(const std::string &r) {
  size_t i = 0;
  while (i < r.size() && r[i] >= '0' && r[i] <= '9') i++;
  if (i == 0) return false;
  while (i < r.size() && r[i] == ' ') i++;
  return i < r.size();
}
// B310: RUN / LIST (i zkratka L.) - konec davky
static bool postupJeRunList(const std::string &r) {
  auto slovo = [&](const char *s) {
    const size_t n = std::strlen(s);
    if (r.compare(0, n, s) != 0) return false;
    if (r.size() == n) return true;
    const char c = r[n];
    return !((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
  };
  return slovo("RUN") || slovo("LIST") || r.compare(0, 2, "L.") == 0;
}
// B310: jedna klavesa pro postup hrace (v = scankod, -1 = BREAK; vlozeno = z Ctrl+V).
// POZOR: volat jen pod g_mStroj.
static void postupKlavesaLocked(int v, bool vlozeno) {
  if (v < 0) { g_postupRadek.clear(); g_postupRadekVlozeny = false; return; }   // BREAK = radek zrusen
  const int sc = v & 0x3F;
  if (sc == 12) {                                                  // RETURN
    std::string r = g_postupRadek;
    const bool vl = g_postupRadekVlozeny;
    g_postupRadek.clear(); g_postupRadekVlozeny = false;
    size_t a = 0, b = r.size();
    while (a < b && r[a] == ' ') a++;
    while (b > a && r[b - 1] == ' ') b--;
    r = r.substr(a, b - a);
    if (vl || r.empty()) return;
    if (postupJeRadekProgramu(r)) { g_postupDavka++; return; }
    if (postupJeRunList(r) && g_postupDavka > 0) {
      const int n = g_postupDavka;
      g_postupDavka = 0;
      { std::lock_guard<std::mutex> o(g_mOut); if (g_postupHotove.size() < 64) g_postupHotove.push_back(n); }
      devLog("B310 POSTUP HRACE: davka " + std::to_string(n) + " radku programu (" + r.substr(0, 6) + ") -> Java (Sega se otevre po 50)");
    }
    return;
  }
  if (vlozeno) g_postupRadekVlozeny = true;
  if (sc == 52) {                                                  // BACK S / DELETE
    if (v & 0x40) g_postupRadek.clear();                           // SHIFT+DELETE = smazat cely radek
    else if (!(v & 0x80) && !g_postupRadek.empty()) g_postupRadek.pop_back();   // BACK S = smazat znak
    return;
  }
  const std::string t = nap::napScanText(v);
  if (t == "<CLEAR>") { g_postupRadek.clear(); return; }
  if (t.size() == 1 && g_postupRadek.size() < 120) g_postupRadek += t;   // jen znaky (ne sipky, ESC, ^X ...)
}

// B299/B302: co uzivatel napsal - radek do logu po RETURN (kvuli chybam, ktere na PC
// nejdou zopakovat). v = scankod, -1 = BREAK; zPc = z klavesnice pocitace (TV/PC).
// B310: bit $100 = klavesa z vlozeneho textu (Ctrl+V); a navic postup hrace.
// POZOR: volat jen pod g_mStroj.
static void zalogujKlavesuLocked(int v, bool zPc) {
  const bool vlozeno = v >= 0 && (v & 0x100);
  if (v >= 0) v &= 0xFF;
  postupKlavesaLocked(v, vlozeno);
  if (zPc) g_klavLogPc = true;
  const char *pred = g_klavLogPc ? "B299 KLAVESY (PC): " : "B299 KLAVESY: ";
  if (v < 0) {
    devLog(pred + g_klavLog + "<BREAK>");
    g_klavLog.clear(); g_klavLogPc = false;
    return;
  }
  const bool ret = (v & 0x3F) == 12;
  if (!ret) g_klavLog += nap::napScanText(v);
  if (ret || g_klavLog.size() > 160) {
    // prazdny RETURN (hry, potvrzovani) jen 3x po sobe, at log nezahlti
    if (!g_klavLog.empty()) g_klavPrazdnych = 0;
    if (!g_klavLog.empty() || ++g_klavPrazdnych <= 3)
      devLog(pred + g_klavLog + (ret ? (g_klavLog.empty() ? "<RETURN>" : " <RETURN>") : " ..."));
    g_klavLog.clear(); g_klavLogPc = false;
  }
}

// B302: jeden snimek klavesnice pocitace (emulacni vlakno, pod g_mStroj, PRED runFrame):
// akce z webserveru vyzvednout, joystick a konzoli hned, klavesy v tempu pro OS.
// B353: posledni stisk hrace (pristroj, herni ovladac, PC) - turbo pri nahravani se pak 3 s nezapne
static std::atomic<long long> g_posledniVstupMs{0};
static void vstupHraceTed() { g_posledniVstupMs.store(nap_ted_ms()); }
static void pcKlavesniceSnimekLocked() {
  std::vector<nap::PcAkce> a;
  bool ticho = false;
  {
    std::lock_guard<std::mutex> p(g_mPc);
    a.swap(g_pcVstup);
    // B303: vlozeny text - po malych davkach (PcPrehravac drzi nejvys 256 akci)
    if (g_pcHrac.q.size() < 16) for (int i = 0; i < 32 && !g_pcText.empty(); i++) { a.push_back(g_pcText.front()); g_pcText.pop_front(); }
    ticho = g_pcMapa.hlidej(nap_ted_ms(), a);   // prohlizec se odmlcel - vse pustit
  }
  if (ticho) devLog("B302 PC KLAVESNICE: prohlizec se 2,5 s neozval (zavreny, spadla Wi-Fi?) - drzene klavesy, joystick a konzole pusteny");
  if (!a.empty()) vstupHraceTed();                       // B353: hrac hraje z PC -> zadne turbo
  for (const auto &x : a) {
    if (x.t == nap::PcAkce::JOY) {
      g_joyPc = x.v & 31;
      aplikujJoyLocked();
      if (!g_joyHlasenoPc && g_joyPc) { g_joyHlasenoPc = true; devLog("B302 JOYSTICK 1 z klavesnice pocitace (HRANI: WASD/sipky smer, K/mezernik skok, L FIRE)"); }
    } else if (x.t == nap::PcAkce::KONZOLE) {
      const int pc = x.v & 7;
      // kratky stisk START/SELECT/OPTION musi stroj videt aspon 5 snimku
      for (int b = 0; b < 3; b++)
        if ((pc & (1 << b)) && !(g_pcConsol & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
      g_pcConsol = pc;
      if (g_stroj && !g_xex.aktivni() && !onlineVstupyZRelace()) g_stroj->consol = consolEfektivni();
    } else {
      g_pcHrac.pridej(x);
    }
  }
  if (!g_stroj) return;
  // B363: za online hry jen joystick a START / SELECT / OPTION z PC (vyse) - psani klaves z PC jde jen mimo online
  if (onlineVstupyZRelace()) { g_pcHrac.zrus(); return; }
  // psani programu z TXT / textu ma prednost (nove klavesy z PC pockaji ve fronte)
  const bool smiStisk = !g_psani.aktivni() && g_typeQ.empty();
  std::vector<int> st;
  g_pcHrac.krok(*g_stroj, &st, smiStisk);
  for (int v : st) zalogujKlavesuLocked(v, true);
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

// B352: program zapisuje do okna rozsirene pameti pod vic nez 16 ruznymi PORTB = hleda vic bank, nez 320K ma
// (Tony VBXE: test pameti "BRAK RAM - WYMAGANE 496 KB + VBXE" chce 27 bank). Spustit ho znovu od zacatku
// s 1088K - XEX znovu ze zapamatovanych dat, disketa / cartridge zustava ve stroji (studeny start jako POWER).
static void restart1088Locked() {
  Machine &m = *g_stroj;
  m.mem.chce1088 = false;
  const int ruznych = m.mem.pbRuznych;
  std::string co, jm;
  if (g_xexPosledni && !g_xexData.empty()) { co = "XEX"; jm = g_xexJmeno; }
  else if (m.disk.mounted) { co = "disketa"; jm = m.disk.name; }
  else if (m.cart.vlozena()) { co = "cartridge"; jm = m.cart.jmeno; }
  char b[400];
  if (co.empty()) {
    // BASIC / kazeta: neni z ceho spustit znovu - 1088K hned za behu (a pri dalsim zapnuti)
    m.mem.rezim1088 = true; g_pamet1088 = true;
    snprintf(b, sizeof(b), "B352 PAMET: program hleda vic bank nez 320K (zapsal do okna $4000-$7FFF pod %d ruznymi PORTB) - "
             "neni z ceho ho spustit znovu (BASIC / kazeta), 1088K zapnuto hned za behu", ruznych);
    devLog(b);
    return;
  }
  g_jmena1088.insert(jm);
  g_pamet1088 = true;
  snprintf(b, sizeof(b), "B352 PAMET: %s %s hleda vic bank nez 320K (zapsal do okna $4000-$7FFF pod %d ruznymi PORTB, 320K ma 16 bank) "
           "- spoustim znovu od zacatku s 1088K (64 bank = 1 MB jako Ultimate 1MB)", co.c_str(), jm.c_str(), ruznych);
  devLog(b);
  if (co == "XEX") {
    g_xex.priprav(g_xexData.data(), g_xexData.size(), g_xexJmeno);
    studenyStartLocked(3);                       // jako devLoadXexNative: OPTION drzene = BASIC vypnuty
    if (g_stroj->disk.mounted) g_stroj->disk = nap::AtrDisk();   // jinak by OS bootoval disketu misto XEX
    g_stroj->sioRychlyTimeout = true;
  } else if (co == "disketa") {
    g_xex.zrus();
    studenyStartLocked(3);                       // jako devLoadAtrNative: OPTION 1 s, bootuje D1:
    g_stroj->sioRychlyTimeout = false;
    g_consolChtene = 7;
    g_consolDrzetDo[2] = g_snimkuCelkem.load() + B348_OPTION_PRI_BOOTU;
    g_consolDrzetDo[0] = g_consolDrzetDo[1] = 0;
  } else {
    g_xex.zrus();
    studenyStartLocked(7);                       // cartridge ve slotu zustava
    g_stroj->sioRychlyTimeout = true;
  }
  g_strojBezi = true;
  g_statusText = nap::tx(u8"HRA CHCE VÍC PAMĚTI - SPOUŠTÍM JI S 1 MB", "THE GAME WANTS MORE MEMORY - RESTARTING WITH 1 MB");
  g_statusMs = 6000;
}

// =====================================================================
//  B353: TURBO PRI NAHRAVANI. Rene: "to atr urychli prosim". Hra s vlastnim zavadecem (Prince of
//  Persia, xBIOS) cte disketu pres POKEY skutecnou rychlosti 1050 (19200 Bd, sektor 256 B = 0,13 s;
//  level PoP ~70 s), OS cte pres SIO zkratku hned, ale zavadec mezi sektory taky pracuje v realnem case;
//  Tony VBXE rozbaluje data do bank ~26 s ("PROSZE CZEKAC"). Nic z toho se NEMENI: cely stroj (procesor,
//  ANTIC, POKEY, casovace, SIO) bezi presne stejne, jen se v periode 20 ms emuluje vic snimku - az
//  TURBO_MAX, kolik telefon stihne za TURBO_ROZPOCET_NS (pomaly telefon = mensi zrychleni, nic se nerozbije).
//  KDY: disketa v D1: a program s ni mluvil (SIO pres POKEY i pres OS) v posledni 1 s emulovaneho casu,
//  nebo zavadec XEX (start OS, segmenty, kratke INIT; INIT dele nez 2 s = program uz bezi / titulka).
//  NE: hrac neco drzi (joystick, FIRE, klavesa) nebo v poslednich 3 s neco zmackl - hra muze disketu cist
//  i behem hrani. Zvuk je pri turbu ztlumeny (OpenSL a TV dostavaji ticho v realnem case - fronta neroste),
//  na displej a TV jde jen posledni snimek davky. Log "B353 TURBO", pod pristrojem "TURBO - NAHRAVAM".
// =====================================================================
static const int TURBO_MAX = 8;
static const long long TURBO_ROZPOCET_NS = 16000000LL;   // z periody 20 ms (zbytek pro kresleni a system)
static bool g_turboPovoleno = true;                       // (pod g_mStroj)
static long long g_turboSioPred = -1, g_turboDoSnimku = 0;
static bool g_turboBezi = false;
static long long g_turboOdMs = 0, g_turboSnimku = 0, g_turboHlaskaMs = 0;
static int g_turboDuvod = 0;
static unsigned g_turboGen = 0;                          // g_strojGen relace (novy stroj = novy program)
// proc ted smi bezet turbo: 0 = ne, 1 = disketa D1:, 2 = zavadec XEX (pod g_mStroj)
static int turboDuvodLocked() {
  if (!g_turboPovoleno || !g_stroj || g_stroj->cpu.jam || !g_strojBezi.load()) return 0;
  if (onlineVstupyZRelace()) return 0;                       // B363: za online hry bez turba (stroj jen podle relace)
  Machine &m = *g_stroj;
  const long long f = g_snimkuCelkem.load();
  int duvod = 0;
  if (m.disk.mounted) {
    const long long p = m.sio.prikazu + m.sioPrikazu;      // prikazy pro D1: pres POKEY + pres OS (SIO zkratka)
    if (p != g_turboSioPred) { g_turboDoSnimku = f + 50; g_turboSioPred = p; }
    if (f < g_turboDoSnimku) duvod = 1;
  }
  if (g_xex.aktivni() && !(g_xex.stav == nap::XexLoader::INIT && g_xex.initSnimku > 100)) duvod = 2;
  if (!duvod) return 0;
  if ((m.porta & 0x0F) != 0x0F || m.trig[0] == 0 || !(m.skstat & 0x04)) return 0;   // hrac neco drzi
  if (nap_ted_ms() - g_posledniVstupMs.load() < 3000) return 0;                     // hrac prave hral
  return duvod;
}
// po kazde periode: zacatek / konec turba do logu, hlaska pod pristrojem (pod g_mStroj)
static void turboKonecLocked(long long ted) {
  g_turboBezi = false;
  const double s = (ted - g_turboOdMs) / 1000.0, atari = g_turboSnimku / 50.0;
  char b[200];
  if (s > 0.05) snprintf(b, sizeof(b), "B353 TURBO konec: %.1f s, emulovano %.1f s Atari (%.1fx rychleji)", s, atari, atari / s);
  else snprintf(b, sizeof(b), "B353 TURBO konec: %.2f s, emulovano %.2f s Atari (kratke - rychlost se neda zmerit)", s, atari);   // B354: drive "0.0x"
  devLog(b);
}
static void turboHlidejLocked(int duvod, int snimku) {
  const long long ted = nap_ted_ms();
  if (g_turboBezi && (!duvod || duvod != g_turboDuvod || g_strojGen != g_turboGen)) turboKonecLocked(ted);   // konec / jiny program
  if (duvod) {
    if (!g_turboBezi) {
      g_turboBezi = true; g_turboOdMs = ted; g_turboSnimku = 0; g_turboHlaskaMs = 0;
      g_turboDuvod = duvod; g_turboGen = g_strojGen;
      const std::string co = duvod == 1 ? "cteni diskety D1: " + g_stroj->disk.name : "zavadec XEX " + g_xex.jmeno;
      devLog("B353 TURBO start: " + co + " - az " + std::to_string(TURBO_MAX) + " snimku za periodu (kolik telefon stihne), zvuk ztlumeny");
    }
    g_turboSnimku += snimku;
    if (ted - g_turboHlaskaMs > 2500) {
      g_turboHlaskaMs = ted;
      g_statusText = nap::tx(u8"TURBO - NAHRÁVÁM RYCHLEJI", "TURBO - LOADING FASTER"); g_statusMs = 3000;
    }
  }
}

// =====================================================================================================
//  B363: ONLINE ATARI 130XE (Rene 25. 11. 2026: "mohl by jsi se prosim te uz pustit do online atari - at se
//  nezdrzujeme"). Dva telefony, kazdy emuluje cele Atari sam, po siti (engine N&P, online/nap_online*.h) jdou jen
//  stisky. Hrac 1 = kdo hru zalozil (joystick 1), hrac 2 = kdo se pripojil (joystick 2; nebo oba na joysticku 1).
//   - START ZE STAVU: az se telefony domluvi, hrac 1 posle CELY stav sveho Atari i s hrou (cartridge, disketa,
//     zavadec XEX - nap_atari_online.h, zlib) a hrac 2 ho nahraje: hrac 2 hru mit nemusi a hraje se od stejneho
//     okamziku. Kdyz hrac 1 hru jeste nahrava (zavadec XEX, kazeta, psani programu), pocka, az dobehne.
//   - za hry jdou do stroje JEN stisky z relace (atariOnlineVstupy pred kazdym snimkem); dotyk, herni ovladac
//     a klavesnice PC jen skladaji stisky tohoto telefonu. Veci "bokem", ktere by stroj zmenily jen na jednom
//     telefonu, jsou za online hry vypnute: turbo, psani TXT / textu z PC, kazeta, restart s 1088K, zaznam CSAVE.
//     Nova hra (XEX / ATR / CAR / TXT), POWER nebo vysunuti = konec online (druhy telefon dostane KONEC).
//   - ODHAD + OPRAVA jako u Segy (B362): stisk druheho, ktery jeste neprisel, se odhadne; pred takovym snimkem se
//     ulozi stav (kruh), a kdyz skutecny stisk prijde jiny, stroj se vrati a snimky se spocitaji znovu (bez zvuku
//     a obrazu). Otisk stavu po castech kazdych 60 snimku, rozjeti = synchronizace (hrac 1 posle stav znovu).
//   - SETRNE KRESLENI (Rene: "proc treba na slabsi telefony nepridat optimalizaci? ... stacilo by jen ponizit
//     grafiku"): emulace musi na obou telefonech bezet CELA (jinak by se hra rozjela), ale obraz se kreslit nemusi
//     kazdy - kdyz je telefon pomaly (jadro + opravy zaberou vic nez pulku snimku), jde na displej (a TV) jen kazdy
//     druhy snimek (25/s). Zapne / vypne se samo, je v logu i ve statistice pro druhy telefon.
// =====================================================================================================
static nap::online::OnlineKlient g_online;
static std::atomic<int> g_onlineAktivni{0};          // online zapnuto (pripojuje se, ceka na hrace, hraje)
static std::atomic<int> g_onlineStartu{0};           // kolikate spojeni (Java pozna novy pokus)
static std::atomic<uint32_t> g_onlineSnimkuCelkem{0};
static std::atomic<uint32_t> g_onlineStavKb{0};      // posledni stav hry poslany / prijaty (KB zlib)
static std::atomic<int> g_onlineStartStav{0};        // 0 nic, 1 hrac 1 dohrava (nahravani hry), 2 posila / ceka na STAV_OK, 3 hrac 2 ceka na stav, 4 hotovo
// Rizeni po jednom: onlineStart / onlineStop / nova hra / konec zjisteny vlaknem emulace (jako Sega B357).
static std::mutex g_onlineRizeni;
static std::mutex g_onlineM;                         // g_onlineKonec, g_onlineHraNazev
static std::string g_onlineKonec;
static std::atomic<bool> g_onlineObaNaJ1Volba{false};   // hrac 1: "oba na joysticku 1" pri zalozeni
static std::atomic<bool> g_onlinePortyJ1{false};        // kopie g_onlineKontext.obaNaJ1 pro onlineStav() (cte se bez g_mStroj)
// jen vlakno emulace (a pod g_mStroj, kde sahaji na stroj)
static uint32_t g_onlineSnimek = 0;
static bool g_onlineCekaStart = false;               // hrac 1: START prisel, hra se jeste nahrava - az dobehne
static long long g_onlineCekaStartOdMs = 0;
static nap::AtariOnlineKontext g_onlineKontext;      // pod g_mStroj
static nap::AtariOnlineKruh<nap::online::ODHAD_MAX + 2> g_onlineKruh;   // pod g_mStroj
static std::vector<uint8_t> g_onlineSurova;          // pod g_mStroj
// stisky tohoto telefonu za online hry (pod g_mStroj): klavesa z pristroje drzena aspon 2 snimky, BREAK a RESET
// jako udalost do pristiho snimku; bity pro snimek se slozi jednou (opakovany pokus po cekani posle stejne)
static int g_onlineKlavesa = -1;
static uint32_t g_onlineKlavesaOd = 0;
static bool g_onlineKlavesaPustit = false, g_onlineBreak = false, g_onlineReset = false;
static uint32_t g_onlineLokalniSnimek = UINT32_MAX, g_onlineLokalniBity = 0;
static long long g_onlineBlokHlasenoMs = 0;          // hlaska "za online nejde" (kazeta ...) ne kazdy dotek
// vykon (vlakno emulace)
static uint64_t g_onlineZatezEmaUs = 5000;          // snimek + opravy + otisk za periodu (prumer)
static std::atomic<uint32_t> g_onlineOdhadMax{nap::online::ODHAD_MAX};
static std::atomic<uint32_t> g_onlineChybOpravy{0}, g_onlineNavratZe{0}, g_onlineNavratNa{0};
static uint64_t g_onlineVykonOdUs = 0, g_onlineVykonJadroUs = 0, g_onlineVykonJadroMaxUs = 0;
static uint32_t g_onlineVykonSnimku = 0, g_onlineVykonKresleno = 0;
static std::atomic<int> g_onlineSetrne{0};           // setrne kresleni zapnuto
static std::atomic<uint32_t> g_onlineSetrneZapnuto{0};   // kolikrat se zaplo (Java do logu)
static std::string g_onlineSetrneProc;               // pod g_onlineM
static uint64_t g_onlineOknoZatezUs = 0;             // zatez za aktualni 2s okno (soucet)
static int g_onlineKlidnychOken = 0;                 // 2s oken po sobe pod 6 ms (setrne kresleni vypnout po 3)
static long long g_onlineStatusMs = 0;               // naposledy hlaska pod pristrojem
static uint64_t g_onlinePoslanoUs = 0;               // hrac 1 poslal stav (kdy) - do logu, jak dlouho STAV_OK trval

// AHOJ: oba telefony musi mit stejne jadro Atari (stav i chovani) - hra se posila ve stavu, otisk hry tedy pevny
static const uint8_t ONLINE_ATARI_HRA[20] = {'N', '&', 'P', ' ', 'A', 'T', 'A', 'R', 'I', ' ', '1', '3', '0', 'X', 'E', ' ', 'S', 'T', 'A', 'V'};
static const uint32_t ONLINE_ATARI_JADRO = 363000u + nap::ATARI_STAV_VERZE;   // zmena jadra / stavu = jine cislo

/** Stisky tohoto telefonu pro snimek (jednou za snimek - opakovany pokus po cekani dostane stejne). Pod g_mStroj. */
static uint32_t onlineLokalniBityLocked(uint32_t snimek) {
  if (snimek == g_onlineLokalniSnimek) return g_onlineLokalniBity;
  g_onlineLokalniSnimek = snimek;
  const int joy = (g_joyDotyk | g_joyPad | g_joyPc) & 0x1F;
  const int konzole = (~consolEfektivni()) & 7;                 // 1 = stisknuto (kratky stisk drzen 5 snimku)
  int kod = -1;
  if (g_onlineKlavesa >= 0) kod = g_onlineKlavesa;
  else if (g_padKlav) kod = g_padKlav - 1;                       // B355: tlacitko ovladace s klavesou
  if (g_onlineKlavesaPustit && snimek >= g_onlineKlavesaOd + 2) { g_onlineKlavesa = -1; g_onlineKlavesaPustit = false; }
  g_onlineLokalniBity = nap::atariOnlineBity(joy, konzole, kod >= 0, kod < 0 ? 0 : kod, g_onlineBreak, g_shiftZamek, g_onlineReset);
  g_onlineBreak = g_onlineReset = false;
  return g_onlineLokalniBity;
}

/** Otisk stavu po snimku (kazdych 60) - i pri oprave (relace ho odesle, az je snimek jisty). Pod g_mStroj. */
static void onlineOtiskLocked(const std::shared_ptr<nap::online::Relace> &r, uint32_t snimek) {
  if (!r || snimek % nap::online::HASH_KAZDYCH != 0 || !g_stroj) return;
  uint64_t c[nap::ATARI_STAV_CASTI];
  const uint64_t h = nap::atariOnlineOtisk(*g_stroj, g_onlineKontext, c);
  r->otiskStavu(snimek, h, c, (uint8_t)nap::ATARI_STAV_CASTI);
}

// test (test_overeni/b363) si sem dosadi "poruchu" stroje hrace 2 a "pomaly telefon" (zatez navic na snimek, us) -
// v appce prazdne / 0 (nic se neprelozi)
#ifndef NAP_ATARI_ONLINE_TEST_PO_SNIMKU
#define NAP_ATARI_ONLINE_TEST_PO_SNIMKU(snimek) do {} while (0)
#endif
#ifndef NAP_ATARI_ONLINE_TEST_ZATEZ_US
#define NAP_ATARI_ONLINE_TEST_ZATEZ_US 0
#endif

/** Oprava: stav pred snimkem 'od' zpet, snimky od .. g_onlineSnimek-1 znovu se skutecnymi stisky. Pod g_mStroj. */
static void onlineOpravLocked(const std::shared_ptr<nap::online::Relace> &r, uint32_t od) {
  const uint32_t ted = g_onlineSnimek;
  if (od >= ted || !g_stroj) return;
  if (!g_onlineKruh.vrat(*g_stroj, g_onlineKontext, od)) {
    // nemelo by nastat (stav se uklada pred kazdym odhadnutym snimkem) - otisky rozdil poznaji, synchronizace spravi
    if (g_onlineChybOpravy.fetch_add(1) < 5) devLog("B363 ATARI ONLINE oprava od snimku " + std::to_string(od) + ": ulozeny stav chybi");
    return;
  }
  const long long t0 = nap_ted_ns();
  AnticView *obraz = g_stroj->view;
  g_stroj->view = nullptr;                          // snimky opravy se nekresli (obraz uz jednou byl)
  uint32_t f = od;
  for (; f < ted; f++) {
    uint32_t p1 = 0, p2 = 0;
    bool odhad = false;
    if (r->vstupyOdhad(f, p1, p2, odhad, 0, nap::online::ODHAD_OPRAVA) != 1) break;   // konec relace
    if (odhad) g_onlineKruh.uloz(*g_stroj, g_onlineKontext, f);
    nap::atariOnlineVstupy(*g_stroj, p1, p2, g_onlineKontext.obaNaJ1);
    if (!g_stroj->cpu.jam) g_stroj->runFrame();     // presne jako normalni snimek (zaseknuty procesor = stroj stoji)
    g_stroj->srovnatSledovaniZvuku();               // zvuk techto snimku uz jednou hral
    g_xex.poSnimku(*g_stroj);
    if (g_stroj->mem.chce1088) g_stroj->mem.chce1088 = false;
    NAP_ATARI_ONLINE_TEST_PO_SNIMKU(f);
    onlineOtiskLocked(r, f);
  }
  g_stroj->view = obraz;
  if (f < ted) g_onlineSnimek = f;                   // relace skoncila uprostred - dal se stejne nehraje
  r->opraveno(od, f, (uint64_t)((nap_ted_ns() - t0) / 1000));
}

/** Konec online hry zjisteny vlaknem emulace (relace r skoncila / spojeni skoncilo). Stroj jede dal sam. */
static void onlineKonec(const std::shared_ptr<nap::online::Relace> &r) {
  std::lock_guard<std::mutex> rz(g_onlineRizeni);
  if (r ? g_online.relace() != r : g_online.faze() != nap::online::OnlineKlient::Faze::KONEC) return;
  std::string proc = g_online.chyba();
  if (proc.empty() && r) proc = r->textKonce();
  {
    std::lock_guard<std::mutex> l(g_onlineM);
    g_onlineKonec = proc.empty() ? "online skoncilo" : proc;
  }
  g_onlineHraje.store(0);
  g_onlineAktivni.store(0);
  g_onlineSetrne.store(0);
  g_onlineCekaStart = false;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    g_onlineKruh.zahod(true);                        // stavy pro opravu uz netreba (pamet zpet)
    g_onlineKlavesa = -1; g_onlineKlavesaPustit = false; g_onlineBreak = g_onlineReset = false;
    aplikujJoyLocked();                              // joystick zase primo z tohoto telefonu
    g_statusText = nap::atariEn() ? "ONLINE ENDED - " + proc : "ONLINE SKONCILO - " + proc;
    if (g_statusText.size() > 46) g_statusText = g_statusText.substr(0, 44) + "..";
    g_statusMs = 6000;
  }
  devLog("B363 ATARI ONLINE konec po " + std::to_string(g_onlineSnimek) + " snimcich: " + proc + " - Atari bezi dal jen na tomto telefonu");
}

/**
 * Nova hra / POWER / vysunuti uprostred online = konec online (druhy telefon dostane KONEC). Z Javy (JNI) i z
 * vlakna emulace - NE pod g_mStroj (stop ceka na vlakno site).
 */
static void onlineKonecProNovouHru(const char *proc) {
  std::lock_guard<std::mutex> rz(g_onlineRizeni);
  if (!g_onlineAktivni.load()) return;
  g_onlineAktivni.store(0);
  g_onlineHraje.store(0);
  g_onlineSetrne.store(0);
  g_online.stop();
  {
    std::lock_guard<std::mutex> l(g_onlineM);
    g_onlineKonec = std::string(proc) + " - online skoncilo";
  }
  devLog(std::string("B363 ATARI ONLINE konec - ") + proc);
}

/** Hrac 1: je stroj pripraveny poslat stav? (hra uz se nenahrava). Pod g_mStroj. "" = ano, jinak proc ne. */
static std::string onlineNepripravenLocked() {
  if (!g_stroj || !g_strojBezi.load()) return "Atari je vypnute";
  if (g_xex.aktivni()) return "XEX se nahrava";
  if (g_kazBoot.aktivni()) return "hra se nahrava z kazety";
  if (g_psani.aktivni()) return "appka pise program";
  if (!g_typeQ.empty()) return "appka pise text";
  return "";
}

/** Zacatek online hry v tomto stroji (oba telefony): veci bokem pryc, stisky jen z relace. Pod g_mStroj. */
static void onlineZacniLocked(bool hrac1) {
  g_typeQ.clear();
  g_psani.zrus();
  g_kazBoot.zrus();
  g_pcHrac.zrus();
  g_rec.reset();
  { std::lock_guard<std::mutex> p(g_mPc); g_pcText.clear(); }
  if (g_stroj && g_stroj->tape.play) {
    g_stroj->tape.play = false;                      // kazeta za online hry stoji (na druhem telefonu neni)
    devLog("B363 ATARI ONLINE: kazetak zastaven (za online hry se kazeta nepouziva)");
  }
  if (g_stroj) { g_stroj->tape.line = 1; g_stroj->tape.rxPhase = 0; }
  if (!hrac1) g_xex.zrus();                          // hrac 2: zavadec prijde ve stavu od hrace 1
  g_onlineKontext.obaNaJ1 = hrac1 && g_onlineObaNaJ1Volba.load();
  g_onlinePortyJ1.store(g_onlineKontext.obaNaJ1);
  g_onlineKontext.xex = &g_xex;
  g_onlineKruh.zahod(false);
  g_onlineSnimek = 0;
  g_onlineLokalniSnimek = UINT32_MAX;
  g_onlineKlavesa = -1; g_onlineKlavesaPustit = false; g_onlineBreak = g_onlineReset = false;
  g_onlineVykonOdUs = 0; g_onlineVykonSnimku = 0; g_onlineVykonKresleno = 0; g_onlineVykonJadroUs = g_onlineVykonJadroMaxUs = 0;
  g_onlineZatezEmaUs = 5000;
  g_onlineOknoZatezUs = 0;
  g_onlineKlidnychOken = 0;
  g_onlineNavratZe.store(0);
  g_onlineNavratNa.store(0);
  g_onlineHraje.store(1);
}

/** Hrac 2 dostal stav: Atari zapnout (i na pristroji), jinak se snimky nepocitaji. Pod g_mStroj. */
static void onlineZapniPoStavuLocked() {
  if (!g_strojBezi.load()) devLog("B363 ATARI ONLINE: Atari bylo vypnute - zapinam ho (stav od hrace 1)");
  g_strojBezi = true;
  g_atariRing.clear();
}

/**
 * Pred snimkem (vlakno emulace, BEZ g_mStroj - ceka se tu na stisky druheho nejvys 30 ms):
 * 1 = emulovat snimek se stisky p1 / p2 z relace, 0 = ted ne (stisky / stav druheho jeste neprisly), -1 = bez online.
 */
static int onlinePredSnimkem(uint32_t &p1, uint32_t &p2) {
  if (!g_onlineAktivni.load()) { g_onlineCekaStart = false; return -1; }   // (konec z Javy - dalsi hra zacne od nuly)
  if (g_online.vezmiStart()) {
    std::shared_ptr<nap::online::Relace> r = g_online.relace();
    const bool hrac1 = r && r->hrac1();
    std::lock_guard<std::mutex> l(g_mStroj);
    if (hrac1) {
      g_onlineCekaStart = true;                      // stav pujde, az hra dobehne do pripraveneho stavu
      g_onlineCekaStartOdMs = nap_ted_ms();
      g_onlineStartStav.store(1);
      devLog("B363 ATARI ONLINE start: domluveno s " + g_online.jmenoDruheho() + " - posilam mu cely stav Atari i s hrou");
    } else {
      onlineZacniLocked(false);
      g_onlineStartStav.store(3);
      devLog("B363 ATARI ONLINE start: domluveno s " + g_online.jmenoDruheho() + " - cekam na stav Atari od hrace 1 (hra prijde s nim)");
    }
  }
  if (g_onlineCekaStart) {
    std::string proc;
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      proc = onlineNepripravenLocked();
      if (proc.empty()) {
        onlineZacniLocked(true);
        g_onlineCekaStart = false;
        g_onlineStartStav.store(2);
      }
    }
    if (!proc.empty()) {
      if (g_online.faze() == nap::online::OnlineKlient::Faze::KONEC) { onlineKonec(nullptr); return -1; }
      std::shared_ptr<nap::online::Relace> r = g_online.relace();
      if (r && r->konec()) { onlineKonec(r); return -1; }
      static long long hlaseno = 0;
      if (nap_ted_ms() - hlaseno > 5000) {
        hlaseno = nap_ted_ms();
        devLog("B363 ATARI ONLINE: stav jeste neposilam - " + proc + " (ceka se " + std::to_string((nap_ted_ms() - g_onlineCekaStartOdMs) / 1000) + " s)");
      }
      return -1;                                     // hra se dohrava na tomto telefonu (zavadec, kazeta ...)
    }
  }
  if (!g_onlineHraje.load()) {
    // ceka se na druheho hrace: hra zatim bezi normalne jen na tomto telefonu
    if (g_online.faze() == nap::online::OnlineKlient::Faze::KONEC) { onlineKonec(nullptr); return -1; }
    return -1;
  }
  std::shared_ptr<nap::online::Relace> r = g_online.relace();
  if (!r) return 0;
  long long prace = 0;                              // cas prace (oprava, stav) - ne cekani na sit
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) return 0;
    r->lokalni(g_onlineSnimek, onlineLokalniBityLocked(g_onlineSnimek));
  }
  const uint32_t opravitOd = r->zkontrolujOdhady(g_onlineSnimek);
  if (opravitOd != UINT32_MAX) {
    const long long a = nap_ted_ns();
    std::lock_guard<std::mutex> l(g_mStroj);
    onlineOpravLocked(r, opravitOd);
    prace += nap_ted_ns() - a;
  }
  if (r->hrac1()) {
    if (r->chciPoslatStav(g_onlineSnimek)) {
      std::vector<uint8_t> st;
      bool ok;
      const long long a = nap_ted_ns();
      {
        std::lock_guard<std::mutex> l(g_mStroj);
        ok = g_stroj && nap::atariOnlineZabal(*g_stroj, g_onlineKontext, g_onlineSurova, st);
      }
      prace += nap_ted_ns() - a;
      if (ok && st.size() < (3u << 20) + (900u << 10)) {   // engine predava zpravy do 4 MB
        r->posliStav(g_onlineSnimek, st);
        g_onlineStavKb.store((uint32_t)((st.size() + 1023) / 1024));
        g_onlinePoslanoUs = nap::online::tedUs();
        char b[260];
        snprintf(b, sizeof(b), "B363 ATARI ONLINE %s: stav Atari pred snimkem %u odeslan (%u KB zlib, surove %u KB, zabaleno za %lld ms)",
                 g_onlineSnimek == 0 ? "START" : "synchronizace", g_onlineSnimek, g_onlineStavKb.load(),
                 (unsigned)(g_onlineSurova.size() / 1024), (nap_ted_ns() - a) / 1000000);
        devLog(b);
      } else {
        devLog(std::string("B363 ATARI ONLINE: stav Atari ") + (ok ? "je moc velky (" + std::to_string(st.size() / 1024) + " KB, engine predava do 4 MB)" : "nejde zabalit") + " - konec");
        r->ukonci(g_onlineSnimek == 0 ? nap::online::D_START : nap::online::D_ROZJETO);
      }
    }
  } else {
    std::vector<uint8_t> st;
    uint32_t g = 0;
    const int k = r->stavPro(g_onlineSnimek, st, g, 30);
    if (k < 0) return 0;                             // stav od hrace 1 jeste neprisel - Atari stoji
    if (k > 0) {
      const long long a = nap_ted_ns();
      std::lock_guard<std::mutex> l(g_mStroj);
      if (g_stroj && nap::atariOnlineRozbal(*g_stroj, g_onlineKontext, st, g_onlineSurova)) {
        const uint32_t bylo = g_onlineSnimek;
        g_onlineSnimek = g;
        g_onlineKruh.zahod(false);
        g_onlineLokalniSnimek = UINT32_MAX;
        r->stavNahran(g);
        g_onlinePortyJ1.store(g_onlineKontext.obaNaJ1);   // porty urcil hrac 1 (prislo ve stavu)
        g_onlineStavKb.store((uint32_t)((st.size() + 1023) / 1024));
        if (bylo > g) { g_onlineNavratZe.store(bylo); g_onlineNavratNa.store(g); }
        if (g == 0) { onlineZapniPoStavuLocked(); g_onlineStartStav.store(4); }
        g_xexPosledni = false;                       // program ve stroji neni XEX z tohoto telefonu (restart 1088K ne)
        char b[300];
        snprintf(b, sizeof(b), "B363 ATARI ONLINE %s: stav Atari pred snimkem %u nahran (%u KB zlib, nahrano za %lld ms)%s - %s%s, D1: %s",
                 g == 0 ? "START" : "synchronizace", g, g_onlineStavKb.load(), (nap_ted_ns() - a) / 1000000,
                 bylo > g ? (" - vracen ze snimku " + std::to_string(bylo)).c_str() : "",
                 g_stroj->cart.vlozena() ? "cartridge " : "", g_stroj->cart.vlozena() ? g_stroj->cart.jmeno.c_str() : "bez cartridge",
                 g_stroj->disk.mounted ? g_stroj->disk.name.c_str() : "prazdna");
        devLog(b);
        if (g == 0) { g_statusText = nap::tx(u8"ONLINE: HRA OD HRÁČE 1 NAHRÁNA", "ONLINE: GAME FROM PLAYER 1 LOADED"); g_statusMs = 4000; }
        prace += nap_ted_ns() - a;
      } else {
        // poskozeny / cizi stav (jina verze) - stroj muze byt napul prepsany: zapnout znovu, konec (oba vedi proc)
        devLog("B363 ATARI ONLINE: stav od hrace 1 nejde nahrat (" + std::to_string(st.size() / 1024) + " KB) - konec, Atari zapnuto znovu");
        g_xex.zrus();
        studenyStartLocked(7);
        r->ukonci(g == 0 ? nap::online::D_START : nap::online::D_ROZJETO);
      }
    }
  }
  bool odhad = false;
  const int v = r->vstupyOdhad(g_onlineSnimek, p1, p2, odhad, 30, g_onlineOdhadMax.load());
  if (v == 0) return 0;
  if (v < 0) { onlineKonec(r); return -1; }
  if (odhad) {
    const long long a = nap_ted_ns();
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) g_onlineKruh.uloz(*g_stroj, g_onlineKontext, g_onlineSnimek);
    prace += nap_ted_ns() - a;
  }
  if (g_onlineStartStav.load() == 2 && !r->startZeStavu()) {
    g_onlineStartStav.store(4);
    devLog("B363 ATARI ONLINE START: hrac 2 stav nahral (STAV_OK za " + std::to_string((nap::online::tedUs() - g_onlinePoslanoUs) / 1000) + " ms) - hraje se");
  }
  g_onlineVykonJadroUs += (uint64_t)(prace / 1000);   // oprava a stav se pocitaji do zateze telefonu
  return 1;
}

/**
 * Po snimku online hry (pod g_mStroj): otisk stavu, dalsi snimek, vykon telefonu (limit odhadu, setrne kresleni).
 * zatezUs = jadro tohoto snimku (stisky, snimek, zvuk, zavadec); opravy a stav pricetl onlinePredSnimkem.
 * Vraci true = tento snimek jde na displej (setrne kresleni: jen kazdy druhy).
 */
static bool onlinePoSnimkuLocked(uint64_t zatezUs) {
  std::shared_ptr<nap::online::Relace> r = g_online.relace();
  NAP_ATARI_ONLINE_TEST_PO_SNIMKU(g_onlineSnimek);
  onlineOtiskLocked(r, g_onlineSnimek);
  const uint32_t snimek = g_onlineSnimek++;
  g_onlineSnimkuCelkem.store(g_onlineSnimek);
  // zatez za periodu = tento snimek + opravy / stav (g_onlineVykonJadroUs uz ma pricteno)
  const uint64_t zatez = zatezUs + g_onlineVykonJadroUs + (uint64_t)(NAP_ATARI_ONLINE_TEST_ZATEZ_US);
  g_onlineVykonJadroUs = 0;
  g_onlineZatezEmaUs = (g_onlineZatezEmaUs * 15 + zatez) / 16;
  // limit odhadu: oprava nejvys ~66 ms pocitani (pomaly telefon = mene snimku napred)
  uint32_t m = (uint32_t)(66000 / std::max<uint64_t>(g_onlineZatezEmaUs, 1000));
  m = std::max<uint32_t>(2, std::min<uint32_t>(nap::online::ODHAD_MAX, m));
  g_onlineOdhadMax.store(m);
  // kazde 2 s: prumer zateze za okno (snimky + opravy + stav) -> setrne kresleni a text pro druhy telefon (STATISTIKA).
  // Prumer za 2 s (ne kratky prumer): davka oprav pri spicce site setrne kresleni nezapne, pomaly telefon ano.
  const uint64_t ted = nap::online::tedUs();
  const bool kreslit = !g_onlineSetrne.load() || (snimek & 1) == 0;
  if (!g_onlineVykonOdUs) g_onlineVykonOdUs = ted;
  g_onlineVykonSnimku++;
  if (kreslit) g_onlineVykonKresleno++;
  g_onlineOknoZatezUs += zatez;
  if (zatez > g_onlineVykonJadroMaxUs) g_onlineVykonJadroMaxUs = zatez;
  if (ted - g_onlineVykonOdUs >= 2000000) {
    const double s = (ted - g_onlineVykonOdUs) / 1e6;
    const double prumerMs = g_onlineOknoZatezUs / 1000.0 / std::max<uint32_t>(g_onlineVykonSnimku, 1);
    // setrne kresleni: zapnout, kdyz telefon za 2 s bral vic nez pulku snimku (10 ms z 20 ms); vypnout po 3 oknech
    // (6 s) pod 6 ms
    if (!g_onlineSetrne.load()) {
      if (prumerMs > 10.0 && g_onlineSnimek > 100) {
        g_onlineSetrne.store(1);
        g_onlineSetrneZapnuto.fetch_add(1);
        g_onlineKlidnychOken = 0;
        char b[200];
        snprintf(b, sizeof(b), "jadro a opravy %.1f ms na snimek (z 20 ms, prumer za 2 s)", prumerMs);
        { std::lock_guard<std::mutex> l(g_onlineM); g_onlineSetrneProc = b; }
        devLog(std::string("B363 ATARI ONLINE SETRNE KRESLENI zapnuto: ") + b + " - na displej jde kazdy 2. snimek (25/s), emulace a zvuk dal 50/s");
      }
    } else if (prumerMs < 6.0) {
      if (++g_onlineKlidnychOken >= 3) {
        g_onlineSetrne.store(0);
        char b[200];
        snprintf(b, sizeof(b), "B363 ATARI ONLINE SETRNE KRESLENI vypnuto: telefon zase stiha (%.1f ms na snimek) - obraz zase 50/s", prumerMs);
        devLog(b);
      }
    } else {
      g_onlineKlidnychOken = 0;
    }
    char b[200];
    snprintf(b, sizeof(b), "%.1f snimku/s (obraz %.0f/s%s), jadro %.1f ms (nejvys %.1f), odhad nejvys %u", g_onlineVykonSnimku / s,
             g_onlineVykonKresleno / s, g_onlineSetrne.load() ? " setrne" : "", prumerMs, g_onlineVykonJadroMaxUs / 1000.0, m);
    if (r) r->vykon(b);
    g_onlineVykonOdUs = ted;
    g_onlineVykonSnimku = g_onlineVykonKresleno = 0;
    g_onlineVykonJadroMaxUs = 0;
    g_onlineOknoZatezUs = 0;
  }
  return kreslit;
}

/** Hlaska pod pristrojem za online (kazde 2 s): cislo hry, s kym, odezva; vlakno emulace, pod g_mStroj. */
static void onlineStatusLocked() {
  if (!g_onlineAktivni.load()) return;
  const long long ted = nap_ted_ms();
  if (ted - g_onlineStatusMs < 2000) return;
  g_onlineStatusMs = ted;
  const bool en = nap::atariEn();
  std::string t;
  using F = nap::online::OnlineKlient::Faze;
  const F f = g_online.faze();
  std::shared_ptr<nap::online::Relace> r = g_online.relace();
  if (f == F::PRIPOJUJE) t = en ? "ONLINE: CONNECTING..." : u8"ONLINE: PŘIPOJUJI SE...";
  else if (f == F::CEKA_NA_HRACE) t = (en ? "ONLINE: WAITING - GAME NUMBER " : u8"ONLINE: ČEKÁM - ČÍSLO HRY ") + g_online.kodMistnosti();
  else if (f == F::HRAJE && r) {
    const int s = g_onlineStartStav.load();
    if (g_onlineCekaStart) t = en ? "ONLINE: FINISHING LOADING..." : u8"ONLINE: DOKONČUJI NAHRÁVÁNÍ...";
    else if (s == 2) t = en ? "ONLINE: SENDING THE GAME..." : u8"ONLINE: POSÍLÁM HRU HRÁČI 2...";
    else if (s == 3) t = en ? "ONLINE: RECEIVING THE GAME..." : u8"ONLINE: PŘIJÍMÁM HRU OD HRÁČE 1...";
    else if (!r->bezi()) t = en ? "ONLINE: CONNECTING PHONES..." : u8"ONLINE: DOMLOUVÁM SE...";
    else {
      std::string jm = g_online.jmenoDruheho();
      if (jm.size() > 12) jm = jm.substr(0, 12);
      for (char &c : jm) if ((unsigned char)c < 32 || (unsigned char)c > 126) c = '?';
      t = (en ? "ONLINE P" : u8"ONLINE H") + std::string(r->hrac1() ? "1" : "2") + (en ? " WITH " : " S ") + jm + " - " +
          std::to_string(r->odezvaMs()) + " MS" + (g_onlineSetrne.load() ? (en ? " (ECO)" : u8" (ŠETRNĚ)") : "");
    }
  }
  if (t.empty()) return;
  g_statusText = t;
  g_statusMs = 2600;
}

static void emuVlaknoMain() {
  std::vector<float> zvuk(882), pasek(882), zvukVen(882), zvukR(882), zvukVenR(882);   // B361: R = druhy POKEY
  bool stereoBylo = false, pk2Bezel = false;
  std::string statusText; int statusMs = 0;
  long long dalsi = nap_ted_ns();
  devLog("B291 EMULACE_VLAKNO start (50 snimku/s, zvuk OpenSL, vse v C++)");
  nastavPriorituVlakna(-8, "EMULACE_VLAKNO");
  bool posunout = true;   // B363: dalsi snimek az za periodu; online po cekani na druheho hned (bez dalsi periody)
  while (g_run.load()) {
    // 1) udalosti z doteku na pristroji
    std::vector<nap::dev::Ev> ev;
    { std::lock_guard<std::mutex> l(g_mEv); ev.swap(g_evQ); }
    const char *koncitOnline = nullptr;   // B363: POWER za online hry = konec online (az mimo g_mStroj)
    if (!ev.empty()) {
      std::lock_guard<std::mutex> l(g_mStroj);
      const bool onl = onlineVstupyZRelace();   // B363: stisky jdou do stisku tohoto telefonu, ne do stroje
      for (const auto &e : ev) {
        switch (e.t) {
          case nap::dev::Ev::POWER_ON: {
            if (g_onlineAktivni.load()) koncitOnline = "POWER (studeny start Atari)";
            g_xex.zrus();
            // B298: START/SELECT/OPTION, ktere uzivatel PRAVE DRZI na pristroji,
            // plati i pri zapnuti - jako na skutecnem 130XE (START = boot
            // z kazety, OPTION = bez BASICu). Drive se pri zapnuti pustily.
            const int drzene = (g_consolChtene & ~(g_padConsol | g_pcConsol)) & 7;   // prst na pristroji, herni ovladac nebo F1/F3/F4 na PC
            g_xexPosledni = false; g_pamet1088 = chce1088(programVeStrojiLocked());   // B352
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
              g_statusText = nap::tx(u8"BOOT Z KAZETY - RETURN STISKNU SÁM", "BOOT FROM TAPE - I WILL PRESS RETURN"); g_statusMs = 8000;
            } else if (g_stroj && g_stroj->disk.mounted) {
              // B295: s disketou v D1: se startuje jako s hrou - OPTION drzene (BASIC vypnuty)
              // B348: jen 1 s (50 snimku) - OS OPTION cte ~8. snimek po zapnuti; dele drzene
              // OPTION videly zavadece her (xBIOS v Prince of Persia -> nabidka misto hry)
              g_consolDrzetDo[2] = g_snimkuCelkem.load() + B348_OPTION_PRI_BOOTU;
              devLog("B295 POWER s disketou v D1: (" + g_stroj->disk.name + ") - OPTION drzeno pri startu, BASIC vypnuty, bootuje disketa");
              // B298: at je videt, proc se zase nahrava hra (Rene: "W3D nepomuze ani vypnout a zapnout")
              {   // nazev zkratit, at se zprava vejde na radek pod pristrojem
                std::string jm = g_stroj->disk.name; if (jm.size() > 18) jm = jm.substr(0, 16) + "..";
                g_statusText = nap::atariEn() ? "IN D1: " + jm + " - EJECT: ATR/DISK" : "V D1: JE " + jm + " - VYSUNOUT: ATR/DISK"; g_statusMs = 7000;
              }
            } else if (drzene != 7) {
              devLog(std::string("B298 POWER s drzenym") + ((drzene & 1) ? "" : " START") + ((drzene & 2) ? "" : " SELECT") + ((drzene & 4) ? "" : " OPTION"));
            }
            break;
          }
          case nap::dev::Ev::POWER_OFF:
            if (g_onlineAktivni.load()) koncitOnline = "Atari vypnute (POWER)";
            g_strojBezi = false; g_typeQ.clear(); g_rec.reset(); g_xex.zrus(); g_atariRing.clear(); g_psani.zrus();
            g_pcHrac.zrus();                                       // B302: klavesy z PC ve fronte zahodit
            { std::lock_guard<std::mutex> p(g_mPc); g_pcVstup.clear(); g_pcText.clear(); }
            devLog("B291 POWER VYPNUTO");
            break;
          case nap::dev::Ev::KEY:
            vstupHraceTed();                                   // B353: hrac hraje -> zadne turbo
            if (onl) {                                         // B363: klavesa do stisku tohoto telefonu (drzena aspon 2 snimky)
              g_onlineKlavesa = e.v & 0xFF; g_onlineKlavesaOd = g_onlineSnimek; g_onlineKlavesaPustit = false;
              zalogujKlavesuLocked(e.v, false);
              break;
            }
            // B292: klavesa drzena, dokud je na ni prst (OS ji pak opakuje)
            if (g_stroj && g_strojBezi) {
              g_stroj->klavesa(e.v, true);
              // B299: co presne uzivatel napsal (radek do logu po RETURN) - kvuli chybam, ktere
              // na PC nejdou zopakovat (TBXL LIST u Reneho)
              zalogujKlavesuLocked(e.v, false);
            }
            break;
          case nap::dev::Ev::KEYUP:
            if (onl) { g_onlineKlavesaPustit = true; break; }   // B363
            if (g_stroj) g_stroj->klavesaPustena();
            break;
          case nap::dev::Ev::SHIFT:
            g_shiftZamek = e.v != 0;
            if (g_stroj && !onl) g_stroj->shiftDrzen(g_shiftZamek);   // B363: online - SHIFT jde ve stiscich
            break;
          case nap::dev::Ev::TAPE:
          case nap::dev::Ev::REWIND:
          case nap::dev::Ev::FFWD:
            if (onl) {                                         // B363: kazeta jen na jednom telefonu by hru rozjela
              if (nap_ted_ms() - g_onlineBlokHlasenoMs > 3000) {
                g_onlineBlokHlasenoMs = nap_ted_ms();
                g_statusText = nap::tx(u8"ZA ONLINE HRY KAZETA NEJDE", "NO TAPE DURING AN ONLINE GAME"); g_statusMs = 3000;
              }
              break;
            }
            if (e.t == nap::dev::Ev::REWIND) {
              if (g_stroj && g_stroj->tape.loaded) {
                g_stroj->tape.pos = 0; g_stroj->tape.rxPhase = 0;
                devLog("B292 KAZETAK REW - pasek pretocen na zacatek");
              }
              break;
            }
            if (e.t == nap::dev::Ev::FFWD) {
              if (g_stroj && g_stroj->tape.loaded) {
                nap::TapeDeck &t = g_stroj->tape;
                t.pos = std::min((double)t.img.n, t.pos + 10.0 * t.img.rate); t.rxPhase = 0;
                char b[120];
                snprintf(b, sizeof(b), "B292 KAZETAK FWD - pasek +10 s (pozice %.1f / %.1f s)", t.pos / t.img.rate, t.img.seconds());
                devLog(b);
              }
              break;
            }
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
          case nap::dev::Ev::CONSOL: {
            vstupHraceTed();                                   // B353: hrac hraje -> zadne turbo
            int nove = e.v & 7;
            for (int b = 0; b < 3; b++)
              if (!(nove & (1 << b)) && (g_consolChtene & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
            g_consolChtene = nove;
            if (g_stroj && !onl) g_stroj->consol = consolEfektivni();   // B363: online - konzole ve stiscich
            break;
          }
          case nap::dev::Ev::RESET:
            vstupHraceTed();                                   // B353: hrac hraje -> zadne turbo
            if (onl) { g_onlineReset = true; devLog("B363 ATARI ONLINE: RESET (pujde ve stiscich - oba telefony ve stejnem snimku)"); break; }
            if (g_stroj && g_strojBezi) {
              g_stroj->reset(); g_stroj->consol = 7;
              g_kazBoot.zrus();          // B298: teply start boot z kazety nedela
              g_psani.zrus();
              devLog("B291 RESET (tlacitko RESET na pristroji - pamet zustava)");
            }
            break;
          case nap::dev::Ev::BREAK:
            vstupHraceTed();                                   // B353: hrac hraje -> zadne turbo
            if (onl) { g_onlineBreak = true; zalogujKlavesuLocked(-1, false); break; }   // B363
            if (g_stroj && g_strojBezi) {
              g_stroj->breakKey();
              zalogujKlavesuLocked(-1, false);
            }
            break;
          case nap::dev::Ev::JOY:
            if ((e.v & 31) != g_joyDotyk) vstupHraceTed();     // B353/B355: hrac hraje -> zadne turbo (jen skutecna zmena)
            g_joyDotyk = e.v & 31;
            aplikujJoyLocked();
            if (!g_joyHlasenoDotyk) { g_joyHlasenoDotyk = true; devLog("B296 JOYSTICK 1 dotykem na obrazovce (leva pulka = smer, prava = FIRE)"); }
            break;
          case nap::dev::Ev::PAD: {
            // B355: do turba (B353) se pocita jen SKUTECNA zmena - "pust vse" pri odchodu ze hry zustalo ve
            // fronte a na zacatku dalsi hry drzelo turbo 3 s (log Reneho: Tony a PoP turbo az po ~3 s)
            if (e.v & PAD_HRAC2) {                             // B354: druhy ovladac = joystick 2 + START / SELECT / OPTION
              const int pc2 = (e.v >> 5) & 7;
              if ((e.v & 31) != g_joyPad2 || pc2 != g_padConsol2) vstupHraceTed();
              g_joyPad2 = e.v & 31;
              aplikujJoyLocked();
              for (int b = 0; b < 3; b++)
                if ((pc2 & (1 << b)) && !(g_padConsol2 & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
              g_padConsol2 = pc2;
              if (g_stroj && !g_xex.aktivni() && !onl) g_stroj->consol = consolEfektivni();
              if (!g_joyHlasenoPad2 && (e.v & 0xFF)) { g_joyHlasenoPad2 = true; devLog("B354 JOYSTICK 2 z druheho herniho ovladace (hrac 2: smer, FIRE, START / SELECT / OPTION)"); }
              break;
            }
            const int pc = (e.v >> 5) & 7;
            const int kl = (int)((e.v & nap::ovladac::AP_KLAVESA_MASKA) >> nap::ovladac::AP_KLAVESA_POSUN);   // KBCODE + 1, 0 = zadna
            const bool brk = (e.v & nap::ovladac::AP_BREAK) != 0;
            if ((e.v & 31) != g_joyPad || pc != g_padConsol || kl != g_padKlav || brk != g_padBreak) vstupHraceTed();
            g_joyPad = e.v & 31;
            aplikujJoyLocked();
            if (pc != g_padConsol) {
              // kratky stisk START/SELECT/OPTION musi stroj videt aspon 5 snimku
              for (int b = 0; b < 3; b++)
                if ((pc & (1 << b)) && !(g_padConsol & (1 << b))) g_consolDrzetDo[b] = g_snimkuCelkem.load() + 5;
              g_padConsol = pc;
              if (g_stroj && !g_xex.aktivni() && !onl) g_stroj->consol = consolEfektivni();
            }
            // B355: tlacitko s klavesou (mapa: MEZERA, RETURN, ESC, HELP, P ...) - drzena jako prstem
            // B363: za online hry jde klavesa ovladace ve stiscich tohoto telefonu (g_padKlav)
            if (kl != g_padKlav && g_stroj && g_strojBezi && !onl) {
              if (kl) g_stroj->klavesa(kl - 1, true);
              else g_stroj->klavesaPustena();
            }
            g_padKlav = kl;
            if (brk && !g_padBreak && g_stroj && g_strojBezi) {   // B355: BREAK z ovladace
              if (onl) g_onlineBreak = true;                 // B363: online - ve stiscich
              else g_stroj->breakKey();
            }
            g_padBreak = brk;
            if (!g_joyHlasenoPad) { g_joyHlasenoPad = true; devLog("B296 JOYSTICK 1 z herniho ovladace / klavesnice telefonu (smer, FIRE, START, SELECT, OPTION)"); }
            break;
          }
          default: break;
        }
      }
      vyzvednoutVystupyLocked();
    }
    if (koncitOnline) onlineKonecProNovouHru(koncitOnline);   // B363: mimo g_mStroj (ceka na vlakno site)
    if (!g_strojBezi.load()) {
      // B302: pocitac vypnuty - klavesy z PC zahodit, ale joystick a START/SELECT/OPTION
      // drzene na PC plati (F1 drzene pri POWER = boot z kazety jako na skrini)
      {
        std::lock_guard<std::mutex> l(g_mStroj);
        std::vector<nap::PcAkce> a;
        { std::lock_guard<std::mutex> p(g_mPc); a.swap(g_pcVstup); }
        for (const auto &x : a) {
          if (x.t == nap::PcAkce::JOY) g_joyPc = x.v & 31;
          else if (x.t == nap::PcAkce::KONZOLE) g_pcConsol = x.v & 7;
        }
      }
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
    if (posunout) dalsi += perioda;
    posunout = true;
    long long ted = nap_ted_ns();
    if (dalsi > ted) std::this_thread::sleep_for(std::chrono::nanoseconds(dalsi - ted));
    else if (ted - dalsi > 200000000LL) dalsi = ted;   // zpozdeni (pozadi): nedohanet naraz
    // B363: online - snimek jen se stisky obou hracu z relace (ceka se tu na druheho nejvys 30 ms, mimo g_mStroj);
    // hrac 1 pred snimkem 0 posle cely stav Atari, hrac 2 ho nahraje. 0 = ted ne (Atari stoji) - hned znovu
    uint32_t onlineP1 = 0, onlineP2 = 0;
    const long long onlineOd = nap_ted_ns();
    const int online = onlinePredSnimkem(onlineP1, onlineP2);
    if (online == 0) {
      posunout = false;
      if (nap_ted_ns() - onlineOd < 1000000LL) std::this_thread::sleep_for(std::chrono::milliseconds(2));   // pojistka: zadne tocení naprazdno
      {
        std::lock_guard<std::mutex> l(g_mStroj);
        onlineStatusLocked();
        if (!g_statusText.empty()) { statusText.swap(g_statusText); statusMs = g_statusMs; g_statusText.clear(); }
      }
      if (!statusText.empty()) {
        std::lock_guard<std::mutex> dl(g_mDev);
        devGet().setStatusMessage(statusText.c_str(), nap_ted_ms(), statusMs);
        statusText.clear();
        pokeRender();
      }
      continue;
    }
    const bool onl = online == 1;
    // 3) jeden snimek stroje; B353: pri nahravani (turbo) az TURBO_MAX snimku v teto periode
    const long long davkaOd = nap_ted_ns();
    int vDavce = 0;
    for (bool pokracovat = true; pokracovat; ) {
      // mezi snimky turba pustit zamek stroje i ostatnim (JNI z Javy, TV) - mutex neni ferovy
      if (vDavce > 0) std::this_thread::sleep_for(std::chrono::microseconds(50));
      pokracovat = false;
      std::lock_guard<std::mutex> l(g_mStroj);
      if (!g_stroj) { g_pamet1088 = false; studenyStartLocked(7); }
      pcKlavesniceSnimekLocked();                                 // B302: klavesnice pocitace (TV/PC)
      if (onl) {
        // B363: online - do stroje jen stisky obou hracu z relace (stejne na obou telefonech)
        nap::atariOnlineVstupy(*g_stroj, onlineP1, onlineP2, g_onlineKontext.obaNaJ1);
      } else {
        if (!g_xex.aktivni()) g_stroj->consol = consolEfektivni();   // pri zavadeni XEX drzi OPTION zavadec
        if (g_kazBoot.aktivni()) g_stroj->consol = nap::KazetaBoot::konzole(g_stroj->consol);   // B298: boot z kazety
        g_typeQ.step(*g_stroj);
      }
      const long long jadroOd = nap_ted_ns();
      // B299: pri psani programu z TXT jde dalsi klavesa hned, jak ji OS prevezme
      if (!g_stroj->cpu.jam) {
        if (!onl && g_psani.pise()) g_stroj->runFrameRadky([](Machine &m) { g_psani.radek(m); });
        else g_stroj->runFrame();
      }
      if (g_stroj->cpu.jam) { std::fill(zvuk.begin(), zvuk.end(), 0.f); std::fill(zvukR.begin(), zvukR.end(), 0.f); }
      else g_stroj->genAudioStereo(zvuk.data(), zvukR.data(), 882, 44100.0);   // B361: levy POKEY 1, pravy POKEY 2
      if (vDavce == 0) { zvukVen = zvuk; zvukVenR = zvukR; }   // B353: ven jde zvuk 1. snimku periody (pri turbu ticho)
      // B361: do logu, kdyz program poprve sahne na druhy POKEY a kdy zacne / skonci stereo
      if (g_stroj->pk2.bezi && !pk2Bezel)
        devLog("B361 POKEY 2: program pouziva druhy POKEY ($D210-$D21F, stereo upgrade) - hleda ho, nebo na nem hraje");
      pk2Bezel = g_stroj->pk2.bezi;
      if (g_stroj->stereoAktivni() != stereoBylo) {
        stereoBylo = g_stroj->stereoAktivni();
        devLog(stereoBylo ? "B361 POKEY STEREO: program hraje na druhy POKEY - levy kanal POKEY 1, pravy POKEY 2"
               : g_pokeyStereo.load() ? "B361 POKEY STEREO konec (studeny start) - pravy kanal zase jako levy"
                                      : "B361 POKEY STEREO konec (vypnuto v menu) - pravy kanal zase jako levy");
      }
      // B292: CSAVE nahrava linku SIO DATA OUT (signal pro magnetofon), ne
      // reproduktor - presne to, co by zapsal skutecny Atari 410/1010
      g_stroj->genTape(pasek.data(), 882);
      for (float &v : pasek) v *= 0.8f;
      if (!onl) g_rec.snimek(*g_stroj, pasek.data(), 882);   // B363: za online hry zadny zaznam CSAVE
      g_xex.poSnimku(*g_stroj);                              // B363: online taky - zavadec je ve stavu (oba stejne)
      if (g_stroj->mem.chce1088 && !g_stroj->mem.rezim1088) {
        if (!onl) restart1088Locked();                       // B352
        else {                                               // B363: online - restart jen na jednom telefonu ne
          g_stroj->mem.chce1088 = false;
          static bool hlaseno = false;
          if (!hlaseno) { hlaseno = true; devLog("B363 ATARI ONLINE: program chce 1088K - za online hry se nerestartuje (spust ho s 1 MB pred online hrou)"); }
        }
      }
      if (!onl) {
        if (g_kazBoot.poSnimku(*g_stroj)) { g_statusText = nap::tx(u8"NAHRÁVÁ SE Z KAZETY...", "LOADING FROM TAPE..."); g_statusMs = 15000; }
        g_psani.poSnimku(*g_stroj);
        if (!g_psani.status.empty()) { g_statusText = g_psani.status; g_statusMs = g_psani.statusMs; g_psani.status.clear(); }
      }
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
      // B348: hra cte disketu vlastnim zavadecem (SIO primo pres POKEY) - do logu prvni prikaz
      // a pak souhrn kazdych ~10 s, dokud prikazy chodi (NAK / spatne soucty / cizi jednotky)
      {
        static long long pred = 0, nakPred = 0, souPred = 0, ciziPred = 0, posledniSnimek = -1000;
        const nap::SioBus &s = g_stroj->sio;
        if (s.prikazu < pred) pred = nakPred = souPred = ciziPred = 0;          // novy stroj (studeny start)
        const long long f = g_snimkuCelkem.load();
        if (s.prikazu > pred && (pred == 0 || f - posledniSnimek >= 500)) {
          char b[300];
          snprintf(b, sizeof(b), "B348 SIO: program mluvi s D1: primo pres POKEY (vlastni zavadec) - prikazu %lld (posledni $%02X '%c' sektor %d), "
                   "nacteno sektoru %lld, zapsano %lld, NAK %lld, spatny soucet %lld, jina jednotka %lld",
                   s.prikazu, s.posledni[1], (s.posledni[1] >= 32 && s.posledni[1] < 127) ? s.posledni[1] : '?', s.posledni[2] | (s.posledni[3] << 8),
                   g_stroj->disk.reads, g_stroj->disk.writes, s.nak - nakPred, s.spatnySoucet - souPred, s.cizi - ciziPred);
          devLog(b);
          pred = s.prikazu; nakPred = s.nak; souPred = s.spatnySoucet; ciziPred = s.cizi; posledniSnimek = f;
        }
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
            g_statusText = t.loaded ? (t.play ? nap::tx("MOTOR BEZI - CTU PASKU", "MOTOR RUNNING - READING THE TAPE")
                                              : nap::tx("MOTOR BEZI - ZMACKNI PLAY!", "MOTOR RUNNING - PRESS PLAY!"))
                                    : nap::tx("MOTOR BEZI", "MOTOR RUNNING");
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
      g_snimkuCelkem++;
      vDavce++;
      // B363: online - otisk stavu, dalsi snimek, vykon telefonu; setrne kresleni = na displej jen kazdy 2. snimek
      bool kreslit = true;
      if (onl) kreslit = onlinePoSnimkuLocked((uint64_t)((nap_ted_ns() - jadroOd) / 1000));
      // B353: dalsi snimek hned (turbo), dokud se nahrava, je cas v rozpoctu periody a nejvys TURBO_MAX
      const int turbo = turboDuvodLocked();
      pokracovat = turbo && vDavce < TURBO_MAX && nap_ted_ns() - davkaOd < TURBO_ROZPOCET_NS;
      if (!pokracovat) {
        if (vDavce > 1) { std::fill(zvukVen.begin(), zvukVen.end(), 0.f); std::fill(zvukVenR.begin(), zvukVenR.end(), 0.f); }   // turbo: ticho
        nap_atari_audio_push_stereo(zvukVen.data(), zvukVenR.data(), 882);
        if (kreslit) {
          {
            std::lock_guard<std::mutex> f(g_mFrame);
            if (g_sdilenySnimek.size() != (size_t)AnticView::FW * AnticView::H)
              g_sdilenySnimek.assign((size_t)AnticView::FW * AnticView::H, 0xFF000000u);
            std::memcpy(g_sdilenySnimek.data(), g_view->fb, g_sdilenySnimek.size() * 4);
          }
          g_snimekSeq++;
          tvProudSnimek();                               // B303: prohlizec na TV/PC - novy snimek
        }
        turboHlidejLocked(turbo, vDavce);
      }
      onlineStatusLocked();                              // B363: hlaska pod pristrojem za online (kazde 2 s)
      vyzvednoutVystupyLocked();
      if (!g_statusText.empty()) { statusText.swap(g_statusText); statusMs = g_statusMs; g_statusText.clear(); }
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
      g_padBreak = false;                                    // B355
      g_joyPad2 = g_padConsol2 = 0;                          // B354
      g_joyPc = g_pcConsol = 0;                              // B302: klavesnice pocitace pustena
      g_pcHrac.zrus();
      { std::lock_guard<std::mutex> p(g_mPc); g_pcVstup.clear(); g_pcMapa.drzene.clear(); g_pcText.clear(); }
      // B298: START/OPTION drzene pri POWER plati (boot z kazety) - novy vstup do
      // HELP ale zacina s pustenou konzoli
      g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
      // B310: postup hrace - novy vstup do Atari zacina novou davku (jako drive nova
      // stranka stareho Atari); hotove davky, ktere Java jeste nevyzvedla, zustanou
      g_postupDavka = 0;
    }
    if (studeny || !g_stroj) {
      g_xex.zrus();
      g_xexPosledni = false; g_pamet1088 = chce1088(programVeStrojiLocked());   // B352
      studenyStartLocked(7);
      devLog(studeny ? "B291 HELP otevren - pristroj ZAPNUT, studeny start Atari"
                     : "B291 HELP - stroj neexistoval, studeny start");
      // B293: co presne se emuluje - at je to v logu cerne na bilem
      char b[700];
      const unsigned osSum = (unsigned)NAP_OS_ROM[0] | ((unsigned)NAP_OS_ROM[1] << 8);
      snprintf(b, sizeof(b),
               "B293 JADRO = ATARI 130XE PAL: RAM 64 kB + rozsirena 16x16 kB (B323: 320K Compy Shop - PORTB bity 2-3 a 6-7, "
               "banky 130XE beze zmeny; procesor bit 4, ANTIC bit 5; B352: program, ktery hleda vic nez 16 bank, se sam spusti "
               "znovu s 1088K = 64 bank jako Ultimate 1MB - PORTB bity 1-3 a 5-7), "
               "OS XL/XE rev.%u (soucet $%04X), BASIC rev.C vestaveny, self-test ROM $5000, TRIG3 = RD5 cartridge (prazdny slot = 0), "
               "RESET = reset CPU+ANTIC+PIA/MMU, plovouci datova sbernice, ANTIC/GTIA PAL 312 radku, POKEY 1,773 MHz",
               (unsigned)NAP_OS_ROM[0x3FF7], osSum);
      devLog(b);
      devLog("B316 CARTRIDGE: soubory .car (XEX/MOBIL, NET, ZIP) - slot $8000-$9FFF / $A000-$BFFF pred BASICem, banky pres "
             "$D500-$D5FF, 67 typu (standard 2-16 kB, XEGS a Switchable XEGS, MegaCart, Atarimax 128 kB / 1 MB, SIC!, "
             "Williams, Express, Diamond, SpartaDOS X, OSS, Blizzard, Phoenix, Turbosoft, Atrax, Bounty Bob, J(atari)Cart, DCart ...); "
             "jiny program (XEX / ATR / TXT) cartridge vytahne, POWER ji necha ve slotu.");
      devLog("B296 VBXE FX 1.26 v jadre: registry $D640, 512 kB VRAM, MEMAC A/B, XDL (overlay LR/SR/HR/text 80 sloupcu), "
             "atributova mapa, palety 4x256, blitter (rezimy 0-6, zoom, vzor, kolize) + IRQ; obraz 4 body na barevny takt. "
             "Joystick 1: leva pulka obrazovky = smer, prava = FIRE; herni ovladac taky.");
      devLog("B298 ZVUK s pasmem jako TV (FIR 16 kHz, bez prekladu ultrazvuku do slysitelna), asynchronni prijem POKEY "
             "restartuje casovace 3+4 start bitem; KAZETA: hra (boot) = START+OPTION a RETURN samo, stereo WAV = zvukova "
             "stopa do TV, CLOAD i s disketou v D1:; DISKETA: ATR/DISK = vysunout / vymenit bez restartu.");
      devLog("B299 BASIC/TBXL TXT: TXT soubor s programem napise appka sama do ATARI BASICu / Turbo-BASICu XL; "
             "klavesy z pristroje do logu (B299 KLAVESY); LOG/CHYBA = stav a cela pamet Atari v logu; "
             "SIO zkratka jen se zapnutou ROM OS (Turbo-BASIC XL ma na $E459 vlastni kod).");
      devLog("B300 POKEY: casovac s AUDF=$FF (perioda 256) tika a dava preruseni - drive stal "
             "(Ghostbusters: hudba a rec z preruseni casovace 1, bez nej titulka stala; nejhlubsi tony byly potichu).");
      devLog("B301 TV/PC (web prohlizec v appce): obraz Atari 130XE primo z jadra (vyrez 336x240 jako na telefonu) "
             "a zvuk 44100 Hz stereo - drive bylo na TV videt jen stranku s testy pod pristrojem.");
      devLog("B302 KLAVESNICE POCITACE (prohlizec na TV/PC) -> Atari 130XE primo v C++: psani podle rozlozeni "
             "(cesky: diakritika bez hacku, AltGr znaky), sipky = kurzor, F1/F3/F4 START/SELECT/OPTION, F2 BREAK, "
             "F6 HELP, F8 INVERZE, F9 HRANI (WASD/sipky joystick, K skok, L FIRE); klavesy v tempu, ktere OS stihne.");
      devLog("B303 TV/PC CELE V C++: kazdy snimek Atari (50/s) jako paleta + jen zmenene radky a zvuk v jednom proudu "
             "(/atari.stream) - Java uz nezvetsuje ani nekomprimuje (H.264/JPEG); Ctrl+V nebo pretazeny TXT v prohlizeci "
             "= program do BASICu bez NEW (UTF-8 s e s hackem uz neni ATASCII), jiny text = klavesy.");
      devLog("B348 DISKETA D1: i na urovni bajtu SIO jako skutecna 1050 (hry s vlastnim zavadecem - xBIOS, Prince of "
             "Persia: ramec prikazu pres POKEY a linku COMMAND, odpovedi A/C/N/E, cteni/zapis/STATUS/FORMAT, 19200 Bd); "
             "OPTION pri bootu z diskety jen 1 s; Acid800 V0.1 38/38 (LXA jako SALLY, asynchronni prijem POKEY "
             "drzi casovace 3+4, hraci ve VBLANK po hires radku, fantomove DMA hracu).");
      devLog("B349 ARCHIVY: hra v ZIP / RAR (XEX/MOBIL, ATR/DISK, NET HRY) se rozbali v C++ (deflate, RAR 2.9 vcetne "
             "pevneho a filtru); spusti se samostatna disketa > sada (strana A / Disk 1) > XEX > CAR, ostatni diskety "
             "z archivu nabidne ATR/DISK (i VYMENIT BEZ RESTARTU).");
      devLog("B350 RAR5 (novy WinRAR 5 / 7): hlavicky vint + CRC32, komprese v blocich, filtry DELTA / E8 / E8E9 / ARM, "
             "pevny archiv (i s prazdnym souborem uprostred), odkazy se preskoci.");
      devLog("B351 PPMd (RAR 4 \"nejlepsi\" = komprese textu): model PPMd var.H + rozsahovy dekoder RAR v C++, "
             "prechody PPMd <-> LZ, unikove prikazy, pevny archiv; heslo, vic dilu a RAR 1.5 / 2.0 zatim ne.");
      devLog("B352 PAMET 1 MB: vychozi 320K (130XE + Compy Shop); program, ktery do rozsirene pameti zapisuje pod vic nez "
             "16 ruznymi PORTB (hleda vic bank), se sam spusti znovu s 1088K (64 bank jako Ultimate 1MB) - Tony VBXE 496 KB.");
      devLog("B353 TURBO pri nahravani: disketa D1: (SIO pres POKEY i OS v posledni 1 s) nebo zavadec XEX -> az 8 snimku "
             "za periodu 20 ms (kolik telefon stihne), zvuk ztlumeny; ne, kdyz hrac drzi joystick / klavesu nebo 3 s po stisku.");
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
  onlineKonecProNovouHru("hrac odesel z Atari (appka na pozadi / jina obrazovka)");   // B363: bez vlakna emulace online nejde
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
  bool vibrace = false, odkryto = false;
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &d = devGet();
    vibrace = d.hapticReq; d.hapticReq = false;
    odkryto = d.ovladacOdkryto; d.ovladacOdkryto = false;     // B320
  }
  if (odkryto) devLog("B320 OVLADAC: dotyk na schovane tlacitko - dotykove ovladani zase videt");
  if (akce != 3 || n > 0 || odkryto) pokeRender();
  // B297: bit 0x1000 = Java kratce zavibruje; B320: bit 0x2000 = dotyk ukazal schovane ovladani
  return svc | (vibrace ? 0x1000 : 0) | (odkryto ? 0x2000 : 0);
}

/** B297: ovladani na sirku (D-pad a tlacitka jako u Segy).
 *  "get" -> nastaveni, "set:<nastaveni>" -> pouzit, "edit" / "mirror" / "reset" /
 *  "done" -> prikazy z menu D-PAD A OVLADANI, "stav" -> "land=0/1;edit=0/1".
 *  Vraci aktualni nastaveni (Java ho uklada do SharedPreferences). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(JNIEnv *env, jclass, jstring jcmd) {
  std::string cmd;
  if (jcmd) { const char *c = env->GetStringUTFChars(jcmd, nullptr); if (c) { cmd = c; env->ReleaseStringUTFChars(jcmd, c); } }
  // B349: archiv (ZIP / RAR) s hrou - jmeno, vybrany soubor, chyba a seznam pro ATR / DISK
  //   "archiv_seznam": radky "index<TAB>druh (A/X/C)<TAB>sada (-1 = samostatna)<TAB>aktualni (1/0)<TAB>jmeno"
  if (cmd == "archiv" || cmd == "archiv_vybrany" || cmd == "archiv_chyba" || cmd == "archiv_seznam") {
    std::string out;
    {
      std::lock_guard<std::mutex> a(g_mArchiv);
      if (cmd == "archiv") out = g_archiv.soubory.empty() ? std::string() : g_archiv.jmeno;
      else if (cmd == "archiv_vybrany") out = (g_archivAktualni >= 0 && g_archivAktualni < (int)g_archiv.soubory.size()) ? g_archiv.soubory[(size_t)g_archivAktualni].jmeno : std::string();
      else if (cmd == "archiv_chyba") out = g_archivChyba;
      else {
        for (size_t i = 0; i < g_archiv.soubory.size(); i++) {
          const nap::AtariSoubor &s = g_archiv.soubory[i];
          out += std::to_string(i) + "\t" + s.druh + "\t" + std::to_string(s.sada) + "\t" + ((int)i == g_archivAktualni ? "1" : "0") + "\t" + s.jmeno + "\n";
        }
      }
    }
    // jen ASCII (JNI chce platne UTF-8); tabulatory a konce radku zustanou
    for (char &c : out) { const unsigned char u = (unsigned char)c; if (!(u == '\t' || u == '\n' || (u >= 0x20 && u <= 0x7E))) c = '?'; }
    return env->NewStringUTF(out.c_str());
  }
  // B361: "pokey_stereo:1" / "pokey_stereo:0" - druhy POKEY ($D210, stereo upgrade) zapnuty / vypnuty (zrcadlo
  // prvniho jako na Atari bez upgradu). Plati hned (adresy) i pro kazdy dalsi studeny start. Vraci "1" / "0".
  if (cmd == "pokey_stereo:1" || cmd == "pokey_stereo:0" || cmd == "pokey_stereo") {
    if (cmd != "pokey_stereo") {
      const bool zap = cmd == "pokey_stereo:1";
      // B363: za online hry se POKEY 2 neprepina (zmenil by stroj jen na tomto telefonu) - plati od dalsiho zapnuti
      if (g_onlineAktivni.load()) {
        g_pokeyStereo.store(zap);
        devLog(std::string("B363 ATARI ONLINE: POKEY STEREO ") + (zap ? "zapnuto" : "vypnuto") + " - plati az po online hre (dalsi zapnuti)");
        return env->NewStringUTF(zap ? "1" : "0");
      }
      const bool zmena = g_pokeyStereo.exchange(zap) != zap;
      {
        std::lock_guard<std::mutex> l(g_mStroj);
        if (g_stroj) {
          g_stroj->stereoPokey = zap;
          if (!zap) { g_stroj->pk2.cold(); g_stroj->updateIrq(); }   // vypnuty druhy POKEY nehraje ani neprerusuje
        }
      }
      if (zmena) devLog(zap ? "B361 POKEY STEREO zapnuto: $D210-$D21F = druhy POKEY (levy kanal POKEY 1, pravy POKEY 2)"
                            : "B361 POKEY STEREO vypnuto: $D210-$D21F = zrcadlo prvniho POKEYe (jako Atari bez upgradu), mono");
    }
    return env->NewStringUTF(g_pokeyStereo.load() ? "1" : "0");
  }
  // B314: "jazyk:en" / "jazyk:cs" - hlasky a napisy appky na pristroji (OPTIONS -> JAZYK)
  if (cmd == "jazyk:en" || cmd == "jazyk:cs") {
    const bool en = cmd == "jazyk:en";
    const bool zmena = nap::atariJazykEn().exchange(en) != en;
    if (zmena) {
      devLog(std::string("B314 JAZYK pristroje: ") + (en ? "anglicky" : "cesky"));
      std::lock_guard<std::mutex> dl(g_mDev);
      devGet().jazykZmenen();      // znovu rozlozit (stitky D-padu na sirku) a prekreslit vse
      pokeRender();
    }
    return env->NewStringUTF(en ? "en" : "cs");
  }
  // B320: "ovladac:0/1/2" - herni ovladac (Retroid Pocket 6, gamepad): 0 dotykove ovladani
  // na sirku videt, 1 schovane (dotyk na tlacitko ho ukaze), 2 schovane vzdy. Vraci rezim.
  if (cmd.rfind("ovladac:", 0) == 0) {
    const int r = std::atoi(cmd.c_str() + 8);
    int bylo, je;
    {
      std::lock_guard<std::mutex> dl(g_mDev);
      nap::dev::Device &d = devGet();
      bylo = d.ovladacRezim;
      d.nastavOvladac(r);
      je = d.ovladacRezim;
    }
    if (je != bylo) {
      static const char *const R[3] = {"videt", "schovane (hraje herni ovladac, dotyk ho ukaze)", "schovane vzdy"};
      devLog(std::string("B320 OVLADAC: dotykove ovladani na sirku ") + R[je]);
      pokeRender();
    }
    return env->NewStringUTF(std::to_string(je).c_str());
  }
  // B310: "postup" = hotove davky napsanych radku programu pro postup hrace ("12,7",
  // "" = zadna) - vyzvedne je Java (NapPostup) a tim se smazou. Bez logu (vola se casto).
  if (cmd == "postup") {
    std::string out;
    {
      std::lock_guard<std::mutex> o(g_mOut);
      for (size_t i = 0; i < g_postupHotove.size(); i++) { if (i) out += ','; out += std::to_string(g_postupHotove[i]); }
      g_postupHotove.clear();
    }
    return env->NewStringUTF(out.c_str());
  }
  // B316: slot pro cartridge - "cart" = co je ve slotu ("" = prazdny),
  // "cart_eject" = vytahnout cartridge a zapnout znovu (BASIC)
  if (cmd == "cart" || cmd == "cart_eject") {
    if (cmd == "cart_eject") onlineKonecProNovouHru("cartridge vysunuta");   // B363
    std::string jm;
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      if (g_stroj && g_stroj->cart.vlozena()) jm = g_stroj->cart.jmeno;
      if (cmd == "cart_eject" && g_stroj) {
        g_xex.zrus();
        g_stroj->cart.vyjmout();                   // do logu jeden radek nize ("vysunuta")
        g_xexPosledni = false; g_pamet1088 = chce1088(programVeStrojiLocked());   // B352
        studenyStartLocked(7);
        g_stroj->sioRychlyTimeout = false;
        g_strojBezi = true;
        g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
        vyzvednoutVystupyLocked();
      }
    }
    if (cmd == "cart_eject") {
      devLog(jm.empty() ? std::string("B316 CARTRIDGE: slot byl prazdny - studeny start (BASIC)")
                        : "B316 CARTRIDGE " + jm + " vysunuta - studeny start (BASIC)");
      std::lock_guard<std::mutex> dl(g_mDev);
      nap::dev::Device &dv = devGet();
      if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
      dv.atariValid = false; dv.markScreen(); dv.markLegend();
      dv.setStatusMessage(nap::tx("CARTRIDGE VYSUNUTA", "CARTRIDGE REMOVED"), nap_ted_ms(), 4000);
      pokeRender();
      return env->NewStringUTF("");
    }
    return env->NewStringUTF(nap::pcAscii(jm, 120, true).c_str());
  }
  // B298: disketova mechanika D1: - "d1" = co je v mechanice ("" = prazdna),
  // "disk_eject" = vysunout a zapnout znovu (BASIC), "atr_vymena" = dalsi ATR
  // jen vymenit bez restartu (druha strana hry), "atr_normal" = zrusit
  if (cmd == "d1" || cmd == "disk_eject" || cmd == "atr_vymena" || cmd == "atr_normal") {
    if (cmd == "disk_eject") onlineKonecProNovouHru("disketa vysunuta");   // B363
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
        vyjmoutCartridgeLocked("vysunout disketu = zapnout s BASICem");
        g_xexPosledni = false; g_pamet1088 = false;   // B352: BASIC = 320K
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
      dv.setStatusMessage(nap::tx(u8"DISKETA VYSUNUTA - D1: PRÁZDNÁ", "DISK EJECTED - D1: EMPTY"), nap_ted_ms(), 4000);
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
 *  maska (Java devPad): bit0 nahoru, 1 dolu, 2 vlevo, 3 vpravo, 4 FIRE, 5 START, 6 SELECT, 7 OPTION.
 *  B355: herni ovladac navic bity 8-16 klavesa (KBCODE + 1) a 17 BREAK (nap_ovladac.h atariMapuj). */
// B320: herni ovladac (nap_ovladac.h) a klavesnice telefonu (Java devPad) maji kazdy svou masku - do stroje
// jde jejich soucet (jedno druhemu nic nepusti). B355: tlacitka ovladace podle mapy (TLACITKA OVLADACE v appce).
static std::mutex g_ovl_m;
static nap::ovladac::Stav g_ovl_stav;      // pod g_ovl_m
static uint32_t g_ovl_maska = 0, g_padJava = 0, g_padPoslano = 0xFFFFFFFFu;   // pod g_ovl_m
static int g_ovl_udalosti = 0;             // pod g_ovl_m
// B354: druhy herni ovladac (hrac 2) = joystick 2; udalost PAD s PAD_HRAC2 (pod g_ovl_m)
static nap::ovladac::Stav g_ovl_stav2;
static uint32_t g_ovl_maska2 = 0, g_pad2Poslano = 0xFFFFFFFFu;
// B355: mapa tlacitek; radek naposled stisknuteho tlacitka s klavesou (drzi-li hrac vic klaves, plati posledni)
// (pod g_ovl_m). Prvni stisk kazdeho tlacitka v kazdem emulatoru loguje Java (NapOvladac.klavesa, B329/B355).
static nap::ovladac::Mapa g_ovl_mapa = nap::ovladac::vychoziMapa(nap::ovladac::J_ATARI);
static int g_ovl_posledni = -1;
static bool g_ovl_mapaVLogu = false;
static void padPosliLocked(bool vzdy) {
  const uint32_t m = (g_ovl_maska & (0xFFu | nap::ovladac::AP_KLAVESA_MASKA | nap::ovladac::AP_BREAK)) | (g_padJava & 0xFFu);
  if (!vzdy && m == g_padPoslano) return;
  g_padPoslano = m;
  nap::dev::Ev e; e.t = nap::dev::Ev::PAD; e.v = (int)m;
  std::lock_guard<std::mutex> l(g_mEv); g_evQ.push_back(e);
}
static void pad2PosliLocked() {
  const uint32_t m = g_ovl_maska2 & 0xFF;            // smer, FIRE, START / SELECT / OPTION (klavesy a BREAK jen hrac 1)
  if (m == g_pad2Poslano) return;
  g_pad2Poslano = m;
  nap::dev::Ev e; e.t = nap::dev::Ev::PAD; e.v = (int)m | PAD_HRAC2;
  std::lock_guard<std::mutex> l(g_mEv); g_evQ.push_back(e);
}
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(JNIEnv *, jclass, jint maska) {
  std::lock_guard<std::mutex> o(g_ovl_m);
  g_padJava = (uint32_t)maska & 0xFF;
  padPosliLocked(true);
}

/** B320: klavesa z herniho ovladace (KeyEvent.KEYCODE_*) -> joystick, konzole, klavesy podle mapy (B355).
 *  Vraci logicka tlacitka, 0 = neni klavesa ovladace. B354: hrac 0 / 1 (= joystick 2). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKlavesaHracNative(JNIEnv *, jclass, jint hrac, jint keyCode, jboolean dolu) {
  uint32_t t, maska = 0;
  int n = 99;
  {
    std::lock_guard<std::mutex> o(g_ovl_m);
    const int h = hrac == 1 ? 1 : 0;
    t = nap::ovladac::klavesa(h ? g_ovl_stav2 : g_ovl_stav, keyCode, dolu == JNI_TRUE);
    if (!t) return 0;
    if (h) {                                         // B354: hrac 2 = joystick 2
      g_ovl_maska2 = nap::ovladac::atariMapuj(g_ovl_stav2.vse(), g_ovl_mapa);
      pad2PosliLocked();
    } else {
      const int r = nap::ovladac::radekTlacitka(t);
      if (dolu && r >= 0 && g_ovl_mapa.cil[r] >= nap::ovladac::A_KLAVESA && g_ovl_mapa.cil[r] != nap::ovladac::CIL_MENU) g_ovl_posledni = r;
      g_ovl_maska = nap::ovladac::atariMapuj(g_ovl_stav.vse(), g_ovl_mapa, g_ovl_posledni);
      padPosliLocked(false);
      n = g_ovl_udalosti++;
      maska = nap::ovladac::atariMapuj(t, g_ovl_mapa);
    }
  }
  if (n < 10) {
    char b[200];
    std::snprintf(b, sizeof(b), "B320 OVLADAC: klavesa %d %s -> %s -> Atari maska 0x%05x", (int)keyCode, dolu ? "dolu" : "nahoru",
                  nap::ovladac::jmena(t).c_str(), (unsigned)maska);
    devLog(b);
  }
  return (jint)t;
}
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKlavesaNative(JNIEnv *env, jclass c, jint keyCode, jboolean dolu) {
  return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKlavesaHracNative(env, c, 0, keyCode, dolu);
}
/** B320: osy herniho ovladace (leva packa, krizek HAT = smer; spouste L2 / R2 podle mapy). B354: hrac 0 / 1. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacOsyHracNative(JNIEnv *env, jclass, jint hrac, jfloatArray josy, jint maska) {
  float v[nap::ovladac::O_POCET] = {0};
  if (josy) {
    const jsize n = env->GetArrayLength(josy);
    env->GetFloatArrayRegion(josy, 0, n < nap::ovladac::O_POCET ? n : nap::ovladac::O_POCET, v);
  }
  std::lock_guard<std::mutex> o(g_ovl_m);
  const int h = hrac == 1 ? 1 : 0;
  nap::ovladac::Stav &st = h ? g_ovl_stav2 : g_ovl_stav;
  const uint32_t spoustePred = st.spouste;
  nap::ovladac::osy(st, v, maska);
  if (h) {
    g_ovl_maska2 = nap::ovladac::atariMapuj(g_ovl_stav2.vse(), g_ovl_mapa);
    pad2PosliLocked();
    return;
  }
  const uint32_t noveSpouste = st.spouste & ~spoustePred;                    // spoust zmacknuta prave ted (L2 / R2)
  for (int r : {nap::ovladac::R_L2, nap::ovladac::R_R2})
    if ((noveSpouste & nap::ovladac::R_BIT[r]) && g_ovl_mapa.cil[r] >= nap::ovladac::A_KLAVESA && g_ovl_mapa.cil[r] != nap::ovladac::CIL_MENU)
      g_ovl_posledni = r;
  g_ovl_maska = nap::ovladac::atariMapuj(g_ovl_stav.vse(), g_ovl_mapa, g_ovl_posledni);
  padPosliLocked(false);
}
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacOsyNative(JNIEnv *env, jclass c, jfloatArray josy, jint maska) {
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacOsyHracNative(env, c, 0, josy, maska);
}
/** B320: vse z ovladace pustit (ovladac odpojen, odchod z Atari). B354: oba hraci. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacPustNative(JNIEnv *, jclass) {
  std::lock_guard<std::mutex> o(g_ovl_m);
  nap::ovladac::pustVse(g_ovl_stav);
  g_ovl_maska = 0;
  g_ovl_posledni = -1;
  padPosliLocked(false);
  nap::ovladac::pustVse(g_ovl_stav2);
  g_ovl_maska2 = 0;
  pad2PosliLocked();
}
/** B355: mapa tlacitek z Javy (NapOvladacMapa, "A=FIRE;B=FIRE;X=K33;..."; "" = vychozi). Vraci mapu tak,
 *  jak ji jadro pouziva (nezname dvojice zustanou vychozi). Log jen pri zmene. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacMapaNative(JNIEnv *env, jclass, jstring jmapa) {
  std::string s;
  if (jmapa) {
    const char *c = env->GetStringUTFChars(jmapa, nullptr);
    if (c) { s.assign(c, strnlen(c, 4096)); env->ReleaseStringUTFChars(jmapa, c); }
  }
  int nezname = 0;
  bool zmena;
  std::string norm, popis;
  {
    std::lock_guard<std::mutex> o(g_ovl_m);
    const nap::ovladac::Mapa m = nap::ovladac::mapaZRetezce(nap::ovladac::J_ATARI, s, &nezname);
    zmena = !g_ovl_mapaVLogu || !nap::ovladac::stejnaMapa(m, g_ovl_mapa);
    g_ovl_mapa = m;
    g_ovl_mapaVLogu = true;
    g_ovl_posledni = -1;
    g_ovl_maska = nap::ovladac::atariMapuj(g_ovl_stav.vse(), g_ovl_mapa);
    g_ovl_maska2 = nap::ovladac::atariMapuj(g_ovl_stav2.vse(), g_ovl_mapa);
    padPosliLocked(false);
    pad2PosliLocked();
    norm = nap::ovladac::mapaNaRetezec(m);
    popis = nap::ovladac::mapaPopis(m);
  }
  if (zmena) devLog("B355 OVLADAC TLACITKA Atari: " + popis + (nezname ? " | neznamych dvojic " + std::to_string(nezname) + " (zustaly vychozi)" : ""));
  return env->NewStringUTF(norm.c_str());
}
/** B355: katalog pro dialog TLACITKA OVLADACE - radky, cile a vychozi mapa pro jadro 0 Atari / 1 Sega / 2 PS1
 *  (nap_ovladac.h je jedine misto - Java nic neopisuje; knihovna Atari je nactena vzdy). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKatalogNative(JNIEnv *env, jclass, jint jadro, jboolean en) {
  return env->NewStringUTF(nap::ovladac::katalog((int)jadro, en == JNI_TRUE).c_str());
}

/** B302: klavesa z prohlizece na TV/PC (web viewer appky). code = e.code (misto
 *  klavesy), znak = e.key (co napsala), mod = 1 Shift, 2 Ctrl, 4 Alt, 8 AltGr,
 *  16 opakovani; dolu = stisk/pusteni. code "STAV" = jen vratit rezim.
 *  Vse (mapovani, rezim PSANI/HRANI, drzene klavesy) resi C++; vraci text pro
 *  prohlizec ("PSANI"/"HRANI", "SKOK", "VYSTREL", "OK:...", "NEZNAMA:..."). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPcKlavesaNative(JNIEnv *env, jclass, jstring jcode, jstring jznak, jint mod, jboolean dolu) {
  std::string code, znak;
  if (jcode) { const char *c = env->GetStringUTFChars(jcode, nullptr); if (c) { code.assign(c, strnlen(c, 64)); env->ReleaseStringUTFChars(jcode, c); } }
  if (jznak) { const char *c = env->GetStringUTFChars(jznak, nullptr); if (c) { znak.assign(c, strnlen(c, 16)); env->ReleaseStringUTFChars(jznak, c); } }
  std::string r, log1, log2;
  {
    std::lock_guard<std::mutex> p(g_mPc);
    if (code != "STAV") g_pcMapa.zprava(nap_ted_ms());   // hlidani spojeni (i opakovani a "Zije")
    if (code == "STAV") {
      r = g_pcMapa.hrani ? "HRANI" : "PSANI";
    } else if (dolu && !g_run.load()) {
      // emulace stoji (appka na pozadi) - stisk zahodit, at se po navratu nenapise
      // naraz vsechno, co se mezitim mackalo (pusteni projde: joystick nesmi zustat drzeny)
      r = "NEBEZI";
    } else {
      std::vector<nap::PcAkce> a;
      r = g_pcMapa.udalost(code, znak, (int)mod, dolu != 0, a);
      if (g_pcVstup.size() + a.size() > 512) {
        // emulace nebezi (napr. appka na pozadi) - nehromadit
        g_pcVstup.clear();
        log2 = "B302 PC KLAVESNICE: fronta plna (emulace nebezi?) - zahozeno";
      }
      g_pcVstup.insert(g_pcVstup.end(), a.begin(), a.end());
      if (!g_pcPrvni && dolu) {
        g_pcPrvni = true;
        log1 = "B302 PC KLAVESNICE: prvni klavesa z prohlizece na TV/PC (" + nap::pcAscii(code) + " -> " + r +
               ") - jde primo do Atari 130XE (C++), rezim " + (g_pcMapa.hrani ? "HRANI" : "PSANI");
      }
      if (r == "HRANI" || r == "PSANI")
        log2 = "B302 PC KLAVESNICE: rezim " + r + (r == "HRANI" ? " (WASD/sipky = joystick, K/mezernik = skok, L = FIRE, F9 = psani)"
                                                                : " (klavesy pisou do Atari, F9 = hrani)");
      else if (r.rfind("NEZNAMA", 0) == 0 && g_pcNeznamych < 20) { g_pcNeznamych++; log2 = "B302 PC KLAVESNICE: " + r + " - tahle klavesa na Atari nic nedela"; }
    }
  }
  if (!log1.empty()) devLog(log1);
  if (!log2.empty()) devLog(log2);
  return env->NewStringUTF(nap::pcAscii(r, 64).c_str());
}

/** Napsat text do Atari (tlacitko BASIC/TBXL TXT). runPotom: na konec RUN. */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(JNIEnv *env, jclass, jstring text, jboolean runPotom) {
  if (!text) return 0;
  const char *c = env->GetStringUTFChars(text, nullptr);
  std::string t = c ? c : "";
  if (c) env->ReleaseStringUTFChars(text, c);
  if (runPotom) { if (!t.empty() && t.back() != '\n') t.push_back('\n'); t += "RUN"; }
  if (g_onlineAktivni.load()) {   // B363: psani textu by stroj zmenilo jen na tomto telefonu
    devLog("B363 ATARI ONLINE: psani textu za online hry nejde (" + std::to_string(t.size()) + " znaku nenapsano)");
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage(nap::tx(u8"ZA ONLINE HRY SE TEXT NEPÍŠE", "NO TYPING DURING AN ONLINE GAME"), nap_ted_ms(), 4000);
    return 0;
  }
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
  onlineKonecProNovouHru("appka pise program z TXT");   // B363
  std::string vysl;
  bool odpojena = false;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) zaloz();
    if (rezim == 0) {
      g_xex.zrus();
      if (g_stroj->disk.mounted) { odpojena = true; g_stroj->disk = nap::AtrDisk(); }
      vyjmoutCartridgeLocked("TXT program do ATARI BASICu");
      g_xexPosledni = false; g_pamet1088 = false;  // B352: BASIC = 320K
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

// B316: cartridge do LOG/CHYBA - typ, stav banky, okna, TRIG3, kolikrat se prepnulo
static std::string cartPopis(const Machine &m) {
  if (!m.cart.vlozena()) return "ZADNA (slot prazdny, TRIG3=0)";
  char b[300];
  snprintf(b, sizeof(b), "%s typ %d (%s) stav $%X, okna %s%s, TRIG3=%d, prepnuti %lld", m.cart.jmeno.c_str(), m.cart.typ,
           nap::cartJmeno(m.cart.typ), (unsigned)m.cart.stav, m.cart.rd4() ? "$8000-$9FFF " : "", m.cart.rd5() ? "$A000-$BFFF" : (m.cart.rd4() ? "" : "VYPNUTA"),
           m.cart.rd5() ? 1 : 0, m.cart.zmen);
  return b;
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
             "rozsirena pamet pro CPU %s banka %d%s), NMIEN=$%02X, DLIST=$%04X, SAVMSC=$%04X, DDEVIC=$%02X, D1: %s, rychly timeout SIO %s, "
             "XEX stav %d, psani TXT stav %d, snimek %lld, cartridge %s",
             m.cpu.pc, m.cpu.a, m.cpu.x, m.cpu.y, m.cpu.s, m.cpu.p, m.cpu.jam ? " JAM" : "", pb, (pb & 1) ? "zap" : "VYP",
             (pb & 2) ? "vyp" : "ZAP", (pb & 0x10) ? "vyp" : "ZAP", m.mem.bankaCpu(pb), m.mem.rezim1088 ? " z 64 (1088K)" : " z 16 (320K)", m.nmien, w(0x230), w(0x58), r[0x300],
             m.disk.mounted ? m.disk.name.c_str() : "prazdna", m.sioRychlyTimeout ? "ANO" : "ne", (int)g_xex.stav, (int)g_psani.stav,
             (long long)m.frame, cartPopis(m).c_str());
    out += b; out += "\n";
    out += nap::napDiagPameti(m);
  }
  return env->NewStringUTF(out.c_str());
}

/** B299: cela pamet Atari (64 kB + rozsirenych 64 kB + registry) pro LOG/CHYBA -
 *  Java ji prilozi k logu. Hlavicka 16 B "NAP130XE-B299", 64 B registru.
 *  B323: rozsirena pamet ma 320K (16 bank) - do vypisu jdou dal jen 4 banky 130XE (ext[0..64 kB]). */
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

/** B301: obraz Atari pro TV / PC (web prohlizec v appce). Pristroj Atari kresli
 *  C++ primo na displej (SurfaceView) - snimani okna appky ho nevidi a na TV
 *  byla jen stranka s testy pod nim. Tohle da obraz primo z jadra: vyrez jako
 *  na displeji telefonu (sloupce 48..719 ze 768, vsech 240 radku), ARGB pro
 *  Bitmap. Vraci (sirka << 16) | vyska, zaporne = pole je male, 0 = neni snimek. */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devGrabFrameNative(JNIEnv *env, jclass, jintArray out) {
  const int X0 = 48, W = 672, H = AnticView::H, FW = AnticView::FW;
  const jint wh = (jint)((W << 16) | H);
  if (!out) return 0;
  if (env->GetArrayLength(out) < W * H) return -wh;
  std::vector<jint> buf((size_t)W * H);
  {
    std::lock_guard<std::mutex> f(g_mFrame);
    if (g_sdilenySnimek.size() != (size_t)FW * H) return 0;
    const bool zap = g_strojBezi.load();             // POWER vypnuto = cerna obrazovka
    for (int y = 0; y < H; y++) {
      const uint32_t *src = &g_sdilenySnimek[(size_t)y * FW + X0];
      jint *dst = &buf[(size_t)y * W];
      for (int x = 0; x < W; x++) {
        const uint32_t c = zap ? src[x] : 0xFF000000u;   // snimek je ABGR (R v nizkem bajtu)
        dst[x] = (jint)(0xFF000000u | ((c & 0xFFu) << 16) | (c & 0xFF00u) | ((c >> 16) & 0xFFu));
      }
    }
  }
  env->SetIntArrayRegion(out, 0, W * H, buf.data());
  return wh;
}

/** B301: zvuk Atari pro TV / PC - vyzvedne, co se od minula prehralo (stereo
 *  int16, 44100 Hz). Vraci pocet shortu (sude). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPullTvAudioNative(JNIEnv *env, jclass, jshortArray out) {
  if (!out) return 0;
  const size_t kap = (size_t)env->GetArrayLength(out) & ~(size_t)1;
  std::vector<int16_t> tmp;
  {
    std::lock_guard<std::mutex> l(g_mTvZvuk);
    size_t k = (size_t)std::min<unsigned long long>(kap, g_tvZvukZapis - g_tvZvukCteni) & ~(size_t)1;
    tmp.resize(k);
    for (size_t i = 0; i < k; i++) tmp[i] = g_tvZvuk[(size_t)((g_tvZvukCteni + i) % TV_ZVUK_KAP)];
    g_tvZvukCteni += k;
  }
  if (!tmp.empty()) env->SetShortArrayRegion(out, 0, (jsize)tmp.size(), (const jshort *)tmp.data());
  return (jint)tmp.size();
}

// ===================================================================
//  B303: PRENOS NA TV / PC CELY V C++ (male zpozdeni)
//  Rene: "ma to na TV zpozdeni ... Opravdu prevod jede jak ma v C++?"
//  Java jen otevre spojeni a posila, co jsme tady zabalili: kazdy snimek
//  Atari (50/s) jako paleta + zmenene radky + zvuk (nap_atari_tv_proud.h).
// ===================================================================
/** Novy prohlizec. Vraci cislo klienta (> 0). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudOtevriNative(JNIEnv *, jclass) {
  int id, celkem;
  {
    std::lock_guard<std::mutex> l(g_mTvProud);
    id = g_tvDalsiId++;
    auto kn = std::make_unique<TvKlientNat>();
    kn->id = id; kn->startMs = kn->logMs = nap_ted_ms();
    g_tvKlienti[id] = std::move(kn);
    celkem = (int)g_tvKlienti.size();
    g_tvKlientu = celkem;
  }
  char b[200];
  snprintf(b, sizeof(b), "B303 TV PROUD: prohlizec %d pripojen (celkem %d) - obraz a zvuk Atari jdou primo z C++, 50 snimku/s, jen zmeny", id, celkem);
  devLog(b);
  return id;
}

/** Ceka (nejvys cekejMs) na novy snimek a zabali ho pro prohlizec do out:
 *  4 bajty delka (LE) + paket. Vraci pocet bajtu, 0 = nic noveho, zaporne =
 *  out je male (potreba -n bajtu; pristi paket bude klicovy), INT32_MIN =
 *  neznamy klient (Java spojeni zavre - smycka se nesmi tocit naprazdno). */
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudDalsiNative(JNIEnv *env, jclass, jint id, jbyteArray out, jint cekejMs) {
  if (!out) return INT32_MIN;
  TvKlientNat *kn = nullptr;
  {
    std::unique_lock<std::mutex> l(g_mTvProud);
    auto it = g_tvKlienti.find((int)id);
    if (it == g_tvKlienti.end()) return INT32_MIN;
    kn = it->second.get();
    nap::TvProudKlient &k0 = kn->k;
    auto jeNovy = [&]() { return k0.obraz.empty() || g_snimekSeq.load() != k0.snimek || g_strojBezi.load() != k0.zapnuto; };
    if (!jeNovy()) g_cvTvProud.wait_for(l, std::chrono::milliseconds(std::max(1, std::min(1000, (int)cekejMs))), jeNovy);
    if (!jeNovy()) return 0;
  }
  // klienta meni jen tohle vlakno (prohlizec ma svoje); vymaze ho az devTvProudZavriNative z nej
  nap::TvProudKlient &k = kn->k;
  const int X0 = 48, W = 672, H = AnticView::H, FW = AnticView::FW;
  const bool zap = g_strojBezi.load();
  unsigned long long seq;
  kn->rgb.resize((size_t)W * H);
  {
    std::lock_guard<std::mutex> f(g_mFrame);
    seq = g_snimekSeq.load();
    if (!zap || g_sdilenySnimek.size() != (size_t)FW * H) std::fill(kn->rgb.begin(), kn->rgb.end(), 0u);   // POWER vypnuto = cerna
    else
      for (int y = 0; y < H; y++) {
        const uint32_t *src = &g_sdilenySnimek[(size_t)y * FW + X0];
        uint32_t *dst = &kn->rgb[(size_t)y * W];
        for (int x = 0; x < W; x++) { const uint32_t c = src[x]; dst[x] = ((c & 0xFFu) << 16) | (c & 0xFF00u) | ((c >> 16) & 0xFFu); }   // ABGR -> RGB
      }
  }
  // zvuk od minula (nejvys 0,25 s - kdyz prohlizec zaostal, stare se zahodi, zpozdeni neroste)
  kn->zvuk.clear();
  {
    std::lock_guard<std::mutex> z(g_mTvZvuk);
    const unsigned long long konec = g_tvZvukZapis;
    const unsigned long long MAXZ = 44100ULL * 2 / 4;
    if (!k.zvukJede || konec < k.zvukPoz) { k.zvukPoz = konec; k.zvukJede = true; }
    else if (konec - k.zvukPoz > MAXZ) k.zvukPoz = konec - 882 * 2;
    if (g_tvZvuk.size() == TV_ZVUK_KAP) {
      const size_t kolik = (size_t)((konec - k.zvukPoz) & ~1ULL);
      kn->zvuk.resize(kolik);
      for (size_t i = 0; i < kolik; i++) kn->zvuk[i] = g_tvZvuk[(size_t)((k.zvukPoz + i) % TV_ZVUK_KAP)];
      k.zvukPoz += kolik;
    }
  }
  nap::tvProudPaket(k, kn->rgb.data(), W, H, seq, zap, kn->zvuk.data(), kn->zvuk.size(), kn->pk);
  const size_t delka = kn->pk.size() + 4;
  if ((jsize)delka > env->GetArrayLength(out)) { k.zapomen(); return -(jint)delka; }
  const uint32_t n32 = (uint32_t)kn->pk.size();
  const uint8_t hl[4] = {(uint8_t)n32, (uint8_t)(n32 >> 8), (uint8_t)(n32 >> 16), (uint8_t)(n32 >> 24)};
  env->SetByteArrayRegion(out, 0, 4, (const jbyte *)hl);
  env->SetByteArrayRegion(out, 4, (jsize)kn->pk.size(), (const jbyte *)kn->pk.data());
  kn->paketu++; kn->bajtu += (long long)delka; kn->logPaketu++; kn->logBajtu += (long long)delka;
  const long long ted = nap_ted_ms();
  if (ted - kn->logMs >= 30000) {
    const double s = (ted - kn->logMs) / 1000.0;
    char b[260];
    snprintf(b, sizeof(b), "B303 TV PROUD: prohlizec %d - %.1f snimku/s, %.2f Mbit/s (obraz + zvuk), prumer %.1f kB na snimek, klicovych celkem %lld",
             kn->id, kn->logPaketu / s, kn->logBajtu * 8.0 / 1e6 / s, kn->logBajtu / 1024.0 / std::max(1LL, kn->logPaketu), k.klicovych);
    devLog(b);
    kn->logMs = ted; kn->logPaketu = kn->logBajtu = 0;
  }
  return (jint)delka;
}

/** Prohlizec odpojen. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudZavriNative(JNIEnv *, jclass, jint id) {
  std::unique_ptr<TvKlientNat> kn;
  int celkem;
  {
    std::lock_guard<std::mutex> l(g_mTvProud);
    auto it = g_tvKlienti.find((int)id);
    if (it == g_tvKlienti.end()) return;
    kn = std::move(it->second);
    g_tvKlienti.erase(it);
    celkem = (int)g_tvKlienti.size();
    g_tvKlientu = celkem;
  }
  const double s = std::max(0.001, (nap_ted_ms() - kn->startMs) / 1000.0);
  char b[260];
  snprintf(b, sizeof(b), "B303 TV PROUD: prohlizec %d odpojen po %.0f s - %lld snimku (%.1f/s), %.1f MB celkem, klicovych %lld (zbyva %d)",
           kn->id, s, kn->paketu, kn->paketu / s, kn->bajtu / 1048576.0, kn->k.klicovych, celkem);
  devLog(b);
}

/** B303: text vlozeny z PC (Ctrl+V v prohlizeci, pretazeny TXT). Program (radky
 *  s cislem) napise appka do BASICu / TBXL bez NEW, jiny text napise jako klavesy.
 *  Vraci zpravu pro prohlizec. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devVlozNative(JNIEnv *env, jclass, jbyteArray data) {
  if (!data) return env->NewStringUTF("VLOZENI: PRAZDNE");
  const jsize n = env->GetArrayLength(data);
  if (n <= 0) return env->NewStringUTF("VLOZENI: PRAZDNE");
  std::vector<uint8_t> buf((size_t)n);
  env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  std::string vysl;
  if (!nap::pcJeText(buf.data(), buf.size())) {
    vysl = "VLOZENI: TO NENI TEXT (XEX / ATR / BAS NAHRAJ V APPCE)";
    devLog("B303 VLOZENI z PC: " + std::to_string(n) + " B neni text - nic se nepise");
  } else if (g_onlineAktivni.load()) {
    vysl = "VLOZENI: ZA ONLINE HRY NEJDE";             // B363: text by stroj zmenil jen na jednom telefonu
    devLog("B303 VLOZENI z PC: " + std::to_string(n) + " B - za online hry se nepise (B363)");
  } else if (!g_strojBezi.load()) {
    vysl = "VLOZENI: ATARI JE VYPNUTE";                // text by se jinak napsal az po zapnuti
    devLog("B303 VLOZENI z PC: " + std::to_string(n) + " B - Atari je vypnute, nic se nepise");
  } else if (nap::pcJeProgram(buf.data(), buf.size())) {
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      if (!g_stroj || !g_strojBezi.load()) vysl = "VLOZENI: ATARI JE VYPNUTE";
      else if (g_psani.aktivni()) vysl = "VLOZENI: APPKA PRAVE PISE JINY PROGRAM - POCKEJ";
      else {
        const bool tbxl = !(g_stroj->mem.portB() & 1);    // TBXL bezi s vypnutou ROM OS
        g_psani.start(buf.data(), buf.size(), tbxl, "vlozeno z PC", false, false);
        // pise se, az editor ceka na novy radek (READY / po radku programu); bezi-li program
        // nebo hra, ceka nejvys 45 s
        const bool ready = nap::PsaniProgramu::ready(*g_stroj) || nap::PsaniProgramu::cekaNaRadek(*g_stroj);
        char b[160];
        snprintf(b, sizeof(b), "VLOZENO: %d RADKU PROGRAMU (BEZ NEW) - %s", g_psani.radku, ready ? "PISU" : "PISU AZ BUDE READY (NEJVYS 45 S)");
        vysl = b;
        if (g_psani.bezCisla) vysl += " (BEZ CISLA PRESKOCENO " + std::to_string(g_psani.bezCisla) + ")";
        vyzvednoutVystupyLocked();
      }
    }
    devLog("B303 VLOZENI z PC: program " + std::to_string(n) + " B -> " + vysl);
  } else {
    std::vector<nap::PcAkce> a;
    const int nelze = nap::pcTextNaKlavesy(buf.data(), buf.size(), a, 4000);
    for (auto &x : a) x.vlozeno = true;               // B310: vlozeny text se do postupu hrace nepocita
    {
      std::lock_guard<std::mutex> p(g_mPc);
      if (g_pcText.size() + a.size() > 20000) g_pcText.clear();
      g_pcText.insert(g_pcText.end(), a.begin(), a.end());
    }
    vysl = "NAPSANO: " + std::to_string(a.size() / 2) + " ZNAKU" + (nelze ? " (" + std::to_string(nelze) + " NA ATARI NENI)" : "");
    devLog("B303 VLOZENI z PC: text " + std::to_string(n) + " B -> " + vysl);
  }
  return env->NewStringUTF(nap::pcAscii(vysl, 120, true).c_str());
}

/** Spustit XEX (XEX/MOBIL, TURBO/BASIC, NET HRY). */
extern "C" JNIEXPORT jboolean JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno) {
  if (!data) return JNI_FALSE;
  onlineKonecProNovouHru("spustena jina hra (XEX)");   // B363: nova hra uprostred online = konec online
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
      vyjmoutCartridgeLocked("spousti se XEX");
      g_xexData = buf; g_xexJmeno = nm; g_xexPosledni = true; g_pamet1088 = chce1088(nm);   // B352
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
    d.setStatusMessage((nap::tx("SPOUSTIM ", "STARTING ") + nm).c_str(), nap_ted_ms(), 4000);
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
  onlineKonecProNovouHru("vlozena kazeta");   // B363
  nap::TapeImage img; std::string err;
  const long long t0 = nap_ted_ms();
  const bool ok = nap::napTapeFromWav(buf.data(), buf.size(), img, err);
  const long long ms = nap_ted_ms() - t0;
  char b[400];
  if (!ok) {
    snprintf(b, sizeof(b), "CHYBA %s: %s", nm.c_str(), err.c_str());
    devLog(std::string("B292 KAZETA ") + b);
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage((nap::tx("KAZETA NEJDE NACIST: ", "CANNOT LOAD THE TAPE: ") + nap::chybaKazety(err)).c_str(), nap_ted_ms(), 7000);
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
      vyjmoutCartridgeLocked("bootuje kazeta");
      if (g_stroj->disk.mounted) { odpojenaDisketa = true; g_stroj->disk = nap::AtrDisk(); }
      g_xexPosledni = false; g_pamet1088 = chce1088(nm);   // B352
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
    stav = nap::tx("HRA Z KAZETY - NAHRAJE SE SAMA", "GAME FROM TAPE - IT LOADS BY ITSELF");
  } else if (druh == nap::TapeImage::TEXT) {
    devLog("B298 KAZETA s vypisem BASICu (text) - napis ENTER \"C:\", RETURN, po pipnuti RETURN");
    stav = nap::tx(u8"NAPIŠ ENTER \"C:\" A 2x RETURN", "TYPE ENTER \"C:\" AND RETURN 2x");
  } else {
    devLog("B295 KAZETA PLAY zmacknuto automaticky - ted napis CLOAD, RETURN, a po pipnuti jeste jednou RETURN");
    stav = druh == nap::TapeImage::BASIC ? nap::tx(u8"NAPIŠ CLOAD + RETURN, PO PÍPNUTÍ RETURN", "TYPE CLOAD + RETURN, AFTER THE BEEP RETURN")
                                         : nap::tx(u8"NAPIŠ CLOAD, HRA: START+OPTION+POWER", "TYPE CLOAD; GAME: START+OPTION+POWER");
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

// ---------------------------------------------------------------------
//  B349: hra zabalena v ZIP / RAR (Rene: "pokud to bude zabalene v rar v zipu
//  tak at si s tim atari poradi"). Rozbaleni i vyber v C++ (nap_archiv.h,
//  nap_atari_archiv.h): co spustit (samostatna disketa > sada disket A/B >
//  XEX > CAR) a ktere diskety z archivu nabidnout v ATR / DISK k vymene.
// ---------------------------------------------------------------------
// (stav archivu: g_mArchiv, g_archiv, g_archivAktualni, g_archivChyba - nahore u g_mOut)

// kratka hlaska pro pristroj podle chyby archivu
static std::string hlaskaArchivu(const std::string &ch) {
  // B350: RAR5 uz umi - zbyva novejsi verze komprese nez RAR 7
  if (ch.find("neznama verze komprese") != std::string::npos) return nap::tx(u8"TENHLE RAR JE MOC NOVÝ - ZABAL JAKO ZIP", "THIS RAR IS TOO NEW - USE ZIP");
  // B351: PPMd (komprese textu RAR 4) uz umi - zbyva malo pameti pro jeho model (az 256 MB)
  if (ch.find("malo pameti") != std::string::npos) return nap::tx(u8"MÁLO PAMĚTI NA ROZBALENÍ - ZABAL JAKO ZIP", "NOT ENOUGH MEMORY TO UNPACK - USE ZIP");
  if (ch.find("stary format") != std::string::npos) return nap::tx(u8"STARÝ RAR (1.5 / 2.0) NEUMÍM - ZABAL JAKO ZIP", "OLD RAR (1.5 / 2.0) NOT SUPPORTED - USE ZIP");
  if (ch.find("heslo") != std::string::npos || ch.find("zaheslovan") != std::string::npos) return nap::tx(u8"ARCHIV MÁ HESLO", "THE ARCHIVE HAS A PASSWORD");
  if (ch.find("vice dilu") != std::string::npos || ch.find("dalsim dilu") != std::string::npos) return nap::tx(u8"ARCHIV NA VÍCE DÍLŮ NEUMÍM", "MULTI-PART ARCHIVES NOT SUPPORTED");
  if (ch.find("neni hra pro Atari") != std::string::npos) return nap::tx(u8"V ARCHIVU NENÍ HRA PRO ATARI", "NO ATARI GAME IN THE ARCHIVE");
  return nap::tx(u8"ARCHIV JE POŠKOZENÝ NEBO NEZNÁMÝ", "THE ARCHIVE IS DAMAGED OR UNKNOWN");
}

static jbyteArray archivPole(JNIEnv *env, const std::vector<uint8_t> &d) {
  jbyteArray a = env->NewByteArray((jsize)d.size());
  if (a && !d.empty()) env->SetByteArrayRegion(a, 0, (jsize)d.size(), (const jbyte *)d.data());
  return a;
}

/** B349: data z XEX/MOBIL, ATR/DISK nebo NET HRY. Neni to archiv -> null (Java pokracuje s daty,
 *  jak jsou; mimo vymenu diskety se zapomene predchozi archiv). Archiv -> obsah souboru, ktery
 *  se ma spustit (jmeno: devCtl "archiv_vybrany"); archiv bez hry -> prazdne pole (duvod:
 *  devCtl "archiv_chyba", hlaska je uz na pristroji). */
extern "C" JNIEXPORT jbyteArray JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devArchivNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno, jboolean vymena) {
  if (!data) return nullptr;
  std::string nm = "archiv";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  for (auto &ch : nm) if ((unsigned char)ch >= 0x80) ch = '?';
  jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  onlineKonecProNovouHru("spustena jina hra (archiv)");   // B363
  if (!nap::AtariArchiv::jeArchiv(buf.data(), buf.size())) {
    if (!vymena) {
      std::lock_guard<std::mutex> a(g_mArchiv);
      if (!g_archiv.soubory.empty()) devLog("B349 ARCHIV " + g_archiv.jmeno + " zapomenut (spousti se jiny soubor: " + nm + ")");
      g_archiv = nap::AtariArchiv(); g_archivAktualni = -1;
    }
    return nullptr;
  }
  const long long t0 = nap_ted_ms();
  nap::AtariArchiv ar;
  const bool ok = ar.otevri(buf.data(), buf.size(), nm);
  const long long ms = nap_ted_ms() - t0;
  if (!ok) {
    {
      std::lock_guard<std::mutex> a(g_mArchiv);
      g_archivChyba = ar.chyba;
    }
    std::string zbytek;
    for (auto &c : ar.chybyRozbaleni) zbytek += " | " + c;
    devLog("B349 ARCHIV " + nm + " (" + (ar.typ.empty() ? std::string("?") : ar.typ) + "): NIC KE SPUSTENI - " + ar.chyba + zbytek
           + (ar.preskoceno.empty() ? std::string() : " | preskoceno: " + std::to_string(ar.preskoceno.size()) + " souboru"));
    {
      std::lock_guard<std::mutex> dl(g_mDev);
      nap::dev::Device &d = devGet();
      if (!d.power) { d.power = true; d.powerAt = nap_ted_ms(); }
      d.setStatusMessage(hlaskaArchivu(ar.chyba).c_str(), nap_ted_ms(), 9000);
    }
    pokeRender();
    return env->NewByteArray(0);
  }
  // do logu: co v archivu je a co se spusti (u velke sbirky jen zacatek seznamu)
  std::string co;
  for (size_t i = 0; i < ar.soubory.size() && i < 24; i++) {
    const nap::AtariSoubor &s = ar.soubory[i];
    char b[200];
    snprintf(b, sizeof(b), "%s%s %s (%zu B%s%s)", i ? ", " : "", s.druh == 'A' ? "ATR" : s.druh == 'X' ? "XEX" : "CAR", s.jmeno.c_str(), s.data.size(),
             s.druh == 'A' && !s.boot ? ", nebootuje" : "", s.sada >= 0 ? ", sada" : "");
    co += b;
  }
  if (ar.soubory.size() > 24) co += ", ... (celkem " + std::to_string(ar.soubory.size()) + " souboru pro Atari)";
  std::string pres;
  size_t presN = 0;
  for (auto &p : ar.preskoceno) if (presN++ < 12) pres += (pres.empty() ? "" : ", ") + p;
  for (auto &c : ar.chybyRozbaleni) if (presN++ < 12) pres += (pres.empty() ? "" : ", ") + c;
  if (presN > 12) pres += ", ... (celkem " + std::to_string(presN) + ")";
  const nap::AtariSoubor &v = ar.soubory[(size_t)ar.vybrany];
  devLog("B349 ARCHIV " + nm + " (" + ar.typ + ", rozbaleno za " + std::to_string(ms) + " ms): " + co
         + (pres.empty() ? std::string() : " | preskoceno: " + pres) + " -> SPOUSTIM " + v.jmeno
         + (vymena ? " (vymena diskety bez restartu)" : ""));
  std::vector<uint8_t> vysledek = v.data;
  {
    std::lock_guard<std::mutex> a(g_mArchiv);
    g_archiv = std::move(ar);
    g_archivAktualni = g_archiv.vybrany;
    g_archivChyba.clear();
  }
  return archivPole(env, vysledek);
}

/** B349: soubor 'index' z posledniho archivu (nabidka ATR / DISK) - obsah; jmeno pak devCtl "archiv_vybrany". */
extern "C" JNIEXPORT jbyteArray JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devArchivSouborNative(JNIEnv *env, jclass, jint index) {
  std::vector<uint8_t> d;
  std::string jm, arch;
  {
    std::lock_guard<std::mutex> a(g_mArchiv);
    if (index < 0 || index >= (jint)g_archiv.soubory.size()) return nullptr;
    d = g_archiv.soubory[(size_t)index].data;
    jm = g_archiv.soubory[(size_t)index].jmeno;
    arch = g_archiv.jmeno;
    g_archivAktualni = index;
  }
  devLog("B349 ARCHIV " + arch + ": vybrano " + jm + " (" + std::to_string(d.size()) + " B)");
  return archivPole(env, d);
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
  onlineKonecProNovouHru("vlozena jina disketa (ATR)");   // B363
  nap::AtrDisk d;
  char b[300];
  if (!d.load(buf.data(), buf.size(), nm)) {
    snprintf(b, sizeof(b), "CHYBA %s neni platny ATR (chybi hlavicka $96 $02 nebo je prazdny)", nm.c_str());
    devLog(std::string("B295 DISKETA ") + b);
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage((nap::tx("NENI ATR: ", "NOT AN ATR: ") + nm).c_str(), nap_ted_ms(), 6000);
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
      devGet().setStatusMessage((nap::tx("V D1: JE ", "IN D1: ") + (nm.size() > 26 ? nm.substr(0, 24) + ".." : nm)).c_str(), nap_ted_ms(), 5000);
    }
    pokeRender();
    return env->NewStringUTF(b);
  }
  snprintf(b, sizeof(b), "OK %s: %d sektoru po %d B (%u kB) -> mechanika D1:, studeny start s OPTION (BASIC vypnuty)",
           nm.c_str(), d.sectors, d.sectorSize, (unsigned)(d.data.size() / 1024));
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    g_xex.zrus();
    vyjmoutCartridgeLocked("bootuje disketa ATR");
    g_xexPosledni = false; g_pamet1088 = chce1088(nm);   // B352
    studenyStartLocked(3);
    g_stroj->disk = std::move(d);
    g_stroj->sioRychlyTimeout = false;
    g_strojBezi = true;
    // OPTION drzet pri startu, pak pustit - hry ho pak ctou samy
    // (B348: 1 s misto 3 s - viz B348_OPTION_PRI_BOOTU)
    g_consolChtene = 7;
    g_consolDrzetDo[2] = g_snimkuCelkem.load() + B348_OPTION_PRI_BOOTU;
    g_consolDrzetDo[0] = g_consolDrzetDo[1] = 0;
    vyzvednoutVystupyLocked();
  }
  devLog(std::string("B295 DISKETA VLOZENA ") + b);
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &dv = devGet();
    if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
    dv.atariValid = false; dv.markScreen(); dv.markLegend();
    dv.setStatusMessage((nap::tx("DISKETA D1: ", "DISK D1: ") + nm).c_str(), nap_ted_ms(), 5000);
  }
  pokeRender();
  return env->NewStringUTF(b);
}

/** B316: cartridge (.car) do slotu a studeny start (jako zasunout cartridge
 *  a zapnout pocitac). Vraci "OK ..." / "CHYBA ..." pro log. */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadCarNative(JNIEnv *env, jclass, jbyteArray data, jstring jmeno) {
  std::string nm = "cartridge.car";
  if (jmeno) { const char *c = env->GetStringUTFChars(jmeno, nullptr); if (c) { nm = c; env->ReleaseStringUTFChars(jmeno, c); } }
  for (auto &ch : nm) if ((unsigned char)ch >= 0x80) ch = '?';
  if (!data) return env->NewStringUTF("CHYBA zadna data");
  jsize n = env->GetArrayLength(data);
  std::vector<uint8_t> buf((size_t)n);
  if (n > 0) env->GetByteArrayRegion(data, 0, n, (jbyte *)buf.data());
  onlineKonecProNovouHru("vlozena jina cartridge (CAR)");   // B363
  const std::string kratke = nm.size() > 26 ? nm.substr(0, 24) + ".." : nm;   // na pristroj (jako u diskety)
  nap::Cartridge c;
  const nap::Cartridge::Chyba ch = c.nactiCar(buf.data(), buf.size(), nm);
  char b[500];
  if (ch != nap::Cartridge::OK) {
    const int t = c.posledniTyp;
    std::string st;
    switch (ch) {
      case nap::Cartridge::KRATKY: case nap::Cartridge::NENI_CAR:
        snprintf(b, sizeof(b), "CHYBA %s neni .car (chybi hlavicka CART)", nm.c_str());
        st = nap::tx(std::string(u8"NENÍ .CAR: "), std::string("NOT A .CAR: ")) + kratke;
        break;
      case nap::Cartridge::NEZNAMY_TYP:
        snprintf(b, sizeof(b), "CHYBA %s: neznamy typ cartridge %d", nm.c_str(), t);
        st = nap::tx(u8"NEZNÁMÝ TYP CARTRIDGE ", "UNKNOWN CARTRIDGE TYPE ") + std::to_string(t);
        break;
      case nap::Cartridge::PRO_5200:
        snprintf(b, sizeof(b), "CHYBA %s: typ %d (%s) je cartridge pro Atari 5200, ne pro 130XE", nm.c_str(), t, nap::cartJmeno(t));
        st = nap::tx("CARTRIDGE PRO ATARI 5200", "AN ATARI 5200 CARTRIDGE");
        break;
      case nap::Cartridge::PRAVY_SLOT:
        snprintf(b, sizeof(b), "CHYBA %s: typ %d (%s) patri do praveho slotu Atari 800 - 130XE ho nema", nm.c_str(), t, nap::cartJmeno(t));
        st = nap::tx(u8"JEN PRO PRAVÝ SLOT ATARI 800", "RIGHT SLOT (ATARI 800) ONLY");
        break;
      case nap::Cartridge::NEUMIM:
        snprintf(b, sizeof(b), "CHYBA %s: typ %d (%s, %d kB) zatim neumim", nm.c_str(), t, nap::cartJmeno(t), nap::cartKb(t));
        st = nap::tx(u8"CARTRIDGE TYP ", "CARTRIDGE TYPE ") + std::to_string(t) + nap::tx(u8" ZATÍM NEUMÍM", " NOT SUPPORTED YET");
        break;
      default:
        snprintf(b, sizeof(b), "CHYBA %s: soubor je kratsi nez typ %d (%s) potrebuje (%d kB dat)", nm.c_str(), t, nap::cartJmeno(t), nap::cartKb(t));
        st = nap::tx(u8"SOUBOR .CAR JE NEÚPLNÝ", "THE .CAR FILE IS INCOMPLETE");
        break;
    }
    devLog(std::string("B316 CARTRIDGE ") + b);
    std::lock_guard<std::mutex> dl(g_mDev);
    devGet().setStatusMessage(st.c_str(), nap_ted_ms(), 8000);
    pokeRender();
    return env->NewStringUTF(b);
  }
  char soucet[120];
  if (c.soucetHlavicka == c.soucetDat) snprintf(soucet, sizeof(soucet), "OK ($%08X)", (unsigned)c.soucetDat);
  else snprintf(soucet, sizeof(soucet), "NESEDI (hlavicka $%08X, data $%08X) - nahravam i tak, jako Atari800",
                (unsigned)c.soucetHlavicka, (unsigned)c.soucetDat);
  snprintf(b, sizeof(b), "OK %s: typ %d (%s), %d kB, kontrolni soucet %s, po zapnuti %s (stav $%X) -> slot, studeny start",
           nm.c_str(), c.typ, nap::cartJmeno(c.typ), nap::cartKb(c.typ), soucet,
           c.rd4() && c.rd5() ? "$8000-$BFFF" : c.rd5() ? "$A000-$BFFF" : c.rd4() ? "$8000-$9FFF" : "okna vypnuta", (unsigned)c.stav);
  const size_t navic = c.prebytek(buf.size());
  bool odpojena = false;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) zaloz();
    g_xex.zrus();
    g_kazBoot.zrus();
    if (g_stroj->disk.mounted) { odpojena = true; g_stroj->disk = nap::AtrDisk(); }
    g_stroj->cart = std::move(c);
    g_xexPosledni = false; g_pamet1088 = chce1088(nm);   // B352
    studenyStartLocked(7);                   // zapnout s cartridge (OPTION netreba - cartridge prekryje BASIC)
    g_stroj->sioRychlyTimeout = true;        // D1: je prazdna - OS nema na co cekat
    g_strojBezi = true;
    g_consolChtene = 7; g_consolDrzetDo[0] = g_consolDrzetDo[1] = g_consolDrzetDo[2] = 0;
    vyzvednoutVystupyLocked();
  }
  devLog(std::string("B316 CARTRIDGE VLOZENA ") + b);
  if (navic) devLog("B316 CARTRIDGE: za daty je jeste " + std::to_string(navic) + " B navic - nevadi, nepouziji se");
  if (odpojena) devLog("B316 CARTRIDGE: disketa vyjmuta z D1: (bootuje cartridge)");
  {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &dv = devGet();
    if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
    dv.atariValid = false; dv.markScreen(); dv.markLegend();
    dv.setStatusMessage(("CARTRIDGE: " + kratke).c_str(), nap_ted_ms(), 5000);
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

// =====================================================================================================
//  B363: ONLINE ATARI - JNI (NativeAtariCoreBridge.online*). Hrani je cele v C++ (vlakno emulace + vlakno site);
//  Java jen sklada dotazy na engine (NapOnlineHra), podepisuje je a ukazuje okno ONLINE na pristroji.
// =====================================================================================================
static std::string onlineJstr(JNIEnv *env, jstring js, size_t max = 4096) {
  std::string s;
  if (!js) return s;
  const char *c = env->GetStringUTFChars(js, nullptr);
  if (c) { s.assign(c, strnlen(c, max)); env->ReleaseStringUTFChars(js, c); }
  return s;
}

/**
 * Hra pro zalozeni online hry: "nazev\npripraveno\nproc" - nazev = XEX / disketa / cartridge, ktera v Atari bezi
 * ("ATARI BASIC" = nic), pripraveno 1 = Atari je zapnute (hra se posle ve stavu - druhy ji mit nemusi).
 */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineHra(JNIEnv *env, jclass) {
  std::string jm, proc;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) {
      if (g_xexPosledni && !g_xexJmeno.empty()) jm = g_xexJmeno;
      else if (g_stroj->disk.mounted) jm = g_stroj->disk.name;
      else if (g_stroj->cart.vlozena()) jm = g_stroj->cart.jmeno;
      else jm = "ATARI BASIC";
    }
    if (!g_stroj || !g_strojBezi.load()) proc = "Atari je vypnute";
  }
  std::string r = nap::pcAscii(jm, 120, true) + "\n" + (proc.empty() ? "1" : "0") + "\n" + proc;
  return env->NewStringUTF(r.c_str());
}

/**
 * Zacne online hru (hrac1 = zaklada s hrou, ktera ted v Atari bezi; obaNaJ1 = hrac 2 hraje joystickem 1 - hry, kde
 * se hraci stridaji). "OK" nebo text chyby. Do domluvy telefonu Atari bezi normalne; hrac 2: Atari se zapne
 * a ceka na stav od hrace 1.
 */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStart(JNIEnv *env, jclass, jstring jadresa, jstring jcesta, jobjectArray jhlavicky,
                                                         jboolean hrac1, jstring jjmeno, jboolean obaNaJ1) {
  nap::online::OnlineKlient::Nastaveni n;
  n.adresa = onlineJstr(env, jadresa);
  n.cesta = onlineJstr(env, jcesta);
  if (jhlavicky) {
    const jsize k = env->GetArrayLength(jhlavicky);
    for (jsize i = 0; i < k; i++) {
      jstring h = (jstring)env->GetObjectArrayElement(jhlavicky, i);
      n.hlavicky.push_back(onlineJstr(env, h, 1024));
      if (h) env->DeleteLocalRef(h);
    }
  }
  n.hrac1 = hrac1 == JNI_TRUE;
  n.ja.jadro = nap::online::J_ATARI;
  n.ja.jmeno = onlineJstr(env, jjmeno, 60);
  std::memcpy(n.ja.hra, ONLINE_ATARI_HRA, 20);
  n.ja.nastaveni = ONLINE_ATARI_JADRO;
  n.snimekMs = 20.0;                         // PAL 50 snimku/s
  // jako Sega B362: odhad + oprava (zpozdeni na startu jen na jednosmernou cestu, za hry roste po malych krocich);
  // B363: start ze stavu (hrac 1 posle cely stav Atari i s hrou)
  n.ladeni.odhad = true;
  n.ladeni.zvysitNejvys = 2;
  n.ladeni.zpozdeniNavic = 4;
  n.ladeni.startZeStavu = true;
  if (n.adresa.empty() || n.cesta.empty()) return env->NewStringUTF("CHYBA: chybi adresa enginu");
  if (n.hrac1 && !g_strojBezi.load()) return env->NewStringUTF("CHYBA: Atari je vypnute - zapni ho a spust hru");
  {
    std::lock_guard<std::mutex> rz(g_onlineRizeni);
    g_onlineHraje.store(0);
    g_onlineAktivni.store(0);
    g_online.start(n);
    {
      std::lock_guard<std::mutex> l(g_onlineM);
      g_onlineKonec.clear();
      g_onlineSetrneProc.clear();
    }
    g_onlineObaNaJ1Volba.store(n.hrac1 && obaNaJ1 == JNI_TRUE);
    g_onlineStartu.fetch_add(1);
    g_onlineSnimkuCelkem.store(0);
    g_onlineStavKb.store(0);
    g_onlineStartStav.store(0);
    g_onlineSetrne.store(0);
    g_onlineSetrneZapnuto.store(0);
    g_onlineChybOpravy.store(0);
    g_onlineOdhadMax.store(nap::online::ODHAD_MAX);
    g_onlineAktivni.store(1);
  }
  bool zapnuto = false;
  if (!n.hrac1 && !g_strojBezi.load()) {     // hrac 2: Atari zapnout (stav od hrace 1 ho stejne cely prepise)
    std::lock_guard<std::mutex> l(g_mStroj);
    if (!g_stroj) { g_pamet1088 = false; studenyStartLocked(7); }
    g_strojBezi = true;
    zapnuto = true;
  }
  if (zapnuto) {
    std::lock_guard<std::mutex> dl(g_mDev);
    nap::dev::Device &dv = devGet();
    if (!dv.power) { dv.power = true; dv.powerAt = nap_ted_ms(); }
    dv.atariValid = false; dv.markScreen(); dv.markLegend();
  }
  devLog(std::string("B363 ATARI ONLINE ") + (n.hrac1 ? "zaklada hru" : "pripojuje se ke hre") + " " + n.cesta.substr(0, n.cesta.find('?')) +
         (n.hrac1 ? (g_onlineObaNaJ1Volba.load() ? " - porty: oba hraci na joysticku 1" : " - porty: hrac 1 = joystick 1, hrac 2 = joystick 2") : "") +
         (zapnuto ? " (Atari bylo vypnute - zapnuto)" : ""));
  pokeRender();
  return env->NewStringUTF("OK");
}

/** Konec online hry (hrac odchazi). Atari bezi dal samo (ovladani zase jen tento telefon). */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStop(JNIEnv *, jclass) {
  bool bylo;
  {
    std::lock_guard<std::mutex> rz(g_onlineRizeni);
    bylo = g_onlineAktivni.load() != 0;
    g_onlineAktivni.store(0);
    g_onlineHraje.store(0);
    g_onlineSetrne.store(0);
    g_online.stop();
    std::lock_guard<std::mutex> l(g_mStroj);
    g_onlineKruh.zahod(true);
    g_onlineKlavesa = -1; g_onlineKlavesaPustit = false; g_onlineBreak = g_onlineReset = false;
    aplikujJoyLocked();
  }
  if (bylo) devLog("B363 ATARI ONLINE konec (hrac) - Atari bezi dal jen na tomto telefonu");
}

/** Stav pro Javu: radky klic=hodnota (stejne klice jako Sega - NapOnlineHlidac; navic system, start_stav, setrne). */
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStav(JNIEnv *env, jclass) {
  using F = nap::online::OnlineKlient::Faze;
  const F f = g_online.faze();
  const char *fs = f == F::PRIPOJUJE ? "PRIPOJUJE" : f == F::CEKA_NA_HRACE ? "CEKA_NA_HRACE" : f == F::HRAJE ? "HRAJE" : f == F::KONEC ? "KONEC" : "NIC";
  std::shared_ptr<nap::online::Relace> r = g_online.relace();
  std::string konec, setrneProc;
  {
    std::lock_guard<std::mutex> l(g_onlineM);
    konec = g_onlineKonec;
    setrneProc = g_onlineSetrneProc;
  }
  // jina hra / nastaveni u Atari = jina verze appky (hra jde ve stavu)
  std::string chyba = g_online.chyba();
  auto verze = [](std::string &t) {
    const std::string jh = nap::online::popisDuvodu(nap::online::D_JINA_HRA);
    const size_t i = t.find(jh);
    if (i != std::string::npos) t.replace(i, jh.size(), "druhy hrac ma jinou verzi appky (Atari online) - aktualizujte obe");
  };
  verze(konec);
  verze(chyba);
  std::ostringstream o;
  o << "system=atari\nfaze=" << fs << "\nkod=" << g_online.kodMistnosti() << "\ndruhy=" << g_online.jmenoDruheho()
    << "\npopis=" << g_online.popis() << "\nchyba=" << chyba
    << "\nodezva=" << (r ? r->odezvaMs() : 0) << "\nzpozdeni=" << (r ? r->zpozdeni() : 0)
    << "\novereno=" << (r ? r->overenychOtisku() : 0) << "\ncekani=" << (r ? r->cekani() : 0)
    << "\nsnimek=" << g_onlineSnimkuCelkem.load() << "\naktivni=" << g_onlineAktivni.load()
    << "\nhraje=" << ((g_onlineHraje.load() && g_onlineStartStav.load() == 4) ? 1 : 0) << "\nhrac1=" << (r ? (r->hrac1() ? 1 : 0) : 0)
    << "\nkonec=" << konec << "\nstart=" << g_onlineStartu.load() << "\nstart_stav=" << g_onlineStartStav.load()
    << "\nsetrne=" << g_onlineSetrne.load() << "\nsetrne_c=" << g_onlineSetrneZapnuto.load() << "\nsetrne_proc=" << setrneProc
    << "\nporty=" << (g_onlinePortyJ1.load() ? "J1" : "J1J2");
  if (r) {
    o << "\nzpozdeni_ted=" << r->zpozdeniTed() << "\ncekani_ms=" << r->cekaniMs() << "\ncekani_max_ms=" << r->cekaniMaxMs()
      << "\ndlouhych=" << r->dlouhychCekani() << "\nzadosti=" << r->zadosti() << "\nzvyseni=" << r->zvyseni()
      << "\nsnizeni=" << r->snizeni() << "\nsync=" << (r->synchronizuje() && !r->startZeStavu() ? 1 : 0) << "\nsynchronizaci=" << r->synchronizaci()
      << "\nsynchronizovano=" << r->synchronizovano() << "\nrozjeti=" << r->rozjeti() << "\nrozjeti_snimek=" << r->rozjetoVeSnimku()
      << "\nrozjeti_casti=" << nap::online::nazvyCasti(r->rozdilCasti(), nap::atariStavNazvyCasti(), nap::ATARI_STAV_CASTI)
      << "\nstav_kb=" << g_onlineStavKb.load() << "\nticho_ms=" << r->tichoMs() << "\nstoji_ms=" << r->stojiMs()
      << "\nodhad=" << r->odhadnuto() << "\nodhad_sedelo=" << r->odhaduSedelo() << "\noprav=" << r->oprav()
      << "\noprava_max=" << r->opravaMax() << "\noprava_max_ms=" << r->opravaMaxMs() << "\nopravy_ms=" << r->opravyMs()
      << "\nprepocteno=" << r->prepocteno() << "\nodhad_max=" << g_onlineOdhadMax.load()
      << "\nchyb_opravy=" << g_onlineChybOpravy.load() << "\nnavrat_ze=" << g_onlineNavratZe.load()
      << "\nnavrat_na=" << g_onlineNavratNa.load() << "\nposledni_zprava=" << r->posledniZprava()
      << "\njeho_do=" << r->jehoDo() << "\ndruhy_info_c=" << r->statistikDruheho()
      << "\ndruhy_info_snimek=" << r->mujSnimekPriStatistice() << "\ndruhy_info=" << r->statistikaDruheho();
  }
  return env->NewStringUTF(o.str().c_str());
}

/** Telefon o sobe (model, Android, Wi-Fi, baterie - Java kazdych par s) - jde druhemu telefonu ve STATISTIKA. */
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineTelefon(JNIEnv *env, jclass, jstring jinfo) {
  std::string s = onlineJstr(env, jinfo, 200);
  for (char &c : s) if ((unsigned char)c < 32 || (unsigned char)c > 126) c = '?';   // jen ASCII (log druheho)
  g_online.infoTelefonu(s);
}
