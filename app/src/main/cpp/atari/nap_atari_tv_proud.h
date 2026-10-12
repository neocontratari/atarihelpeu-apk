// nap_atari_tv_proud.h
// B303: PRENOS ATARI 130XE (C++) NA TV / PC S MALYM ZPOZDENIM - CELY V C++.
//
// Rene: "ma to na TV zpozdeni - nejde to vylepsit? Chtelo by to mensi lagy.
// Opravdu prevod jede jak ma v C++?" Nejel: v B301 dalo C++ jen hotovy
// snimek a Java ho zvetsovala na 1280x720 a komprimovala (H.264 / JPEG) na
// hlavnim vlakne appky - v jeho logu snimek jen kazdych 36 ms, JPEG 27-33 ms
// na snimek (~14 snimku/s) a prohlizec si H.264 jeste bufferoval.
//
// Ted: kazdy snimek Atari (50/s) jde do prohlizece tak, jak je - v barvach
// Atari (paleta, nejvys 256 barev, posila se jen nova barva) a JEN ZMENENE
// RADKY (a z nich jen zmeneny usek, RLE nebo primo). Staticka obrazovka
// BASICu = par bajtu, hra = jednotky az desitky kB. Zvuk jde ve stejnem
// paketu (stereo int16 44100 Hz), takze obraz a zvuk prijdou spolu.
// Prohlizec paket rozbali a hned nakresli - zadny enkoder, zadny buffer.
//
// Bez Androidu - testuje se na PC (test_core tv-proud, prohlizec v Chromiu).
//
// PAKET (little endian):
//   0  'N' 'A'           znacka
//   2  u8  verze (1)
//   3  u8  priznaky      bit0 = klicovy (cely obraz od znovu), bit1 = POWER vypnuto
//   4  u16 W, u16 H      rozmer obrazu (672 x 240 = vyrez jako na displeji telefonu)
//   8  u32 cislo snimku jadra
//  12  u16 palOd, u16 palPocet, pak palPocet x (R,G,B) - barvy pridane do palety
//      radku: u16, pak kazdy radek: u16 y, u16 x0, u16 n, u8 typ, u16 delka, data
//        typ bity 0-1: 0 = primo (indexy), 1 = RLE (dvojice pocet 1..255, index),
//                      2 = cely radek stejny jako radek nad nim (zadna data)
//        typ bity 2-3: sirka bodu 1 / 2 / 4 (0 / 1 / 2) - v datech je kazdy f-ty
//                      bod (Atari ma body 2 a vic sloupcu siroke, VBXE LR 4)
//      zvuk: u16 pocet vzorku, u8 kanalu (1 = mono: L = R, 2 = stereo), pak int16
//      (Atari hraje mono, stereo jen zvukova stopa kazety - mono = polovicni tok)
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace nap {

// Stav jednoho prohlizece: co uz ma (obraz v indexech a paletu), at se posila
// jen rozdil. Kazdy prohlizec ma svuj - kdyz nestiha, dostane proste
// posledni snimek oproti tomu, co ma (zadna fronta, zadne zpozdeni).
struct TvProudKlient {
  int W = 0, H = 0;
  std::vector<uint8_t> obraz;          // indexy barev, ktere prohlizec ma (W*H); prazdne = nema nic
  std::vector<uint32_t> paleta;        // 0xRRGGBB podle indexu (nejvys 256)
  std::vector<uint8_t> idx;            // pracovni: snimek prevedeny na indexy (jen zmenene radky)
  std::vector<uint32_t> predRgb;       // posledni poslany snimek v RGB (zmeneny radek = jiny nez tady)
  std::vector<uint8_t> zmena;          // pracovni: ktere radky se zmenily
  std::vector<uint8_t> pom;            // pracovni: usek radku
  static const int HK = 4096;          // hash barva -> index (otevrene adresovani)
  std::vector<uint32_t> hKlic;         // barva + 1 (0 = volno)
  std::vector<uint8_t> hIdx;
  int hPocet = 0;
  unsigned long long snimek = 0;       // posledni poslany snimek jadra (0 = zadny)
  bool zapnuto = true;                 // posledni poslany stav POWER
  unsigned long long zvukPoz = 0;      // pozice ve fronte zvuku (shorty, absolutne)
  bool zvukJede = false;
  // statistika (do logu)
  long long paketu = 0, bajtu = 0, klicovych = 0, radkuCelkem = 0;

  TvProudKlient() { hashReset(); }
  void hashReset() { hKlic.assign(HK, 0); hIdx.assign(HK, 0); hPocet = 0; }
  // prohlizec nema nic (paket se k nemu nedostal) - pristi paket je klicovy
  void zapomen() { obraz.clear(); predRgb.clear(); paleta.clear(); hashReset(); }
  static uint32_t hs(uint32_t c) { return (c * 2654435761u) >> 20; }   // 12 bitu
  int hledej(uint32_t c) const {
    for (uint32_t i = 0, s = hs(c); i < (uint32_t)HK; i++, s = (s + 1) & (HK - 1)) {
      if (hKlic[s] == 0) return -1;
      if (hKlic[s] == c + 1) return hIdx[s];
    }
    return -1;
  }
  void vloz(uint32_t c, int index) {
    if (hPocet >= HK * 3 / 4) return;  // plna tabulka (tisice barev) - pak se jen hleda nejblizsi
    for (uint32_t s = hs(c);; s = (s + 1) & (HK - 1)) {
      if (hKlic[s] == 0) { hKlic[s] = c + 1; hIdx[s] = (uint8_t)index; hPocet++; return; }
      if (hKlic[s] == c + 1) return;
    }
  }
  int nejblizsi(uint32_t c) const {
    int best = 0; long bd = 0x7FFFFFFF;
    const int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    for (size_t i = 0; i < paleta.size(); i++) {
      const uint32_t p = paleta[i];
      const long dr = r - (int)((p >> 16) & 255), dg = g - (int)((p >> 8) & 255), db = b - (int)(p & 255);
      const long d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
      if (d < bd) { bd = d; best = (int)i; }
    }
    return best;
  }
  // body [od, od+n) -> indexy. nejblizsi = false: kdyz je paleta plna a prijde
  // nova barva, vrati false (volajici zalozi paletu znovu z celeho snimku);
  // nejblizsi = true: barva navic dostane nejblizsi (vic nez 256 barev v jednom
  // snimku - VBXE prechody).
  bool naIndexy(const uint32_t *rgb, size_t od, size_t n, bool nejblizsiDovolen) {
    uint32_t posl = 0xFFFFFFFFu; int poslI = 0;
    for (size_t i = od; i < od + n; i++) {
      const uint32_t c = rgb[i] & 0xFFFFFFu;
      if (c != posl) {
        int k = hledej(c);
        if (k < 0) {
          if (paleta.size() < 256) { k = (int)paleta.size(); paleta.push_back(c); vloz(c, k); }
          else if (!nejblizsiDovolen) return false;
          else { k = nejblizsi(c); vloz(c, k); }
        }
        posl = c; poslI = k;
      }
      idx[i] = (uint8_t)poslI;
    }
    return true;
  }
};

namespace tvproud {
inline void u8(std::vector<uint8_t> &o, unsigned v) { o.push_back((uint8_t)v); }
inline void u16(std::vector<uint8_t> &o, unsigned v) { o.push_back((uint8_t)v); o.push_back((uint8_t)(v >> 8)); }
inline void u32(std::vector<uint8_t> &o, unsigned long v) { u16(o, (unsigned)(v & 0xFFFF)); u16(o, (unsigned)((v >> 16) & 0xFFFF)); }
inline size_t rleDelka(const uint8_t *p, int n) {
  size_t b = 0;
  for (int i = 0; i < n;) { int j = i + 1; while (j < n && p[j] == p[i] && j - i < 255) j++; b += 2; i = j; }
  return b;
}
// jsou v useku [a, a+n) vsechny skupiny po f bodech (zarovnane od 0) stejne?
inline bool skupinyStejne(const uint8_t *r, int a, int n, int f) {
  for (int x = a; x < a + n; x += f)
    for (int i = 1; i < f; i++) if (r[x + i] != r[x]) return false;
  return true;
}
}  // namespace tvproud

// Jeden paket pro prohlizec: rgb = snimek 0xRRGGBB (W*H), zvuk = stereo int16
// (zvukShortu sude). Klient si zapamatuje, co poslal.
inline void tvProudPaket(TvProudKlient &k, const uint32_t *rgb, int W, int H, unsigned long long seq, bool zapnuto,
                         const int16_t *zvuk, size_t zvukShortu, std::vector<uint8_t> &out) {
  using namespace tvproud;
  out.clear();
  const size_t WH = (size_t)W * H;
  if (k.W != W || k.H != H) { k.W = W; k.H = H; k.obraz.clear(); k.paleta.clear(); k.hashReset(); }
  k.idx.resize(WH);
  bool klic = k.obraz.empty() || k.predRgb.size() != WH;
  size_t palOd = k.paleta.size();
  // 1) ktere radky se zmenily (porovnani v RGB je rychle - staticka obrazovka
  //    BASICu tak stoji jen porovnani pameti, zadny prevod barev)
  k.zmena.assign((size_t)H, 1);
  if (!klic)
    for (int y = 0; y < H; y++)
      k.zmena[(size_t)y] = std::memcmp(rgb + (size_t)y * W, &k.predRgb[(size_t)y * W], (size_t)W * 4) != 0;
  // 2) zmenene radky na indexy; plna paleta a nova barva = paleta znovu z celeho snimku
  bool ok = true;
  for (int y = 0; y < H && ok; y++)
    if (k.zmena[(size_t)y]) ok = k.naIndexy(rgb, (size_t)y * W, (size_t)W, false);
  if (!ok) {
    k.paleta.clear(); k.hashReset();
    klic = true; palOd = 0;
    k.naIndexy(rgb, 0, WH, true);
  }
  if (klic) { k.obraz.assign(WH, 0); k.zmena.assign((size_t)H, 1); palOd = 0; }   // klicovy: i cela paleta
  const size_t palPocet = k.paleta.size() - palOd;
  out.reserve(64 + palPocet * 3 + (size_t)H * 16 + zvukShortu * 2);
  u8(out, 'N'); u8(out, 'A'); u8(out, 1);
  u8(out, (klic ? 1u : 0u) | (zapnuto ? 0u : 2u));
  u16(out, (unsigned)W); u16(out, (unsigned)H);
  u32(out, (unsigned long)(seq & 0xFFFFFFFFull));
  u16(out, (unsigned)palOd); u16(out, (unsigned)palPocet);
  for (size_t i = palOd; i < k.paleta.size(); i++) {
    const uint32_t c = k.paleta[i];
    u8(out, (c >> 16) & 255); u8(out, (c >> 8) & 255); u8(out, c & 255);
  }
  const size_t radkuPoz = out.size();
  u16(out, 0);
  unsigned radku = 0;
  for (int y = 0; y < H; y++) {
    if (!k.zmena[(size_t)y]) continue;           // v RGB beze zmeny
    const uint8_t *nr = &k.idx[(size_t)y * W];
    uint8_t *sr = &k.obraz[(size_t)y * W];
    int a = 0, b = W - 1;
    if (!klic) {
      while (a < W && nr[a] == sr[a]) a++;
      if (a == W) continue;                       // radek beze zmeny
      while (b > a && nr[b] == sr[b]) b--;
    }
    // cely radek stejny jako radek nad nim (Atari: bod pres 2 a vic radku);
    // radek nad nim musi byt v idx platny (zmeneny), jinak se porovna s obrazem klienta
    const uint8_t *nad = (y > 0) ? (k.zmena[(size_t)y - 1] ? nr - W : sr - W) : nullptr;
    if (nad && std::memcmp(nr, nad, (size_t)W) == 0) {
      u16(out, (unsigned)y); u16(out, 0); u16(out, (unsigned)W); u8(out, 2u); u16(out, 0);
      std::memcpy(sr, nr, (size_t)W);
      radku++;
      continue;
    }
    // sirka bodu: 4 nebo 2 sloupce stejne (zarovnane) -> posle se kazdy f-ty
    int f = 1, fk = 0;
    for (int zk = 2; zk >= 1; zk--) {
      const int ff = 1 << zk;
      const int aa = a - a % ff, bb = b - b % ff + ff - 1;
      if (bb < W && skupinyStejne(nr, aa, bb - aa + 1, ff)) { f = ff; fk = zk; a = aa; b = bb; break; }
    }
    const int n = b - a + 1, m = n / f;
    std::vector<uint8_t> &sek = k.pom; sek.resize((size_t)m);
    for (int i = 0; i < m; i++) sek[(size_t)i] = nr[a + i * f];
    const size_t rle = rleDelka(sek.data(), m);
    const bool pouzijRle = rle < (size_t)m;
    u16(out, (unsigned)y); u16(out, (unsigned)a); u16(out, (unsigned)n);
    u8(out, (pouzijRle ? 1u : 0u) | ((unsigned)fk << 2));
    u16(out, (unsigned)(pouzijRle ? rle : (size_t)m));
    if (pouzijRle) {
      for (int i = 0; i < m;) {
        int j = i + 1; while (j < m && sek[(size_t)j] == sek[(size_t)i] && j - i < 255) j++;
        u8(out, (unsigned)(j - i)); u8(out, sek[(size_t)i]);
        i = j;
      }
    } else {
      out.insert(out.end(), sek.begin(), sek.end());
    }
    std::memcpy(sr + a, nr + a, (size_t)n);
    radku++;
  }
  out[radkuPoz] = (uint8_t)radku; out[radkuPoz + 1] = (uint8_t)(radku >> 8);
  if (k.predRgb.size() != WH) k.predRgb.assign(rgb, rgb + WH);
  else for (int y = 0; y < H; y++) if (k.zmena[(size_t)y]) std::memcpy(&k.predRgb[(size_t)y * W], rgb + (size_t)y * W, (size_t)W * 4);
  const size_t vzorku = zvukShortu / 2;
  bool mono = true;
  for (size_t i = 0; i < vzorku && mono; i++) if (zvuk[2 * i] != zvuk[2 * i + 1]) mono = false;
  u16(out, (unsigned)vzorku); u8(out, mono ? 1u : 2u);
  if (mono) for (size_t i = 0; i < vzorku; i++) u16(out, (unsigned)(uint16_t)zvuk[2 * i]);
  else for (size_t i = 0; i < vzorku * 2; i++) u16(out, (unsigned)(uint16_t)zvuk[i]);
  k.snimek = seq; k.zapnuto = zapnuto;
  k.paketu++; k.bajtu += (long long)out.size(); k.radkuCelkem += radku;
  if (klic) k.klicovych++;
}

// Rozbaleni paketu (stejne jako prohlizec) - pro testy na PC.
struct TvProudDekoder {
  int W = 0, H = 0;
  std::vector<uint8_t> idx;
  uint32_t pal[256] = {0};
  std::vector<int16_t> zvuk;           // zvuk z posledniho paketu (vzdy stereo)
  unsigned kanalu = 0;                 // jak prisel (1 mono, 2 stereo)
  bool klic = false, vypnuto = false;
  unsigned long seq = 0;
  unsigned radku = 0;
  bool paket(const uint8_t *p, size_t n) {
    size_t o = 0;
    auto r8 = [&](unsigned &v) { if (o + 1 > n) return false; v = p[o]; o += 1; return true; };
    auto r16 = [&](unsigned &v) { if (o + 2 > n) return false; v = p[o] | (p[o + 1] << 8); o += 2; return true; };
    unsigned a, b, c, d;
    if (n < 16 || p[0] != 'N' || p[1] != 'A' || p[2] != 1) return false;
    klic = (p[3] & 1) != 0; vypnuto = (p[3] & 2) != 0;
    o = 4; r16(a); r16(b);
    if ((int)a != W || (int)b != H) { W = (int)a; H = (int)b; idx.assign((size_t)W * H, 0); }
    seq = (unsigned long)(p[8] | (p[9] << 8) | (p[10] << 16)) | ((unsigned long)p[11] << 24); o = 12;
    unsigned palOd, palPocet;
    if (!r16(palOd) || !r16(palPocet) || palOd + palPocet > 256) return false;
    for (unsigned i = 0; i < palPocet; i++) {
      if (!r8(a) || !r8(b) || !r8(c)) return false;
      pal[palOd + i] = (a << 16) | (b << 8) | c;
    }
    if (!r16(radku)) return false;
    for (unsigned rr = 0; rr < radku; rr++) {
      unsigned y, x0, cnt, typ, del;
      if (!r16(y) || !r16(x0) || !r16(cnt) || !r8(typ) || !r16(del)) return false;
      if ((int)y >= H || (int)(x0 + cnt) > W || o + del > n) return false;
      uint8_t *dst = &idx[(size_t)y * W + x0];
      const unsigned druh = typ & 3, f = 1u << ((typ >> 2) & 3);
      if (druh == 2) { if (y == 0) return false; std::memcpy(&idx[(size_t)y * W], &idx[(size_t)(y - 1) * W], (size_t)W); continue; }
      if (cnt % f) return false;
      const unsigned m = cnt / f;
      std::vector<uint8_t> sek(m);
      if (druh == 0) { if (del != m) return false; std::memcpy(sek.data(), p + o, m); o += del; }
      else if (druh == 1) {
        unsigned w = 0; const size_t konec = o + del;
        if (del & 1) return false;
        while (o + 2 <= konec) { c = p[o]; d = p[o + 1]; o += 2; for (unsigned i = 0; i < c && w < m; i++) sek[w++] = (uint8_t)d; }
        if (w != m) return false;
      } else return false;
      for (unsigned i = 0; i < m; i++) for (unsigned j = 0; j < f; j++) dst[i * f + j] = sek[i];
    }
    unsigned kan;
    if (!r16(a) || !r8(kan) || (kan != 1 && kan != 2)) return false;
    kanalu = kan;
    if (o + (size_t)a * 2 * kan > n) return false;
    zvuk.resize((size_t)a * 2);
    for (size_t i = 0; i < (size_t)a; i++) {
      for (unsigned ch = 0; ch < kan; ch++) { zvuk[2 * i + ch] = (int16_t)(p[o] | (p[o + 1] << 8)); o += 2; }
      if (kan == 1) zvuk[2 * i + 1] = zvuk[2 * i];
    }
    return o == n;
  }
  uint32_t barva(int x, int y) const { return pal[idx[(size_t)y * W + x]]; }
};

}  // namespace nap
