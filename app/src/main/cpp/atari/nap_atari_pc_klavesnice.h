// nap_atari_pc_klavesnice.h
// B302: KLAVESNICE POCITACE -> ATARI 130XE V HELP (prohlizec na TV / PC).
//
// Rene: "udelej to (klavesnici z PC do C++ Atari ve web vieweru) - a mysli
// na to, ze vse v C++ pokud mozno". Prohlizec jen preda, co mu dal system:
//   code = MISTO klavesy (e.code: "KeyA", "Digit1", "Enter", "ArrowUp" ...)
//   znak = co klavesa NAPSALA podle rozlozeni (e.key: "a", "A", "+", "ě" ...)
//   mod  = 1 Shift, 2 Ctrl, 4 Alt, 8 AltGr, 16 opakovani od systemu (drzena)
// Vsechno ostatni je tady v C++ (bez Androidu - testuje se na PC):
//
//  1) PcKlavesnice::udalost - co ktera klavesa udela na Atari:
//     - PSANI: znak podle rozlozeni (ceska klavesnice: ě -> E, AltGr znaky
//       # & @ < > [ ] \ | jako na PC), Enter, Backspace, sipky = kurzor
//       (CONTROL + - = + *), Delete / Insert / Home = DELETE / INSERT / CLEAR,
//       Esc, Tab, CapsLock = CAPS, F6 = HELP, F8 = INVERZE (logo Atari),
//       Ctrl + klavesa = CONTROL (graficke znaky).
//     - HRANI (F9): WASD / sipky = joystick, K nebo mezernik = skok (nahoru
//       + MEZERA), L = FIRE; WASD jde i jako klavesa (hry v BASICu s PEEK(764)).
//     - vzdy: F1 START, F3 SELECT, F4 OPTION (drzene jako na skrini), F2 BREAK.
//     Pamatuje si, co ktera DRZENA klavesa udelala, a pri pusteni vrati
//     presne to (i kdyz se mezitim zmenil Shift nebo rezim).
//  2) PcPrehravac::krok - jednou za snimek preda klavesy stroji tak, aby je
//     OS Atari opravdu prevzal (po siti muze prijit nekolik klaves naraz):
//     - stisk nejdriv 2 snimky po predchozim,
//     - STEJNA klavesa znovu az 5 snimku po pusteni (OS ji jinak zahodi:
//       KEYDEL $02F1 odpocitava ve VBI jen pri pustene klavese, OS $C1A1),
//     - pri psani az kdyz OS prevzal predchozi znak (CH $02FC = $FF),
//     - klavesa drzena aspon 3 snimky; drzi-li se dal jina, zustava stisknuta
//       (KBCODE = ta drzena), opakovani dela OS sam jako na skutecnem Atari.
//     Cekani ma vzdy strop (hra, ktera CH necte, nic nezablokuje).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_runtime.h"

