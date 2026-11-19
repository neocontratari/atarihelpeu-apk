// nap_ppmd.h
// B351: PPMd var.H - "komprese textu" v RAR 2.9 (WinRAR 3 / 4, volba "nejlepsi"). Model
// predikce + rozsahovy dekoder RAR, cele v C++ bez knihoven.
//
// Podle verejnych (public domain) algoritmu a kodu:
//   PPMd var.H (2001) - Dmitry Shkarin, public domain
//   Ppmd7.c / Ppmd7aDec.c (LZMA SDK, 7-Zip; i v libarchive) - Igor Pavlov, public domain
//   carryless rangecoder (1999) - Dmitry Subbotin, public domain
// Prepsano do C++: pamet modelu je pole bajtu a "ukazatele" jsou posuny (offsety) od jeho
// zacatku; kazdy pristup do pole je hlidany a kazda smycka ma strop - poskozeny archiv nesmi
// appku shodit ani zaseknout (chyba = dekoduj() vrati -2).
#pragma once
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

namespace nap {
namespace archiv {

class Ppmd7 {
 public:
  // vstup rozsahoveho dekoderu: bajty souboru RAR, posun 'adr' patri dekoderu RAR 2.9
  struct Vstup {
    const uint8_t *d = nullptr; size_t n = 0; size_t *adr = nullptr;
    uint8_t cti() { const size_t a = (*adr)++; return a < n ? d[a] : 0; }
  };

  Ppmd7() { tabulky(); }

  // pamet modelu v bajtech (RAR: (MaxMB + 1) MB)
  bool alokuj(uint32_t velikost) {
    if (velikost < UNIT || velikost > (256u << 20)) return false;
    if (!pam.empty() && Size == velikost) return true;
    AlignOffset = 4 - (velikost & 3);
    try { pam.assign((size_t)AlignOffset + velikost + UNIT, 0); } catch (const std::bad_alloc &) { pam.clear(); Size = 0; return false; }
    Size = velikost;
    return true;
  }
  bool alokovano() const { return !pam.empty(); }

  void init(unsigned maxRad) {
    MaxOrder = maxRad;
    spatne = false;
    RestartModel();
    DummySee.Shift = PERIOD_BITS; DummySee.Summ = 0; DummySee.Count = 64;
  }

  // RAR (Ppmd7a): Code ze 4 bajtu, Bottom 0x8000
  bool initDekoder(Vstup *v) {
    in = v;
    Low = 0; Range = 0xFFFFFFFFu; Code = 0;
    for (int i = 0; i < 4; i++) Code = (Code << 8) | in->cti();
    Bottom = 0x8000;
    return true;
  }

  // dalsi znak: 0-255, -1 = unik z korenoveho kontextu (v RAR chyba), -2 = poskozena data
  int dekoduj() {
    if (spatne || pam.empty() || !in) return -2;
    const int r = dekodujSymbol();
    return spatne ? -2 : r;
  }

 protected:                                      // (testy B351 z modelu odvozuji koder - test_overeni/b351)
  static constexpr uint32_t UNIT = 12, MAX_FREQ = 124;
  static constexpr int NUM_INDEXES = 38, PERIOD_BITS = 7, INT_BITS = 7, MAX_O = 64;
  static constexpr uint32_t BIN_SCALE = 1u << (INT_BITS + PERIOD_BITS);
  struct See { uint16_t Summ = 0; uint8_t Shift = 0; uint8_t Count = 0; };
  struct Stav { uint8_t sym = 0, freq = 0; uint32_t succ = 0; };

  std::vector<uint8_t> pam;                     // pamet modelu (Base); posun 0 = "nic"
  uint32_t Size = 0, AlignOffset = 0;
  uint32_t Text = 0, UnitsStart = 0, LoUnit = 0, HiUnit = 0, GlueCount = 0;
  uint32_t FreeList[NUM_INDEXES] = {};
  uint8_t Indx2Units[NUM_INDEXES] = {}, Units2Indx[128] = {};
  uint8_t NS2Indx[256] = {}, NS2BSIndx[256] = {}, HB2Flag[256] = {};
  uint32_t MinContext = 0, MaxContext = 0, FoundState = 0;
  unsigned OrderFall = 0, InitEsc = 0, PrevSuccess = 0, MaxOrder = 0, HiBitsFlag = 0;
  int32_t RunLength = 0, InitRL = 0;
  uint16_t BinSumm[128][64] = {};
  See SeeTab[25][16], DummySee;
  uint32_t Range = 0, Code = 0, Low = 0, Bottom = 0;
  Vstup *in = nullptr;
  mutable bool spatne = false;

