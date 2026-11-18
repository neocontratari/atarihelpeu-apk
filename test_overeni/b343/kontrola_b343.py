#!/usr/bin/env python3
"""B343: TOTALNI KONTROLA NAPOVEDY - kazde tvrzeni, ktere B343 v napovede opravil, proti skutecnemu kodu appky.

Rene: "udelej totalni kontrolu HELP - mam pocit ze tam neco nesedi a nedelas v kazdem Buildu aktualizaci".

  N1)  zapati: zadne natvrdo cislo buildu (do B342 "B322"), verze z appky (AHJAZYK.verze jako OPTIONS)
  N2)  klavesnice SEGA v napovede = MAPA v emu_sega/index.html (K i mezernik = A, P i Enter = START, sipky)
  N3)  klavesnice PS1: F / G = L3 / R3 (cpp/nap_ps1_native.cpp KeyF 14, KeyG 15)
  N4)  menu televize (ZPET) - kazda polozka z napovedy je v MainActivity (Atari, SEGA, PS1; CZ i EN)
  N5)  slabe zarizeni: pravidlo v napovede = NapPravidla.jeSlabeZarizeni (32 bitu, < 1500 MHz, <= 2 jadra)
  N6)  prehravac: formaty v castych otazkach = AUDIO_JMENO v player/index.html
  N7)  misto na PS1 hru: 2x velikost + 200 MB = NapPravidla.ps1PotrebaMista / PS1_REZERVA, hlaska MALO MISTA
  N8)  prvni spusteni: poradi jako v appce (jazyk -> stazeni Sonic / BIOS -> aktivace -> film), film na TV OK,
       na LOW se nepousti (MainActivity)
  N9)  soubory: BIOS je v PS1_BIOS (ne emu/ps1), Android 9 bez povoleni -> slozka appky
  N10) OPTIONS: Vykon zarizeni a Uloziste popsane, popisky v assets/index.html
  N11) log z televize: /log a /log/predchozi na portu 8765 (MainActivity)
  N12) nove caste otazky (otoceni, dalkovy ovladac) vedou na existujici mista
  N13) cesky a anglicky navod maji stejnou stavbu
  A1)  PS1 panel kolecka na vysku: bez Google Drive, tlacitko HRY ZE SERVERU N&P -> AHNET.openPs1Games
  A2)  PS1 panel: kazdy cesky text ma anglicky preklad (en_ps1.js)
  A3)  SEGA NAVODY: kratky navod pro hrace (ne BUILD2RV), vejde se na obrazovku konzole, anglicky preklad
  A4)  Atari HELP (MainActivity): NA SIRKU tlacitkem, N&P HRY, L1 / R1, televize, odkaz na napovedu, CZ s diakritikou
  A5)  SEGA: ULOŽENÉ POZICE (kde je export logu), menu vykonu (LOW zarizeni drzi LOW), navod = napisy z obrazku
  A6)  PS1: pruh stahovani "pripojuji server" (ne Drive)
  V)   verze 386 "EMU10-B343-HELP-KONTROLA"
Pouziti (z test_overeni/b343): python3 kontrola_b343.py   -> VYSTUP_kontrola_b343.txt
"""
import html
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
M = os.path.join(KOREN, 'app/src/main')
A = os.path.join(M, 'assets')
J = os.path.join(M, 'java/eu/atarihelp/emu10')

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


def text(h):
    """viditelny text HTML (bez skriptu a stylu, entity rozbalene, mezery slite)"""
    h = re.sub(r'<(script|style)\b.*?</\1>', ' ', h, flags=re.S)
    h = re.sub(r'<!--.*?-->', ' ', h, flags=re.S)
    h = re.sub(r'<[^>]+>', '', h)
    return re.sub(r'\s+', ' ', html.unescape(h))


def kapitola(h, kid):
    a = h.index('id="%s"' % kid)
    b = h.find('<section class="kap"', a + 10)
    return h[a:b if b > 0 else len(h)]


CZ = cti(A, 'napoveda/index.html')
EN = cti(A, 'napoveda/en.html')
CZT, ENT = text(CZ), text(EN)
MA = cti(J, 'MainActivity.java')
NP = cti(J, 'NapPravidla.java')

