#!/usr/bin/env python3
"""B340: kontrola zapojeni (hra jen dalkovym ovladacem TV: cisla podle konzole, START v menu ZPET, podrzeni
sipky, log tlacitek, rozlozeni na obrazovce, oprava testu b314).
Pouziti: python3 test_overeni/b340/kontrola_b340.py   -> VYSTUP_kontrola_b340.txt
"""
import os
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))


def cti(p):
    return open(os.path.join(R, p), encoding='utf-8').read()


MA = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
ND = cti('app/src/main/java/eu/atarihelp/emu10/NapDalkovy.java')
DS = cti('app/src/main/java/eu/atarihelp/emu10/NapDrzeniSmeru.java')
OV = cti('app/src/main/cpp/nap_ovladac.h')
GR = cti('app/build.gradle')
NCZ = cti('app/src/main/assets/napoveda/index.html')
NEN = cti('app/src/main/assets/napoveda/en.html')
T314 = cti('test_overeni/b314/test_b314_jazyk_menu.py')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


def blok(text, zac, kon):
    i = text.index(zac)
    return text[i:text.index(kon, i + len(zac))]


# ---- D) dalkovy ve hre ----
nk = blok(MA, '    private boolean napDalkovyKlavesa(KeyEvent e) {', '\n    }\n')
hra = blok(nk, 'if (kam == OVL_PS1 || kam == OVL_SEGA || kam == OVL_ATARI) {', '        // stranky:')
over('NapDalkovy.proHru(kod, kon)' in hra and 'NapDalkovy.KONZOLE_PS1' in hra and 'NapDalkovy.KONZOLE_SEGA' in hra,
     'D1 ve hre klavesa podle konzole (NapDalkovy.proHru(kod, kon) - cisla PS1 / SEGA)')
over('kon == NapDalkovy.KONZOLE_ATARI && cis >= 0' in hra and 'NativeAtariCoreBridge.devPcKlavesaSafe("Digit" + cis, String.valueOf(cis), 0, dolu)' in hra
     and hra.index('devPcKlavesaSafe') < hra.index('NapDalkovy.proHru(kod, kon)'),
     'D2 Atari: cislice = klavesy 0-9 Atari (klavesnice v C++ B302 - drzeni, tempo pro OS), pred prevodem na gamepad')
over('napDrzeni.udalost(k, NapDalkovy.jeSmer(k), dolu, android.os.SystemClock.uptimeMillis(),' in hra
     and '(kk, dd) -> t[0] = napDalkovyDoJadra(kam, kk, dd)' in hra and 'if (napDrzeni.cekaPusteni()) napDrzeniHlidej();' in hra
     and '"B340 PODRZENI SIPKY ("' in hra,
     'D3 klavesy jdou do jadra pres podrzeni sipky (NapDrzeniSmeru) + casovac + log B340 PODRZENI SIPKY')
over('napDalkovyZapis(e, OVL_KAM[kam])' in hra and 'e.getScanCode()' in MA and '"B340 DALKOVY TLACITKO: "' in MA
     and 'napDalkovyZapsano.size() >= 60' in MA,
     'D4 kazde tlacitko ovladace ve hre jednou do logu (kod, scan, jmeno zarizeni) - i ta, co nic nedelaji')
dj = blok(MA, '    private int napDalkovyDoJadra(int kam, int k, boolean dolu) {', '\n    }\n')
over('NativePs1CoreBridge.ovladacKlavesaSafe(k, dolu)' in dj and 'NativeSegaCoreBridge.ovladacKlavesaSafe(k, dolu)' in dj
     and 'NativeAtariCoreBridge.devOvladacKlavesaSafe(k, dolu)' in dj,
     'D5 do jadra stejne mosty jako B334 (C++ nap_ovladac.h rozhodne, co tlacitko udela)')
hl = blok(MA, '    private void napDrzeniHlidej() {', '\n    }\n')
over('napDrzeni.tik(android.os.SystemClock.uptimeMillis()' in hl and 'ui.postDelayed(this, 50)' in hl and 'napDrzeniHlidaSe = false' in hl,
     'D6 casovac podrzene sipky na vlaknu UI (jako udalosti), konci, kdyz se neceka')
ps = blok(MA, '                napAktualniUrl = url == null ? "" : url;', '// B321: nesparovany')
over('napDrzeni.zrus();' in ps, 'D7 nova stranka: podrzena sipka se zapomene')

# ---- P) prevod (NapDalkovy) a podrzeni (NapDrzeniSmeru) ----
over('static int proHru(int k, int konzole)' in ND and 'PS1_CISLA[c]' in ND and 'SEGA_CISLA[c]' in ND
     and 'static int proHru(int k) {' in ND and 'static boolean jeSmer(int k)' in ND and 'static int cislice(int k)' in ND,
     'P1 NapDalkovy: cisla PS1 (kolem petky) / SEGA (rady A B C) / ATARI, puvodni proHru(k) zustava (JVM TestB340)')
over('if (t & (T_X | T_L1)) r |= 1u << 4;      // leve (a L1) = A' in OV and 'if (t & T_A) r |= 1u << 5;               // spodni = B' in OV
     and 'if (t & T_Y) r |= 1u << 9;                // horni = TROJUHELNIK' in OV,
     'P2 nap_ovladac.h beze zmeny: SEGA leve = A, spodni = B; PS1 horni = TROJUHELNIK (na tom stoji rozlozeni cisel)')
over('static final long OKNO_MS = 120;' in DS and 'static final long CEKANI_MS = 250;' in DS and 'void zrus()' in DS
     and 'import android' not in DS,
     'P3 NapDrzeniSmeru: cisty Java (okno 120 ms, cekani 250 ms, zrus) - JVM test vcetne 5000 nahodnych sekvenci')

