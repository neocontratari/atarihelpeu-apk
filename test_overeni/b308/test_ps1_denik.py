#!/usr/bin/env python3
"""B308: DENIK PS1, klavesnice z TV v C++ a zamek kolem retro_run - SKUTECNY
nap_ps1_native.cpp (knihovna z ps1_harness.cpp, viz JAK_SPUSTIT.txt).

Pouziti: python3 test_ps1_denik.py KNIHOVNA.so PRACOVNI_ADRESAR
"""
import ctypes
import os
import re
import shutil
import sys
import threading
import time

LIB = ctypes.CDLL(sys.argv[1] if len(sys.argv) > 1 else 'libps1_b308.so')
PRAC = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else 'prac_denik')
for f, typ, arg in (('h_start', ctypes.c_char_p, [ctypes.c_char_p, ctypes.c_char_p]),
                    ('h_stop', ctypes.c_char_p, []),
                    ('h_klavesa', ctypes.c_char_p, [ctypes.c_char_p, ctypes.c_int]),
                    ('h_dotyk', None, [ctypes.c_int, ctypes.c_int]),
                    ('h_bity_jadra', ctypes.c_uint, []),
                    ('h_behu', ctypes.c_longlong, []),
                    ('h_soubeznych', ctypes.c_int, []),
                    ('h_ulozeni', ctypes.c_int, []),
                    ('h_zpomal', None, [ctypes.c_int]),
                    ('h_hlasky', None, [ctypes.c_int]),
                    ('h_obraz', None, [ctypes.c_int] * 5),
                    ('h_tenke', None, [ctypes.c_uint]),
                    ('h_pozice', ctypes.c_char_p, [ctypes.c_char_p, ctypes.c_int]),
                    ('h_denik', ctypes.c_char_p, [])):
    fn = getattr(LIB, f)
    fn.restype = typ
    fn.argtypes = arg

chyb = 0
kontrol = 0
VYSTUP = []


def tisk(s):
    print(s, flush=True)
    VYSTUP.append(s)


def ok(podminka, text):
    global chyb, kontrol
    kontrol += 1
    if not podminka:
        chyb += 1
    tisk(('OK     ' if podminka else 'CHYBA  ') + text)


def denik():
    return LIB.h_denik().decode('utf-8', 'replace')


def radky(vzor):
    return [r for r in denik().splitlines() if re.search(vzor, r)]


def cekej(vzor, s=8.0):
    t0 = time.time()
    while time.time() - t0 < s:
        r = radky(vzor)
        if r:
            return r
        time.sleep(0.05)
    return []


def klav(code, dolu):
    return LIB.h_klavesa(code.encode(), 1 if dolu else 0).decode()


def bity_po(ms=120):
    time.sleep(ms / 1000.0)
    return LIB.h_bity_jadra()


