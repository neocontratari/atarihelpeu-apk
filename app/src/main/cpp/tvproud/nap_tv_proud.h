// nap_tv_proud.h
// B305: SEGA A PS1 NA TV / PC CELE V C++ (jako Atari 130XE v B303).
//
// Rene: "udelas ten tv prenos u segi a ps1 na tv a pc jako si udelal u atari
// emu c++, aby to bylo taky v c++ a tak super jak v emu atari c++?"
//
// Doted Sega a PS1 sly na TV takhle: Java si vzala snimek z jadra, zvetsila
// ho na 1280x720, zkomprimovala (H.264 nebo JPEG) a prohlizec si H.264 jeste
// bufferoval - zvuk sel zvlast (/audio.raw kazdych 20 ms). Ted stejne jako
// Atari:
//
//   jadro (vlakno emulace) --snimek--> Hub --kazdy prohlizec: vlastni vlakno-->
//   paket (obraz + zvuk dohromady) --> Java jen posle bajty --> prohlizec
//   nakresli do canvasu a zvuk zaradi do WebAudio
//
// - snimek se bere hned, jak ho jadro dokresli (zadne cekani na tik Javy),
// - kazdy prohlizec dostane vzdy POSLEDNI snimek oproti tomu, co uz ma (zadna
//   fronta, kdyz nestiha - zpozdeni neroste),
// - zvuk je ve stejnem paketu jako obraz (nejvys 0,25 s; kdyz prohlizec
//   zaostane, stare se zahodi),
// - Sega: kodek indexy (bez ztraty, nap_tv_proud_index.h), PS1: JPEG po
//   castech (nap_tv_proud_jpeg.h).
//
// Bez Androidu - testuje se na PC (test_overeni/b305).
//
// PAKET (little endian), pred nim 4 bajty delka:
//   0  'N' 'A'             znacka (stejna jako Atari, B303)
//   2  u8  verze           2 = Sega (indexy + LZ), 3 = PS1 (JPEG po castech)
//   3  u8  priznaky        bit0 klicovy (prohlizec zacina od nuly)
//                          bit2 obraz beze zmeny
//                          bit3 B313: dalsi cast tehoz snimku (JPEG menu po obdelnicich,
//                               jde hned za prvni casti, zvuk 0 vzorku)
//   4  u16 W, u16 H        rozmer snimku jadra
//   8  u32 cislo snimku jadra
//  12  u16 x, y, w, h      obsah bez cernych okraju (pro "vyplnit obrazovku")
//  20  u8  pomer X, Y      pomer stran celeho snimku na TV (4:3)
//  22  u16 0
//  24  u32 vzorkovani zvuku (Sega 48000, PS1 44100)
//  28  ... obraz podle verze
//      zvuk: u16 pocet vzorku, u8 kanalu (1 mono = L i R, 2 stereo), int16
#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "nap_tv_proud_spol.h"