  // ---- hlidany pristup do pameti modelu ----
  uint8_t b(uint32_t o) const { if (o >= pam.size()) { spatne = true; return 0; } return pam[o]; }
  void sb(uint32_t o, uint8_t v) { if (o >= pam.size()) { spatne = true; return; } pam[o] = v; }
  uint16_t w(uint32_t o) const { if ((uint64_t)o + 2 > pam.size()) { spatne = true; return 0; } return (uint16_t)(pam[o] | (pam[o + 1] << 8)); }
  void sw(uint32_t o, uint32_t v) { if ((uint64_t)o + 2 > pam.size()) { spatne = true; return; } pam[o] = (uint8_t)v; pam[o + 1] = (uint8_t)(v >> 8); }
  uint32_t d(uint32_t o) const { if ((uint64_t)o + 4 > pam.size()) { spatne = true; return 0; } return (uint32_t)pam[o] | ((uint32_t)pam[o + 1] << 8) | ((uint32_t)pam[o + 2] << 16) | ((uint32_t)pam[o + 3] << 24); }
  void sd(uint32_t o, uint32_t v) { if ((uint64_t)o + 4 > pam.size()) { spatne = true; return; } pam[o] = (uint8_t)v; pam[o + 1] = (uint8_t)(v >> 8); pam[o + 2] = (uint8_t)(v >> 16); pam[o + 3] = (uint8_t)(v >> 24); }
  void kopiruj12(uint32_t cil, uint32_t zdroj, uint32_t jednotek) {
    const uint64_t n = (uint64_t)jednotek * UNIT;
    if (cil + n > pam.size() || zdroj + n > pam.size()) { spatne = true; return; }
    std::memmove(pam.data() + cil, pam.data() + zdroj, (size_t)n);
  }
  // kontext (12 B): NumStats, SummFreq, Stats, Suffix; jediny stav lezi na +2 (prekryva SummFreq + Stats)
  unsigned NumStats(uint32_t c) const { return w(c); }
  void setNumStats(uint32_t c, unsigned v) { sw(c, v); }
  unsigned SummFreq(uint32_t c) const { return w(c + 2); }
  void setSummFreq(uint32_t c, unsigned v) { sw(c + 2, v); }
  uint32_t Stats(uint32_t c) const { return d(c + 4); }
  void setStats(uint32_t c, uint32_t v) { sd(c + 4, v); }
  uint32_t Suffix(uint32_t c) const { return d(c + 8); }
  void setSuffix(uint32_t c, uint32_t v) { sd(c + 8, v); }
  static uint32_t OneState(uint32_t c) { return c + 2; }
  // stav (6 B): Symbol, Freq, Successor (2 x 16 bitu)
  unsigned Sym(uint32_t s) const { return b(s); }
  unsigned Freq(uint32_t s) const { return b(s + 1); }
  void setSym(uint32_t s, unsigned v) { sb(s, (uint8_t)v); }
  void setFreq(uint32_t s, unsigned v) { sb(s + 1, (uint8_t)v); }
  uint32_t Succ(uint32_t s) const { return (uint32_t)w(s + 2) | ((uint32_t)w(s + 4) << 16); }
  void setSucc(uint32_t s, uint32_t v) { sw(s + 2, v & 0xFFFF); sw(s + 4, v >> 16); }
  Stav nacti(uint32_t s) const { Stav t; t.sym = (uint8_t)Sym(s); t.freq = (uint8_t)Freq(s); t.succ = Succ(s); return t; }
  void zapis(uint32_t s, const Stav &t) { setSym(s, t.sym); setFreq(s, t.freq); setSucc(s, t.succ); }
  void prohod(uint32_t a, uint32_t c) { const Stav t = nacti(a); zapis(a, nacti(c)); zapis(c, t); }
  bool platnyKontext(uint32_t c) const { return c > Text && (uint64_t)c + UNIT <= (uint64_t)AlignOffset + Size + UNIT; }