namespace nap {

// Co se ma stat se strojem. KLAVESA / PUSTIT / BREAK jdou pres PcPrehravac
// (v poradi a v tempu, ktere OS stihne), JOY a KONZOLE hned.
struct PcAkce {
  enum T { KLAVESA = 0, PUSTIT = 1, BREAK = 2, JOY = 3, KONZOLE = 4 };
  int t = KLAVESA;
  int v = 0;               // KLAVESA/PUSTIT: scankod (0-63, +$40 SHIFT, +$80 CONTROL)
                           // JOY: 1 nahoru, 2 dolu, 4 vlevo, 8 vpravo, 16 FIRE
                           // KONZOLE: 1 START, 2 SELECT, 4 OPTION (1 = drzeno)
  bool cekatNaOs = false;  // KLAVESA: pockat, az OS prevezme predchozi znak (jen PSANI)
};

// ---------------------------------------------------------------------
//  znak (e.key) -> scankod Atari
// ---------------------------------------------------------------------
// UTF-8 -> kod jednoho znaku; -1, kdyz to neni prave jeden znak (nazvy jako
// "Enter", "Shift", "Unidentified" maji vic znaku)
inline long pcJedenZnak(const std::string &s) {
  if (s.empty()) return -1;
  const unsigned char *p = (const unsigned char *)s.data();
  const size_t n = s.size();
  long cp;
  size_t len;
  if (p[0] < 0x80) { cp = p[0]; len = 1; }
  else if ((p[0] & 0xE0) == 0xC0 && n >= 2 && (p[1] & 0xC0) == 0x80) { cp = ((long)(p[0] & 0x1F) << 6) | (p[1] & 0x3F); len = 2; }
  else if ((p[0] & 0xF0) == 0xE0 && n >= 3 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
    cp = ((long)(p[0] & 0x0F) << 12) | ((long)(p[1] & 0x3F) << 6) | (p[2] & 0x3F); len = 3;
  } else return -1;
  return len == n ? cp : -1;
}

// Pismena s diakritikou -> zakladni pismeno: napBezDiakritiky (nap_atari_runtime.h)
inline int pcBezDiakritiky(long cp) { return napBezDiakritiky(cp); }

// Napsany znak -> scankod. Male pismeno = klavesa sama (OS Atari po startu
// pise velka - BASIC chce prikazy velkymi), velke pismeno = SHIFT + klavesa,
// symboly jako na klavesnici 130XE (" = SHIFT+2, @ = SHIFT+8, : = SHIFT+; ...).
inline int pcZnakNaScan(const std::string &znak) {
  long c = pcJedenZnak(znak);
  if (c < 0) return -1;
  if (c >= 0x80) { const int z = pcBezDiakritiky(c); if (z < 0) return -1; c = z; }
  if (c >= 'A' && c <= 'Z') return scanZakladni((int)(c - 'A' + 'a')) | 0x40;
  if (c == 0) return -1;
  const int b = scanZakladni((int)c);
  if (b >= 0) return b;
  static const char SH[] = "!\"#$%&'@():?^_|\\[]";
  static const char BASE[] = "1234567890;/*-=+,.";
  for (int i = 0; SH[i]; i++) if (SH[i] == c) return scanZakladni(BASE[i]) | 0x40;
  return -1;
}

// Kdyz klavesa zadny znak nenapsala (rozlozeni, ktere prohlizec nezna, IME,
// mrtva klavesa): podle MISTA na klavesnici jako na americke.
inline int pcKodNaScan(const std::string &c) {
  if (c.size() == 4 && c.compare(0, 3, "Key") == 0 && c[3] >= 'A' && c[3] <= 'Z') return scanZakladni(c[3] - 'A' + 'a');
  if (c.size() == 6 && c.compare(0, 5, "Digit") == 0 && c[5] >= '0' && c[5] <= '9') return scanZakladni(c[5]);
  if (c.size() == 7 && c.compare(0, 6, "Numpad") == 0 && c[6] >= '0' && c[6] <= '9') return scanZakladni(c[6]);
  struct K { const char *kod; char z; bool shift; };
  static const K T[] = {
    {"Minus", '-', false}, {"Equal", '=', false}, {"Comma", ',', false}, {"Period", '.', false},
    {"Slash", '/', false}, {"Semicolon", ';', false}, {"Quote", '7', true}, {"BracketLeft", ',', true},
    {"BracketRight", '.', true}, {"Backslash", '+', true}, {"IntlBackslash", '<', false}, {"Space", ' ', false},
    {"NumpadAdd", '+', false}, {"NumpadSubtract", '-', false}, {"NumpadMultiply", '*', false},
    {"NumpadDivide", '/', false}, {"NumpadDecimal", '.', false}, {"NumpadComma", ',', false},
    {"NumpadEqual", '=', false},
  };
  for (const K &k : T) if (c == k.kod) { const int b = scanZakladni(k.z); return b < 0 ? -1 : (b | (k.shift ? 0x40 : 0)); }
  return -1;
}

// Klavesy, ktere nic nepisou, ale na Atari neco delaji (podle MISTA).
inline int pcSpecialni(const std::string &c, bool shift, bool ctrl) {
  if (c == "Enter" || c == "NumpadEnter") return 12;                          // RETURN
  if (c == "Backspace") return 52 | (shift ? 0x40 : (ctrl ? 0x80 : 0));      // BACK S / SHIFT = smazat radek / CTRL = smazat znak
  if (c == "Delete") return 52 | (shift ? 0x40 : 0x80);                       // DELETE znak (CTRL+BACK S) / SHIFT = smazat radek
  if (c == "Insert") return 55 | (shift ? 0x40 : 0x80);                       // INSERT znak (CTRL+>) / SHIFT = vlozit radek
  if (c == "Home") return 54 | 0x40;                                           // CLEAR (SHIFT+<) - smazat obrazovku
  if (c == "Escape") return 28;                                                // ESC
  if (c == "Tab") return 44 | (shift ? 0x40 : (ctrl ? 0x80 : 0));            // TAB / SHIFT = SET TAB / CTRL = CLR TAB
  if (c == "CapsLock") return 60 | (shift ? 0x40 : (ctrl ? 0x80 : 0));       // CAPS: mala/velka, SHIFT = velka, CTRL = grafika
  if (c == "ArrowUp") return 14 | 0x80;                                        // kurzor = CONTROL + - = + *
  if (c == "ArrowDown") return 15 | 0x80;
  if (c == "ArrowLeft") return 6 | 0x80;
  if (c == "ArrowRight") return 7 | 0x80;
  if (c == "F6") return 17;                                                    // HELP
  if (c == "F8") return 39;                                                    // INVERZE (klavesa s logem Atari)
  return -1;
}

inline bool pcJePrehazovac(const std::string &c) {
  return c == "ShiftLeft" || c == "ShiftRight" || c == "ControlLeft" || c == "ControlRight" ||
         c == "AltLeft" || c == "AltRight" || c == "MetaLeft" || c == "MetaRight" || c == "OSLeft" ||
         c == "OSRight" || c == "ContextMenu" || c == "NumLock" || c == "ScrollLock" || c == "AltGraph" ||
         c == "Fn" || c == "FnLock" || c == "Hyper" || c == "Super";
}

// pro log a pro prohlizec: jen tisknutelne ASCII (JNI chce platne UTF-8)
inline std::string pcAscii(const std::string &s, size_t max = 40, bool mezery = false) {
  std::string o;
  for (unsigned char ch : s) {
    if (o.size() >= max) break;
    o.push_back((ch >= 0x21 && ch <= 0x7E) || (mezery && ch == ' ') ? (char)ch : '?');
  }
  return o;
}

// ---------------------------------------------------------------------
//  1) MAPOVANI, REZIM A DRZENE KLAVESY
// ---------------------------------------------------------------------
struct PcKlavesnice {
  enum { SHIFT = 1, CTRL = 2, ALT = 4, ALTGR = 8, OPAKOVANI = 16 };
  bool hrani = false;                  // F9: PSANI <-> HRANI
  struct Drzena { std::string code; int sc; int joy; int kon; };
  std::vector<Drzena> drzene;          // co ktera prave drzena klavesa udelala