namespace nap {
namespace proud {

// Obsah snimku bez cernych okraju (hry casto kresli do mensi plochy - 224 radku
// z 240, Sega s vypnutym levym sloupcem). Stejna mez jako dosud v Jave
// (napTvRowIsBlack, TMAVA = 12). Rozsiri se hned (obsah se nesmi useknout),
// zmensi az po ~1,5 s (aby obraz necukal pri tmave scene).
struct Orez {
  static constexpr int TMAVA = 12;
  static constexpr int OKNO = 90;
  int W = 0, H = 0;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;       // aktualni (x1, y1 za koncem)
  int kx0 = 0, ky0 = 0, kx1 = 0, ky1 = 0;   // co bylo videt v tomhle okne
  int okno = 0;
  bool platny = false;
  static bool svetly(uint32_t c) { return ((c >> 16) & 255) > TMAVA || ((c >> 8) & 255) > TMAVA || (c & 255) > TMAVA; }
  void spocti(const uint32_t *F, int w, int h) {
    if (w != W || h != H) { W = w; H = h; platny = false; }
    int t = 0, b = H - 1, l = 0, r = W - 1;
    auto radek = [&](int y) { const uint32_t *p = F + (size_t)y * W; for (int x = 0; x < W; x += 2) if (svetly(p[x])) return true; return false; };
    auto sloupec = [&](int x) { for (int y = t; y <= b; y += 2) if (svetly(F[(size_t)y * W + x])) return true; return false; };
    while (t < H && !radek(t)) t++;
    if (t >= H) return;                         // cely snimek cerny (nacitani) - nechat, jak je
    while (b > t && !radek(b)) b--;
    while (l < W - 1 && !sloupec(l)) l++;
    while (r > l && !sloupec(r)) r--;
    int dx0 = l, dy0 = t, dx1 = r + 1, dy1 = b + 1;
    if (dx1 - dx0 < W / 3 || dy1 - dy0 < H / 3) { dx0 = 0; dy0 = 0; dx1 = W; dy1 = H; }   // tma ve hre - neorezavat
    if (!platny) {
      x0 = dx0; y0 = dy0; x1 = dx1; y1 = dy1;
      kx0 = dx0; ky0 = dy0; kx1 = dx1; ky1 = dy1;
      okno = 0; platny = true;
      return;
    }
    x0 = std::min(x0, dx0); y0 = std::min(y0, dy0); x1 = std::max(x1, dx1); y1 = std::max(y1, dy1);
    kx0 = std::min(kx0, dx0); ky0 = std::min(ky0, dy0); kx1 = std::max(kx1, dx1); ky1 = std::max(ky1, dy1);
    if (++okno >= OKNO) {
      x0 = kx0; y0 = ky0; x1 = kx1; y1 = ky1;
      kx0 = dx0; ky0 = dy0; kx1 = dx1; ky1 = dy1;
      okno = 0;
    }
  }
};

template <class Kodek>
class Hub {
 public:
  static constexpr size_t ZVUK_KAP = 1u << 16;        // shortu (~0,68 s stereo 48 kHz)
  static constexpr long long ZVUK_SAM_MS = 60;        // B306: bez noveho snimku dele -> paket jen se zvukem

  // orez = hledat cerne okraje (hry); B306: uvodni demo je navrzene 16:9 cele - bez orezu
  Hub(const char *jmeno, int vzorkovani, int pomerX = 4, int pomerY = 3, bool orez = true)
      : jmeno_(jmeno), vzorkovani_(vzorkovani), pomerX_(pomerX), pomerY_(pomerY), orez_(orez) {}

  // ---- jadro (vlakno emulace): hotovy snimek 0x??RRGGBB ----
  void snimek(const uint32_t *argb, int w, int h) {
    posledniMs_.store(tedMs(), std::memory_order_relaxed);
    snimku_.fetch_add(1, std::memory_order_relaxed);
    if (pocet_.load(std::memory_order_acquire) <= 0 || !argb || w <= 0 || h <= 0) return;
    {
      std::lock_guard<std::mutex> l(m_);
      const size_t n = (size_t)w * h;
      if (snimek_.size() != n) snimek_.resize(n);
      std::memcpy(snimek_.data(), argb, n * 4);
      sw_ = w; sh_ = h;
      surovy_ = false;
      seq_++;
    }
    cv_.notify_all();
  }

  // B313: snimek primo z glReadPixels (bajty R G B A, radky ZDOLA nahoru). Vlakno kresleni
  // jen vymeni buffer (zadna kopie ani prevod - menu 1280x720 = 3,7 MB); otoceni a prevod
  // na 0x00RRGGBB udela az vlakno prohlizece, MIMO zamek (kresleni na nej nikdy neceka
  // dele nez na vymenu ukazatele). buf dostane zpet volny buffer (k dalsimu pouziti).
  void snimekRgbaZdola(std::vector<uint8_t> &buf, int w, int h) {
    posledniMs_.store(tedMs(), std::memory_order_relaxed);
    snimku_.fetch_add(1, std::memory_order_relaxed);
    if (pocet_.load(std::memory_order_acquire) <= 0 || w <= 0 || h <= 0 || buf.size() < (size_t)w * h * 4) return;
    Surovy *nova = nullptr;
    {
      std::lock_guard<std::mutex> l(m_);
      // volny buffer: neni vystaveny a zadny prohlizec ho prave neprevadi (pocita se pod zamkem)
      for (auto &z : zasoba_)
        if (z.get() != surova_ && z->ctenaru == 0) { nova = z.get(); break; }
      if (!nova) {
        zasoba_.push_back(std::unique_ptr<Surovy>(new Surovy()));
        nova = zasoba_.back().get();
      }
    }
    nova->data.swap(buf);                  // nikdo jiny ho nema (vystavuje jen tohle vlakno) - bez zamku
    {
      std::lock_guard<std::mutex> l(m_);
      surova_ = nova;
      sw_ = w; sh_ = h;
      surovy_ = true;
      seq_++;
    }
    cv_.notify_all();
  }