  uint32_t U2B(uint32_t nu) const { return nu * UNIT; }
  unsigned U2I(unsigned nu) const { return (nu >= 1 && nu <= 128) ? Units2Indx[nu - 1] : (spatne = true, 0u); }
  unsigned I2U(unsigned i) const { return i < (unsigned)NUM_INDEXES ? Indx2Units[i] : (spatne = true, 1u); }

  void tabulky() {
    for (unsigned i = 0, k = 0; i < (unsigned)NUM_INDEXES; i++) {
      unsigned step = (i >= 12 ? 4 : (i >> 2) + 1);
      do { Units2Indx[k++] = (uint8_t)i; } while (--step);
      Indx2Units[i] = (uint8_t)k;
    }
    NS2BSIndx[0] = 0 << 1; NS2BSIndx[1] = 1 << 1;
    std::memset(NS2BSIndx + 2, 2 << 1, 9);
    std::memset(NS2BSIndx + 11, 3 << 1, 256 - 11);
    unsigned i = 0;
    for (; i < 3; i++) NS2Indx[i] = (uint8_t)i;
    for (unsigned m = i, k = 1; i < 256; i++) {
      NS2Indx[i] = (uint8_t)m;
      if (--k == 0) k = (++m) - 2;
    }
    std::memset(HB2Flag, 0, 0x40);
    std::memset(HB2Flag + 0x40, 8, 0x100 - 0x40);
  }

  // ---- alokator po jednotkach 12 B ----
  void InsertNode(uint32_t node, unsigned indx) { if (indx >= (unsigned)NUM_INDEXES) { spatne = true; return; } sd(node, FreeList[indx]); FreeList[indx] = node; }
  uint32_t RemoveNode(unsigned indx) { const uint32_t node = FreeList[indx]; FreeList[indx] = d(node); return node; }
  void SplitBlock(uint32_t ptr, unsigned oldIndx, unsigned newIndx) {
    const unsigned nu = I2U(oldIndx) - I2U(newIndx);
    ptr += U2B(I2U(newIndx));
    unsigned i = U2I(nu);
    if (I2U(i) != nu) {
      const unsigned k = I2U(--i);
      InsertNode(ptr + U2B(k), nu - k - 1);
    }
    InsertNode(ptr, i);
  }
  void GlueFreeBlocks() {
    // uzel: Stamp (+0, 0 = volny), NU (+2), Next (+4), Prev (+8)
    const uint32_t head = AlignOffset + Size;
    uint32_t n = head;
    GlueCount = 255;
    for (unsigned i = 0; i < (unsigned)NUM_INDEXES; i++) {
      const unsigned nu = I2U(i);
      uint32_t next = FreeList[i];
      FreeList[i] = 0;
      unsigned strop = 0;
      while (next != 0) {
        if (++strop > (1u << 24)) { spatne = true; return; }
        const uint32_t node = next;
        sd(node + 4, n);              // node->Next = n
        sd(n + 8, next);              // NODE(n)->Prev = next
        n = next;
        next = d(node);               // dalsi volny (byl na +0)
        sw(node, 0);                  // Stamp = 0
        sw(node + 2, nu);             // NU
        if (spatne) return;
      }
    }
    sw(head, 1);
    sd(head + 4, n);
    sd(n + 8, head);
    if (LoUnit != HiUnit) sw(LoUnit, 1);
    // slepit sousedni volne bloky
    unsigned strop = 0;
    while (n != head) {
      if (++strop > (1u << 24) || spatne) { spatne = true; return; }
      const uint32_t node = n;
      uint32_t nu = w(node + 2);
      for (;;) {
        const uint32_t node2 = node + nu * UNIT;
        nu += w(node2 + 2);
        if (w(node2) != 0 || nu >= 0x10000 || spatne) break;
        sd(d(node2 + 8) + 4, d(node2 + 4));     // NODE(node2->Prev)->Next = node2->Next
        sd(d(node2 + 4) + 8, d(node2 + 8));     // NODE(node2->Next)->Prev = node2->Prev
        sw(node + 2, nu);
      }
      n = d(node + 4);
    }
    // naplnit seznamy volnych bloku
    strop = 0;
    for (n = d(head + 4); n != head;) {
      if (++strop > (1u << 24) || spatne) { spatne = true; return; }
      uint32_t node = n;
      const uint32_t next = d(node + 4);
      unsigned nu = w(node + 2);
      for (; nu > 128; nu -= 128, node += 128 * UNIT) InsertNode(node, NUM_INDEXES - 1);
      unsigned i = U2I(nu);
      if (I2U(i) != nu) {
        const unsigned k = I2U(--i);
        InsertNode(node + k * UNIT, nu - k - 1);
      }
      InsertNode(node, i);
      n = next;
    }
  }
  uint32_t AllocUnitsRare(unsigned indx) {
    if (GlueCount == 0) {
      GlueFreeBlocks();
      if (spatne) return 0;
      if (FreeList[indx] != 0) return RemoveNode(indx);
    }
    unsigned i = indx;
    do {
      if (++i == (unsigned)NUM_INDEXES) {
        const uint32_t numBytes = U2B(I2U(indx));
        GlueCount--;
        return (UnitsStart - Text > numBytes) ? (UnitsStart -= numBytes) : 0;
      }
    } while (FreeList[i] == 0);
    const uint32_t retVal = RemoveNode(i);
    SplitBlock(retVal, i, indx);
    return retVal;
  }
  uint32_t AllocUnits(unsigned indx) {
    if (indx >= (unsigned)NUM_INDEXES) { spatne = true; return 0; }
    if (FreeList[indx] != 0) return RemoveNode(indx);
    const uint32_t numBytes = U2B(I2U(indx));
    if (numBytes <= HiUnit - LoUnit) { const uint32_t r = LoUnit; LoUnit += numBytes; return r; }
    return AllocUnitsRare(indx);
  }
  uint32_t ShrinkUnits(uint32_t oldPtr, unsigned oldNU, unsigned newNU) {
    const unsigned i0 = U2I(oldNU), i1 = U2I(newNU);
    if (i0 == i1) return oldPtr;
    if (FreeList[i1] != 0) {
      const uint32_t ptr = RemoveNode(i1);
      kopiruj12(ptr, oldPtr, newNU);
      InsertNode(oldPtr, i0);
      return ptr;
    }
    SplitBlock(oldPtr, i0, i1);
    return oldPtr;
  }

