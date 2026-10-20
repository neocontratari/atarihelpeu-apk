#!/usr/bin/env python3
# B311: DZUBOX (assets/dj/index.html) - RECORD ulozi nahravku mixu.
# Drive: MediaRecorder -> blob -> <a download>. WebView v appce blob: odkaz stahnout neumi
# (DownloadListener ho zahodil) - nahravka se nikam neulozila. Ted jde po kouscich (base64)
# do Javy: AHSAVE.souborZacni('DJ_mixy', jmeno, typ) / souborPridej / souborDokonci.
# Most AHSAVE je napodoben (sklada bajty, jako by je Java psala do souboru).
#  1) appka: nahravani -> SAVING -> SAVED, soubor = skutecna nahravka (WebM, EBML hlavicka),
#     jmeno NP_VISION_mix_RRRRMMDD_HHMMSS.webm do DJ_mixy, hlaska MIX SAVED s cestou
#  2) chyba zapisu -> souborZrus, SAVE FAILED, dalsi nahravani jde
#  3) prohlizec na PC (bez AHSAVE) -> normalni stazeni souboru (jako drive)
#   python3 test_b311_dj_nahravka.py [PRAC]      -> VYSTUP_test_b311_dj_nahravka.txt
import os
import re
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b311'
os.makedirs(PRAC, exist_ok=True)
vystup = []
kontrol = chyb = 0


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    r = ('OK     ' if ok else 'CHYBA  ') + co
    print(r, flush=True)
    vystup.append(r)


MOST = """
window.__dj = {zacni: [], kusu: 0, bajty: [], dokonci: 0, zrus: 0, padniPri: %d};
window.AHSAVE = {
  save: function(){ return 'DOWNLOADS_OK:x'; },
  souborZacni: function(slozka, jm, typ){ __dj.zacni.push([slozka, jm, typ]); __dj.jm = jm; return 7; },
  souborPridej: function(id, b64){
    if (id !== 7) return false;
    __dj.kusu++;
    if (__dj.padniPri && __dj.kusu >= __dj.padniPri) return false;
    var s = atob(b64); for (var i = 0; i < s.length; i++) __dj.bajty.push(s.charCodeAt(i)); return true; },
  souborDokonci: function(id){ __dj.dokonci++; return 'OK:Download/AtariHelp/DJ_mixy/' + __dj.jm; },
  souborZrus: function(id){ __dj.zrus++; }
};
"""
URL = 'file://' + os.path.join(A, 'dj/index.html')
STAV = """() => ({tl: document.getElementById('recBtn').textContent.trim(), on: document.getElementById('recBtn').classList.contains('on'),
  hl: (document.getElementById('asstText') || {}).textContent || '', dj: window.__dj ? {zacni: __dj.zacni, kusu: __dj.kusu, n: __dj.bajty.length,
  hlava: __dj.bajty.slice(0, 4), dokonci: __dj.dokonci, zrus: __dj.zrus} : null})"""


def nahraj(pg, sekund=2.5):
    pg.click('#recBtn')
    pg.wait_for_timeout(300)
    zap = pg.evaluate(STAV)
    # neco zahrat do masteru: pady sampleru
    pady = pg.locator('#sampler .sPad')
    n = pady.count()
    kroku = 8
    for i in range(kroku):
        if n:
            pady.nth(i % min(n, 5)).click()
        pg.wait_for_timeout(int(sekund * 1000 / kroku))
    pg.click('#recBtn')
    return zap


