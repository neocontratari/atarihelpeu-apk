// nap_atari_audio_ring.h
//
// B287: Rene - "Ne - tu kontrolu udelej jeste pred testem - chci ciste
// jadro atari emu v c++ - Java odhaduje a to je problem - atari emu v jave
// je v aplikaci. Proto ted delame atari emu v c++ !!!" Po kontrole (viz
// PREDAVACI_PROTOKOL): appka ma skutecne DVE oddelene Atari jadra - stare
// cisto-JS (emu_vbxe, hlavni tlacitko menu) a tohle C++ (emu_atari_cpp, pod
// HELP). Rene: "nechytej [na to] az bude pod tlacitkem help atari emu v
// c++ kompletne hotove... vyzaduji aby... bylo opravdu emu atari pod
// tlacitkem HELP ciste v c++." Tenhle soubor je ta posledni chybejici
// cast: nahrazuje JS Web Audio planovani/odhadovani (dalsiZvukStart,
// orezavani fronty...) primym nativnim zvukem (OpenSL ES), presne podle uz
// overeneho a v produkci bezicimo PS1 vzoru (nap_ps1_native.cpp, "CESTA A:
// zvuk bez Javy").
//
// SCHVALNE ODDELENO od nap_atari_native.cpp (ktery potrebuje <jni.h> a
// <SLES/OpenSLES.h> - realne Android hlavicky, ktere se v tomhle CLI
// prostredi nedaji prelozit): tahle hlavicka je CISTE prenositelny C++,
// zadna Android zavislost. Diky tomu jde logika (kruhovy buffer, zisk +
// oriznuti + prevod na int16) overit poctivym CLI testem na pocitaci -
// presne stejna disciplina "zmer, nehaduj", jakou uz pouziva zbytek jadra
// (nap_atari_machine.h apod.). Samotne OpenSL ES volani (nap_atari_native.cpp)
// uz CLI testem overit nejde - to umi jen realny telefon.
#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace nap {

// Jednoduchy lock-free kruhovy buffer pro 16-bit PCM vzorky (interleaved
// stereo shorty: L,R,L,R...). Jeden producent (emulace/genAudio), jeden
// konzument (zvukovy callback) - presny vzor jako u PS1 (g_aring v
// nap_ps1_native.cpp), jen vytazeny sem, aby ho slo sdilet/testovat.
template <unsigned CAPACITY_SHORTS>
struct AtariAudioRing {
  static_assert((CAPACITY_SHORTS & (CAPACITY_SHORTS - 1)) == 0,
                "kapacita kruhoveho bufferu musi byt mocnina dvou");

  int16_t data[CAPACITY_SHORTS];
  std::atomic<unsigned> w{0};
  std::atomic<unsigned> r{0};

  // Zahodi vse cekajici (nastavi cteci konec na psaci) - pouziva se pri
  // zastaveni/startu, aby se neprehralo "stare" zvuky z doby pred pauzou.
  void clear() {
    r.store(w.load(std::memory_order_acquire), std::memory_order_release);
  }

  unsigned avail() const {
    return w.load(std::memory_order_acquire) - r.load(std::memory_order_relaxed);
  }

  // Zapise n shortu. Kdyz se fronta prepliti, nejstarsi vzorky se zahodi
  // (posune se cteci konec) - radsi kratky skok v case nez neomezene
  // rostouci zpozdeni (presne ten problem, co resil B285 na JS strane -
  // tady uz to resi primo buffer sam, zadny JS odhad netreba).
  void write(const int16_t *src, unsigned n) {
    unsigned ww = w.load(std::memory_order_relaxed);
    unsigned rr = r.load(std::memory_order_acquire);
    unsigned free_ = CAPACITY_SHORTS - (ww - rr);
    if (n > free_) {
      unsigned drop = n - free_;
      r.store(rr + drop, std::memory_order_release);
    }
    for (unsigned i = 0; i < n; i++) data[(ww + i) & (CAPACITY_SHORTS - 1)] = src[i];
    w.store(ww + n, std::memory_order_release);
  }

  // Precte az n shortu (muze vratit min, kdyz neni dost k dispozici).
  // Chybejici vzorky NEDOPLNUJE tichem - to je ponechano na volajicim
  // (viz dozvuk posledniho vzorku v nap_atari_sl_callback, mirror PS1).
  unsigned read(int16_t *dst, unsigned n) {
    unsigned rr = r.load(std::memory_order_relaxed);
    unsigned have = w.load(std::memory_order_acquire) - rr;
    unsigned take = have < n ? have : n;
    for (unsigned i = 0; i < take; i++) dst[i] = data[(rr + i) & (CAPACITY_SHORTS - 1)];
    r.store(rr + take, std::memory_order_release);
    return take;
  }
};

// B287: zisk + oriznuti + prevod na int16, zdvojeny na stereo (L=R).
// POKEY/Atari genAudio() vraci MONO float vzorky v rozsahu <-1,1>. Presne
// stejny vzorec (oriznuti na <-1,1>, *32767.0, lround), jaky uz 3x pouziva
// nap_atari_native.cpp pro base64/WAV cestu - jen TADY navic s 4x ziskem,
// protoze ten driv aplikoval az JS GainNode (gain.gain.value=4.0, zavedeno
// kvuli slysitelnosti na repro telefonu - viz historie B257/B294). Bez
// tohohle zisku by nativni prehravani bylo 4x tisi, nez na co je Rene
// zvykly - skutecna regrese hlasitosti, ne jen kosmeticky rozdil.
inline void atariGainClampToStereoInt16(const float *mono, int n, float gain,
                                         int16_t *outStereoInterleaved) {
  for (int i = 0; i < n; i++) {
    float f = mono[i] * gain;
    if (f > 1.0f) f = 1.0f; else if (f < -1.0f) f = -1.0f;
    int16_t v = (int16_t)std::lround((double)f * 32767.0);
    outStereoInterleaved[i * 2]     = v;
    outStereoInterleaved[i * 2 + 1] = v;
  }
}

}  // namespace nap