  // ---- model ----
  void RestartModel() {
    static const uint16_t kInitBinEsc[8] = {0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};
    std::memset(FreeList, 0, sizeof FreeList);
    Text = AlignOffset;
    HiUnit = Text + Size;
    LoUnit = UnitsStart = HiUnit - Size / 8 / UNIT * 7 * UNIT;
    GlueCount = 0;
    OrderFall = MaxOrder;
    RunLength = InitRL = -(int32_t)((MaxOrder < 12) ? MaxOrder : 12) - 1;
    PrevSuccess = 0;
    HiUnit -= UNIT;
    MinContext = MaxContext = HiUnit;
    setSuffix(MinContext, 0);
    setNumStats(MinContext, 256);
    setSummFreq(MinContext, 256 + 1);
    FoundState = LoUnit;
    LoUnit += U2B(256 / 2);
    setStats(MinContext, FoundState);
    for (unsigned i = 0; i < 256; i++) { const uint32_t s = FoundState + i * 6; setSym(s, i); setFreq(s, 1); setSucc(s, 0); }
    for (unsigned i = 0; i < 128; i++)
      for (unsigned k = 0; k < 8; k++) {
        const uint16_t val = (uint16_t)(BIN_SCALE - kInitBinEsc[k] / (i + 2));
        for (unsigned m = 0; m < 64; m += 8) BinSumm[i][k + m] = val;
      }
    for (unsigned i = 0; i < 25; i++)
      for (unsigned k = 0; k < 16; k++) {
        See &s = SeeTab[i][k];
        s.Shift = PERIOD_BITS - 4;
        s.Summ = (uint16_t)((5 * i + 10) << s.Shift);
        s.Count = 4;
      }
  }