  // ---- zvuk (vlakno zvuku): stereo int16, co se prave prehrava ----
  void zvuk(const int16_t *s, size_t shortu) {
    if (pocet_.load(std::memory_order_acquire) <= 0 || !s || shortu < 2) return;
    shortu &= ~(size_t)1;
    std::lock_guard<std::mutex> l(m_);
    if (zv_.size() != ZVUK_KAP) zv_.assign(ZVUK_KAP, 0);
    for (size_t i = 0; i < shortu; i++) zv_[(size_t)((zvZapis_ + i) % ZVUK_KAP)] = s[i];
    zvZapis_ += shortu;
  }

  // B313: nastaveni kodeku pro kazdeho noveho prohlizece (menu: vic pasu, vyssi kvalita)
  void nastavKodek(std::function<void(Kodek &)> f) {
    std::lock_guard<std::mutex> l(m_);
    nastaveniKodeku_ = std::move(f);
  }
  // B319: volby, ktere si prohlizec rekl (stranka WEB TV B319: posun obrazu, polovicni rozliseni) -
  // f je nastavi kodeku tohoto prohlizece, popis jde do logu
  void nastavVolby(std::function<void(Kodek &, int)> f, const char *popis) {
    std::lock_guard<std::mutex> l(m_);
    nastaveniVoleb_ = std::move(f);
    popisVoleb_ = popis ? popis : "";
  }

  // ---- prohlizec ----
  int otevri(int volby = 0) {
    int id, celkem;
    bool sVolbami = false;
    std::string popis;
    {
      std::lock_guard<std::mutex> l(m_);
      id = dalsiId_++;
      auto k = std::make_shared<Klient>();
      k->id = id;
      if (nastaveniKodeku_) nastaveniKodeku_(k->kodek);
      if (volby && nastaveniVoleb_) { nastaveniVoleb_(k->kodek, volby); sVolbami = true; popis = popisVoleb_; }
      k->seq = seq_;                    // ceka na cerstvy snimek (stary muze byt z jine hry)
      k->startMs = k->logMs = tedMs();
      klienti_[id] = k;
      celkem = (int)klienti_.size();
      pocet_.store(celkem, std::memory_order_release);
    }
    char b[420];
    snprintf(b, sizeof(b), "B306 TV PROUD %s: prohlizec %d pripojen (celkem %d) - obraz a zvuk primo z C++ (%s), kazdy snimek, zvuk %d Hz v tomtez paketu%s%s",
             jmeno_, id, celkem, Kodek::nazev(), vzorkovani_, sVolbami ? " | B319: " : "", sVolbami ? popis.c_str() : "");
    log(b);
    return id;
  }

