// nap_atari_stav.h
// =====================================================================================================
//  B363: CELY STAV ATARI 130XE - ULOZIT / NAHRAT / OTISK (pro online hru dvou telefonu)
// =====================================================================================================
//  Rene 25. 11. 2026: "mohl by jsi se prosim te uz pustit do online atari - at se nezdrzujeme".
//
//  Online hra (online/nap_online.h) potrebuje od jadra tri veci:
//   1) ODHAD + OPRAVA (B362): pred snimkem spocitanym s odhadem stisku druheho se ulozi stav celeho stroje; kdyz
//      skutecny stisk prijde jiny, stroj se vrati k ulozenemu stavu a snimky se spocitaji znovu,
//   2) START ZE STAVU a SYNCHRONIZACE: hrac 1 posle cely stav sveho Atari (i s cartridge a disketou v D1:) - hrac 2
//      ho nahraje a hraje se od stejneho okamziku (hrac 2 tak hru vubec mit nemusi),
//   3) OTISK STAVU: kazdou sekundu se porovna - lisi se = "rozjelo se" (s casti, at je v logu videt CO).
//
//  Jeden seznam poli (atariStavPole) pro vsechno: zapis, cteni i otisk prochazeji stroj stejne, takze ulozeni
//  a nahrani nemuze zapomenout jine pole. Ulozi se vse, co meni dalsi beh stroje: procesor (mezi instrukcemi -
//  stav se uklada jen mezi snimky), casovani (cyklus, radek, snimek), RAM a rozsirena pamet (jen banky, ktere
//  rezim pameti ma: 320K = 16 bank, 1088K = 64), PIA a MMU, ANTIC (display list, DMA, rolovani, NMI, WSYNC),
//  GTIA (barvy, hraci a strely, kolize, priority, rozpracovany radek), POKEY 1 a 2 (citace, poly citace, preruseni,
//  klavesnice, seriovy port), linka SIO a jednotka D1:, cartridge (typ, banka) a VBXE (registry, palety, blitter,
//  512 KB VRAM - jen kdyz ji program pouzil). Neulozi se vystup (obraz, zvuk po filtru) a diagnostika (pocty do logu).
//  Kazeta se za online hry nepouziva (PLAY vypnute na obou telefonech) - ulozi se jen uroven datove linky.
//
//  Cisla little-endian v pevne velikosti (int 4 B, long long 8 B, double 8 B) - stejne na arm64 i armeabi-v7a.
//  Bez Androidu: test test_overeni/b363 (ulozit -> nahrat do noveho stroje -> stejne snimky a zvuk bit po bitu).
// =====================================================================================================
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "nap_atari_machine.h"