  uint32_t CreateSuccessors(bool skip) {
    uint32_t c = MinContext;
    const uint32_t upBranch = Succ(FoundState);
    uint32_t ps[MAX_O];
    unsigned numPs = 0;
    if (!skip) ps[numPs++] = FoundState;
    const unsigned symbol = Sym(FoundState);
    unsigned strop = 0;
    while (Suffix(c)) {
      if (++strop > 300 || spatne) { spatne = true; return 0; }
      c = Suffix(c);
      uint32_t s;
      const unsigned ns = NumStats(c);
      if (ns != 1) {
        s = Stats(c);
        unsigned k = 0;
        while (Sym(s) != symbol) { if (++k >= ns || spatne) { spatne = true; return 0; } s += 6; }
      } else s = OneState(c);
      const uint32_t successor = Succ(s);
      if (successor != upBranch) {
        c = successor;
        if (numPs == 0) return c;
        break;
      }
      if (numPs >= (unsigned)MAX_O) { spatne = true; return 0; }
      ps[numPs++] = s;
    }
    Stav up;
    up.sym = b(upBranch);
    up.succ = upBranch + 1;
    if (NumStats(c) == 1) up.freq = (uint8_t)Freq(OneState(c));
    else {
      uint32_t s = Stats(c);
      const unsigned ns = NumStats(c);
      unsigned k = 0;
      while (Sym(s) != up.sym) { if (++k >= ns || spatne) { spatne = true; return 0; } s += 6; }
      const uint32_t cf = Freq(s) - 1;
      const uint32_t s0 = SummFreq(c) - NumStats(c) - cf;
      if (s0 == 0 && 2 * cf > s0) { spatne = true; return 0; }
      up.freq = (uint8_t)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((2 * cf + 3 * s0 - 1) / (2 * s0))));
    }
    while (numPs != 0) {
      uint32_t c1;
      if (HiUnit != LoUnit) c1 = (HiUnit -= UNIT);
      else if (FreeList[0] != 0) c1 = RemoveNode(0);
      else { c1 = AllocUnitsRare(0); if (!c1) return 0; }
      setNumStats(c1, 1);
      zapis(OneState(c1), up);
      setSuffix(c1, c);
      setSucc(ps[--numPs], c1);
      c = c1;
    }
    return c;
  }

  void UpdateModel() {
    uint32_t fSuccessor = Succ(FoundState);
    const unsigned fSym = Sym(FoundState), fFreq = Freq(FoundState);
    if (fFreq < MAX_FREQ / 4 && Suffix(MinContext) != 0) {
      const uint32_t c = Suffix(MinContext);
      if (NumStats(c) == 1) {
        const uint32_t s = OneState(c);
        if (Freq(s) < 32) setFreq(s, Freq(s) + 1);
      } else {
        uint32_t s = Stats(c);
        if (Sym(s) != fSym) {
          const unsigned ns = NumStats(c);
          unsigned k = 0;
          do { s += 6; if (++k >= ns || spatne) { spatne = true; return; } } while (Sym(s) != fSym);
          if (Freq(s) >= Freq(s - 6)) { prohod(s, s - 6); s -= 6; }
        }
        if (Freq(s) < MAX_FREQ - 9) { setFreq(s, Freq(s) + 2); setSummFreq(c, SummFreq(c) + 2); }
      }
    }
    if (OrderFall == 0) {
      MinContext = MaxContext = CreateSuccessors(true);
      if (MinContext == 0 || spatne) { if (!spatne) RestartModel(); return; }
      setSucc(FoundState, MinContext);
      return;
    }
    sb(Text++, (uint8_t)fSym);
    uint32_t successor = Text;
    if (Text >= UnitsStart) { RestartModel(); return; }
    if (fSuccessor) {
      if (fSuccessor <= successor) {
        const uint32_t cs = CreateSuccessors(false);
        if (cs == 0 || spatne) { if (!spatne) RestartModel(); return; }
        fSuccessor = cs;
      }
      if (--OrderFall == 0) {
        successor = fSuccessor;
        Text -= (MaxContext != MinContext);
      }
    } else {
      setSucc(FoundState, successor);
      fSuccessor = MinContext;
    }
    const unsigned ns = NumStats(MinContext);
    const unsigned s0 = SummFreq(MinContext) - ns - (Freq(FoundState) - 1);
    unsigned strop = 0;
    for (uint32_t c = MaxContext; c != MinContext; c = Suffix(c)) {
      if (++strop > 300 || spatne || c == 0) { spatne = true; return; }
      const unsigned ns1 = NumStats(c);
      uint32_t cf, sf;
      if (ns1 != 1) {
        if (ns1 > 255) { spatne = true; return; }
        if ((ns1 & 1) == 0) {
          // pridat jednotku
          const unsigned oldNU = ns1 >> 1;
          const unsigned i = U2I(oldNU);
          if (i != U2I(oldNU + 1)) {
            const uint32_t ptr = AllocUnits(i + 1);
            if (!ptr) { RestartModel(); return; }
            const uint32_t oldPtr = Stats(c);
            kopiruj12(ptr, oldPtr, oldNU);
            InsertNode(oldPtr, i);
            setStats(c, ptr);
          }
        }
        setSummFreq(c, (uint16_t)(SummFreq(c) + (2 * ns1 < ns) + 2 * ((4 * ns1 <= ns) & (SummFreq(c) <= 8 * ns1))));
      } else {
        const uint32_t s = AllocUnits(0);
        if (!s) { RestartModel(); return; }
        const Stav t = nacti(OneState(c));
        zapis(s, t);
        setStats(c, s);
        if (Freq(s) < MAX_FREQ / 4 - 1) setFreq(s, Freq(s) << 1);
        else setFreq(s, MAX_FREQ - 4);
        setSummFreq(c, (uint16_t)(Freq(s) + InitEsc + (ns > 3)));
      }
      cf = 2 * (uint32_t)Freq(FoundState) * (SummFreq(c) + 6);
      sf = (uint32_t)s0 + SummFreq(c);
      if (cf < 6 * sf) {
        cf = 1 + (cf > sf) + (cf >= 4 * sf);
        setSummFreq(c, SummFreq(c) + 3);
      } else {
        cf = 4 + (cf >= 9 * sf) + (cf >= 12 * sf) + (cf >= 15 * sf);
        setSummFreq(c, (uint16_t)(SummFreq(c) + cf));
      }
      const uint32_t s = Stats(c) + ns1 * 6;
      setSucc(s, successor);
      setSym(s, fSym);
      setFreq(s, cf);
      setNumStats(c, ns1 + 1);
    }
    MaxContext = MinContext = fSuccessor;
  }