# ---------------- N1 zapati ----------------
for jm, h, t in (('CZ', CZ, CZT), ('EN', EN, ENT)):
    zap = re.search(r'<footer class="zapati">(.*?)</footer>', h, re.S).group(1)
    over('<span id="zapatiVerze"></span>' in zap and not re.search(r'B\d{3}', text(zap)),
         'N1) %s zapati bez natvrdo cisla buildu (do B342 "ATARIHELP.EU · B322"), misto pro verzi z appky' % jm)
    js = re.search(r"getElementById\('zapatiVerze'\).*?\}\)\(\);", h, re.S)
    over(js is not None and 'AHJAZYK.verze' in js.group(0) and "split('|')" in js.group(0) and 'try{' in js.group(0),
         'N1) %s zapati: verze z AHJAZYK.verze() ("B343 (386)"), bez mostu nic (try)' % jm)
    over(not re.search(r'\bB3\d\d\b', t), 'N1) %s viditelny text navodu nema zadne natvrdo cislo buildu' % jm)
over('public String verze()' in MA or 'String verze()' in MA, 'N1) most AHJAZYK.verze() v MainActivity existuje')

# ---------------- N2 SEGA klavesy ----------------
sega = cti(A, 'emu_sega/index.html')
mapa = re.search(r"var MAPA=\{(.*?)\};", sega, re.S).group(1)
ocek = {'KeyK': 'A', 'Space': 'A', 'KeyL': 'B', 'KeyO': 'C', 'KeyP': 'START', 'Enter': 'START',
        'KeyW': 'UP', 'ArrowUp': 'UP'}
over(all(re.search(r"\b%s:'%s'" % (k, v), mapa) for k, v in ocek.items()),
     'N2) MAPA Segy: K i mezernik = A, L = B, O = C, P i Enter = START, WASD i sipky')
for jm, t, mez, sip in (('CZ', CZT, 'mezerník', 'šipky'), ('EN', ENT, 'space', 'arrows')):
    tip = kapitola(CZ if jm == 'CZ' else EN, 'sega')
    tt = text(tip)
    over(('K nebo %s = A' % mez if jm == 'CZ' else 'K or %s = A' % mez) in tt
         and ('P nebo Enter = START' if jm == 'CZ' else 'P or Enter = START') in tt and sip in tt,
         'N2) %s kapitola Sega (TIP): K / %s = A, P / Enter = START, %s' % (jm, mez, sip))
    tv = text(kapitola(CZ if jm == 'CZ' else EN, 'tv'))
    over(('WASD / %s %s, K / %s = A, L = B, O = C, P / Enter = START' % (sip, 'pohyb' if jm == 'CZ' else 'move', mez)) in tv,
         'N2) %s kapitola WEB TV (klavesnice SEGA): stejne' % jm)

# ---------------- N3 PS1 L3 / R3 ----------------
ps1c = cti(M, 'cpp/nap_ps1_native.cpp')
over('{"KeyF", 14}' in ps1c and '{"KeyG", 15}' in ps1c, 'N3) cpp/nap_ps1_native.cpp: KeyF = 14 (L3), KeyG = 15 (R3)')
for jm, h in (('CZ', CZ), ('EN', EN)):
    for kap in ('ps1', 'tv'):
        tt = text(kapitola(h, kap))
        over(re.search(r'F / G =? ?L3 / R3', tt) is not None, 'N3) %s kapitola %s: F / G = L3 / R3' % (jm, kap))