namespace nap {

constexpr uint32_t ATARI_STAV_VERZE = 1;          // jina = jiny seznam poli (jina verze appky) - nenahrat
constexpr uint32_t ATARI_STAV_MEDIA = 1;          // priznak: v datech je obsah cartridge a diskety (start / synchronizace)
constexpr size_t ATARI_STAV_MEDIA_MAX = 16u << 20; // cartridge nebo disketa nejvys 16 MB
constexpr int ATARI_STAV_CASTI = 8;                // casti otisku (online: HASH s castmi, CASTI_MAX = 8)

inline const char *const *atariStavNazvyCasti() {
  static const char *const n[ATARI_STAV_CASTI] = {"RAM 64K", "ROZSIRENA PAMET", "CPU A CASOVANI", "ANTIC", "GTIA",
                                                   "POKEY A PIA", "VBXE", "DISK A CARTRIDGE"};
  return n;
}

/** Otisk po 8 bajtech (rychly i pro 1 MB pameti) - stejny na vsech telefonech (little-endian). */
inline uint64_t atariOtisk64(const void *d, size_t n, uint64_t h) {
  const uint8_t *p = (const uint8_t *)d;
  while (n >= 8) {
    uint64_t w;
    std::memcpy(&w, p, 8);
    h ^= w;
    h *= 0x100000001B3ULL;
    h ^= h >> 29;
    p += 8;
    n -= 8;
  }
  while (n--) { h ^= *p++; h *= 0x100000001B3ULL; }
  return h;
}

// ---------------------------------------------------------------------------------------------- navstevnici
/** Zapis: pole za sebou do bajtu. */
struct AtariStavZapis {
  std::vector<uint8_t> &o;
  explicit AtariStavZapis(std::vector<uint8_t> &out) : o(out) {}
  template <class T> void operator()(const T &v) {
    static_assert(std::is_arithmetic<T>::value, "jen cisla");
    bajty(&v, sizeof(T));
  }
  void bajty(const void *p, size_t n) {
    const uint8_t *b = (const uint8_t *)p;
    o.insert(o.end(), b, b + n);
  }
  void vektor(std::vector<uint8_t> &v, size_t) {
    const uint32_t n = (uint32_t)v.size();
    (*this)(n);
    bajty(v.data(), v.size());
  }
  void retezec(std::string &s) {
    const uint32_t n = (uint32_t)std::min<size_t>(s.size(), 255);
    (*this)(n);
    bajty(s.data(), n);
  }
  void cast(int) {}
  bool chyba = false;
};

/** Cteni: stejne poradi jako zapis. chyba = data dosla driv / nesmysl (stroj je pak napul prepsany). */
struct AtariStavCteni {
  const uint8_t *p;
  size_t n, i = 0;
  bool chyba = false;
  AtariStavCteni(const uint8_t *d, size_t k) : p(d), n(k) {}
  template <class T> void operator()(T &v) {
    static_assert(std::is_arithmetic<T>::value, "jen cisla");
    bajty(&v, sizeof(T));
  }
  void bajty(void *d, size_t k) {
    if (chyba || k > n - i) { chyba = true; return; }
    std::memcpy(d, p + i, k);
    i += k;
  }
  void vektor(std::vector<uint8_t> &v, size_t max) {
    uint32_t k = 0;
    (*this)(k);
    if (chyba || k > max || k > n - i) { chyba = true; return; }
    v.assign(p + i, p + i + k);
    i += k;
  }
  void retezec(std::string &s) {
    uint32_t k = 0;
    (*this)(k);
    if (chyba || k > 255 || k > n - i) { chyba = true; return; }
    s.assign((const char *)p + i, k);
    i += k;
  }
  void cast(int) {}
};

/** Otisk po castech (RAM, rozsirena pamet, CPU ...) - pri rozjeti je v logu videt, co se lisi. Jmena se nepocitaji. */
struct AtariStavOtisk {
  uint64_t h[ATARI_STAV_CASTI];
  int k = 0;
  bool chyba = false;
  AtariStavOtisk() { for (auto &x : h) x = 1469598103934665603ULL; }
  template <class T> void operator()(const T &v) { bajty(&v, sizeof(T)); }
  void bajty(const void *p, size_t n) { h[k] = atariOtisk64(p, n, h[k]); }
  void vektor(std::vector<uint8_t> &v, size_t) {
    const uint32_t n = (uint32_t)v.size();
    (*this)(n);
    bajty(v.data(), v.size());
  }
  void retezec(std::string &) {}
  void cast(int c) { k = (c >= 0 && c < ATARI_STAV_CASTI) ? c : 0; }
  uint64_t celkem() const { return atariOtisk64(h, sizeof h, 1469598103934665603ULL); }
};

// ---------------------------------------------------------------------------------------------- seznam poli
/**
 * Vsechna pole stroje, ktera meni jeho dalsi beh, v pevnem poradi. priznaky: ATARI_STAV_MEDIA = i obsah cartridge
 * a diskety (start / synchronizace online hry). Bez nej (ulozeni pro opravu odhadu na tomtez telefonu) se disketa
 * resi zvlast (AtariStavDisketa - kopie jen kdyz se na ni zapsalo) a cartridge se nemeni (ROM).
 * Navstevnik V: operator()(pole), bajty(p, n), vektor(v, max), retezec(s), cast(k), chyba.
 */
template <class V> void atariStavPole(Machine &m, V &v, uint32_t priznaky) {
  // ---- 0: RAM 64K
  v.cast(0);
  v.bajty(m.mem.ram, sizeof m.mem.ram);
  // ---- 1: rozsirena pamet (jen banky, ktere rezim ma - rezim se cte driv nez banky)
  v.cast(1);
  v(m.mem.rezim1088);
  v(m.mem.pbZapsano); v(m.mem.pbRuznych); v(m.mem.chce1088);
  v.bajty(m.mem.ext, (size_t)m.mem.bankAktivnich() * 16384u);
  // ---- 2: procesor, casovani, sbernice, vstupy
  v.cast(2);
  v(m.cpu.pc); v(m.cpu.a); v(m.cpu.x); v(m.cpu.y); v(m.cpu.s); v(m.cpu.p);
  v(m.cpu.jam); v(m.cpu.jamOp); v(m.cpu.jamPc); v(m.cpu.takeNmi); v(m.cpu.takeIrq); v(m.cpu.polled);
  v(m.cpu.magicAne); v(m.cpu.magicLxa);
  v(m.cyc); v(m.x); v(m.line); v(m.frame);
  v(m.busData); v(m.nmiLatch); v(m.irqLine);
  v(m.consol); for (int &t : m.trig) v(t); v(m.porta);
  v(m.keyHoldFrames);
  // ---- 3: ANTIC
  v.cast(3);
  v(m.dmactlReg); v(m.chactl); v(m.hscrol); v(m.vscrol); v(m.pmbase); v(m.chbase); v(m.chbaseReg); v(m.nmien); v(m.nmist);
  v(m.dlist); v(m.dlistLatch); v(m.dlControl); v(m.dlControlPrev); v(m.dlNext);
  v(m.rowCounter); v(m.rowCount); v(m.rowStopUseVScroll); v(m.latchedVScroll); v(m.latchedVScroll2);
  v(m.dlActive); v(m.dlExtraLoads); v(m.dlDmaInTime);
  v(m.pfBase); v(m.pfOffset);
  v(m.pfDmaEnabled); v(m.pfDmaActive); v(m.hscrollEnabled); v(m.hscrollDelay); v(m.hscrollDmaOffset);
  v(m.pfWidth); v(m.pfFetchWidth);
  v(m.pfDmaStart); v(m.pfDmaEnd); v(m.pfDmaVEnd); v(m.pfDisplayStart); v(m.pfDisplayEnd);
  v.bajty(m.dmaPat, sizeof m.dmaPat);
  v.bajty(m.pfData, sizeof m.pfData); v.bajty(m.pfChar, sizeof m.pfChar);
  v(m.pfDataW); v(m.pfDataR); v(m.pfCharW);
  v(m.charBase128); v(m.charBase64); v(m.charFetchPtr); v(m.charMask); v(m.charInvert); v(m.charBlink);
  v(m.pushMode); v(m.displayDone);
  v(m.pendingNMIs); v(m.earlyNMIEN); v(m.lateNMI);
  v(m.wsyncPending); v(m.rdyHalt); v(m.rdyStalled);
  v(m.chbaseDelay); v(m.chbaseNew);
  v(m.lineHiresAntic); v(m.vblankHires);
  for (int8_t &q : m.phantomQ) v(q);
  v(m.dlKroku);
  // ---- 4: GTIA
  v.cast(4);
  for (auto &s : m.spr) { v(s.shift); v(s.latch); v(s.size); v(s.state); }
  for (int &p : m.sprPos) v(p);
  v.bajty(m.gReg, sizeof m.gReg);
  v.bajty(m.colpm, sizeof m.colpm); v.bajty(m.colpf, sizeof m.colpf);
  v(m.colbk); v(m.prior); v(m.vdelay); v(m.gractl); v(m.consolOut);
  v.bajty(m.colpmR, sizeof m.colpmR); v.bajty(m.colpfR, sizeof m.colpfR); v(m.colbkR);
  v.bajty(m.collP, sizeof m.collP); v.bajty(m.collM, sizeof m.collM);
  v.bajty(m.merge, sizeof m.merge); v.bajty(m.anData, sizeof m.anData); v.bajty(m.lineOut, sizeof m.lineOut);
  v(m.lineHires); v(m.vblankLine); v(m.lastSyncCc);
  v(m.rcN);
  if (m.rcN < 0 || m.rcN > 256) { m.rcN = 0; v.chyba = true; }
  for (int i = 0; i < m.rcN; i++) { v(m.rc[i].pos); v(m.rc[i].reg); v(m.rc[i].val); }
  v(m.gtiaSpeakerBit);
  // ---- 5: POKEY 1 a 2, PIA, linka kazety
  v.cast(5);
  for (int &a : m.audf) v(a);
  for (int &a : m.audc) v(a);
  v(m.audctl);
  for (int &a : m.audfP1) v(a);
  for (int &a : m.pcnt) v(a);
  for (int &a : m.pborrow) v(a);
  for (int &a : m.pcnt16) v(a);
  for (int &a : m.pborrow16) v(a);
  v.bajty(m.chOut, sizeof m.chOut); v.bajty(m.hpf, sizeof m.hpf);
  v(m.irqen); v(m.irqst); v(m.skctl); v(m.skstat); v(m.kbcode); v(m.serin); v(m.serout);
  v(m.last64); v(m.last15); v(m.polyBase); v(m.polyShutOff);
  v(m.stimerDelay);
  v(m.serOutCounter); v(m.serOutValid); v(m.serOutState); v(m.serOutShift); v(m.serOutBusy); v(m.serOutTickDelay);
  v(m.twoToneReset);
  v(m.audLevel); v(m.tapeLevel);
  v(m.stereoPokey);
  {
    PokeyDruhy &k = m.pk2;
    for (int &a : k.audf) v(a);
    for (int &a : k.audc) v(a);
    v(k.audctl);
    for (int &a : k.audfP1) v(a);
    for (int &a : k.pcnt) v(a);
    for (int &a : k.pborrow) v(a);
    for (int &a : k.pcnt16) v(a);
    for (int &a : k.pborrow16) v(a);
    v.bajty(k.chOut, sizeof k.chOut); v.bajty(k.hpf, sizeof k.hpf);
    v(k.irqen); v(k.irqst); v(k.skctl);
    v(k.last64); v(k.last15); v(k.polyBase); v(k.polyShutOff);
    v(k.stimerDelay); v(k.bezi); v(k.slyset);
  }
  v(m.audLevel2);
  v(m.mem.pia.orA); v(m.mem.pia.ddrA); v(m.mem.pia.orB); v(m.mem.pia.ddrB); v(m.mem.pia.ctlA); v(m.mem.pia.ctlB);
  v(m.tape.line); v(m.tape.rxPhase); v(m.tape.rxNext); v(m.tape.rxByte); v(m.tape.play);
  v(m.tapeTickFrac);
  // ---- 6: VBXE (VRAM a radkove buffery jen kdyz ji program pouzil - jinak jsou od zapnuti nulove)
  v.cast(6);
  v(m.vbxeOn); v(m.vbxeLine);
  {
    Vbxe &b = m.vbx;
    v(b.nWrites);
    v(b.memacCtl); v(b.memacBankA); v(b.memacBankB);
    v(b.xdlBase); v(b.xdlAddr); v(b.xdlActive); v(b.xdlEnabled); v(b.xdlRepeat);
    v(b.ovMode); v(b.ovWidth); v(b.ovTrans); v(b.ovTrans15); v(b.ovHscroll); v(b.ovVscroll);
    v(b.ovMainPri); v.bajty(b.ovPri, sizeof b.ovPri);
    v(b.collMask); v(b.collState);
    v(b.ovAddr); v(b.ovStep); v(b.ovTextRow); v(b.chAddr);
    v(b.pfPal); v(b.ovPal); v(b.extColor); v(b.mapOn);
    v(b.mapAddr); v(b.mapStep); v(b.mapW); v(b.mapH); v(b.mapHs); v(b.mapVs); v(b.mapRow);
    v(b.psel); v(b.csel); v(b.irqEn); v(b.irqReq); v(b.configLatch);
    v.bajty(b.pal, sizeof b.pal);
    v(b.pal0Dirty);
    for (int32_t &a : b.mapCpu) v(a);
    for (int32_t &a : b.mapAntic) v(a);
    v(b.mapAny);
    v(b.blitState); v(b.blitContinue); v(b.blitStopTime); v(b.blitEndScanTime); v(b.stopEvent); v(b.blitMode);
    v(b.blitCyclesLeft); v(b.blitCyclesPerRow); v(b.blitSavedPerZero);
    v(b.blitListAddr); v(b.blitFetch);
    v(b.blitSrc); v(b.blitDst); v(b.blitSrcStepX); v(b.blitSrcStepY); v(b.blitDstStepX); v(b.blitDstStepY);
    v(b.blitWidth); v(b.blitHeight); v(b.blitHeightLeft);
    v(b.blitAnd); v(b.blitXor); v(b.blitCollMask); v(b.blitActiveCollMask); v(b.blitPattern); v(b.blitCollCode);
    v(b.blitZoomX); v(b.blitZoomY); v(b.blitZoomCounterY);
    if (b.nWrites != 0) {
      if (b.vram.size() != 512u * 1024u) b.vram.assign(512u * 1024u, 0);
      v.bajty(b.vram.data(), b.vram.size());
      for (auto &a : b.attrMap) { v(a.pf0); v(a.pf1); v(a.pf2); v(a.ctrl); v(a.hiresFlag); v(a.pri); }
      for (bool &a : b.attrFromMap) v(a);
      v.bajty(b.ovDec, sizeof b.ovDec); v.bajty(b.ovTextTrans, sizeof b.ovTextTrans);
      v.bajty(b.ovPriBuf, sizeof b.ovPriBuf); v.bajty(b.ctrlBuf, sizeof b.ctrlBuf);
      v.bajty(b.lineBuf, sizeof b.lineBuf);
    }
  }
  // ---- 7: SIO (OS i bajty pres POKEY), disketa D1:, cartridge
  v.cast(7);
  v(m.sioRychlyTimeout); v(m.sioNext);
  {
    SioBus &s = m.sio;
    v(s.cmdLine); v.bajty(s.cmd, sizeof s.cmd); v(s.cmdN);
    v(s.frameLen); v(s.frameN); v(s.frameSec); v(s.frameBad); v.bajty(s.frame, sizeof s.frame);
    v(s.outN); v(s.outI); v(s.rxBusy);
    if (s.outN < 0 || s.outN > SioBus::OUT_MAX) { s.outN = s.outI = 0; v.chyba = true; }
    v.bajty(s.outB, (size_t)s.outN);
    for (int i = 0; i < s.outN; i++) v(s.outS[i]);
  }
  v(m.disk.sectorSize); v(m.disk.sectors); v(m.disk.mounted); v(m.disk.writeProtect); v(m.disk.writes);
  v(m.cart.typ); v(m.cart.stav);
  for (int32_t &o : m.cart.okno) v(o);
  if (priznaky & ATARI_STAV_MEDIA) {
    v.retezec(m.disk.name);
    v.vektor(m.disk.data, ATARI_STAV_MEDIA_MAX);
    v.retezec(m.cart.jmeno);
    v.vektor(m.cart.data, ATARI_STAV_MEDIA_MAX);
  }
}

// ---------------------------------------------------------------------------------------------- ulozit / nahrat
/** Hlavicka: "NAPA", verze, priznaky (u32 LE). */
inline void atariStavUloz(Machine &m, std::vector<uint8_t> &out, uint32_t priznaky) {
  out.clear();
  const uint8_t h[4] = {'N', 'A', 'P', 'A'};
  out.insert(out.end(), h, h + 4);
  AtariStavZapis z(out);
  z(ATARI_STAV_VERZE);
  z(priznaky);
  atariStavPole(m, z, priznaky);
}

/**
 * Nahraje stav do stroje. false = cizi / poskozena data (jina verze, chybi bajty) - stroj muze byt napul prepsany
 * (online pak skonci a appka zapne stroj znovu). Po nahrani: zvuk navazuje od ted (fronta urovni POKEY od
 * aktualniho cyklu), kazeta za online stoji.
 */
inline bool atariStavNahraj(Machine &m, const uint8_t *d, size_t n, uint32_t *priznakyVen = nullptr) {
  if (!d || n < 12 || std::memcmp(d, "NAPA", 4) != 0) return false;
  AtariStavCteni c(d + 4, n - 4);
  uint32_t verze = 0, priznaky = 0;
  c(verze);
  c(priznaky);
  if (c.chyba || verze != ATARI_STAV_VERZE) return false;
  const bool media = (priznaky & ATARI_STAV_MEDIA) != 0;
  if (media) {
    // cartridge / disketa z dat - stare pryc (cart.data a disk.data prepise cteni)
    m.cart.vyjmout();
    m.disk = AtrDisk();
  }
  atariStavPole(m, c, priznaky);
  if (c.chyba || c.i != c.n) return false;
  if (media) {
    if (m.cart.typ == 0) m.cart.data.clear();
    m.disk.reads = 0;
    if (!m.disk.mounted) { m.disk.data.clear(); m.disk.sectors = 0; }
    // velikost dat musi sedet s typem cartridge (okna ukazuji do dat)
    for (int32_t o : m.cart.okno) if (o >= 0 && (size_t)o + 0x1000 > m.cart.data.size()) return false;
  }
  if (m.vbx.nWrites == 0) std::fill(m.vbx.vram.begin(), m.vbx.vram.end(), (uint8_t)0);   // VBXE od zapnuti nepouzita
  m.srovnatSledovaniZvuku();
  if (priznakyVen) *priznakyVen = priznaky;
  return true;
}

/** Otisk stavu po castech (c[ATARI_STAV_CASTI]) a celkem. Obsah diskety a cartridge se pocita taky. */
inline uint64_t atariStavOtisk(Machine &m, uint64_t *c) {
  AtariStavOtisk o;
  atariStavPole(m, o, ATARI_STAV_MEDIA);
  if (c) for (int i = 0; i < ATARI_STAV_CASTI; i++) c[i] = o.h[i];
  return o.celkem();
}

// ---------------------------------------------------------------------------------------------- disketa pro opravu
/**
 * Disketa pri oprave odhadu: ulozeni stavu pro opravu ji neobsahuje (az 16 MB kazdy snimek by bylo moc). Kopie se
 * udela, jen kdyz se na disketu od posledni kopie zapsalo (pocitadlo disk.writes - roste pri kazdem zapisu sektoru
 * i FORMAT); vic ulozenych stavu sdili tutez kopii. Nahrani vrati data diskety, jen kdyz se mezitim lisila.
 */
struct AtariStavDisketa {
  std::shared_ptr<const std::vector<uint8_t>> kopie;
  long long zapisu = -1;
  bool mounted = false;
  /** Ulozeni: kopie pro stav snimku (stejna jako minule, kdyz se nezapisovalo). */
  std::shared_ptr<const std::vector<uint8_t>> uloz(const Machine &m) {
    if (!kopie || zapisu != m.disk.writes || mounted != m.disk.mounted) {
      kopie = std::make_shared<const std::vector<uint8_t>>(m.disk.data);
      zapisu = m.disk.writes;
      mounted = m.disk.mounted;
    }
    return kopie;
  }
  /**
   * Nahrani stavu (disk.writes uz je z ulozeneho stavu, zapisuPred = pocitadlo PRED nahranim): data diskety zpet,
   * kdyz se lisi. Data odpovidaji kopii k jen tehdy, kdyz je k posledni kopie a od ni se nezapisovalo (pocitadlo
   * pred nahranim je stale stejne jako pri kopii).
   */
  void vrat(Machine &m, const std::shared_ptr<const std::vector<uint8_t>> &k, long long zapisuPred) {
    if (!k) return;
    if (!(kopie == k && zapisu == zapisuPred)) m.disk.data = *k;
    kopie = k;
    zapisu = m.disk.writes;
    mounted = m.disk.mounted;
  }
  void zapomen() { kopie.reset(); zapisu = -1; mounted = false; }
};

}  // namespace nap