# ---- M) menu ZPET ----
km = blok(MA, '    private void konzoleTvMenu(final boolean ps1) {', '\n    }\n')
over('napDalkovyPustVse("menu konzole " + jm);' in km and 'tx("▶ START (začít hru / pauza)", "▶ START (start the game / pause)")' in km
     and 'tx("TLAČÍTKO A", "BUTTON A")' in km and 'tx("TLAČÍTKO C", "BUTTON C")' in km
     and 'final int[] akce = ps1 ? new int[]{0, 4, 5, 6, 1, 2, 3, 9, -1} : new int[]{0, 5, 7, 8, 10, 1, 2, 9, -1};' in km
     and km.count('tx("OVLÁDÁNÍ DÁLKOVÝM OVLADAČEM", "PLAYING WITH THE TV REMOTE")') == 2,
     'M1 menu konzole SEGA / PS1: START (PS1 i SELECT, Sega A / B / C), OVLADANI, pri otevreni vse pustit')
ka = blok(MA, '    private void konzoleTvAkce(boolean ps1, int akce) {', '\n    }\n')
over('napTvStiskni(ps1 ? OVL_PS1 : OVL_SEGA, KeyEvent.KEYCODE_BUTTON_START, "START")' in ka
     and 'napTvStiskni(OVL_SEGA, KeyEvent.KEYCODE_BUTTON_X, "A")' in ka and 'napTvStiskni(OVL_SEGA, KeyEvent.KEYCODE_BUTTON_A, "B")' in ka
     and 'napTvStiskni(OVL_SEGA, KeyEvent.KEYCODE_BUTTON_B, "C")' in ka and 'napTvOvladaniDialog(ps1 ? OVL_PS1 : OVL_SEGA)' in ka,
     'M2 akce: START / SELECT / A B C jako klavesy gamepadu (stejne jako cisla), OVLADANI = dialog')
am = blok(MA, '    private void atariCppTvMenu() {', '\n    }\n')
over('napDalkovyPustVse("menu pristroje Atari");' in am and '"▶ START",' in am and 'case 1: napTvStiskni(OVL_ATARI, KeyEvent.KEYCODE_BUTTON_START, "START")' in am
     and 'KeyEvent.KEYCODE_BUTTON_MODE, "OPTION")' in am and 'case 10: napTvOvladaniDialog(OVL_ATARI)' in am
     and am.index('tx("⌂ HLAVNÍ MENU (konec Atari)"') < am.index('"▶ START",'),
     'M3 menu pristroje Atari: HLAVNI MENU porad prvni, pak START / SELECT / OPTION, OVLADANI')
st = blok(MA, '    private void napTvStiskni(final int kam, final int k, final String jm) {', '\n    }\n')
over('napDalkovyDoJadra(kam, k, true); } catch (Throwable ignored) {} }, 150);' in st
     and 'napDalkovyDoJadra(kam, k, false); } catch (Throwable ignored) {} }, 400);' in st,
     'M4 stisk z menu: po zavreni dialogu (150 ms), drzeno 250 ms (hra ho uvidi nekolik snimku), pak pusteni')

# ---- O) rozlozeni na obrazovce, napoveda ----
ts = blok(MA, '    private void napTvStrankaNactena(String url) {', '\n    }\n')
over('isSegaNativeOwnerUrl(url) || isPs1OwnerUrl(url) || isAtariCppOwnerUrl(url)' in ts and '2 4 6 8 = △ □ ○ ✕' in ts
     and 'řada čísel = A B C' in ts and 'čísla = klávesy 0–9' in ts,
     'O1 vstup do konzole na TV: kratke rozlozeni (PS1, SEGA, nove i Atari)')
od = blok(MA, '    private void napTvOvladaniDialog(int kam) {', '\n    }\n')
over('2 = △ · 4 = □ · 6 = ○ · 8 = ✕' in od and '1 4 7 = A · 2 5 8 = B · 3 6 9 = C' in od and 'čísla 0–9 = klávesy 0–9 na Atari' in od
     and 'appka šipku podrží' in od,
     'O2 OVLADANI DALKOVYM OVLADACEM: cele rozlozeni vsech tri konzoli + podrzeni sipky (CZ / EN)')
over('<b>Hraní dálkovým ovladačem:</b>' in NCZ and '<span class="t">OVLÁDÁNÍ DÁLKOVÝM OVLADAČEM</span>' in NCZ
     and '<b>Playing with the TV remote:</b>' in NEN and '<span class="t">PLAYING WITH THE TV REMOTE</span>' in NEN,
     'O3 napoveda CZ / EN: hrani dalkovym ovladacem')
over('versionCode 383' in GR and 'versionName "EMU10-B340-TV-DALKOVY-OVLADAC"' in GR, 'O4 build.gradle: versionCode 383 "EMU10-B340-TV-DALKOVY-OVLADAC"')

# ---- T) oci: test b314 ----
over("('B320', ['HERNÍ OVLADAČ', 'AUTOMATICKY', 'VŽDY DOTYK', 'SKRÝT DOTYK'])" in T314
     and "'1c) cesky: zamcena SEGA, SPUSTIT -> stranka ODEMKNOUT (%s)'" in T314
     and "('SPUSTIT u zamcene SEGY (hlaska)'" not in T314,
     'T1 test b314 menu opraven: SPUSTIT u zamcene SEGY = stranka ODEMKNOUT (B322, kontrola 1c), zamerne zmeny B320-B339 vyjmenovane')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b340.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