# ---------------- N4 menu televize ----------------
atv = MA[MA.index('private void atariCppTvMenu()'):MA.index('private void atariTvDoMenu(')]
ktv = MA[MA.index('private void konzoleTvMenu('):MA.index('private void konzoleTvAkce(')]
nob = cti(J, 'NapObrazovka.java')
pol_cz = {'atari': ['HLAVNÍ MENU', '▶ START', '"SELECT"', '"OPTION"', 'hry ze serveru N&P', 'XEX / SOUBOR', 'ATR / DISK',
                    'KAZETA', 'TURBO BASIC XL', 'RESET ATARI', 'NapObrazovka.tlacitko', 'OVLÁDÁNÍ DÁLKOVÝM OVLADAČEM', 'NÁPOVĚDA'],
          'konzole': ['HRY ZE SERVERU N&P', 'HRY V TELEVIZI', '▶ START', 'TLAČÍTKO A', 'ULOŽIT POZICI', 'NAČÍST ULOŽENOU POZICI',
                      'RESET (BIOS BEZ CD)', 'OVLÁDÁNÍ DÁLKOVÝM OVLADAČEM', 'HLAVNÍ MENU']}
over(all(p in atv for p in pol_cz['atari']), 'N4) Java: menu pristroje Atari na TV ma vsechny polozky z napovedy')
over(all(p in ktv for p in pol_cz['konzole']) and 'NapObrazovka.tlacitko' in ktv,
     'N4) Java: menu konzole SEGA / PS1 na TV ma vsechny polozky z napovedy')
over('"OBRAZOVKA NA VÝŠKU"' in nob and '"SCREEN TO PORTRAIT"' in nob, 'N4) NapObrazovka: OBRAZOVKA NA VÝŠKU / SCREEN TO PORTRAIT')
tvc, tve = text(kapitola(CZ, 'tv')), text(kapitola(EN, 'tv'))
over(all(x in tvc for x in ('HLAVNÍ MENU, pod ním ▶ START, SELECT, OPTION', 'XEX / soubor', 'OBRAZOVKA NA VÝŠKU',
                            'OVLÁDÁNÍ DÁLKOVÝM OVLADAČEM', 'u PS1 HRY V TELEVIZI', 'u Segy tlačítka A / B / C')),
     'N4) CZ kapitola WEB TV: menu Atari i SEGA / PS1 jako v appce (B336-B342)')
over(all(x in tve for x in ('MAIN MENU at the top, then ▶ START, SELECT, OPTION', 'XEX / file', 'SCREEN TO PORTRAIT',
                            'PLAYING WITH THE TV REMOTE', 'on the PS1 GAMES ON THIS TV', 'on the Sega the A / B / C buttons')),
     'N4) EN kapitola WEB TV: stejne anglicky')
over(all(x in atv + ktv for x in ('"GAMES FROM THE N&P SERVER"', '"GAMES ON THIS TV (downloaded – play / delete)"',
                                  '"PLAYING WITH THE TV REMOTE"')), 'N4) Java: anglicke nazvy polozek, ktere uvadi en.html')
over('STAHNOUT' in tvc and '"STAHNOUT"' in cti(J, 'NapStahovaniSeSouhlasem.java'),
     'N4) CZ: Sonic a BIOS na TV "po STAHNOUT" = tlacitko okna (NapStahovaniSeSouhlasem)')

# ---------------- N5 slabe zarizeni ----------------
over('return !bit64 || (maxMhz > 0 && maxMhz < 1500) || (jader > 0 && jader <= 2);' in NP,
     'N5) NapPravidla.jeSlabeZarizeni: 32 bitu, pod 1500 MHz nebo nejvys 2 jadra')
over(CZT.count('32bitový Android, procesor pod 1,5 GHz nebo nejvýš 2 jádra') == 2 and 'nejvýš 1,5 GHz' not in CZT,
     'N5) CZ: pravidlo slabeho zarizeni stejne v OPTIONS i WEB TV (do B342 "jen 32 bitů, nejvýš 1,5 GHz")')
over(ENT.count('32-bit Android, a processor under 1.5 GHz or at most 2 cores') == 2, 'N5) EN: stejne')

# ---------------- N6 prehravac ----------------
pl = re.search(r'var AUDIO_JMENO=/\\\.\((.*?)\)\$/i', cti(A, 'player/index.html')).group(1).split('|')
fm = re.search(r'Přehrávač hraje ([^.]*)\.', CZT).group(1)
over(all(f.upper() in fm for f in pl if f != 'oga'), 'N6) CZ FAQ: prehravac hraje %s (AUDIO_JMENO: %s)' % (fm, ','.join(pl)))
fe = re.search(r'The player plays ([^.]*)\.', ENT).group(1)
over(all(f.upper() in fe for f in pl if f != 'oga'), 'N6) EN FAQ: %s' % fe)

