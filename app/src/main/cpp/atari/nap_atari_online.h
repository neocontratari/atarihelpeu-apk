// nap_atari_online.h
// =====================================================================================================
//  B363: ONLINE ATARI 130XE - stisky hracu, stav pro opravu odhadu a pro druhy telefon (bez Androidu)
// =====================================================================================================
//  Rene 25. 11. 2026: "mohl by jsi se prosim te uz pustit do online atari - at se nezdrzujeme".
//
//  Jak hraje Atari online (protokol online/nap_online.h je stejny jako u Segy, jadro J_ATARI):
//   - kazdy telefon emuluje cely stroj sam, po siti jdou jen stisky (32 bitu na hrace a snimek, nize),
//   - START ZE STAVU: hrac 1 po domluve posle CELY stav sveho Atari i s cartridge a disketou v D1: (zlib) -
//     hrac 2 ho nahraje a od snimku 0 hraji oba ze stejneho stavu. Hrac 2 tak hru vubec mit nemusi a nezalezi
//     na tom, co mel v Atari predtim (jine nastaveni, jina pamet ...),
//   - ODHAD + OPRAVA (jako Sega B362): stisk druheho, ktery jeste neprisel, se odhadne (drzi, co drzel); pred
//     takovym snimkem se ulozi stav (AtariOnlineKruh), a kdyz skutecny stisk prijde jiny, stroj se vrati a snimky
//     se spocitaji znovu,
//   - OTISK kazdych 60 snimku po castech (nap_atari_stav.h) - lisi se = synchronizace (hrac 1 posle stav znovu).
//
//  Stisky hrace pro snimek (u32): bity 0-3 joystick (nahoru, dolu, vlevo, vpravo; 1 = stisknuto), 4 FIRE,
//  5 START, 6 SELECT, 7 OPTION, 8-15 klavesa (KBCODE: scankod + $40 SHIFT + $80 CONTROL), 16 klavesa drzena,
//  17 BREAK (udalost - jen v jednom snimku), 18 SHIFT drzeny (zamek na pristroji), 19 RESET (udalost).
//  Porty: hrac 1 = joystick 1, hrac 2 = joystick 2 (hry pro dva hrace naraz). Volba hrace 1 "oba na joysticku 1"
//  (hry, kde se hraci stridaji a hraji jednim joystickem) - joystick a FIRE obou se slozi do joysticku 1.
//  START / SELECT / OPTION a klavesnici maji oba (jedno Atari); klavesa hrace 1 ma prednost.
// =====================================================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>
#include "nap_atari_machine.h"
#include "nap_atari_runtime.h"
#include "nap_atari_stav.h"