  // Ceka (nejvys cekejMs) na novy snimek a zabali ho: pk = 4 B delka + paket.
  // zapisMs = jak dlouho Java zapisovala minuly paket (rizeni kvality JPEG).
  // Vraci 1 = paket, 0 = nic noveho, -1 = neznamy prohlizec.
  int dalsi(int id, int cekejMs, int zapisMs, std::vector<uint8_t> &pk) {
    std::shared_ptr<Klient> kp;
    Surovy *sur = nullptr;                // B313: snimek z glReadPixels (prevod mimo zamek)
    unsigned long long seq;
    int W, H;
    {
      std::unique_lock<std::mutex> l(m_);
      auto it = klienti_.find(id);
      if (it == klienti_.end()) return -1;
      kp = it->second;
      Klient &k = *kp;
      if (!k.odlozeny.empty()) {                  // paket, ktery se minule do Javy nevesel
        pk.swap(k.odlozeny);
        k.odlozeny.clear();
        return 1;
      }
      auto novy = [&]() { return seq_ != k.seq; };
      // B306: kdyz novy snimek dlouho neprijde (obraz se zadrhl - demo na pomalem GPU,
      // nacitani hry), ale zvuk bezi dal, posle se paket jen se zvukem (obraz stejny =
      // "beze zmeny"), at zvuk na TV nevypadne. Pri 30-60 snimcich/s se to nestane.
      bool jenZvuk = false;
      if (!novy()) {
        const long long konecMs = tedMs() + std::max(1, std::min(1000, cekejMs));
        while (!novy()) {
          if (!k.rgb.empty() && k.zvukJede && zvZapis_ > k.zvukPoz && tedMs() - k.poslMs >= ZVUK_SAM_MS) { jenZvuk = true; break; }
          const long long zb = konecMs - tedMs();
          if (zb <= 0) break;
          cv_.wait_for(l, std::chrono::milliseconds(std::min<long long>(zb, 15)), novy);
        }
      }
      if (!novy() && !jenZvuk) return 0;
      if (!jenZvuk) {
        seq = seq_; W = sw_; H = sh_;
        k.seq = seq;
        k.rgb.resize((size_t)W * H);
        if (surovy_ && surova_) { sur = surova_; sur->ctenaru++; }   // B313: prevede se az za zamkem
        else std::memcpy(k.rgb.data(), snimek_.data(), (size_t)W * H * 4);
        k.W = W; k.H = H;
      } else {
        seq = k.seq; W = k.W; H = k.H;          // stejny obraz jako minule
        k.jenZvuk++;
      }
      // zvuk od minula (nejvys 0,25 s - kdyz prohlizec zaostal, stare se zahodi)
      k.zvuk.clear();
      const unsigned long long konec = zvZapis_;
      const unsigned long long MAXZ = (unsigned long long)vzorkovani_ * 2 / 4;
      if (!k.zvukJede || konec < k.zvukPoz) { k.zvukPoz = konec; k.zvukJede = true; }
      else if (konec - k.zvukPoz > MAXZ) { k.zvukPoz = konec - ((unsigned long long)vzorkovani_ / 30 & ~1ULL); k.zvukSkoku++; }
      if (zv_.size() == ZVUK_KAP && konec > k.zvukPoz) {
        const size_t kolik = (size_t)std::min<unsigned long long>(konec - k.zvukPoz, ZVUK_KAP) & ~(size_t)1;
        const unsigned long long od = konec - kolik;
        k.zvuk.resize(kolik);
        for (size_t i = 0; i < kolik; i++) k.zvuk[i] = zv_[(size_t)((od + i) % ZVUK_KAP)];
      }
      k.zvukPoz = konec;
    }
    // klienta meni jen tohle vlakno - koduje se bez zamku (jadro bezi dal)
    Klient &k = *kp;
    if (sur) {
      // B313: RGBA zdola nahoru (glReadPixels) -> 0x00RRGGBB shora dolu
      for (int y = 0; y < H; y++) {
        const uint8_t *r = &sur->data[(size_t)(H - 1 - y) * W * 4];
        uint32_t *o = &k.rgb[(size_t)y * W];
        for (int x = 0; x < W; x++, r += 4) o[x] = ((uint32_t)r[0] << 16) | ((uint32_t)r[1] << 8) | (uint32_t)r[2];
      }
      std::lock_guard<std::mutex> l(m_);
      sur->ctenaru--;
    }
    const long long ted = tedMs();
    k.poslMs = ted;
    if (orez_) k.orez.spocti(k.rgb.data(), W, H);
    else { k.orez.x0 = 0; k.orez.y0 = 0; k.orez.x1 = W; k.orez.y1 = H; }
    pk.clear();
    pk.reserve(64 + k.zvuk.size() * 2 + (size_t)W * H / 2);
    p32(pk, 0);                                   // delka - doplni se
    p8(pk, 'N'); p8(pk, 'A'); p8(pk, Kodek::VERZE); p8(pk, 0);
    p16(pk, (unsigned)W); p16(pk, (unsigned)H);
    p32(pk, (uint32_t)(seq & 0xFFFFFFFFull));
    p16(pk, (unsigned)k.orez.x0); p16(pk, (unsigned)k.orez.y0);
    p16(pk, (unsigned)(k.orez.x1 - k.orez.x0)); p16(pk, (unsigned)(k.orez.y1 - k.orez.y0));
    p8(pk, (unsigned)pomerX_); p8(pk, (unsigned)pomerY_); p16(pk, 0);
    p32(pk, (uint32_t)vzorkovani_);
    const double t0 = tedMsPresne();
    const unsigned pr = k.kodek.zakoduj(k.rgb.data(), W, H, zapisMs, ted, pk);
    const double dt = tedMsPresne() - t0;
    pk[7] = (uint8_t)pr;
    // zvuk
    const size_t vz = k.zvuk.size() / 2;
    bool mono = true;
    for (size_t i = 0; i < vz && mono; i++) if (k.zvuk[2 * i] != k.zvuk[2 * i + 1]) mono = false;
    p16(pk, (unsigned)vz); p8(pk, mono ? 1u : 2u);
    if (mono) for (size_t i = 0; i < vz; i++) p16(pk, (unsigned)(uint16_t)k.zvuk[2 * i]);
    else for (size_t i = 0; i < vz * 2; i++) p16(pk, (unsigned)(uint16_t)k.zvuk[i]);
    const uint32_t delka = (uint32_t)(pk.size() - 4);
    pk[0] = (uint8_t)delka; pk[1] = (uint8_t)(delka >> 8); pk[2] = (uint8_t)(delka >> 16); pk[3] = (uint8_t)(delka >> 24);
    // B313: dalsi casti tehoz snimku (JPEG menu: vic obdelniku) - kazda jako samostatny
    // paket hned za prvnim: stejna hlavicka, priznak 8 = "dalsi cast snimku", zadny zvuk.
    // Java posle vse jednim zapisem, prohlizec kazdou cast nakresli na jeji misto.
    for (size_t i = 0; i < k.kodek.dalsiN && i < k.kodek.dalsi.size(); i++) {
      const size_t zac = pk.size();
      p32(pk, 0);
      p8(pk, 'N'); p8(pk, 'A'); p8(pk, Kodek::VERZE); p8(pk, 8u);
      p16(pk, (unsigned)W); p16(pk, (unsigned)H);
      p32(pk, (uint32_t)(seq & 0xFFFFFFFFull));
      p16(pk, (unsigned)k.orez.x0); p16(pk, (unsigned)k.orez.y0);
      p16(pk, (unsigned)(k.orez.x1 - k.orez.x0)); p16(pk, (unsigned)(k.orez.y1 - k.orez.y0));
      p8(pk, (unsigned)pomerX_); p8(pk, (unsigned)pomerY_); p16(pk, 0);
      p32(pk, (uint32_t)vzorkovani_);
      const std::vector<uint8_t> &c = k.kodek.dalsi[i];
      pk.insert(pk.end(), c.begin(), c.end());
      p16(pk, 0); p8(pk, 1u);                      // zvuk: 0 vzorku
      const uint32_t d = (uint32_t)(pk.size() - zac - 4);
      pk[zac] = (uint8_t)d; pk[zac + 1] = (uint8_t)(d >> 8); pk[zac + 2] = (uint8_t)(d >> 16); pk[zac + 3] = (uint8_t)(d >> 24);
      k.dalsichCasti++;
    }
    // statistika
    k.paketu++; k.bajtu += (long long)pk.size(); k.logPaketu++; k.logBajtu += (long long)pk.size();
    if (pr & 4u) k.logBeze++;
    k.kodMs += dt; if (dt > k.kodMax) k.kodMax = dt;
    k.logZvuku += (long long)vz;
    if (ted - k.logMs >= 30000) {
      const double s = (ted - k.logMs) / 1000.0;
      const long long zmen = k.logPaketu - k.logBeze;
      char b[1100];
      snprintf(b, sizeof(b), "B306 TV PROUD %s: prohlizec %d - %.1f paketu/s (%.1f snimku/s se zmenou), %.2f Mbit/s, prumer %.1f kB na snimek, %dx%d, "
               "baleni prumer %.2f ms max %.1f ms, zvuk %.0f vzorku/s (preskoku %lld, paketu jen se zvukem %lld), %s",
               jmeno_, k.id, k.logPaketu / s, zmen / s, k.logBajtu * 8.0 / 1e6 / s, k.logBajtu / 1024.0 / std::max(1LL, k.logPaketu), W, H,
               k.kodMs / std::max(1LL, k.logPaketu), k.kodMax, k.logZvuku / s, k.zvukSkoku, k.jenZvuk, k.kodek.info().c_str());
      log(b);
      k.logMs = ted; k.logPaketu = k.logBajtu = k.logBeze = k.logZvuku = 0; k.kodMs = 0; k.kodMax = 0;
    }
    return 1;
  }

