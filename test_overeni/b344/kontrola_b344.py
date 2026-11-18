#!/usr/bin/env python3
"""B344: zapojeni ve zdrojich appky (staticka kontrola).

  Z1-Z6) Bluetooth: klepnuti na nalezene zarizeni rozhoduje NapBluetoothTexty.poKlepnuti; sparovany nepripojeny
         ovladac -> znovuParuj (removeBond pres reflexi s nahradni radou, pojistka 8 s), po BOND_NONE createBond,
         "hledani skoncilo" jen u naseho hledani, prijimac zustane, dokud se paruje znovu
  S1)    OPTIONS: popisek "spárovaný, klepni = spárovat znovu" jen u ovladace
  N1-N2) napoveda CZ / EN: "Spárovat znovu" / "Pair it again", stejna stavba
  A1-A3) zadni strana Atari: bez CO TESTOVAT, starych poznamek k buildum a JS kroku; Java bez zapisKrok;
         en_atari.js bez prekladu smazanych textu
  V)     verze 387 "EMU10-B344-BT-ZNOVU-SPAROVAT"
Pouziti (z test_overeni/b344): python3 kontrola_b344.py -> VYSTUP_kontrola_b344.txt
"""
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
M = os.path.join(KOREN, 'app/src/main')
J = os.path.join(M, 'java/eu/atarihelp/emu10')
A = os.path.join(M, 'assets')
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def cti(*c):
    with open(os.path.join(*c), encoding='utf-8') as f:
        return f.read()


def metoda(src, hlava):
    if hlava not in src:
        return ''
    a = src.index(hlava)
    i = src.index('{', a)
    hl = 0
    for k in range(i, len(src)):
        if src[k] == '{':
            hl += 1
        elif src[k] == '}':
            hl -= 1
            if hl == 0:
                return src[a:k + 1]
    return src[a:]


BT = cti(J, 'NapBluetooth.java')
BTT = cti(J, 'NapBluetoothTexty.java')
sp = metoda(BT, 'void sparuj(final String adresa)')
over('NapBluetoothTexty.poKlepnuti(sparovany, NapBluetoothTexty.jeOvladac(tr, jm), pripojeny)' in sp
     and 'K_ZNOVU.equals(co)) {\n                    znovuParuj(d, adresa, jm);' in sp and 'uz je sparovany - staci ho zapnout' not in BT,
     'Z1) klepnuti na nalezene zarizeni: poKlepnuti(sparovany, ovladac, pripojeny); sparovany ovladac -> znovuParuj '
     '(do B343 jen "uz je sparovany - staci ho zapnout" = slepa ulicka)')
over('hidStav(d) == NapBluetoothTexty.HID_PRIPOJEN' in sp and 'mezi(ovladac.jmenaPripojenych(), jm)' in sp,
     'Z2) pripojeny = HID profil Androidu nebo vstupni zarizeni (pripojeny ovladac se nikdy nerusi)')
zp = metoda(BT, 'private void znovuParuj(')
over('zapniPrijimac();' in zp and 'znovuAdresa = adresa' in zp and 'zrusSparovaniSkryte(d)' in zp
     and '"true".equals(r)' in zp and 'znovuRucne(adresa, "Android zruseni nedovolil")' in zp
     and 'NapBluetoothTexty.LIMIT_ZRUSENI_MS' in zp,
     'Z3) znovuParuj: prijimac zapnuty, removeBond, kdyz nejde nebo do 8 s nedobehne -> rada NASTAVENI BLUETOOTH')
over('getMethod("removeBond").invoke(d)' in metoda(BT, 'private static String zrusSparovaniSkryte(')
     and 'catch (Throwable t) { return "nejde ("' in BT,
     'Z4) removeBond jen reflexi s osetrenim (skryte API, Android 9 pusti; novejsi -> nahradni rada)')
zm = metoda(BT, 'private void zmenaParovani(')
over('NapBluetoothTexty.znovuPoZruseni(znovuAdresa, adr, stav)' in zm and 'paruj(d, adr, j, "znovu ")' in zm
     and zm.index('znovuPoZruseni') < zm.index('BOND_BONDING'),
     'Z5) BOND_NONE u ovladace, kteremu se rusilo sparovani -> za 400 ms createBond (pred obecnym "nepovedlo")')