namespace nap {

// ---------------------------------------------------------------------------------------------- stisky
enum : uint32_t {
  AV_JOY = 0x0F,          // nahoru 1, dolu 2, vlevo 4, vpravo 8
  AV_FIRE = 0x10,
  AV_START = 0x20, AV_SELECT = 0x40, AV_OPTION = 0x80,
  AV_KOD_POSUN = 8,       // bity 8-15 kod klavesy
  AV_KLAVESA = 1u << 16,  // klavesa drzena
  AV_BREAK = 1u << 17,
  AV_SHIFT = 1u << 18,
  AV_RESET = 1u << 19,
  AV_UDALOSTI = AV_BREAK | AV_RESET
};

/** Stisky tohoto telefonu do 32 bitu (joystick bity 0-4 jako dotyk pristroje, konzole 1 = stisknuto). */
inline uint32_t atariOnlineBity(int joy, int konzoleStisknuto, bool klavesa, int kod, bool brk, bool shift, bool reset) {
  uint32_t b = (uint32_t)(joy & 0x1F) | ((uint32_t)(konzoleStisknuto & 7) << 5);
  if (klavesa) b |= AV_KLAVESA | ((uint32_t)(kod & 0xFF) << AV_KOD_POSUN);
  if (brk) b |= AV_BREAK;
  if (shift) b |= AV_SHIFT;
  if (reset) b |= AV_RESET;
  return b;
}

/**
 * Stisky obou hracu do stroje PRED snimkem (jen tady - za online hry nic jineho do stroje nesaha). Bez pameti:
 * klavesa se porovna se strojem (stisk / pusteni jen pri zmene), udalosti (BREAK, RESET) plati v jednom snimku.
 */
inline void atariOnlineVstupy(Machine &m, uint32_t p1, uint32_t p2, bool obaNaJ1) {
  const uint32_t ob = p1 | p2;
  if (ob & AV_RESET) m.reset();
  int j1 = (int)(p1 & AV_JOY), j2 = (int)(p2 & AV_JOY);
  bool f1 = (p1 & AV_FIRE) != 0, f2 = (p2 & AV_FIRE) != 0;
  if (obaNaJ1) { j1 |= j2; f1 = f1 || f2; j2 = 0; f2 = false; }
  m.porta = (((~j2 & 15) << 4) | (~j1 & 15)) & 0xFF;
  m.trig[0] = f1 ? 0 : 1;
  m.trig[1] = f2 ? 0 : 1;
  m.consol = 7 & ~(int)((ob >> 5) & 7);
  const uint32_t k = (p1 & AV_KLAVESA) ? p1 : (p2 & AV_KLAVESA) ? p2 : 0;
  if (k) {
    const int kod = (int)((k >> AV_KOD_POSUN) & 0xFF);
    if ((m.skstat & 0x04) || m.kbcode != kod) m.klavesa(kod, true);
  } else if (!(m.skstat & 0x04)) {
    m.klavesaPustena();
  }
  m.shiftDrzen((ob & AV_SHIFT) != 0);
  if (ob & AV_BREAK) m.breakKey();
}

// ---------------------------------------------------------------------------------------------- kontext
/**
 * Co krome stroje patri ke stavu online hry: volba portu a zavadec XEX (hra z XEX, jejiz INIT jeste bezi -
 * titulka "stiskni klavesu" v INIT, Turbo-BASIC XL: zavadec po navratu z INIT nahraje zbytek; bezi na obou
 * telefonech stejne, takze musi byt ve stavu).
 */
struct AtariOnlineKontext {
  bool obaNaJ1 = false;
  XexLoader *xex = nullptr;
};

template <class V> void atariStavKontext(AtariOnlineKontext &k, V &v) {
  v.cast(7);
  v(k.obaNaJ1);
  uint8_t maXex = k.xex ? 1 : 0;
  v(maXex);
  if (!k.xex) {
    if (maXex) v.chyba = true;   // data maji zavadec, ale neni kam ho nahrat
    return;
  }
  if (!maXex) { v.chyba = true; return; }
  XexLoader &x = *k.xex;
  int32_t st = (int32_t)x.stav;
  v(st);
  if (st < (int32_t)XexLoader::NIC || st > (int32_t)XexLoader::CHYBA) { st = XexLoader::NIC; v.chyba = true; }
  x.stav = (XexLoader::Stav)st;
  uint64_t pos = (uint64_t)x.pos;
  v(pos);
  x.pos = (size_t)pos;
  v(x.prvniStart); v(x.segmentu); v(x.bajtu); v(x.initu); v(x.snimku); v(x.initSnimku);
  v.retezec(x.jmeno);
  v.vektor(x.data, ATARI_STAV_MEDIA_MAX);
  // po nahrani (BEZI) jsou data pryc a pozice zustava - nesmysl je jen pozice za daty, ktere jeste jsou
  if (!x.data.empty() && x.pos > x.data.size()) { x.pos = 0; v.chyba = true; }
}

/** Ulozit stroj + kontext (priznaky: ATARI_STAV_MEDIA = i cartridge a disketa). */
inline void atariOnlineUloz(Machine &m, AtariOnlineKontext &k, std::vector<uint8_t> &out, uint32_t priznaky) {
  out.clear();
  const uint8_t h[4] = {'N', 'A', 'P', 'A'};
  out.insert(out.end(), h, h + 4);
  AtariStavZapis z(out);
  z(ATARI_STAV_VERZE);
  z(priznaky);
  atariStavPole(m, z, priznaky);
  atariStavKontext(k, z);
}

/** Nahrat stroj + kontext. false = cizi / poskozena data (stroj muze byt napul prepsany). */
inline bool atariOnlineNahraj(Machine &m, AtariOnlineKontext &k, const uint8_t *d, size_t n) {
  if (!d || n < 12 || std::memcmp(d, "NAPA", 4) != 0) return false;
  AtariStavCteni c(d + 4, n - 4);
  uint32_t verze = 0, priznaky = 0;
  c(verze);
  c(priznaky);
  if (c.chyba || verze != ATARI_STAV_VERZE) return false;
  const bool media = (priznaky & ATARI_STAV_MEDIA) != 0;
  if (media) { m.cart.vyjmout(); m.disk = AtrDisk(); }
  atariStavPole(m, c, priznaky);
  if (c.chyba) return false;
  atariStavKontext(k, c);
  if (c.chyba || c.i != c.n) return false;
  if (media) {
    if (m.cart.typ == 0) m.cart.data.clear();
    m.disk.reads = 0;
    if (!m.disk.mounted) { m.disk.data.clear(); m.disk.sectors = 0; }
    for (int32_t o : m.cart.okno) if (o >= 0 && (size_t)o + 0x1000 > m.cart.data.size()) return false;
  }
  if (m.vbx.nWrites == 0) std::fill(m.vbx.vram.begin(), m.vbx.vram.end(), (uint8_t)0);
  m.srovnatSledovaniZvuku();
  return true;
}

/** Otisk stroje + kontextu po castech (c[ATARI_STAV_CASTI]); kontext patri k casti 7 (disk a cartridge). */
inline uint64_t atariOnlineOtisk(Machine &m, AtariOnlineKontext &k, uint64_t *c) {
  AtariStavOtisk o;
  atariStavPole(m, o, ATARI_STAV_MEDIA);
  atariStavKontext(k, o);
  if (c) for (int i = 0; i < ATARI_STAV_CASTI; i++) c[i] = o.h[i];
  return o.celkem();
}

// ---------------------------------------------------------------------------------------------- pro druhy telefon
inline void atariLe32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
inline uint32_t atariR32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

/** Cely stav i s cartridge a disketou, zabaleny (u32 LE delka + zlib) - START / synchronizace online hry. */
inline bool atariOnlineZabal(Machine &m, AtariOnlineKontext &k, std::vector<uint8_t> &surova, std::vector<uint8_t> &ven) {
  atariOnlineUloz(m, k, surova, ATARI_STAV_MEDIA);
  uLongf mez = compressBound((uLong)surova.size());
  ven.resize(4 + (size_t)mez);
  atariLe32(ven.data(), (uint32_t)surova.size());
  if (compress2(ven.data() + 4, &mez, surova.data(), (uLong)surova.size(), 1) != Z_OK) return false;
  ven.resize(4 + (size_t)mez);
  return true;
}

/** Rozbalit a nahrat stav od druheho telefonu. false = poskozeny / cizi (jina verze appky). */
inline bool atariOnlineRozbal(Machine &m, AtariOnlineKontext &k, const std::vector<uint8_t> &d, std::vector<uint8_t> &surova) {
  if (d.size() < 5) return false;
  const uint32_t n = atariR32(d.data());
  if (n < 12 || n > (64u << 20)) return false;
  surova.resize(n);
  uLongf m2 = n;
  if (uncompress(surova.data(), &m2, d.data() + 4, (uLong)(d.size() - 4)) != Z_OK || m2 != n) return false;
  return atariOnlineNahraj(m, k, surova.data(), surova.size());
}

// ---------------------------------------------------------------------------------------------- kruh pro opravu
/**
 * Ulozene stavy pro opravu odhadu (ODHAD_MAX + 2 snimku zpet - oprava je nejvys ODHAD_MAX + 1). Stav jen pred
 * snimkem spocitanym s odhadem (jinak nic). Bez obsahu cartridge (ROM se nemeni) a diskety (AtariStavDisketa:
 * kopie jen po zapisu). Pamet kruhu zustava (zadne alokace za hry), uvolni se po konci online hry.
 */
template <uint32_t N> class AtariOnlineKruh {
 public:
  void uloz(Machine &m, AtariOnlineKontext &k, uint32_t snimek) {
    Polozka &p = kruh_[snimek % N];
    atariOnlineUloz(m, k, p.data, 0);
    p.disk = disketa_.uloz(m);
    p.snimek = snimek;
  }
  /** Stav pred snimkem zpet do stroje. false = neni ulozeny (nemelo by nastat - otisky rozdil poznaji). */
  bool vrat(Machine &m, AtariOnlineKontext &k, uint32_t snimek) {
    Polozka &p = kruh_[snimek % N];
    if (p.snimek != snimek || p.data.empty()) return false;
    const long long zapisuPred = m.disk.writes;
    if (!atariOnlineNahraj(m, k, p.data.data(), p.data.size())) return false;
    disketa_.vrat(m, p.disk, zapisuPred);
    return true;
  }
  /** Zapomenout (novy start, synchronizace); uvolnit = konec online hry (pamet zpet systemu). */
  void zahod(bool uvolnit) {
    for (Polozka &p : kruh_) {
      p.snimek = UINT32_MAX;
      p.disk.reset();
      if (uvolnit) std::vector<uint8_t>().swap(p.data);
    }
    disketa_.zapomen();
  }
  size_t bajtu() const { size_t s = 0; for (const Polozka &p : kruh_) s += p.data.capacity(); return s; }

 private:
  struct Polozka {
    uint32_t snimek = UINT32_MAX;
    std::vector<uint8_t> data;
    std::shared_ptr<const std::vector<uint8_t>> disk;
  };
  Polozka kruh_[N];
  AtariStavDisketa disketa_;
};

}  // namespace nap