  // paket se do Javy nevesel (malo mista) - Java zvetsi pole a pri pristim
  // volani dostane presne tenhle paket (zadny klicovy snimek, zadny ztraceny zvuk)
  void odloz(int id, std::vector<uint8_t> &pk) {
    std::lock_guard<std::mutex> l(m_);
    auto it = klienti_.find(id);
    if (it != klienti_.end()) it->second->odlozeny.swap(pk);
  }

  void zavri(int id) {
    std::shared_ptr<Klient> k;
    int celkem;
    {
      std::lock_guard<std::mutex> l(m_);
      auto it = klienti_.find(id);
      if (it == klienti_.end()) return;
      k = it->second;
      klienti_.erase(it);
      celkem = (int)klienti_.size();
      pocet_.store(celkem, std::memory_order_release);
    }
    const double s = std::max(0.001, (tedMs() - k->startMs) / 1000.0);
    char b[1100];
    snprintf(b, sizeof(b), "B306 TV PROUD %s: prohlizec %d odpojen po %.0f s - %lld paketu (%.1f/s), %.1f MB celkem, %s, zvuk preskoku %lld, paketu jen se zvukem %lld (zbyva %d)",
             jmeno_, k->id, s, k->paketu, k->paketu / s, k->bajtu / 1048576.0, k->kodek.info().c_str(), k->zvukSkoku, k->jenZvuk, celkem);
    log(b);
  }

