#!/usr/bin/env python3
"""B345 v Chromiu - skutecna stranka PS1 (assets/emu_ps1/index.html) + falesny most AHPS1, ktery hlasi
PRESNE ty stavy, co posila MainActivity (startPs1RemoteDownloadAndBoot / bootPs1FileOnCurrentThread):
  "PS1_REMOTE_START ...", "PS1_REMOTE_CONNECT ... path=...", "PS1_REMOTE_DOWNLOAD 37% 182.4MB/492.0MB soubor",
  "PS1_REMOTE_ZIP_READY ...", "PS1_REMOTE_READY ...", "PS1_REMOTE_BOOTING #n <hezke jmeno>", "PS1_REMOTE_BOOT_OK ...",
  "PS1_REMOTE_FAIL <hlaska>", "PS1_REMOTE_CACHE_HIT ...", "PS1_REMOTE_BUSY"; ps1RemoteAktivni() = bezi stahovani,
  ps1RemoteJmeno() = hezke jmeno hry, ps1StartStav() = "pocet startu|vysledek".

  S1-S7) cele stahovani CZ na vysku: PRIPOJUJI SE -> STAHUJI HRU (jmeno, 37 %, "182,4 MB z 492,0 MB", pruh 37 %)
         -> ROZBALUJI HRU -> SPOUSTIM HRU -> NAHRAVAM HRU <hezke jmeno> -> hra bezi; plocha jadra (BIOS) po celou
         dobu schovana, az pak ukazana; pruh pod monitorem se pri stahovani neukazuje (drive surovy stav a cache)
  V1)    velikost neznama: MB a cas misto procent
  C1-C3) chyba stahovani: okno pryc, plocha zpet (nabehne BIOS), pruh POD monitorem s nadpisem a hlaskou bez kodu
         PS1_REMOTE_FAIL; po 12 s zmizi a uz se znovu neukaze (drive se ukazoval porad dokola)
  Z1)    stary stav "stahuji" po zruseni (ps1RemoteAktivni = false) -> zadne okno
  P1)    panel otevren a zavren behem stahovani -> plocha zustava schovana (na vysku by okno zakryla)
  R1)    RESET behem stahovani -> okno RESETUJI (BIOS BEZ CD), nevrati se na STAHUJI
  B1)    hra se po stazeni nespustila -> HRA SE NESPUSTILA (toast i pruh)
  K1-K2) hra uz stazena (CACHE_HIT) -> SPOUSTIM HRU + jmeno; jine stahovani bezi (BUSY)
  E1-E3) anglicky: texty okna i pruhu anglicky, MB s teckou, zadna cestina
  H1)    panel PS1 (kolecko): LOAD GAME - velikost + 200 MB, ZIP 2x, hra se spusti sama; CZ i EN
  J)     zadna chyba JavaScriptu
Pouziti (z test_overeni/b345): python3 test_b345_stranka.py [PRAC] -> VYSTUP_test_b345_stranka.txt
"""
import os
import re
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b345')
os.makedirs(PRAC, exist_ok=True)

vystup = []
chyb = 0
chybyJs = []


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


# falesna Java: jen co stranka PS1 pri stahovani a nahravani pouziva; ostatni metody vraci ''
MOST = r'''
(function(){
  var M = window.__m = {status:'idle', aktivni:false, jmeno:'', n:0, v:'', plocha:[], reset:0};
  var zaklad = {
    ps1RemoteStatus: function(){ return M.status; },
    ps1RemoteAktivni: function(){ return M.aktivni; },
    ps1RemoteJmeno: function(){ return M.jmeno; },
    ps1StartStav: function(){ return M.n + '|' + M.v; },
    ps1PlochaVisible: function(s){ M.plocha.push(!!s); },
    ps1Status: function(){ return 'PS1_IDLE'; },
    tvCastActive: function(){ return false; },
    // RESET jako v Jave: stopPs1SessionHard zrusi stahovani (stav CANCELLED) - tady schvalne az za 600 ms,
    // at stranka mezitim jeste par dotazu vidi "stahuji"; BIOS nabehne za 900 ms
    ps1Reset: function(){ M.reset++;
      setTimeout(function(){ M.status='PS1_REMOTE_CANCELLED reason=jsPs1Reset'; M.aktivni=false; }, 600);
      setTimeout(function(){ M.n++; M.v='BIOS_OK'; }, 900);
      return 'PS1_RESET_OK stop=PS1_STOPPED'; }
  };
  window.AHPS1 = new Proxy(zaklad, {
    get: function(t, p){ if (p in t) return t[p]; if (p === 'then' || typeof p === 'symbol') return undefined;
                          return function(){ return ''; }; },
    has: function(){ return true; }
  });
})();
'''