  int joyMaska() const { int m = 0; for (const auto &d : drzene) m |= d.joy; return m & 31; }
  int konMaska() const { int m = 0; for (const auto &d : drzene) m |= d.kon; return m & 7; }

  // pustit vse (F9, okno prohlizece ztratilo zamereni, odpojeni)
  void pustitVse(std::vector<PcAkce> &out) {
    for (const auto &d : drzene) if (d.sc >= 0) out.push_back({PcAkce::PUSTIT, d.sc, false});
    drzene.clear();
    out.push_back({PcAkce::JOY, 0, false});
    out.push_back({PcAkce::KONZOLE, 0, false});
  }

  // Hlidani spojeni: drzi-li prohlizec klavesu, posila kazdych 0,8 s "Zije".
  // Kazda zprava (i opakovani a "Zije") = prohlizec zije. Kdyz se 2,5 s neozve
  // (zavreny panel, spadla Wi-Fi), vse se pusti - jinak by Atari drzenou
  // klavesu opakovalo do nekonecna a joystick by zustal drzeny.
  static const long long TICHO_MS = 2500;
  long long posledniMs = 0;
  void zprava(long long ms) { posledniMs = ms; }
  bool hlidej(long long ms, std::vector<PcAkce> &out) {
    if (drzene.empty() || ms - posledniMs <= TICHO_MS) return false;
    pustitVse(out);
    return true;
  }