hs = metoda(BT, 'private void hledaniSkoncilo()')
zs = metoda(BT, 'private void zastavUi()')
over('if (nase) log("hledani skoncilo' in hs and 'if (!znovu) vypniPrijimac();' in zs,
     'Z6) "hledani skoncilo" jen u naseho hledani (log TV: Nastaveni Androidu 12x); prijimac zustane pri parovani znovu')
over('static String poKlepnuti(boolean sparovany, boolean ovladac, boolean pripojeny)' in BTT
     and 'case F_ZNOVU:' in BTT and 'case F_ZNOVU_RUCNE:' in BTT and 'Jinak ho spáruj znovu (PŘIPOJIT OVLADAČ)' not in BTT,
     'Z7) NapBluetoothTexty: poKlepnuti, faze ZNOVU / ZNOVU_RUCNE, "neozval se" uz neposila do slepe ulicky')

IX = cti(A, 'index.html')
over("(d.s?(d.o?J(' – spárovaný, klepni = spárovat znovu',' – paired, tap = pair again'):J(' – spárovaný',' – paired')):'')" in IX,
     'S1) OPTIONS: "spárovaný, klepni = spárovat znovu" jen u ovladace (reproduktor dal "spárovaný")')

CZ, EN = cti(A, 'napoveda/index.html'), cti(A, 'napoveda/en.html')
over('<li><b>Spárovat znovu:</b>' in CZ and 'appka staré spárování zruší a spáruje ho znovu sama' in CZ
     and 'vrací jen k poslednímu zařízení, se kterým hrál' in CZ
     and '(vypni ho a zapni znovu, případně ho v <span class="t">NASTAVENÍ BLUETOOTH</span> odeber' not in CZ,
     'N1) napoveda CZ: "Spárovat znovu" v appce, proc se ovladac neozve (vraci se k poslednimu zarizeni)')
over('<li><b>Pair it again:</b>' in EN and 'the app removes the old pairing and pairs it again by itself' in EN,
     'N1) napoveda EN: "Pair it again"')
st = lambda h: [len(re.findall(v, h)) for v in (r'<li[ >]', r'<details>', r'<dt>', r'class="blok"', r'href="#')]
over(st(CZ) == st(EN), 'N2) stejna stavba CZ / EN (li, details, dt, bloky, odkazy): %s' % st(CZ))

AT = cti(A, 'emu_atari_cpp/index.html')
over('CO TESTOVAT' not in re.sub(r'<!--.*?-->', '', AT, flags=re.S) and 'id="kroky"' not in AT and 'var KROKY' not in AT
     and 'vykresliKroky' not in AT and '.krok{' not in AT,
     'A1) zadni strana Atari: bez sekce CO TESTOVAT (od B314 prazdna) a jejiho JS / CSS')
VIDET = re.sub(r'<script\b.*?</script>', '', re.sub(r'<!--.*?-->', '', AT, flags=re.S), flags=re.S)   # bez komentaru a skriptu
over(not re.search(r'\bB3\d\d: ', VIDET) and not re.search(r'\bB29\d: ', VIDET) and 'Tenhle test pousti jadro' in AT
     and 'This test runs the core' in AT and 'id="btnPoznamka"' in AT and 'id="btn"' in AT,
     'A2) bez starych poznamek k buildum B296-B312 (CZ i EN); vysvetleni testu jadra, ZAPSAT a test zustaly')
MA = cti(J, 'MainActivity.java')
EA = cti(A, 'jazyk/en_atari.js')
over('zapisKrok' not in MA and 'CO TESTOVAT' not in EA and 'odklepnuto' not in EA and 'V poradku:' not in EA
     and "'Zadni strana pristroje: log a test jadra.': 'The back of the machine: log and the core test.'" in EA,
     'A3) Java bez zapisKrok, en_atari.js bez prekladu smazanych textu, novy podtitul prelozeny')

bg = cti(KOREN, 'app/build.gradle')
over('versionCode 387' in bg and 'versionName "EMU10-B344-BT-ZNOVU-SPAROVAT"' in bg, 'V) build.gradle: 387 EMU10-B344-BT-ZNOVU-SPAROVAT')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_kontrola_b344.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