  void Rescale() {
    const uint32_t stats = Stats(MinContext);
    uint32_t s = FoundState;
    if (s < stats || (s - stats) % 6 != 0 || (s - stats) / 6 >= 256) { spatne = true; return; }
    {
      const Stav tmp = nacti(s);
      for (; s != stats; s -= 6) zapis(s, nacti(s - 6));
      zapis(s, tmp);
    }
    unsigned escFreq = SummFreq(MinContext) - Freq(s);
    setFreq(s, Freq(s) + 4);
    const unsigned adder = (OrderFall != 0);
    setFreq(s, (Freq(s) + adder) >> 1);
    unsigned sumFreq = Freq(s);
    unsigned i = NumStats(MinContext) - 1;
    if (NumStats(MinContext) < 2 || NumStats(MinContext) > 256) { spatne = true; return; }
    do {
      s += 6;
      escFreq -= Freq(s);
      setFreq(s, (Freq(s) + adder) >> 1);
      sumFreq += Freq(s);
      if (Freq(s) > Freq(s - 6)) {
        uint32_t s1 = s;
        const Stav tmp = nacti(s1);
        do zapis(s1, nacti(s1 - 6));
        while ((s1 -= 6) != stats && tmp.freq > Freq(s1 - 6));
        zapis(s1, tmp);
      }
    } while (--i);
    if (Freq(s) == 0) {
      const unsigned numStats = NumStats(MinContext);
      unsigned k = 0;
      do { i++; s -= 6; if (++k > 256) { spatne = true; return; } } while (Freq(s) == 0);
      escFreq += i;
      setNumStats(MinContext, NumStats(MinContext) - i);
      if (NumStats(MinContext) == 1) {
        Stav tmp = nacti(stats);
        do { tmp.freq = (uint8_t)(tmp.freq - (tmp.freq >> 1)); escFreq >>= 1; } while (escFreq > 1);
        InsertNode(stats, U2I((numStats + 1) >> 1));
        FoundState = OneState(MinContext);
        zapis(FoundState, tmp);
        return;
      }
      const unsigned n0 = (numStats + 1) >> 1, n1 = (NumStats(MinContext) + 1) >> 1;
      if (n0 != n1) setStats(MinContext, ShrinkUnits(stats, n0, n1));
    }
    setSummFreq(MinContext, (uint16_t)(sumFreq + escFreq - (escFreq >> 1)));
    FoundState = Stats(MinContext);
  }