# ---------------- N7 misto na PS1 hru ----------------
over('PS1_REZERVA = 200L * 1024 * 1024' in NP and 'delka * 2 + PS1_REZERVA' in NP,
     'N7) NapPravidla: stazeni PS1 hry chce 2x velikost + 200 MB')
over('asi 2× velikost hry + 200 MB' in CZT and 'MÁLO MÍSTA' in CZT and 'about 2× the game\'s size + 200 MB' in ENT
     and 'LOW STORAGE' in ENT, 'N7) navod: 2× velikost + 200 MB, MÁLO MÍSTA / LOW STORAGE')
over('"MÁLO MÍSTA: potřeba ~"' in NP and '"LOW STORAGE: needs ~"' in NP and '"CD ISO → koš"' in NP
     and '"BACK → GAMES ON THIS TV"' in NP, 'N7) NapPravidla.ps1MaloMista: stejne nazvy jako navod')
over('MÁLO MÍSTA V TELEVIZI' in MA and 'tx("HRY V TELEVIZI", "GAMES ON THIS TV"), (d, w) -> ps1HryVTv()' in MA,
     'N7) TV: okno MALO MISTA nabidne rovnou HRY V TELEVIZI (navod: "appka rovnou nabídne")')

# ---------------- N8 prvni spusteni ----------------
for jm, h, slova, nad in (('CZ', CZ, ('Nejdřív si vybereš', 'stáhnout z atarihelp.eu', 'aktivuje', 'úvodní film'), 'První spuštění'),
                          ('EN', EN, ('First you choose', 'download them from atarihelp.eu', 'activates', 'intro film'), 'The first start')):
    blok = re.search(r'<ul>(.*?)</ul>', h[h.index('<h3>%s</h3>' % nad):], re.S).group(1)
    li = [text(x) for x in re.findall(r'<li>(.*?)</li>', blok, re.S)]
    poz = [next((i for i, x in enumerate(li) if s in x), -1) for s in slova]
    over(-1 not in poz and poz == sorted(poz), 'N8) %s prvni spusteni: jazyk -> Sonic / BIOS -> aktivace -> film (%s)' % (jm, poz))
over('napIntroUkazano && napVykonLow()' in MA and 'NapDalkovy.preskociDemo(kod)' in MA,
     'N8) Java: na LOW se demo nepousti, OK / PLAY / START ho preskoci (navod: "na televizi tlačítkem OK")')
over('tlačítkem <span class="t">OK</span>' in CZ and 'se film nepouští' in CZT, 'N8) CZ: film na TV tlacitkem OK, na LOW se nepousti')

# ---------------- N9 soubory ----------------
sb = text(kapitola(CZ, 'soubory'))
over('emu/ps1' not in CZ and 'emu/ps1' not in EN and '<dt>PS1_BIOS</dt><dd>BIOS pro PlayStation' in CZ
     and '<dt>PS1_BIOS</dt><dd>The PlayStation BIOS' in EN,
     'N9) Soubory: BIOS v PS1_BIOS (do B342 "emu/sega, emu/ps1" - emu/ps1 appka nepouziva)')
over('new File(root, "PS1_BIOS")' in MA and 'new File(root, "BIOS")' in MA, 'N9) Java: BIOS hleda v PS1_BIOS, BIOS i v koreni')
over('Na Androidu 9 a starším bez povolení úložiště' in sb, 'N9) CZ: Android 9 bez povoleni -> slozka appky (getPublicAtariHelpDownloadsDir)')

# ---------------- N10 OPTIONS ----------------
ix = cti(A, 'index.html')
op = text(kapitola(CZ, 'options'))
over('>VÝKON ZAŘÍZENÍ<' in ix and '>ÚLOŽIŠTĚ<' in ix and 'stVykonAuto' in ix and 'stUloziste' in ix,
     'N10) assets/index.html: OPTIONS -> VÝKON ZAŘÍZENÍ (AUTO / LOW / HIGH) a ÚLOŽIŠTĚ')
