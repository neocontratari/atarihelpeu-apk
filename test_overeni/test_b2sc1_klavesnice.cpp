/* test_b2sc1_klavesnice.cpp
 *
 * BUILD2SC1: klavesnice + konzolova tlacitka atari 130XE v C++
 * (app/src/main/cpp/atari/nap_atari_keyboard.h). Stejny princip jako
 * ostatni testy v tomto adresari - #include REALNEHO produkcniho
 * zdroje (ne kopie/prepis), spustitelne na jakemkoli Linuxu s g++,
 * BEZ Android/NDK (ten tu k dispozici neni - viz PREDAVACI_PROTOKOL).
 *
 * CO TENHLE TEST OVERUJE (a co NE):
 *  - OVERUJE: render() nespadne a vyrobi rozmanity obrazek (ne jen
 *    jedna barva); hitTest() trefi presne tu klavesu/tlacitko, na
 *    jehoz STRED se "sahne" (tj. souradnice z vlastni geometrie
 *    souhlasi s tim, co hitTest pocita - nejde o nahodu); SHIFT
 *    tuknuti-vs-drzeni (320ms), CTRL instantni zamek, konzolovy
 *    soucasny drzeny soucet (START+OPTION najednou), a jednorazove
 *    akce (HELP/RESET/BREAK) delaji presne to, co ma - vse porovnano
 *    se zdrojovymi daty (scankody), ne odhadnuto.
 *  - NEOVERUJE (a nemuze): jak to vypada na skutecnem telefonu (zadne
 *    opravdove vykreslovani na obrazovku, jen pocitani pixelu do
 *    pameti), ani realny NDK/arm64 preklad (ten dela az GitHub
 *    Actions - zadny NDK v tomhle kontejneru neni).
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../app/src/main/cpp/atari/nap_atari_keyboard.h"

using namespace nap;

static int g_fail = 0;
#define OK(cond, msg) do { if(!(cond)){ printf("FAIL  %s\n", msg); g_fail++; } else printf("OK    %s\n", msg); } while(0)

int main() {
  KbdDeck d;

  // ---- 1) tabulka: 57 klaves, vsechny scankody v platnem rozsahu ----
  int realKeys = 0, special = 0;
  for (int i = 0; i < 57; i++) {
    int s = NAP_KBD_KEYS[i].scan;
    if (s == -1 || s == -2 || s == -3) special++;
    else if (s >= 0 && s <= 63) realKeys++;
    else { printf("FAIL  scankod mimo rozsah u klavesy %d: %d\n", i, s); g_fail++; }
  }
  printf("info  klaves celkem=57 realnych_scankodu=%d specialnich(ctrl/shift/break)=%d\n", realKeys, special);
  OK(realKeys + special == 57, "vsechny scankody klasifikovany (realny 0-63, nebo -1/-2/-3)");

  // ---- 2) render() nespadne a neni to jedna plocha barvy ----
  d.render(1000);
  int ruznychBarev = 0; uint8_t prvni[3] = { d.fb[0], d.fb[1], d.fb[2] };
  for (int i = 0; i < KbdDeck::W * KbdDeck::H; i++) {
    if (d.fb[i*3]!=prvni[0] || d.fb[i*3+1]!=prvni[1] || d.fb[i*3+2]!=prvni[2]) { ruznychBarev=1; break; }
  }
  OK(ruznychBarev == 1, "render() vyrobil neco jineho nez jednu plochou barvu");

  // Syrovy RGB dump pro VIZUALNI kontrolu (prevod na PNG je v
  // JAK_SPUSTIT.txt) - cislicove kontroly vyse nestaci, tohle je jediny
  // zpusob, jak se na vysledek doopravdy "podivat" bez Android telefonu.
  { FILE *f = fopen("/tmp/kbd_render.rgb", "wb"); if (f) { fwrite(d.fb, 1, sizeof(d.fb), f); fclose(f); } }
  // a jeste jednou se SHIFT zamceny (zlata klavesa) a par klavesami/
  // konzoli "zmacknutymi", at je vizualne videt i pressed/shiftLatched stav
  {
    KbdDeck d2;
    d2.touchDown(43, 9000); d2.touchUp(43, 9010);     // SHIFT zamek ON
    d2.touchDown(30, 9100);                            // 'A' drzene (jen pro obrazek, neuvolnujeme)
    d2.touchDown(101, 9100); d2.touchDown(103, 9100);   // START+OPTION drzene
    d2.render(9200);
    FILE *f = fopen("/tmp/kbd_render_pressed.rgb", "wb"); if (f) { fwrite(d2.fb, 1, sizeof(d2.fb), f); fclose(f); }
  }

  // ---- 3) hitTest trefi presne tu klavesu, na jejiz stred sahneme ----
  int hitFail = 0;
  for (int i = 0; i < 57; i++) {
    const KbdKeyDef &k = NAP_KBD_KEYS[i];
    int cx = KbdDeck::X(k.x + k.w/2), cy = KbdDeck::Y(k.y + k.h/2);
    int got = d.hitTest(cx, cy);
    if (got != i) { printf("FAIL  hitTest klavesa %d (scan=%d lab=%s): stred (%d,%d) vratil %d\n", i, k.scan, k.lab, cx, cy, got); hitFail++; }
  }
  for (int i = 0; i < 5; i++) {
    const KbdConsoleDef &c = NAP_KBD_CONSOLE[i];
    int cx = KbdDeck::X(c.x + c.w/2), cy = KbdDeck::Y(c.y + c.h/2);
    int got = d.hitTest(cx, cy);
    if (got != 100+i) { printf("FAIL  hitTest konzole %d (%s): stred (%d,%d) vratil %d\n", i, c.lab, cx, cy, got); hitFail++; }
  }
  OK(hitFail == 0, "hitTest() trefuje presny stred VSECH 57 klaves + 5 konzolovych tlacitek");
  OK(d.hitTest(0,0) == -1, "hitTest() v levem hornim rohu (mimo klavesy) vraci -1");

  // ---- 4) normalni klavesa: 'A' (index v tabulce = radek2, 2. klaves = id 28) ----
  // rada0=15(id0-14), rada1=14(id15-28)... pozor, 'A' je PRVNI klavesa rady 2 (po CTRL).
  // rada0: id 0-14 (15 klaves), rada1: id 15-28 (14 klaves), rada2 zacina id 29 (CTRL), 'A'=id 30.
  int idA = 30;
  OK(strcmp(NAP_KBD_KEYS[idA].lab, "A") == 0, "index 30 v tabulce je skutecne klavesa 'A' (sanity select)");
  KbdEvent ev = d.touchDown(idA, 2000);
  OK(ev.typ == KbdEvent::KLAVESA && ev.scan == 63, "tuknuti na 'A' bez SHIFT/CTRL posle scankod 63 (KLAVESA)");
  d.touchUp(idA, 2001);

  // ---- 5) SHIFT: kratke tuknuti = zamek (tap-lock) ----
  // rada2 ma 14 klaves (id29-42), rada3 zacina id43 (SHIFT).
  int idShift1 = 43;
  OK(NAP_KBD_KEYS[idShift1].scan == -2, "index 43 je skutecne SHIFT (sanity select)");
  d.touchDown(idShift1, 3000);
  d.touchUp(idShift1, 3050);      // pusteno za 50ms - KRATKE tuknuti
  OK(d.shiftLatched == true, "kratke tuknuti SHIFT: zamek SE ZAPNE");
  ev = d.touchDown(idA, 3100);
  OK(ev.scan == (63|0x40), "po zamceném SHIFT dá 'A' scankod s bitem 0x40 (127)");
  d.touchUp(idA, 3101);
  d.touchDown(idShift1, 3200); d.touchUp(idShift1, 3210);
  OK(d.shiftLatched == false, "druhe kratke tuknuti SHIFT: zamek SE VYPNE");

  // ---- 6) SHIFT: drzeni > 320ms = docasne (konci pustenim, ne dalsim tuknutim) ----
  d.touchDown(idShift1, 4000);
  d.render(4350);   // 350ms drzeni ubehlo - render() je ten, kdo kontroluje cas (viz komentar v hlavicce)
  OK(d.shiftHeld == true && d.shiftLatched == true, "drzeni SHIFT > 320ms aktivuje docasny rezim (bez nutnosti pustit)");
  d.touchUp(idShift1, 4400);
  OK(d.shiftLatched == false, "puseni po DRZENI SHIFT zamek VYPNE (ne prepne)");

  // ---- 7) CTRL: instantni zamek PRI STISKU (ne pri pusteni) ----
  int idCtrl = 29; // prvni klavesa rady2
  OK(NAP_KBD_KEYS[idCtrl].scan == -1, "index 29 je skutecne CTRL (sanity select)");
  d.touchDown(idCtrl, 5000);
  OK(d.ctrlLatched == true, "CTRL se prepne OKAMZITE pri stisku");
  d.touchUp(idCtrl, 5001);
  OK(d.ctrlLatched == true, "CTRL puseni samo o sobe nic nemeni (jen dalsi stisk prepne zpet)");
  ev = d.touchDown(idA, 5100);
  OK(ev.scan == (63|0x80), "po zamceném CTRL da 'A' scankod s bitem 0x80");
  d.touchUp(idA, 5101);
  d.touchDown(idCtrl, 5200); // zpet vypnout pro dalsi testy
  OK(d.ctrlLatched == false, "druhy stisk CTRL zamek vypne");

  // ---- 8) konzolovy pas: START/SELECT/OPTION drzi soucasne (cold-boot gesto) ----
  // id 100=HELP,101=START,102=SELECT,103=OPTION,104=RESET
  ev = d.touchDown(101, 6000); // START
  OK(ev.typ == KbdEvent::KONZOLE_MASK && ev.konzoleMask == (7&~1), "START dolu: maska bez bitu0 (6)");
  ev = d.touchDown(103, 6001); // OPTION soucasne
  OK(ev.typ == KbdEvent::KONZOLE_MASK && ev.konzoleMask == (7&~(1|4)), "START+OPTION soucasne drzene: maska bez bitu0+bitu2 (2)");
  ev = d.touchUp(101, 6100); // START puseno, OPTION stale drzene
  OK(ev.konzoleMask == (7&~4), "po puseni START (OPTION stale drzen): maska bez bitu2 (3)");
  ev = d.touchUp(103, 6101);
  OK(ev.konzoleMask == 7, "po puseni obou: maska zpet na 7 (nic drzeno)");

  // ---- 9) jednorazove akce: HELP (klavesova matice, ne konzole), RESET, BREAK ----
  ev = d.touchDown(100, 7000); // HELP
  OK(ev.typ == KbdEvent::KLAVESA && ev.scan == 17, "HELP posila scankod 17 (KBCODE_HELP) jako normalni klavesu, NE konzolovy prepinac");
  ev = d.touchUp(100, 7001);
  OK(ev.typ == KbdEvent::NIC, "puseni HELP nic nedela (jednorazove, stejne jako realna matice)");
  ev = d.touchDown(104, 7100); // RESET
  OK(ev.typ == KbdEvent::RESET, "RESET tlacitko posila RESET akci");
  ev = d.touchDown(14, 7200);  // BREAK je posledni klavesa rady0 (id14)
  OK(NAP_KBD_KEYS[14].scan == -3 && ev.typ == KbdEvent::BREAK, "BREAK (id14) posila BREAK akci");

  printf(g_fail ? "\n%d CHYB(A) - NEOPRAVIT, NEZAMLCET\n" : "\nVSECHNY KONTROLY PROSLY (%d chyb)\n", g_fail);
  return g_fail ? 1 : 0;
}