  See *MakeEscFreq(unsigned numMasked, uint32_t &escFreq) {
    const unsigned ns = NumStats(MinContext);
    const unsigned nonMasked = ns - numMasked;
    if (ns != 256) {
      if (nonMasked < 1 || nonMasked > 256) { spatne = true; escFreq = 1; return &DummySee; }
      const unsigned radek = NS2Indx[nonMasked - 1];
      const unsigned sl = (nonMasked < NumStats(Suffix(MinContext)) - ns) + 2 * (SummFreq(MinContext) < 11 * ns) + 4 * (numMasked > nonMasked) + HiBitsFlag;
      if (radek >= 25 || sl >= 16) { spatne = true; escFreq = 1; return &DummySee; }
      See *see = &SeeTab[radek][sl];
      const unsigned r = (see->Summ >> see->Shift);
      see->Summ = (uint16_t)(see->Summ - r);
      escFreq = r + (r == 0);
      return see;
    }
    escFreq = 1;
    return &DummySee;
  }
  void NextContext() {
    const uint32_t c = Succ(FoundState);
    if (OrderFall == 0 && c > Text) MinContext = MaxContext = c;
    else UpdateModel();
  }
  void Update1() {
    uint32_t s = FoundState;
    setFreq(s, Freq(s) + 4);
    setSummFreq(MinContext, SummFreq(MinContext) + 4);
    if (Freq(s) > Freq(s - 6)) {
      prohod(s, s - 6);
      FoundState = (s -= 6);
      if (Freq(s) > MAX_FREQ) Rescale();
    }
    NextContext();
  }
  void Update1_0() {
    PrevSuccess = (2 * Freq(FoundState) > SummFreq(MinContext));
    RunLength += (int32_t)PrevSuccess;
    setSummFreq(MinContext, SummFreq(MinContext) + 4);
    setFreq(FoundState, Freq(FoundState) + 4);
    if (Freq(FoundState) > MAX_FREQ) Rescale();
    NextContext();
  }
  void UpdateBin() {
    setFreq(FoundState, Freq(FoundState) + (Freq(FoundState) < 128 ? 1 : 0));
    PrevSuccess = 1;
    RunLength++;
    NextContext();
  }
  void Update2() {
    setSummFreq(MinContext, SummFreq(MinContext) + 4);
    setFreq(FoundState, Freq(FoundState) + 4);
    if (Freq(FoundState) > MAX_FREQ) Rescale();
    RunLength = InitRL;
    UpdateModel();
  }

  // ---- rozsahovy dekoder RAR (Subbotin, carryless) ----
  bool normalizuj() {
    for (int k = 0; k < 16; k++) {
      if ((Low ^ (Low + Range)) >= (1u << 24)) {
        if (Range >= Bottom) return true;
        Range = (0u - Low) & (Bottom - 1);
      }
      Code = (Code << 8) | in->cti();
      Range <<= 8; Low <<= 8;
    }
    spatne = true;                 // Range = 0: poskozena data
    return false;
  }
  bool prah(uint32_t celkem, uint32_t &v) {
    if (celkem == 0 || celkem > Range) { spatne = true; return false; }
    Range /= celkem;
    v = (Code - Low) / Range;
    return true;
  }
  void dekodujUsek(uint32_t start, uint32_t velikost) { Low += start * Range; Range *= velikost; normalizuj(); }