  int klientu() const { return pocet_.load(std::memory_order_acquire); }
  // jadro posila snimky (posledni pred nejvys maxStariMs)?
  bool bezi(int maxStariMs) const {
    const long long t = posledniMs_.load(std::memory_order_relaxed);
    return t > 0 && tedMs() - t <= maxStariMs;
  }
  unsigned long long snimkuCelkem() const { return snimku_.load(std::memory_order_relaxed); }
  // cislo posledniho snimku pro prohlizece (v paketu na 8) - pro testy a log
  unsigned long long posledniSeq() {
    std::lock_guard<std::mutex> l(m_);
    return seq_;
  }

  // radky pro log appky (Java si je vyzvedava)
  std::string vezmiLog() {
    std::lock_guard<std::mutex> l(m_);
    std::string s;
    s.swap(log_);
    return s;
  }
  void log(const std::string &s) {
    std::lock_guard<std::mutex> l(m_);
    if (log_.size() > 32768) return;
    log_ += s;
    log_ += '\n';
  }

 private:
  struct Klient {
    int id = 0;
    Kodek kodek;
    Orez orez;
    std::vector<uint32_t> rgb;
    int W = 0, H = 0;                    // rozmer rgb (posledni poslany snimek)
    long long poslMs = 0;                // kdy se balil posledni paket
    long long jenZvuk = 0;               // B306: paketu jen se zvukem (obraz se zadrhl)
    long long dalsichCasti = 0;          // B313: paketu "dalsi cast snimku" (JPEG menu)
    std::vector<int16_t> zvuk;
    std::vector<uint8_t> odlozeny;
    unsigned long long seq = 0;
    unsigned long long zvukPoz = 0;
    bool zvukJede = false;
    long long zvukSkoku = 0;
    long long startMs = 0, logMs = 0, paketu = 0, bajtu = 0, logPaketu = 0, logBajtu = 0, logBeze = 0, logZvuku = 0;
    double kodMs = 0, kodMax = 0;
  };
  const char *jmeno_;
  int vzorkovani_, pomerX_, pomerY_;
  bool orez_ = true;
  std::mutex m_;
  std::condition_variable cv_;
  std::vector<uint32_t> snimek_;
  // B313: snimky z glReadPixels (snimekRgbaZdola): zasoba bufferu, vystaveny je surova_;
  // ctenaru = kolik prohlizecu ho prave prevadi (vse pod m_; buffery se nikdy nemazou)
  struct Surovy { std::vector<uint8_t> data; int ctenaru = 0; };
  std::vector<std::unique_ptr<Surovy>> zasoba_;
  Surovy *surova_ = nullptr;
  bool surovy_ = false;                  // posledni snimek je v surova_ (ne v snimek_)
  int sw_ = 0, sh_ = 0;
  unsigned long long seq_ = 0;
  std::vector<int16_t> zv_;
  unsigned long long zvZapis_ = 0;
  std::map<int, std::shared_ptr<Klient>> klienti_;
  std::function<void(Kodek &)> nastaveniKodeku_;   // B313
  std::function<void(Kodek &, int)> nastaveniVoleb_;   // B319
  std::string popisVoleb_;                              // B319
  int dalsiId_ = 1;
  std::atomic<int> pocet_{0};
  std::atomic<long long> posledniMs_{0};
  std::atomic<unsigned long long> snimku_{0};
  std::string log_;
};

}  // namespace proud
}  // namespace nap