over('Výkon zařízení VÝKON ZAŘÍZENÍ – AUTO / LOW / HIGH' in op and 'Úložiště ÚLOŽIŠTĚ ukazuje' in op,
     'N10) CZ navod OPTIONS: bloky Výkon zařízení a Úložiště (do B342 chybely)')
eo = text(kapitola(EN, 'options'))
over('Device performance DEVICE PERFORMANCE – AUTO / LOW / HIGH' in eo and 'Storage STORAGE shows' in eo,
     'N10) EN navod OPTIONS: Device performance a Storage')
over('když atarihelp.eu nejde, zkusí server N&P' in op, 'N10) CZ STÁHNOUT HRY A BIOS: zaloha server N&P')

# ---------------- N11 log z televize ----------------
over('new ServerSocket(8765)' in MA and '"/log/predchozi".equals(path)' in MA and '"/log".equals(path)' in MA,
     'N11) Java: WEB TV na portu 8765, /log a /log/predchozi')
lg = text(kapitola(CZ, 'log'))
over('Televize Z televize pošleš log' in lg and '…:8765/log/predchozi' in lg, 'N11) CZ kapitola Log: blok Televize')
over('TV From a TV you send the log' in text(kapitola(EN, 'log')), 'N11) EN kapitola Log: blok TV')

# ---------------- N12 nove caste otazky ----------------
for jm, h, q in (('CZ', CZ, ('Jak otočím obrazovku na šířku nebo na výšku?', 'Jak hraju na televizi jen dálkovým ovladačem?')),
                 ('EN', EN, ('How do I turn the screen to landscape or portrait?', 'How do I play on a TV with just the remote?'))):
    ok = True
    for o in q:
        m = re.search(r'<summary>%s</summary>(.*?)</details>' % re.escape(o), h, re.S)
        ok = ok and m is not None and all(('id="%s"' % c) in h for c in re.findall(r'href="#([^"]+)"', m.group(1)))
    over(ok, 'N12) %s caste otazky: otoceni obrazovky a dalkovy ovladac, odkazy vedou na existujici mista' % jm)
over('Šipky = směr · OK = ✕' in MA and 'Šipky = směr · OK = B (skok)' in MA and 'Šipky = joystick · OK = FIRE' in MA,
     'N12) Java (OVLADANI DALKOVYM OVLADACEM): OK = PS1 ✕, Sega B, Atari FIRE - jak pise otazka')

# ---------------- N13 stavba CZ = EN ----------------
def stavba(h):
    return {k: len(re.findall(v, h)) for k, v in (('section', r'<section class="kap"'), ('blok', r'class="blok"'),
                                                   ('details', r'<details>'), ('dt', r'<dt>'), ('li', r'<li[ >]'),
                                                   ('kbd', r'<kbd>'), ('a#', r'href="#'))}
sc, se = stavba(CZ), stavba(EN)
over(sc == se, 'N13) stejna stavba CZ / EN: %s' % ', '.join('%s %d' % kv for kv in sc.items()))

# ---------------- A1 / A2 PS1 panel ----------------
ps1 = cti(A, 'emu_ps1/index.html')
hp = re.search(r'<div id="helpPanel".*?</div>\s*</div>', ps1, re.S).group(0)
over('Google' not in text(hp) and 'Drive' not in text(hp) and 'AtariHelp.eu' not in text(hp),
     'A1) PS1 panel: zadny Google Drive ani "PS1 stranka na AtariHelp.eu" (LOAD GAME je server N&P)')
over('<button id="psGooglePage" type="button">HRY ZE SERVERU N&amp;P</button>' in hp
     and re.search(r"function openPs1GooglePage\(\)\{.*?AHNET\.openPs1Games\(\)", ps1, re.S) is not None
     and "$('btnLoad').addEventListener('click',function(e){e.preventDefault();openPs1GooglePage();}" in ps1,
     'A1) PS1: tlacitko HRY ZE SERVERU N&P (do B342 "PS1 GOOGLE DRIVE") i LOAD GAME -> AHNET.openPs1Games')