with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files', '--autoplay-policy=no-user-gesture-required'])
    chybyJs = []

    # ---- 1) appka: ulozeni po kouscich ----
    ctx = b.new_context(viewport={'width': 915, 'height': 412}, device_scale_factor=2)
    ctx.add_init_script(MOST % 0)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(URL)
    pg.wait_for_timeout(800)
    zap = nahraj(pg)
    over(zap['on'] and zap['tl'] == 'RECORD', '1) RECORD zapnuto (tlacitko sviti)')
    vid = set()
    for _ in range(60):
        s = pg.evaluate(STAV)
        vid.add(s['tl'].split(' ')[0])
        if s['tl'].startswith('SAVED') or s['tl'].startswith('SAVE FAILED'):
            break
        pg.wait_for_timeout(100)
    print('   stav:', s)
    jm = s['dj']['zacni'][0][1] if s['dj']['zacni'] else ''
    over(len(s['dj']['zacni']) == 1 and s['dj']['zacni'][0][0] == 'DJ_mixy' and re.match(r'^NP_VISION_mix_\d{8}_\d{6}\.webm$', jm)
         and s['dj']['zacni'][0][2].startswith('audio/webm'),
         '1) souborZacni(DJ_mixy, %s, %s)' % (jm, s['dj']['zacni'][0][2] if s['dj']['zacni'] else '-'))
    over(s['tl'] == 'SAVED ✓' and s['dj']['dokonci'] == 1 and s['dj']['zrus'] == 0,
         '1) po zastaveni: SAVED, souborDokonci 1x (videne stavy tlacitka: %s)' % ', '.join(sorted(vid)))
    over(s['dj']['n'] > 2000 and s['dj']['hlava'] == [0x1A, 0x45, 0xDF, 0xA3],
         '1) do Javy doslo %d B nahravky ve %d kouscich, zacatek 1A 45 DF A3 = WebM (skutecna nahravka)' % (s['dj']['n'], s['dj']['kusu']))
    over(s['hl'].startswith('MIX SAVED') and 'Download/AtariHelp/DJ_mixy/' + jm in s['hl'], '1) hlaska: "%s"' % s['hl'])
    pg.screenshot(path=os.path.join(PRAC, 'dj_nahravka_ulozena.png'))
    pg.wait_for_timeout(3800)
    over(pg.evaluate(STAV)['tl'] == 'RECORD', '1) za chvili tlacitko zase RECORD')
    # ---- 1b) dlouha nahravka: vic kousku po 384 kB, slozeny soubor = cela nahravka ----
    # kousky po 16 kB (v appce 384 kB) - at je i kratka nahravka ve vic kouscich
    pg.evaluate("() => { __dj.zacni = []; __dj.kusu = 0; __dj.bajty = []; __dj.dokonci = 0; window.napDjKus = 16384; }")
    nahraj(pg, 24.0)
    for _ in range(100):
        s = pg.evaluate(STAV)
        if s['tl'].startswith('SAVED') or s['tl'].startswith('SAVE FAILED'):
            break
        pg.wait_for_timeout(100)
    b64 = pg.evaluate("""() => { const b = __dj.bajty; let s = ''; for (let i = 0; i < b.length; i += 8192)
        s += String.fromCharCode.apply(null, b.slice(i, i + 8192)); return btoa(s); }""")
    import base64
    import subprocess
    soubor = os.path.join(PRAC, 'dj_dlouha.webm')
    open(soubor, 'wb').write(base64.b64decode(b64))
    pr = subprocess.run(['ffprobe', '-v', 'error', '-show_entries', 'format=duration:stream=codec_name', '-of', 'default=nw=1', soubor],
                        capture_output=True, text=True)
    delka = 0.0
    m = re.search(r'duration=([0-9.]+)', pr.stdout)
    if m:
        delka = float(m.group(1))
    if delka == 0.0:
        # WebM z MediaRecorderu nema v hlavicce delku - spocitat dekodovanim
        pr2 = subprocess.run(['ffmpeg', '-v', 'error', '-i', soubor, '-f', 'null', '-'], capture_output=True, text=True)
        pr3 = subprocess.run(['ffprobe', '-v', 'error', '-count_packets', '-select_streams', 'a:0', '-show_entries',
                              'stream=nb_read_packets', '-of', 'csv=p=0', soubor], capture_output=True, text=True)
        try:
            delka = int(pr3.stdout.strip()) * 0.020          # Opus: 20 ms na paket
        except ValueError:
            delka = 0.0
        chybaDek = pr2.stderr.strip()
    else:
        chybaDek = ''
    over(s['tl'] == 'SAVED ✓' and s['dj']['kusu'] >= 2 and 'opus' in pr.stdout and 20.0 <= delka <= 28.0 and not chybaDek,
         '1b) nahravka 24 s po kouscich 16 kB: %d B ve %d kouscich, slozeny soubor jde prehrat: Opus, %.1f s (ffprobe / ffmpeg bez chyby)'
         % (s['dj']['n'], s['dj']['kusu'], delka))
    ctx.close()

    # ---- 2) chyba zapisu ----
    ctx = b.new_context(viewport={'width': 915, 'height': 412})
    ctx.add_init_script(MOST % 1)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(URL)
    pg.wait_for_timeout(800)
    nahraj(pg, 1.2)
    for _ in range(50):
        s = pg.evaluate(STAV)
        if s['tl'].startswith('SAVE'):
            if s['tl'] != 'SAVING' and not s['tl'].startswith('SAVING'):
                break
        pg.wait_for_timeout(100)
    over(s['tl'] == 'SAVE FAILED' and s['dj']['zrus'] == 1 and s['dj']['dokonci'] == 0 and 'MIX NOT SAVED' in s['hl'],
         '2) chyba zapisu v Jave: souborZrus 1x, SAVE FAILED, hlaska "%s"' % s['hl'])
    pg.wait_for_timeout(3800)
    zap = nahraj(pg, 0.8)
    over(zap['on'], '2) po chybe jde nahravat znovu')
    ctx.close()

    # ---- 3) prohlizec na PC (bez AHSAVE): stazeni ----
    ctx = b.new_context(viewport={'width': 1280, 'height': 720}, accept_downloads=True)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(URL)
    pg.wait_for_timeout(800)
    with pg.expect_download(timeout=8000) as d:
        nahraj(pg, 1.2)
    dl = d.value
    cesta = os.path.join(PRAC, dl.suggested_filename)
    dl.save_as(cesta)
    hlava = open(cesta, 'rb').read(4)
    over(re.match(r'^NP_VISION_mix_\d{8}_\d{6}\.webm$', dl.suggested_filename) and hlava == b'\x1a\x45\xdf\xa3' and os.path.getsize(cesta) > 1000,
         '3) prohlizec na PC: stazeni %s (%d B, WebM)' % (dl.suggested_filename, os.path.getsize(cesta)))
    ctx.close()
    b.close()

over(not chybyJs, '4) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b311_dj_nahravka.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