  // HRANI: smer joysticku (bity), klavesa navic (-1 = zadna), popis
  static int hraniJoy(const std::string &c, int *sc, const char **co) {
    *sc = -1; *co = nullptr;
    if (c == "KeyW") { *sc = 46; return 1; }
    if (c == "KeyS") { *sc = 62; return 2; }
    if (c == "KeyA") { *sc = 63; return 4; }
    if (c == "KeyD") { *sc = 58; return 8; }
    if (c == "ArrowUp") return 1;
    if (c == "ArrowDown") return 2;
    if (c == "ArrowLeft") return 4;
    if (c == "ArrowRight") return 8;
    if (c == "KeyK" || c == "Space") { *sc = 33; *co = "SKOK"; return 1; }
    if (c == "KeyL") { *co = "VYSTREL"; return 16; }
    return 0;
  }

  // Jedna udalost z prohlizece. Akce pro stroj prida do out, vrati text pro
  // prohlizec: "PSANI"/"HRANI" (F9), "SKOK", "VYSTREL", "SMER:KeyW",
  // "START"..., "BREAK", "OK:KeyA=63", "PUSTENO", "DRZENO", "NIC", "NEZNAMA:...".
  std::string udalost(const std::string &code0, const std::string &znak, int mod, bool dolu, std::vector<PcAkce> &out) {
    const std::string code = pcAscii(code0, 32);
    if (code == "PustVse") { pustitVse(out); return "PUSTENO_VSE"; }
    if (code == "Zije") return "ZIJE";         // prohlizec: klavesa je porad drzena (hlidani ztraceneho spojeni)
    if (code.empty()) return "PRAZDNA";
    if (!dolu) {
      for (size_t i = 0; i < drzene.size(); i++) {
        if (drzene[i].code != code) continue;
        const Drzena d = drzene[i];
        drzene.erase(drzene.begin() + (long)i);
        if (d.sc >= 0) out.push_back({PcAkce::PUSTIT, d.sc, false});
        if (d.joy) out.push_back({PcAkce::JOY, joyMaska(), false});
        if (d.kon) out.push_back({PcAkce::KONZOLE, konMaska(), false});
        return "PUSTENO";
      }
      return "PUSTENO";
    }
    // drzena klavesa: system ji opakuje - Atari opakuje samo (OS), nic
    for (const auto &d : drzene) if (d.code == code) return "DRZENO";
    const bool opak = (mod & OPAKOVANI) != 0;
    // jednorazove klavesy: opakovani od systemu nic nedela
    if (code == "F9") {
      if (opak) return "DRZENO";
      hrani = !hrani;
      pustitVse(out);
      return hrani ? "HRANI" : "PSANI";
    }
    if (code == "F2" || code == "Break" || code == "Pause") {
      if (opak) return "DRZENO";
      out.push_back({PcAkce::BREAK, 0, false});
      return "BREAK";
    }
    if (code == "F1" || code == "F3" || code == "F4") {
      const int k = code == "F1" ? 1 : (code == "F3" ? 2 : 4);
      drzene.push_back({code, -1, 0, k});
      out.push_back({PcAkce::KONZOLE, konMaska(), false});
      return k == 1 ? "START" : (k == 2 ? "SELECT" : "OPTION");
    }
    if (pcJePrehazovac(code)) return "NIC";
    if (drzene.size() >= 16) pustitVse(out);   // pojistka: ztracene pusteni (nemelo by se stat)
    if (hrani) {
      int sc; const char *co;
      const int joy = hraniJoy(code, &sc, &co);
      if (joy) {
        drzene.push_back({code, sc, joy, 0});
        out.push_back({PcAkce::JOY, joyMaska(), false});
        if (sc >= 0) out.push_back({PcAkce::KLAVESA, sc, false});
        return co ? std::string(co) : "SMER:" + code;
      }
    }
    // AltGr (ceska klavesnice: # & @ < > [ ] \ |) Windows hlasi jako Ctrl+Alt - to neni CONTROL
    const bool altgr = (mod & ALTGR) || ((mod & CTRL) && (mod & ALT));
    const bool ctrl = (mod & CTRL) && !altgr;
    const bool shift = (mod & SHIFT) != 0;
    int sc = pcSpecialni(code, shift, ctrl);
    if (code == "CapsLock") {
      // jen tuknuti (Mac posila stisk pri zapnuti a pusteni az pri vypnuti;
      // drzeny CapsLock by jinak prepinal porad dokola)
      if (opak) return "DRZENO";
      out.push_back({PcAkce::KLAVESA, sc, !hrani});
      out.push_back({PcAkce::PUSTIT, sc, false});
      return "OK:" + code + "=" + std::to_string(sc);
    }
    if (sc < 0) {
      sc = pcZnakNaScan(znak);
      if (sc < 0 && pcJedenZnak(znak) < 0) sc = pcKodNaScan(code);   // klavesa nenapsala zadny znak
      if (sc >= 0 && ctrl) sc = (sc & 0x3F) | 0x80;                   // CONTROL + klavesa (bez SHIFT)
    }
    if (sc < 0) {
      const std::string z = pcAscii(znak, 12);
      return "NEZNAMA:" + code + (z.empty() ? std::string() : "/" + z);
    }
    drzene.push_back({code, sc, 0, 0});
    out.push_back({PcAkce::KLAVESA, sc, !hrani});
    return "OK:" + code + "=" + std::to_string(sc);
  }
};

// ---------------------------------------------------------------------
//  B303: TEXT VLOZENY Z PC (Ctrl+V v prohlizeci, pretazeny TXT soubor)
// ---------------------------------------------------------------------
// Je to text? Pretazeny XEX / ATR / BAS ulozeny pres SAVE / obrazek se psat
// nesmi - Atari by dostalo stovky nesmyslnych klaves.
inline bool pcJeText(const uint8_t *d, size_t n) {
  if (n == 0) return false;
  auto zacina = [&](const char *s, size_t k) { return n >= k && std::memcmp(d, s, k) == 0; };
  if (zacina("\xFF\xFF", 2) || zacina("\x96\x02", 2)) return false;              // XEX, ATR
  if (zacina("PK\x03\x04", 4) || zacina("RIFF", 4) || zacina("FUJI", 4) || zacina("\x89PNG", 4) || zacina("%PDF", 4) ||
      zacina("\xFF\xD8\xFF", 3) || zacina("GIF8", 4))
    return false;                                                                 // ZIP, WAV, CAS, obrazky, PDF
  bool atascii = false;                                                           // vypis z Atari (LIST "D:") ma EOL $9B
  if (!napJeUtf8(d, n)) for (size_t i = 0; i < n; i++) if (d[i] == 0x9B) { atascii = true; break; }   // (v UTF-8 je $9B kus pismene)
  size_t divne = 0;
  for (size_t i = 0; i < n; i++) {
    const uint8_t b = d[i];
    if (b == 0 && !atascii) return false;                                         // v textu z PC nula nebyva (UTF-16, binarka)
    if ((b < 0x20 && b != '\t' && b != '\n' && b != '\r' && b != 0x1B) || b == 0x7F) divne++;
  }
  return divne * 10 <= n;                                                         // ridice znaky nejvys 10 % (grafika v retezcich)
}
// Program = aspon jeden radek zacina cislem (pak ho napise PsaniProgramu
// bez NEW - rychle a s malymi pismeny v retezcich presne).
inline bool pcJeProgram(const uint8_t *d, size_t n) {
  bool atascii = false;
  const auto radky = PsaniProgramu::radkySouboru(d, n, atascii);
  for (const auto &r : radky) {
    size_t a = 0; while (a < r.size() && r[a] == ' ') a++;
    if (a < r.size() && r[a] >= '0' && r[a] <= '9') return true;
  }
  return false;
}
// Jiny text (prikaz, odpoved na INPUT ...) se napise jako klavesy: kazdy znak
// stisk + pusteni, konec radku = RETURN. Vraci pocet znaku, ktere na Atari nejsou.
inline int pcTextNaKlavesy(const uint8_t *d, size_t n, std::vector<PcAkce> &out, size_t maxZnaku) {
  int nelze = 0; size_t znaku = 0;
  for (size_t i = 0; i < n && znaku < maxZnaku;) {
    const uint8_t b = d[i];
    int dl = b < 0x80 ? 1 : (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 1;
    if (i + (size_t)dl > n) dl = 1;
    const std::string z((const char *)d + i, (size_t)dl);
    i += (size_t)dl;
    int sc;
    if (b == '\r') continue;
    if (b == '\n' || b == 0x9B) sc = 12;                 // konec radku = RETURN (i ATASCII EOL)
    else if (b == '\t') sc = 33;
    else sc = pcZnakNaScan(z);
    if (sc < 0) { nelze++; continue; }
    out.push_back({PcAkce::KLAVESA, sc, true});
    out.push_back({PcAkce::PUSTIT, sc, false});
    znaku++;
  }
  return nelze;
}

// ---------------------------------------------------------------------
//  2) PREDANI KLAVES STROJI V TEMPU, KTERE OS STIHNE
// ---------------------------------------------------------------------
struct PcPrehravac {
  std::deque<PcAkce> q;                // KLAVESA / PUSTIT / BREAK v poradi, jak prisly
  struct Drz { int sc; int snimku; };
  std::vector<Drz> drzene;             // klavesy z pocitace prave drzene na Atari (poradi stisku)
  int odStisku = 1000;                 // snimku od posledniho stisku (i BREAK)
  int posledniSc = -1;                 // posledni stisknuta klavesa (OS: CH1 $02F2)
  int uvolneno = 1000;                 // snimku, kdy neni drzena zadna klavesa (OS: KEYDEL odpocitava)
  int cekam = 0;                       // snimku, co ceka akce na rade

  void zrus() { q.clear(); drzene.clear(); odStisku = 1000; posledniSc = -1; uvolneno = 1000; cekam = 0; }
  bool prazdny() const { return q.empty(); }
  void pridej(const PcAkce &a) { if (q.size() < 256) q.push_back(a); }
  int najdi(int sc) const { for (size_t i = 0; i < drzene.size(); i++) if (drzene[i].sc == sc) return (int)i; return -1; }

  // Jednou za KAZDY snimek PRED runFrame (i s prazdnou frontou - pocita
  // snimky). Do 'stisky' prida, co prave slo do stroje (scankod, -1 = BREAK)
  // - kvuli logu. smiStisk = false: nove klavesy pockaji (appka zrovna sama
  // pise program z TXT), pusteni probehne.
  void krok(Machine &m, std::vector<int> *stisky, bool smiStisk = true) {
    if (odStisku < 100000) odStisku++;
    if (drzene.empty()) { if (uvolneno < 100000) uvolneno++; } else uvolneno = 0;
    for (auto &d : drzene) if (d.snimku < 100000) d.snimku++;
    while (!q.empty()) {
      const PcAkce a = q.front();
      if (a.t == PcAkce::KLAVESA) {
        if (najdi(a.v) >= 0) {                       // druha klavesa PC se stejnym vyznamem (Enter + Enter na numericke)
          drzene.push_back({a.v, 0}); q.pop_front(); cekam = 0; continue;
        }
        if (!smiStisk || odStisku < 2) return;
        // stejna klavesa: OS ji vezme az po KEYDEL (3 VBI s pustenou klavesou)
        if (a.v == posledniSc && uvolneno < 5 && cekam < 15) { cekam++; return; }
        // pri psani: predchozi znak musi OS prevzit (jinak by se prepsal)
        if (a.cekatNaOs && m.mem.ram[0x2FC] != 0xFF && cekam < 12) { cekam++; return; }
        m.klavesa(a.v & 0xFF, true);
        drzene.push_back({a.v, 0});
        posledniSc = a.v; odStisku = 0; uvolneno = 0; cekam = 0;
        if (stisky) stisky->push_back(a.v & 0xFF);
        q.pop_front();
        continue;
      }
      if (a.t == PcAkce::PUSTIT) {
        const int i = najdi(a.v);
        if (i < 0) { q.pop_front(); cekam = 0; continue; }     // neni drzena (studeny start mezitim) - nic
        if (drzene[(size_t)i].snimku < 3) return;               // aspon 3 snimky drzena
        drzene.erase(drzene.begin() + i);
        if (drzene.empty()) m.klavesaPustena();
        else m.kbcode = (uint8_t)(drzene.back().sc & 0xFF);    // drzi se dal jina klavesa (bez noveho preruseni)
        q.pop_front(); cekam = 0;
        continue;
      }
      if (a.t == PcAkce::BREAK) {
        if (!smiStisk || odStisku < 2) return;
        m.breakKey();
        odStisku = 0;
        if (stisky) stisky->push_back(-1);
        q.pop_front(); cekam = 0;
        continue;
      }
      q.pop_front();                                             // JOY / KONZOLE resi volajici hned
    }
  }
};

}  // namespace nap