enps1 = cti(A, 'jazyk/en_ps1.js')
uzly = []
for p in re.findall(r'<p>(.*?)</p>', hp, re.S):
    for kus in re.split(r'<b>.*?</b>|<span[^>]*>.*?</span>', p):
        t = html.unescape(kus).strip()
        if re.search(r'[A-Za-zÀ-ž]', t):
            uzly.append(t)
for b in re.findall(r'<b>(.*?)</b>', hp):
    uzly.append(html.unescape(b).strip())
chybi = [u for u in uzly if ("'%s'" % u.replace("'", "\\'")) not in enps1 and u not in ('CD ISO', 'LOAD GAME', 'SAVE GAME')
         and "'%s'" % u not in cti(A, 'jazyk/en_konzole.js')]
over(not chybi, 'A2) PS1 panel: vsech %d ceskych textu ma anglicky preklad%s' % (len(uzly), (' - CHYBI: %s' % chybi) if chybi else ''))
over("'HRY ZE SERVERU N&P': 'GAMES FROM THE N&P SERVER'" in enps1 and 'Google Drive links' not in enps1,
     'A2) en_ps1.js: GAMES FROM THE N&P SERVER, stare Google Drive texty pryc')

# ---------------- A3 SEGA NAVODY ----------------
nav = re.search(r"bindUiAction\('btnManuals','manuals',function\(\)\{showMonitor\('(.*?)'\);\}\);", sega).group(1)
radky = nav.split('\\n')
over('BUILD2RV' not in nav and 'SBÍRKA = hry ze serveru N&P' in nav and 'ULOŽENÉ = uložené pozice' in nav and 'Úplný návod' in nav,
     'A3) SEGA NAVODY: navod pro hrace (do B342 technicke poznamky "BUILD2RV ... safe-audit governor")')
over(len(radky) <= 12 and max(len(r) for r in radky) <= 50,
     'A3) SEGA NAVODY: %d radku, nejdelsi %d znaku (obrazovka konzole ma ~16 radku, overflow hidden)' % (len(radky), max(len(r) for r in radky)))
ens = cti(A, 'jazyk/en_sega.js')
m = re.search(r"  '%s':\n    '(.*?)',\n" % re.escape(nav), ens)
over(m is not None and len(m.group(1).split('\\n')) == len(radky) and 'BUILD2RV SEGA CLEAN UI' not in ens,
     'A3) en_sega.js: anglicky navod se stejnym poctem radku, stary BUILD2RV text pryc')

# ---------------- A5 SEGA: dalsi texty v appce ----------------
over('OVLÁDÁNÍ → NÁVODY' not in sega and 'Export logu je jinde: na šířku kolečko ⚙ → EXPORTOVAT LOG.' in sega
     and 'id="segaBtnExportLog" type="button" class="secondary">EXPORTOVAT LOG (DIAGNOSTIKA)<' in sega,
     'A5) SEGA ULOŽENÉ POZICE: export logu = na sirku kolecko -> EXPORTOVAT LOG (do B342 neexistujici "OVLÁDÁNÍ → NÁVODY")')
over('<div class="pmNote">Podržené SBÍRKA = výkon. Při OPTIONS → VÝKON ZAŘÍZENÍ LOW hraje Sega vždy LOW.</div>' in sega
     and 'B338 VYKON: Sega chtela' in MA,
     'A5) SEGA menu vykonu (podrzene SBÍRKA): pise, ze pri VÝKON ZAŘÍZENÍ LOW hraje vzdy LOW (Java B338; do B342 "SBIRKA = vyber vykonu")')
over("'Podržené SBÍRKA = výkon. Při OPTIONS → VÝKON ZAŘÍZENÍ LOW hraje Sega vždy LOW.': 'Hold CATALOG = performance. With OPTIONS → DEVICE PERFORMANCE LOW the Sega always plays LOW.'" in ens
     and 'CONTROLS → MANUALS' not in ens, 'A5) en_sega.js: nove texty anglicky')