def main():
    if os.path.exists(PRAC):
        shutil.rmtree(PRAC)
    os.makedirs(PRAC)
    # BIOS a hra jako v telefonu: .cue se 3 stopami, treti soubor CHYBI
    with open(os.path.join(PRAC, 'scph5501.bin'), 'wb') as f:
        f.truncate(512 * 1024)
    hra = os.path.join(PRAC, 'hry', 'Doom')
    os.makedirs(hra)
    for jm, vel in (('Doom (Track 1).bin', 3 * 1024 * 1024), ('Doom (Track 2).bin', 1024 * 1024)):
        with open(os.path.join(hra, jm), 'wb') as f:
            f.truncate(vel)
    cue = os.path.join(hra, 'Doom.cue')
    with open(cue, 'w') as f:
        f.write('FILE "Doom (Track 1).bin" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n'
                'FILE "Doom (Track 2).bin" BINARY\n  TRACK 02 AUDIO\n    INDEX 00 00:00:00\n    INDEX 01 00:02:00\n'
                'FILE "Doom (Track 3).bin" BINARY\n  TRACK 03 AUDIO\n    INDEX 01 00:00:00\n')

    tisk('B308: denik PS1 + klavesnice z TV + zamek retro_run - SKUTECNY nap_ps1_native.cpp')
    tisk('== 1. start hry')
    r = LIB.h_start(PRAC.encode(), cue.encode()).decode()
    ok(r == 'PS1_HRA_OK', 'start pres JNI ps1BootDoMonitoru: %s' % r)
    d = denik()
    casovy = re.compile(r'^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3}  PS1 ')
    ok(d and all(casovy.match(x) for x in d.splitlines()), 'kazdy radek deniku ma datum a cas jako radky z Javy (%d radku)' % len(d.splitlines()))
    for vzor, popis in (
            (r'PS1 START: HRA', 'zacatek startu'),
            (r'HRA: Doom\.cue \| typ \.cue \| \d+ B \| cesta .*/Doom\.cue', 'hra: jmeno, typ, velikost, cesta'),
            (r'CUE: 3 stop \(data 1, hudba 2\), souboru 3, na disku 2 \(4\.0 MB\): Doom \(Track 1\)\.bin 3\.0 MB, '
             r'Doom \(Track 2\)\.bin 1\.0 MB, Doom \(Track 3\)\.bin CHYBI', 'CUE: stopy, soubory a velikosti'),
            (r'CHYBA CUE: 1 soubor\(u\) chybi .*Doom \(Track 3\)\.bin', 'CUE: chybejici soubor nahlasen jako CHYBA'),
            (r'BIOS ve slozce appky: scph5501\.bin 512 kB', 'BIOS ve slozce appky'),
            (r'JADRO: CD-ROM Label: DOOM', 'hlaska jadra PCSX (nazev disku) - drive jen v logcatu'),
            (r'JADRO: CD-ROM ID: SLUS00077', 'hlaska jadra PCSX (ID disku)'),
            (r'JADRO VAROVANI: warning: test varovani jadra', 'varovani jadra oznacene'),
            (r'JADRO: BIOS scph5501\.bin \(soubor\) \| region USA, NTSC 60 Hz .*disk ID "SLUS00077"', 'co jadro pouziva (BIOS, region, disk)'),
            (r'NASTAVENI GRAFIKY: 2x \(Enhanced Resolution\) ZAPNUTO', 'nastaveni grafiky'),
            (r'CASOVANI: 59\.940 snimku/s, zvuk jadra 44100 Hz -> reproduktor 44100 Hz \| obraz zakladni 320x240, nejvic 1024x512', 'casovani'),
            (r'PAMETOVA KARTA nova \(soubor zatim neni\): .*memory_card_1\.srm, 131072 B', 'pametova karta pri startu'),
            (r'PAMETOVA KARTA 1: zasunuta', 'pametova karta zasunuta'),
            (r'BEZI HRA', 'bezi hra'),
            (r'start trval \d+ ms', 'doba startu')):
        ok(bool(re.search(vzor, d)), popis)
    ok('ladici hlaska' not in d, 'ladici hlasky jadra (uroven 0) do deniku nejdou')
    ok(bool(cekej(r'OBRAZ: obraz 320x240, 15 bit, NTSC \| 2x ANO', 3)), 'prvni stav obrazu zapsan hned po startu')
    with open(os.path.join(PRAC, 'nap_tv_session_log.txt'), encoding='utf-8', errors='replace') as f:
        soubor = f.read()
    ok('PS1 START: HRA' in soubor and 'CHYBA CUE' in soubor and 'BEZI HRA' in soubor,
       'denik je i v souboru logu appky (to, co ukazuje /log a LOG appky)')

    tisk('== 2. zmeny obrazu')
    LIB.h_obraz(640, 480, 0, 0, 1)
    ok(bool(cekej(r'OBRAZ: obraz 640x480, 15 bit, NTSC', 2)), 'zmena rozliseni zapsana hned')
    LIB.h_obraz(320, 240, 1, 0, 0)
    ok(bool(cekej(r'OBRAZ: obraz 320x240, 24 bit \(video\), NTSC \| 2x NE', 2)), '24 bit video (bez 2x) zapsano')
    LIB.h_obraz(320, 240, 0, 0, 1)

    tisk('== 3. klavesnice z TV (skutecne JNI ps1PcKlavesa) -> co jadro cte pres retro_input_state')
    ok(klav('KeyK', True) == 'PS1:KRIZEK', 'K -> odpoved PS1:KRIZEK')
    ok(bity_po() & 1, 'jadro vidi KRIZEK')
    ok(klav('KeyK', False) == 'PS1:KRIZEK' and bity_po() & 1 == 0, 'K pustena -> jadro uz KRIZEK nevidi')
    mapa = {'ArrowUp': 4, 'KeyW': 4, 'Numpad8': 4, 'ArrowDown': 5, 'KeyS': 5, 'ArrowLeft': 6, 'KeyA': 6, 'ArrowRight': 7,
            'KeyD': 7, 'Space': 0, 'KeyL': 8, 'KeyJ': 1, 'KeyI': 9, 'KeyQ': 10, 'KeyE': 11, 'KeyZ': 12, 'KeyC': 13,
            'KeyF': 14, 'KeyG': 15, 'Enter': 3, 'NumpadEnter': 3, 'KeyP': 3, 'Backspace': 2, 'KeyO': 2}
    spatne = []
    for code, bit in mapa.items():
        klav(code, True)
        b = bity_po(60)
        klav(code, False)
        if b != (1 << bit):
            spatne.append('%s:%x' % (code, b))
        bity_po(40)
    ok(not spatne, 'vsech %d klaves na spravna tlacitka PS1 (smer, krizek, kolecko, ctverec, trojuhelnik, L1-R3, START, SELECT) %s'
       % (len(mapa), spatne))
    LIB.h_dotyk(3, 1)
    klav('ArrowUp', True)
    ok(bity_po() == (1 << 3) | (1 << 4), 'dotyk (START) + klavesnice (NAHORU) najednou - jadro vidi oboje')
    klav('ArrowUp', False)
    ok(bity_po() == (1 << 3), 'pusteni klavesy nepusti tlacitko drzene dotykem')
    LIB.h_dotyk(3, 0)
    ok(bity_po() == 0, 'vse pusteno')
    ok(klav('KeyM', True) == 'NEZNAMA:KeyM', 'neznama klavesa -> NEZNAMA:KeyM (pro PS1 nic)')
    ok(klav('Key<b>"x', True) == 'NEZNAMA:Keybx', 'do odpovedi prohlizeci jdou jen pismena a cisla')
    for c in ('KeyW', 'KeyD', 'KeyK', 'KeyL'):
        klav(c, True)
    ok(bity_po() == (1 << 4) | (1 << 7) | (1 << 0) | (1 << 8), '4 klavesy drzene najednou')
    ok(klav('PustVse', False) == 'PS1:PUSTENO' and bity_po() == 0, 'PustVse (okno prohlizece ztratilo zamereni) -> vse pusteno')
    ok(klav('STAV', True) == 'PS1' and klav('Zije', True) == 'PS1:ZIJE', 'STAV a Zije')
    # hlidac: drzena klavesa a prohlizec posila "Zije" kazdych 0,8 s -> drzi; pak se odmlci -> C++ pusti
    klav('KeyD', True)
    for _ in range(5):
        time.sleep(0.8)
        klav('Zije', True)
    ok(bity_po(50) == (1 << 7), 'drzena klavesa vydrzi 4 s, dokud prohlizec posila "Zije"')
    t0 = time.time()
    while LIB.h_bity_jadra() and time.time() - t0 < 6:
        time.sleep(0.05)
    dt = time.time() - t0
    ok(LIB.h_bity_jadra() == 0 and 2.0 < dt < 3.6, 'prohlizec se odmlcel -> C++ klavesu pustilo samo za %.1f s (hlidac 2,5 s)' % dt)
    ok(bool(cekej(r'TV KLAVESNICE: prohlizec se 2,5 s neozval - drzene klavesy pusteny \(VPRAVO\)', 2)), 'hlidac zapsany v deniku')
    ok(bool(radky(r'TV KLAVESNICE: KeyK dolu -> KRIZEK')), 'stisky z TV v deniku (prvnich 12)')
    ok(bool(radky(r'TV KLAVESNICE: dalsi stisky jen v radcich TLACITKA')), 'po 12 stiscich jen souhrn (denik neroste)')
    ok(bool(radky(r'TV KLAVESNICE: KeyM dolu -> pro PS1 nic \(neznama klavesa\)')), 'neznama klavesa v deniku')
    ok(bool(radky(r'TLACITKA: \+KRIZEK -> drzeno: KRIZEK')), 'radek TLACITKA hned pri stisku (co se zmacklo a co je drzeno)')
    ok(bool(radky(r'TLACITKA: .*START.*NAHORU|TLACITKA: .*NAHORU.*START')), 'TLACITKA ukazuji i dotyk')
    # poradi v radku TLACITKA jako ve skutecnosti (drive vzdy "+X -X", i kdyz bylo drzene a znovu zmacknute)
    time.sleep(0.5)
    klav('ArrowUp', True); bity_po(60)          # radek "+NAHORU" hned
    klav('ArrowUp', False); bity_po(60)         # v dalsim okne (0,3 s): pustit ...
    klav('ArrowUp', True); bity_po(60)          # ... a znovu zmacknout
    ok(bool(cekej(r'TLACITKA: -NAHORU \+NAHORU -> drzeno: NAHORU', 2)),
       'drzena sipka pustena a znovu zmacknuta -> v deniku "-NAHORU +NAHORU" (poradi jako ve skutecnosti)')
    time.sleep(0.4)
    klav('KeyL', True); bity_po(60)             # radek "+KOLECKO" hned, pak v jednom okne tuknuti K
    klav('KeyK', True); bity_po(60); klav('KeyK', False); bity_po(60)
    ok(bool(cekej(r'TLACITKA: \+KRIZEK -KRIZEK -> drzeno: NAHORU KOLECKO', 2)),
       'kratke tuknuti behem drzeni jinych -> "+KRIZEK -KRIZEK", drzeno NAHORU KOLECKO')
    klav('ArrowUp', False); klav('KeyL', False); bity_po(60)

    tisk('== 4. zdravi (kazdych 5 s)')
    LIB.h_tenke(25234)
    z = cekej(r'ZDRAVI', 8)
    t0 = time.time()
    while time.time() - t0 < 7 and not any('oprava Doomu' in x for x in radky(r'ZDRAVI')):
        time.sleep(0.2)
    z = [x for x in radky(r'ZDRAVI') if 'oprava Doomu' in x]
    ok(bool(z), 'radek ZDRAVI je v deniku')
    if z:
        r0 = z[-1]
        tisk('   ' + r0[r0.index('ZDRAVI'):][:400])
        m = re.search(r'emulace ([\d.]+) kroku/s \(cil 59\.94, rezerva (\d+) % casu\)', r0)
        ok(bool(m) and 50 < float(m.group(1)) < 70, 'kroky emulace za sekundu (%s)' % (m.group(1) if m else '?'))
        ok(bool(re.search(r'retro_run prumer [\d.]+ ms, nejdele [\d.]+ ms \| snimky [\d.]+/s \(stejnych [\d.]+/s\) \| '
                          r'zvuk fronta \d+ ms, vypadky \d+', r0)), 'cas retro_run, snimky, zvuk')
        ok('obraz 320x240, 15 bit, NTSC | 2x ANO' in r0, 'stav obrazu v ZDRAVI')
        ok('oprava Doomu: 25234 tenkych trojuhelniku ve 2x' in r0, 'kolik tenkych trojuhelniku (Doom) se ve 2x kreslilo')
        ok(bool(re.search(r'tlacitka nic \(zmen \d+(, cast nezapsana)?, z klavesnice TV \d+\) \| TV divaku 0', r0)), 'tlacitka a divaci')

    tisk('== 5. ulozeni pozice behem hry (zamek kolem retro_run)')
    LIB.h_zpomal(4)          # retro_run trva 4 ms - pomaly telefon, ulozeni se trefi do behu
    cesta = os.path.join(PRAC, 'pozice.state').encode()
    vysl = []

    def ukladej():
        for _ in range(25):
            vysl.append(LIB.h_pozice(cesta, 1).decode())
            time.sleep(0.003)
    vl = [threading.Thread(target=ukladej) for _ in range(2)]
    for v in vl:
        v.start()
    for v in vl:
        v.join()
    LIB.h_zpomal(0)
    ok(all(v.startswith('PS1_STATE_SAVE_OK bytes=4096') for v in vysl) and len(vysl) == 50, '50x ulozeni z 2 vlaken: vse OK')
    ok(LIB.h_soubeznych() == 0, 'ani jednou se pozice neukladala UPROSTRED kroku emulace (soubezne: %d)' % LIB.h_soubeznych())
    ok(len(radky(r'POZICE ULOZENA: 4096 B do .*pozice\.state')) == 50, 'kazde ulozeni v deniku')
    r = LIB.h_pozice(cesta, 0).decode()
    ok(r.startswith('PS1_STATE_LOAD_OK') and bool(radky(r'POZICE NAHRANA: 4096 B')), 'nahrani pozice + denik')
    r = LIB.h_pozice(b'/neexistuje/x/pozice.state', 1).decode()
    ok(r == 'PS1_STATE_SAVE_FAIL open_failed' and bool(radky(r'CHYBA ULOZENI POZICE: open_failed')), 'chyba ulozeni v deniku')

    tisk('== 6. omezeni deniku (hlasky jadra)')
    pred = len(radky(r'JADRO: hlaska jadra behem hry'))
    LIB.h_hlasky(400)
    time.sleep(0.5)
    po = len(radky(r'JADRO: hlaska jadra behem hry'))
    ok(pred == 0 and 100 < po <= 150, 'z 400 hlasek jadra jen prvnich 150 od startu (%d)' % po)

    tisk('== 7. konec, chyba startu, BIOS')
    r = LIB.h_stop().decode()
    ok(r == 'PS1_STOPPED', 'stop: %s' % r)
    ok(bool(radky(r'PS1 KONEC po \d+ s, snimku \d+')), 'konec v deniku (jak dlouho, kolik snimku)')
    r = LIB.h_start(PRAC.encode(), os.path.join(hra, 'ODMITNOUT.cue').encode()).decode()
    ok(r == 'PS1_HRA_FAIL jadro', 'jadro hru odmitlo: %s' % r)
    ok(bool(radky(r'HRA: ODMITNOUT\.cue \| typ \.cue \| CHYBI')) and bool(radky(r'CUE: soubor nejde otevrit'))
       and bool(radky(r'CHYBA STARTU: jadro odmitlo hru')), 'chybejici hra a chyba startu v deniku')
    r = LIB.h_start(PRAC.encode(), b'').decode()
    ok(r == 'PS1_BIOS_OK' and bool(cekej(r'PS1 START: BEZ DISKU')) and bool(radky(r'BEZI BIOS')), 'start bez disku (BIOS): %s' % r)
    ok(LIB.h_bity_jadra() == 0, 'po novem startu zadna klavesa drzena')
    LIB.h_stop()
    n = len(denik().splitlines())
    ok(n <= 800, 'denik v pameti ma %d radku (nejvys 800)' % n)

    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol, chyb))
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'VYSTUP_test_ps1_denik.txt'), 'w') as f:
        f.write('\n'.join(VYSTUP).replace(PRAC, 'PRAC') + '\n')
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'UKAZKA_denik_ps1.txt'), 'w') as f:
        f.write(denik().replace(PRAC, 'PRAC'))
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