  int dekodujSymbol() {
    int8_t maska[256];
    if (!platnyKontext(MinContext)) { spatne = true; return -2; }
    const unsigned ns0 = NumStats(MinContext);
    if (ns0 == 0 || ns0 > 256) { spatne = true; return -2; }
    if (ns0 != 1) {
      uint32_t s = Stats(MinContext);
      uint32_t count;
      if (!prah(SummFreq(MinContext), count)) return -2;
      uint32_t hiCnt = Freq(s);
      if (count < hiCnt) {
        dekodujUsek(0, Freq(s));
        FoundState = s;
        const unsigned symbol = Sym(s);
        Update1_0();
        return (int)symbol;
      }
      PrevSuccess = 0;
      unsigned i = ns0 - 1;
      do {
        s += 6;
        if ((hiCnt += Freq(s)) > count) {
          dekodujUsek(hiCnt - Freq(s), Freq(s));
          FoundState = s;
          const unsigned symbol = Sym(s);
          Update1();
          return (int)symbol;
        }
      } while (--i);
      if (count >= SummFreq(MinContext)) { spatne = true; return -2; }
      HiBitsFlag = HB2Flag[Sym(FoundState)];
      dekodujUsek(hiCnt, SummFreq(MinContext) - hiCnt);
      std::memset(maska, -1, sizeof maska);
      maska[Sym(s)] = 0;
      i = ns0 - 1;
      do { s -= 6; maska[Sym(s)] = 0; } while (--i);
    } else {
      // binarni kontext: jediny stav, pravdepodobnost z BinSumm
      const uint32_t one = OneState(MinContext);
      const unsigned f = Freq(one);
      if (f == 0 || f > 128) { spatne = true; return -2; }
      const unsigned sufNs = NumStats(Suffix(MinContext));
      if (sufNs == 0 || sufNs > 256) { spatne = true; return -2; }
      HiBitsFlag = HB2Flag[Sym(FoundState)];
      const unsigned idx = PrevSuccess + NS2BSIndx[sufNs - 1] + HiBitsFlag + 2 * HB2Flag[Sym(one)] + (unsigned)((RunLength >> 26) & 0x20);
      if (idx >= 64) { spatne = true; return -2; }
      uint16_t &prob = BinSumm[f - 1][idx];
      uint32_t v;
      if (!prah(BIN_SCALE, v)) return -2;
      if (v < prob) {
        dekodujUsek(0, prob);
        prob = (uint16_t)(prob + (1 << INT_BITS) - ((prob + (1 << (PERIOD_BITS - 2))) >> PERIOD_BITS));
        FoundState = one;
        const unsigned symbol = Sym(one);
        UpdateBin();
        return (int)symbol;
      }
      dekodujUsek(prob, BIN_SCALE - prob);
      prob = (uint16_t)(prob - ((prob + (1 << (PERIOD_BITS - 2))) >> PERIOD_BITS));
      static const uint8_t kExpEscape[16] = {25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2};
      InitEsc = kExpEscape[prob >> 10];
      std::memset(maska, -1, sizeof maska);
      maska[Sym(one)] = 0;
      PrevSuccess = 0;
    }
    for (int kolo = 0; kolo < 300; kolo++) {
      uint32_t ps[256];
      unsigned numMasked = NumStats(MinContext);
      unsigned strop = 0;
      do {
        OrderFall++;
        if (!Suffix(MinContext)) return -1;
        MinContext = Suffix(MinContext);
        if (!platnyKontext(MinContext) || ++strop > 300 || spatne) { spatne = true; return -2; }
      } while (NumStats(MinContext) == numMasked);
      const unsigned ns = NumStats(MinContext);
      if (ns <= numMasked || ns > 256) { spatne = true; return -2; }
      uint32_t hiCnt = 0;
      uint32_t s = Stats(MinContext);
      unsigned i = 0;
      const unsigned num = ns - numMasked;
      unsigned prohledano = 0;
      do {
        if (++prohledano > ns || spatne) { spatne = true; return -2; }
        const int k = maska[Sym(s)];
        hiCnt += (Freq(s) & (unsigned)k);
        ps[i] = s;
        s += 6;
        i -= (unsigned)k;
      } while (i != num);
      uint32_t freqSum;
      See *see = MakeEscFreq(numMasked, freqSum);
      freqSum += hiCnt;
      uint32_t count;
      if (!prah(freqSum, count)) return -2;
      if (count < hiCnt) {
        unsigned j = 0;
        hiCnt = 0;
        while ((hiCnt += Freq(ps[j])) <= count) { if (++j >= num) { spatne = true; return -2; } }
        s = ps[j];
        dekodujUsek(hiCnt - Freq(s), Freq(s));
        if (see->Shift < PERIOD_BITS && --see->Count == 0) { see->Summ = (uint16_t)(see->Summ << 1); see->Count = (uint8_t)(3 << see->Shift++); }
        FoundState = s;
        const unsigned symbol = Sym(s);
        Update2();
        return (int)symbol;
      }
      if (count >= freqSum) { spatne = true; return -2; }
      dekodujUsek(hiCnt, freqSum - hiCnt);
      see->Summ = (uint16_t)(see->Summ + freqSum);
      do { maska[Sym(ps[--i])] = 0; } while (i != 0);
    }
    spatne = true;
    return -2;
  }
};

}  // namespace archiv
}  // namespace nap
