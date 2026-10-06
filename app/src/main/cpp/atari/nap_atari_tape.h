// nap_atari_tape.h
// B292: kazeta Atari z WAV souboru (CLOAD) - cista C++ logika bez Androidu,
// overitelna na PC (test_overeni/b292/test_core.cpp - "kazeta").
//
// Jak funguje skutecny magnetofon Atari 410/1010: na pasce je FSK - ton
// 5327 Hz = "mark" (logicka 1) a 3995 Hz = "space" (logicka 0), 600 baudu.
// Demodulator v magnetofonu z toho dela uroven na lince SIO DATA IN a tu
// cte POKEY (bit po bitu jako seriovy port; OS si rychlost zmeri z uvodnich
// bajtu $55 $55 kazdeho zaznamu - SKSTAT bit 4).
//
// Tady se stejny demodulator dela softwarove, JEDNOU pri vlozeni kazety:
//  - kazdy kanal WAV se vynasobi komplexnimi "nosnymi" 5327 a 3995 Hz a
//    secte v okne presne 1/1332 s (delka okna, pri ktere druhy ton dava
//    nulu) -> energie obou tonu,
//  - kde je vic energie "mark", je uroven 1, jinak 0; ticho/sum pod prahem
//    = 1 (klidovy stav linky, jako na skutecne lince bez signalu),
//  - u sterea se vezme kanal, kde je FSK signalu nejvic (na Atari kazetach
//    byva na jedne stope zvuk/hudba a na druhe data).
// Vysledek: posloupnost urovni linky (1 bit na vzorek WAV). Bajty z ni
// sklada az POKEY v emulaci (Machine::tapeAdvance) rychlosti, kterou si
// nastavil OS - presne jako na skutecnem stroji.
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