def otevri(b, zaklad, jazyk='cs', w=412, h=846):
    ctx = b.new_context(viewport={'width': w, 'height': h}, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    ctx.add_init_script(MOST)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e).split('\n')[0][:200]))
    pg.goto(zaklad + 'emu_ps1/index.html')
    pg.wait_for_timeout(900)
    return ctx, pg


def stav(pg, status=None, aktivni=None, jmeno=None, n=None, v=None, cekej=750):
    pg.evaluate('''(a) => { var M = window.__m;
        if (a.status !== null) M.status = a.status; if (a.aktivni !== null) M.aktivni = a.aktivni;
        if (a.jmeno !== null) M.jmeno = a.jmeno; if (a.n !== null) M.n = a.n; if (a.v !== null) M.v = a.v; }''',
                {'status': status, 'aktivni': aktivni, 'jmeno': jmeno, 'n': n, 'v': v})
    pg.wait_for_timeout(cekej)


def okno(pg):
    return pg.evaluate("() => { var o = document.getElementById('psMonitorLoading');"
                       " return o && o.style.display !== 'none' ? o.innerText : ''; }")


def pruh(pg):
    return pg.evaluate("() => { var b = document.getElementById('remoteBanner');"
                       " return b.classList.contains('show') ? b.innerText : ''; }")


def plocha(pg):
    return pg.evaluate('() => window.__m.plocha.slice()')


def obdelnik(pg, sel):
    return pg.evaluate("(s) => { var r = document.querySelector(s).getBoundingClientRect(); return [r.left, r.top, r.right, r.bottom]; }", sel)


srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()

    # ---------------- S) cele stahovani CZ ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_START 192.0.2.10:8080/api/v1/hra', True, '')
    o = okno(pg)
    over(o.startswith('PŘIPOJUJI SE K SERVERU…') and re.search(r'\n\d+ s\n', o) and 'Hra se po stažení spustí sama.' in o
         and plocha(pg)[-1:] == [False] and pruh(pg) == '',
         'S1) START: okno v monitoru "%s", plocha jadra schovana, pruh pod monitorem nic' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_CONNECT 192.0.2.10:8080/api/v1/hra path=/storage/emulated/0/Download/AtariHelp/PS1/nap_ps1-0123456789abcdef',
         None, 'Final Fantasy VII')
    o = okno(pg)
    over('Final Fantasy VII' in o and 'path=' not in o and 'nap_ps1' not in o and pruh(pg) == '',
         'S2) CONNECT: jmeno hry z katalogu, zadna cesta do cache: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_DOWNLOAD 37% 182.4MB/492.0MB Final_Fantasy_VII.zip')
    o = okno(pg)
    bar = pg.evaluate("() => { var d = document.querySelector('#psMonitorLoading .nahrBar > div'); return d ? d.style.width : ''; }")
    over(o.split('\n') == ['STAHUJI HRU…', 'Final Fantasy VII', '37 %', '182,4 MB z 492,0 MB', 'Hra se po stažení spustí sama.']
         and bar == '37%' and pruh(pg) == '' and 'PS1_REMOTE' not in o,
         'S3) DOWNLOAD: "%s", pruh v okne %s, pod monitorem nic' % (o.replace('\n', ' | '), bar))
    pg.screenshot(path=os.path.join(PRAC, 'stahuji_vyska_cz.png'))
    stav(pg, 'PS1_REMOTE_ZIP_READY Final_Fantasy_VII.zip bytes=515899392')
    o = okno(pg)
    over(o.startswith('ROZBALUJI HRU…\nFinal Fantasy VII\n') and 'Hra se po rozbalení spustí sama.' in o and re.search(r'\n\d+ s\n', o),
         'S4) ZIP: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_READY Final_Fantasy_VII.cue fromZip=Final_Fantasy_VII.zip path=/x/y.cue')
    o = okno(pg)
    over(o.startswith('SPOUŠTÍM HRU…\nFinal Fantasy VII\n') and 'spustí sama' not in o, 'S5) READY: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_BOOTING #1 Final Fantasy VII')
    o = okno(pg)
    over(o.startswith('NAHRÁVÁM HRU…\nFinal Fantasy VII\n') and plocha(pg)[-1:] == [False],
         'S6) BOOTING s hezkym jmenem z Javy -> rovnou NAHRAVAM HRU: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_BOOT_OK nap_ps1-0123456789abcdef__Final_Fantasy_VII.cue', False, None, 1, 'OK nap_ps1-0123456789abcdef__Final_Fantasy_VII.cue')
    over(okno(pg) == '' and plocha(pg)[-1:] == [True] and pruh(pg) == '' and plocha(pg).count(True) == 1,
         'S7) hra nabehla: okno pryc, plocha jadra ukazana (jen jednou, az ted): %s' % plocha(pg))
    ctx.close()

    # ---------------- V) velikost neznama ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 182.4MB Hra.chd', True, 'Hra')
    o = okno(pg)
    over(re.fullmatch(r'STAHUJI HRU…\nHra\n\d+ s\n182,4 MB\nHra se po stažení spustí sama\.', o) is not None
         and pg.evaluate("() => !document.querySelector('#psMonitorLoading .nahrBar')"),
         'V1) velikost neznama: MB a cas, bez procent a pruhu: "%s"' % o.replace('\n', ' | '))
    ctx.close()

    # ---------------- C) chyba stahovani ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 12% 60.0MB/492.0MB Final_Fantasy_VII.zip', True, 'Final Fantasy VII')
    hl = 'MÁLO MÍSTA: potřeba ~1,2 GB, volno 800 MB – smaž starou PS1 hru: CD ISO → koš'
    stav(pg, 'PS1_REMOTE_FAIL ' + hl, False)
    t = pruh(pg)
    mon, ban = obdelnik(pg, '#psMonitor'), obdelnik(pg, '#remoteBanner')
    over(okno(pg) == '' and plocha(pg)[-1:] == [True] and t == 'HRU SE NEPODAŘILO STÁHNOUT\n' + hl,
         'C1) chyba: okno pryc, plocha zpet (BIOS), pruh "%s"' % t.replace('\n', ' | '))
    over(ban[1] >= mon[3] - 0.5, 'C2) na vysku je pruh s chybou POD monitorem (monitor %s, pruh %s)'
         % ([round(x) for x in mon], [round(x) for x in ban]))
    pg.screenshot(path=os.path.join(PRAC, 'chyba_vyska_cz.png'))
    pg.wait_for_timeout(12600)
    t1 = pruh(pg)
    pg.wait_for_timeout(1500)
    over(t1 == '' and pruh(pg) == '', 'C3) po 12 s pruh zmizel a s tim samym stavem se uz neukaze (drive porad dokola)')
    ctx.close()

    # ---------------- Z) stary stav po zruseni ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 45% 1.0MB/2.0MB stara.zip', False, 'Stara hra')
    over(okno(pg) == '' and plocha(pg) == [], 'Z1) stav "stahuji" zbyl po zruseni (ps1RemoteAktivni = false) -> zadne okno, plocha nedotcena')
    ctx.close()

    # ---------------- P) panel behem stahovani ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 20% 98.4MB/492.0MB Final_Fantasy_VII.zip', True, 'Final Fantasy VII')
    pg.click('#btnSettings')
    pg.wait_for_timeout(400)
    pg.click('#helpClose')
    pg.wait_for_timeout(600)
    over(plocha(pg)[-1:] == [False] and okno(pg).startswith('STAHUJI HRU…'),
         'P1) panel otevren a zavren behem stahovani -> plocha zustava schovana %s, okno STAHUJI dal' % plocha(pg))
    ctx.close()

    # ---------------- R) RESET behem stahovani ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 20% 98.4MB/492.0MB Final_Fantasy_VII.zip', True, 'Final Fantasy VII')
    pg.click('#btnReset')
    pg.wait_for_timeout(450)
    o1 = okno(pg)
    pg.wait_for_timeout(250)
    o2 = okno(pg)
    pg.wait_for_timeout(1300)
    over(o1.startswith('RESETUJI (BIOS BEZ CD)…') and o2.startswith('RESETUJI (BIOS BEZ CD)…') and okno(pg) == ''
         and pg.evaluate('() => window.__m.reset') == 1,
         'R1) RESET behem stahovani: "%s" i kdyz Java jeste hlasi stahovani, po BIOS_OK okno pryc' % o1.split('\n')[0])
    ctx.close()

    # ---------------- B) hra se nespustila ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_READY Crash.bin bytes=1 path=/x/Crash.bin', True, 'Crash Bandicoot')
    stav(pg, 'PS1_REMOTE_BOOTING #1 Crash Bandicoot')
    stav(pg, 'PS1_REMOTE_FAIL boot PS1_HRA_FAIL jadro', False, None, 1,
         'CHYBA jadro hru nespustilo (PS1_HRA_FAIL jadro) - podrobnosti v LOGu (denik PS1)')
    toast = pg.evaluate("() => document.getElementById('toast').textContent")
    t = pruh(pg)
    # B346: chybu spusteni rekne jen hlaska nahravaciho okna, pruh pod monitorem ji neopakuje (byla dvakrat)
    over(okno(pg) == '' and toast.startswith('HRA SE NESPUSTILA: jadro hru nespustilo') and t == '',
         'B1) hra se nespustila: hlaska "%s", pruh ji neopakuje' % toast[:60])
    ctx.close()

    # ---------------- K) uz stazena / jine stahovani ----------------
    ctx, pg = otevri(b, zaklad)
    stav(pg, 'PS1_REMOTE_CACHE_HIT Crash.cue path=/x/Crash.cue', True, 'Crash Bandicoot')
    o = okno(pg)
    over(o.startswith('SPOUŠTÍM HRU…\nCrash Bandicoot\n'), 'K1) hra uz v zarizeni: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_BUSY')
    o = okno(pg)
    over(o.startswith('UŽ SE STAHUJE JINÁ HRA…'), 'K2) jine stahovani bezi: "%s"' % o.split('\n')[0])
    ctx.close()

    # ---------------- E) anglicky ----------------
    ctx, pg = otevri(b, zaklad, 'en', 846, 412)
    stav(pg, 'PS1_REMOTE_DOWNLOAD 37% 182.4MB/492.0MB Final_Fantasy_VII.zip', True, 'Final Fantasy VII')
    o = okno(pg)
    pg.screenshot(path=os.path.join(PRAC, 'stahuji_sirka_en.png'))
    over(o.split('\n') == ['DOWNLOADING THE GAME…', 'Final Fantasy VII', '37 %', '182.4 MB of 492.0 MB',
                           'The game starts by itself once it is downloaded.'],
         'E1) EN na sirku: "%s"' % o.replace('\n', ' | '))
    stav(pg, 'PS1_REMOTE_ZIP_PRIMARY Final_Fantasy_VII.bin size=700.0MB')
    o2 = okno(pg)
    stav(pg, 'PS1_REMOTE_FAIL LOW STORAGE: needs ~1.5 GB, free 999 MB – delete an old PS1 game: CD ISO → bin', False)
    t = pruh(pg)
    zb = H.ceska_slova(o + '\n' + o2 + '\n' + t)
    over(o2.startswith('UNPACKING THE GAME…') and 'once it is unpacked' in o2 and t.startswith('THE GAME COULD NOT BE DOWNLOADED\n'),
         'E2) EN: "%s", pruh "%s"' % (o2.split('\n')[0], t.replace('\n', ' | ')))
    over(not zb, 'E3) EN: v okne ani v pruhu zadna cestina%s' % ((' - CESKY: %s' % zb) if zb else ''))
    ctx.close()

    # ---------------- H) panel PS1 (kolecko) ----------------
    for jazyk, ocek in (('cs', 'Poprvé se hra stáhne (volno asi velikost hry + 200 MB, ZIP 2× velikost + 200 MB) a sama se spustí'),
                        ('en', 'The first time a game downloads (needs about its size + 200 MB free, a ZIP 2× its size + 200 MB) and starts by itself')):
        ctx, pg = otevri(b, zaklad, jazyk)
        pg.click('#btnSettings')
        pg.wait_for_timeout(500)
        t = pg.evaluate("() => document.getElementById('helpPanel').innerText")
        over(ocek in t and '2× velikost hry + 200 MB), příště' not in t,
             'H1) %s panel PS1: LOAD GAME "%s…"' % (jazyk.upper(), ocek[:70]))
        ctx.close()
    b.close()
srv.shutdown()

over(not chybyJs, 'J) zadna chyba JavaScriptu' + (': %s' % chybyJs[:3] if chybyJs else ''))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b345_stranka.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