for jm, h, slova in (('CZ', CZ, ('<dt>SBÍRKA</dt>', '<dt>ULOŽENÉ</dt>', '<dt>NÁVODY</dt>', 'hraje Sega vždy LOW')),
                     ('EN', EN, ('<dt>CATALOG</dt>', '<dt>SAVES</dt>', '<dt>MANUALS</dt>', 'the Sega always plays LOW'))):
    over(all(x in h for x in slova), 'A5) %s navod Sega: tlacitka jako na obrazku konzole (%s), LOW drzi OPTIONS' % (jm, ', '.join(slova[:3])))
over("showMonitor('SEGA MEGA DRIVE – PŘIPRAVENO\\nSBÍRKA = hry ze serveru N&P" in sega and "showMonitor('SEGA C++ ONLY READY" not in sega
     and "'SEGA MEGA DRIVE – READY\\nCATALOG = N&P server games" in ens and "'SEGA C++ ONLY READY" not in ens,
     'A5) SEGA obrazovka bez hry: co je kde (do B342 "SEGA C++ ONLY READY / SBIRKA=vykon LOW/HIGH")')
over("'PS1: pripojuji server'" in ps1 and 'pripojuji Drive' not in ps1 and 'PS1 Google stranka' not in ps1,
     'A6) PS1 pruh stahovani: "PS1: pripojuji server" (do B342 "pripojuji Drive"), chyba "PS1 hry N&P"')

# ---------------- A4 Atari HELP ----------------
ah = MA[MA.index('private void atariCppHelpDialog()'):MA.index('// BUILD2SK31: sdilena obranna')]
cz = ah[:ah.index('private static final String ATARI_HELP_EN')]
en = ah[ah.index('private static final String ATARI_HELP_EN'):]
over('tlačítko NA SIRKU pod přístrojem' in cz and 'NET / HRY – N&P HRY: hry pro Atari ze serveru N&P' in cz
     and 'A / B (i L1 / R1) = FIRE' in cz and 'TELEVIZE (dálkový ovladač)' in cz and 'Úplný návod: hlavní menu → karta HELP' in cz
     and 'hry z atarihelp.eu' not in cz,
     'A4) Atari HELP cesky: NA SIRKU tlacitkem (B342), NET / HRY = server N&P (do B342 "z atarihelp.eu"), L1 / R1, televize, odkaz na HELP')
bez = [w for w in ('vypinac', 'pamet ', 'skutecnem', 'klavesa', 'zustava', 'nahrava', 'pasky', 'ZAVRIT"') if w in cz]
over(not bez, 'A4) Atari HELP cesky cely s diakritikou (do B342 pul textu bez hacku)' + (' - ZBYTKY: %s' % bez if bez else ''))
over('the LANDSCAPE button under the device' in en and 'NET / GAMES – N&P GAMES' in en and '(also L1 / R1) = FIRE' in en
     and 'TV (remote)' in en and 'Full guide: main menu → the HELP card' in en and 'games from atarihelp.eu' not in en,
     'A4) Atari HELP anglicky: stejny obsah')
over('NA SIRKU' in cti(M, 'cpp/atari/nap_atari_device.h') and '"LANDSCAPE"' in cti(M, 'cpp/atari/nap_atari_device.h')
     and 'ZPET NA ATARI 130XE' in cti(A, 'emu_atari_cpp/index.html'),
     'A4) nazvy z HELP existuji: pilulka NA SIRKU / LANDSCAPE (C++), ZPET NA ATARI 130XE (stranka logu)')
kart = cti(M, 'cpp/menu/nap_menu_karty.h')
over('"help | HELP | NÁVOD A OTÁZKY' in kart, 'A4) karta HELP v hlavnim menu existuje (odkazy "hlavní menu → karta HELP")')

# ---------------- V verze ----------------
bg = cti(KOREN, 'app/build.gradle')
over('versionCode 386' in bg and 'versionName "EMU10-B343-HELP-KONTROLA"' in bg, 'V) build.gradle: 386 EMU10-B343-HELP-KONTROLA')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_kontrola_b343.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