namespace nap {

struct TapeImage {
  std::vector<uint64_t> w;     // urovne linky, 1 bit na vzorek
  size_t n = 0;                // pocet vzorku
  double rate = 0;             // vzorku za sekundu
  // diagnostika
  int channels = 0, bitsPerSample = 0, srcRate = 0, usedChannel = 0;
  long long bytes600 = 0, records600 = 0, framing600 = 0;
  // B298: chyby ramce JEN uvnitr zaznamu (mezi $55 $55 a kontrolnim
  // souctem) - ty by vadily. Chyby v mezerach mezi zaznamy (sum, dobeh
  // tonu) OS vubec necte, framing600 je pocita vsechny.
  long long framingRec600 = 0, recordsOk600 = 0, recordsBad600 = 0;
  // B298: co je na pasce - podle prvniho platneho zaznamu (kontrolni soucet OK)
  enum Druh { NEZNAMY = 0, BASIC = 1, BOOT = 2, TEXT = 3 };
  int druh = NEZNAMY;
  int prvniCtl = -1;                   // ridici bajt prvniho zaznamu ($FC/$FA/$FE)
  std::vector<uint8_t> prvniData;      // 128 datovych bajtu prvniho zaznamu
  double prvniCas = 0;                 // kde zacina (s)
  // B298: zvukova stopa (u stereo WAV druhy kanal) - skutecny magnetofon ji
  // pousti do televize (AUDIO IN). Prazdne = mono WAV / ticho.
  std::vector<int16_t> audio;
  double audioRate = 0;
  inline int bit(size_t i) const { return i < n ? (int)((w[i >> 6] >> (i & 63)) & 1) : 1; }
  inline void set(size_t i, int v) { if (v) w[i >> 6] |= (1ULL << (i & 63)); else w[i >> 6] &= ~(1ULL << (i & 63)); }
  double seconds() const { return rate > 0 ? n / rate : 0; }
};

// Precte WAV (PCM 8/16/24/32 bit, float 32, i WAVE_FORMAT_EXTENSIBLE) do
// float vzorku po kanalech. Vraci false + popis chyby.
inline bool napWavRead(const uint8_t *d, size_t len, std::vector<std::vector<float>> &ch, int &rate, int &bits, std::string &err) {
  auto u16 = [&](size_t o) { return (uint32_t)(d[o] | (d[o + 1] << 8)); };
  auto u32 = [&](size_t o) { return (uint32_t)(d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | ((uint32_t)d[o + 3] << 24)); };
  if (!d || len < 12 || std::memcmp(d, "RIFF", 4) != 0 || std::memcmp(d + 8, "WAVE", 4) != 0) { err = "neni WAV (chybi RIFF/WAVE)"; return false; }
  size_t p = 12;
  int fmt = 0, nch = 0; rate = 0; bits = 0;
  const uint8_t *data = nullptr; size_t dataLen = 0;
  while (p + 8 <= len) {
    uint32_t sz = u32(p + 4);
    const size_t body = p + 8;
    if (std::memcmp(d + p, "fmt ", 4) == 0 && body + 16 <= len) {
      fmt = (int)u16(body); nch = (int)u16(body + 2); rate = (int)u32(body + 4); bits = (int)u16(body + 14);
      if (fmt == 0xFFFE && sz >= 40 && body + 26 <= len) fmt = (int)u16(body + 24);   // EXTENSIBLE: podformat
    } else if (std::memcmp(d + p, "data", 4) == 0) {
      data = d + body;
      dataLen = std::min<size_t>(sz, len > body ? len - body : 0);
      if (sz == 0 || sz == 0xFFFFFFFFu) dataLen = len - body;   // neuzavreny zaznam
      break;
    }
    p = body + sz + (sz & 1);
  }
  if (!fmt || !data) { err = "WAV bez fmt/data"; return false; }
  if (fmt != 1 && fmt != 3) { err = "WAV neni PCM (format " + std::to_string(fmt) + ")"; return false; }
  if (nch < 1 || nch > 8 || rate < 8000 || rate > 400000) { err = "WAV ma divne parametry"; return false; }
  const int bps = bits / 8;
  if (!(bps == 1 || bps == 2 || bps == 3 || bps == 4) || (fmt == 3 && bps != 4)) { err = "WAV ma nepodporovanou bitovou hloubku " + std::to_string(bits); return false; }
  const size_t frames = dataLen / (size_t)(bps * nch);
  ch.assign((size_t)nch, std::vector<float>(frames));
  for (size_t f = 0; f < frames; f++) {
    for (int c = 0; c < nch; c++) {
      const uint8_t *s = data + (f * nch + c) * bps;
      float v;
      if (fmt == 3) { float x; std::memcpy(&x, s, 4); v = x; }
      else if (bps == 1) v = ((int)s[0] - 128) / 128.0f;
      else if (bps == 2) v = (int16_t)(s[0] | (s[1] << 8)) / 32768.0f;
      else if (bps == 3) { int32_t x = (int32_t)((uint32_t)s[0] << 8 | (uint32_t)s[1] << 16 | (uint32_t)s[2] << 24); v = (float)(x / 2147483648.0); }
      else { int32_t x; std::memcpy(&x, s, 4); v = (float)(x / 2147483648.0); }
      ch[(size_t)c][f] = v;
    }
  }
  return true;
}

// Energie tonu mark/space v klouzavem okne (1 kanal). Vraci sumu energii
// (pro volbu kanalu) a vyplni mark[]/space[].
inline double napFskEnergie(const std::vector<float> &x, int rate, std::vector<float> &mark, std::vector<float> &space) {
  const size_t n = x.size();
  mark.assign(n, 0.f); space.assign(n, 0.f);
  if (n == 0) return 0;
  int L = (int)std::lround(rate / 1332.0);          // okno = 1/(5327-3995) s
  if (L < 4) L = 4;
  const double PI = 3.14159265358979323846;
  const double wm = 2 * PI * 5327.0 / rate, ws = 2 * PI * 3995.0 / rate;
  // nosne rekurzivne (rotace komplexnim cislem), obcas prepocitat presne
  double cm = 1, sm = 0, cs = 1, ss = 0;
  const double rcm = std::cos(wm), rsm = std::sin(wm), rcs = std::cos(ws), rss = std::sin(ws);
  std::vector<float> bIm(L, 0.f), bQm(L, 0.f), bIs(L, 0.f), bQs(L, 0.f);
  double Im = 0, Qm = 0, Is = 0, Qs = 0, celkem = 0;
  // stejnosmerna slozka pryc (jednoduchy hornopropustny filtr ~20 Hz)
  double dcx = 0, dcy = 0;
  const double kdc = std::exp(-2 * PI * 20.0 / rate);
  for (size_t i = 0; i < n; i++) {
    if ((i & 1023) == 0) { cm = std::cos(wm * (double)i); sm = std::sin(wm * (double)i); cs = std::cos(ws * (double)i); ss = std::sin(ws * (double)i); }
    double v = x[i];
    double y = v - dcx + kdc * dcy; dcx = v; dcy = y; v = y;
    const int k = (int)(i % (size_t)L);
    const float im = (float)(v * cm), qm = (float)(v * sm), is = (float)(v * cs), qs = (float)(v * ss);
    Im += im - bIm[k]; Qm += qm - bQm[k]; Is += is - bIs[k]; Qs += qs - bQs[k];
    bIm[k] = im; bQm[k] = qm; bIs[k] = is; bQs[k] = qs;
    const double em = Im * Im + Qm * Qm, es = Is * Is + Qs * Qs;
    mark[i] = (float)em; space[i] = (float)es;
    celkem += em + es;
    // posun nosnych
    double t;
    t = cm * rcm - sm * rsm; sm = sm * rcm + cm * rsm; cm = t;
    t = cs * rcs - ss * rss; ss = ss * rcs + cs * rss; cs = t;
  }
  return celkem;
}

// WAV -> urovne linky DATA IN. Vraci false + popis chyby.
inline bool napTapeFromWav(const uint8_t *d, size_t len, TapeImage &t, std::string &err) {
  std::vector<std::vector<float>> ch; int rate = 0, bits = 0;
  if (!napWavRead(d, len, ch, rate, bits, err)) return false;
  if (ch.empty() || ch[0].size() < (size_t)rate / 10) { err = "WAV je prilis kratky"; return false; }
  // kanal s nejvetsi energii FSK
  std::vector<float> mark, space, bm, bs;
  double best = -1; int bestCh = 0;
  for (size_t c = 0; c < ch.size(); c++) {
    std::vector<float> m, s;
    double e = napFskEnergie(ch[c], rate, m, s);
    if (e > best) { best = e; bestCh = (int)c; bm.swap(m); bs.swap(s); }
  }
  mark.swap(bm); space.swap(bs);
  const size_t n = mark.size();
  // prah ticha: 3 % z "typicke" energie signalu (95. percentil, 1 ze 64 vzorku)
  std::vector<float> vz; vz.reserve(n / 64 + 1);
  for (size_t i = 0; i < n; i += 64) vz.push_back(mark[i] + space[i]);
  std::sort(vz.begin(), vz.end());
  const float p95 = vz.empty() ? 0.f : vz[(size_t)(vz.size() * 0.95)];
  const float prah = p95 * 0.03f;
  if (p95 <= 0) { err = "WAV je ticho - zadny signal kazety"; return false; }
  t = TapeImage();
  t.n = n; t.rate = rate; t.w.assign((n + 63) / 64, 0);
  t.channels = (int)ch.size(); t.bitsPerSample = bits; t.srcRate = rate; t.usedChannel = bestCh;
  // B298: zvukova stopa = druhy kanal stereo WAV (ne ten s daty). Kdyz je
  // v nem jen ticho, nic se neuklada. Nad 30 kHz se ulozi s polovicnim
  // vzorkovanim (staci na hlas/hudbu, setri pamet), nejvys 20 minut.
  if (ch.size() >= 2) {
    int ac = -1; double nej = 0;
    for (size_t c = 0; c < ch.size(); c++) {
      if ((int)c == bestCh) continue;
      double s = 0; const size_t krok = 7;
      for (size_t i = 0; i < ch[c].size(); i += krok) s += (double)ch[c][i] * ch[c][i];
      const double rms = std::sqrt(s / (double)std::max<size_t>(1, ch[c].size() / krok));
      if (rms > nej) { nej = rms; ac = (int)c; }
    }
    if (ac >= 0 && nej > 0.004) {
      const int dec = rate > 30000 ? 2 : 1;
      const std::vector<float> &x = ch[(size_t)ac];
      size_t m = x.size() / (size_t)dec;
      const size_t maxM = (size_t)(20 * 60 * (double)rate / dec);
      if (m > maxM) m = maxM;
      t.audio.resize(m);
      for (size_t i = 0; i < m; i++) {
        float v = 0;
        for (int k = 0; k < dec; k++) v += x[i * (size_t)dec + (size_t)k];
        v /= (float)dec;
        if (v > 1.f) v = 1.f; else if (v < -1.f) v = -1.f;
        t.audio[i] = (int16_t)std::lround(v * 32767.0);
      }
      t.audioRate = (double)rate / dec;
    }
  }
  int lvl = 1;
  for (size_t i = 0; i < n; i++) {
    const float em = mark[i], es = space[i];
    if (em + es < prah) lvl = 1;                       // ticho = klidova uroven
    else if (lvl == 1 && es > em * 1.15f) lvl = 0;     // mala hystereze proti drnceni
    else if (lvl == 0 && em > es * 1.15f) lvl = 1;
    t.set(i, lvl);
  }
  // diagnostika: kolik bajtu / zaznamu je na pasce pri 600 baudech
  {
    // zaznam = mezera (>= 50 ms bez bajtu) a pak bajty $55 $55, ridici bajt,
    // 128 dat, kontrolni soucet (soucet s prenosem jako v OS)
    const double bitT = rate / 600.0;
    int stav = 0; double dalsi = 0; int bit = 0, bajt = 0, posl = 1;
    size_t konecPosl = 0, zacatek = 0; const size_t mezera = (size_t)(rate * 0.05);
    std::vector<uint8_t> rec; rec.reserve(160);
    bool synch = false, vyhodnocen = false;
    auto uzavri = [&]() {
      // konec zaznamu (mezera): kontrolni soucet a prvni platny zaznam
      if (synch && rec.size() >= 132 && !vyhodnocen) {
        unsigned s = 0;
        for (int k = 0; k < 131; k++) { s += rec[(size_t)k]; if (s > 255) s = (s & 0xFF) + 1; }
        if ((uint8_t)s == rec[131]) {
          t.recordsOk600++;
          if (t.prvniCtl < 0 && (rec[2] == 0xFC || rec[2] == 0xFA)) {
            t.prvniCtl = rec[2];
            t.prvniData.assign(rec.begin() + 3, rec.begin() + 131);
            t.prvniCas = (double)zacatek / rate;
          }
        } else t.recordsBad600++;
      } else if (synch && !vyhodnocen) t.recordsBad600++;
      vyhodnocen = true;
    };
    for (size_t i = 0; i < n; i++) {
      const int v = t.bit(i);
      if (stav == 0) {
        if (posl == 1 && v == 0) {
          stav = 1; dalsi = (double)i + bitT * 0.5;
          if (i - konecPosl >= mezera || t.bytes600 == 0) { uzavri(); rec.clear(); synch = false; vyhodnocen = false; zacatek = i; }
        }
      } else if ((double)i >= dalsi) {
        if (stav == 1) { if (v == 0) { stav = 2; bit = 0; bajt = 0; dalsi += bitT; } else stav = 0; }
        else if (bit < 8) { if (v) bajt |= 1 << bit; bit++; dalsi += bitT; }
        else {
          if (v) {
            t.bytes600++;
            if (rec.size() < 160) rec.push_back((uint8_t)bajt);
            if (rec.size() == 2 && rec[0] == 0x55 && rec[1] == 0x55) { t.records600++; synch = true; }
            if (rec.size() == 132) uzavri();
          } else {
            t.framing600++;
            if (synch && rec.size() >= 2 && rec.size() < 132) { t.framingRec600++; if (rec.size() < 160) rec.push_back((uint8_t)bajt); }
          }
          konecPosl = i;
          stav = 0;
        }
      }
      posl = v;
    }
    uzavri();
    // co je na pasce (podle prvniho platneho zaznamu)
    if (t.prvniData.size() == 128) {
      const uint8_t *dd = t.prvniData.data();
      int tisk = 0;
      for (int k = 0; k < 12; k++) if ((dd[k] >= 0x20 && dd[k] < 0x7F) || dd[k] == 0x9B) tisk++;
      if (dd[0] == 0 && dd[1] == 0) t.druh = TapeImage::BASIC;                       // CSAVE: LOMEM 0, VNT...
      else if (dd[0] >= '0' && dd[0] <= '9' && tisk >= 10) t.druh = TapeImage::TEXT;  // LIST "C:" (ENTER)
      else if (dd[1] >= 1) t.druh = TapeImage::BOOT;                                 // bootovaci soubor: pocet zaznamu, adresa
    }
  }
  return true;
}

// B298: popis druhu kazety pro log / stavovy radek
inline const char *napTapeDruh(const TapeImage &t) {
  switch (t.druh) {
    case TapeImage::BASIC: return "BASIC program (CLOAD)";
    case TapeImage::BOOT: return "bootovaci (hra - START+OPTION pri zapnuti)";
    case TapeImage::TEXT: return "vypis BASIC jako text (ENTER \"C:\")";
    default: return "neznamy (prvni zaznam neprecten)";
  }
}

}  // namespace nap
