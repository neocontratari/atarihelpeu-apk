#!/usr/bin/env python3
# B312: DZUBOX (assets/dj/index.html) - RECORD pres spolecny modul spolecne/nap_nahravka.js:
#  1) appka: M4A (kdyz ho WebView umi), ulozeni po kouscich do Javy (DJ_mixy), soubor jde prehrat
#  2) WebView bez M4A: WebM a v hlavicce DOPLNENA delka (drive WebM bez delky)
#  3) chyba zapisu v Jave -> SAVE FAILED, souborZrus, dalsi nahravani jde
#  4) prohlizec na PC (bez AHSAVE) -> normalni stazeni
#  5) BACK / POWER behem nahravani: mix se nejdriv ulozi, pak menu (drive se zahodil);
#     tlacitko ZPET na telefonu (Java vola napZpet) totez, napoveda se zavre
# Most AHSAVE je napodoben (sklada bajty, jako by je Java psala do souboru).
#   python3 test_b312_dj.py [PRAC]      -> VYSTUP_test_b312_dj.txt
import base64
import json
import os
import re
import subprocess
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b312/dj'
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
  souborZacni: function(slozka, jm, typ){ __dj.zacni.push([slozka, jm, typ]); __dj.jm = jm; __dj.bajty = []; return 7; },
  souborPridej: function(id, b64){
    if (id !== 7) return false;
    __dj.kusu++;
    if (__dj.padniPri && __dj.kusu >= __dj.padniPri) return false;
    var s = atob(b64); for (var i = 0; i < s.length; i++) __dj.bajty.push(s.charCodeAt(i)); return true; },
  souborDokonci: function(id){ __dj.dokonci++; return 'OK:Download/AtariHelp/DJ_mixy/' + __dj.jm; },
  souborZrus: function(id){ __dj.zrus++; }
};
window.napDjKus = 16384;
"""
BEZ_MP4 = """
(function(){ var o = MediaRecorder.isTypeSupported.bind(MediaRecorder);
  MediaRecorder.isTypeSupported = function(t){ return /mp4/i.test(t) ? false : o(t); }; })();
"""
URL = 'file://' + os.path.join(A, 'dj/index.html')
STAV = """() => ({tl: document.getElementById('recBtn').textContent.trim(), on: document.getElementById('recBtn').classList.contains('on'),
  hl: (document.getElementById('asstText') || {}).textContent || '', dj: window.__dj ? {zacni: __dj.zacni, kusu: __dj.kusu, n: __dj.bajty.length,
  dokonci: __dj.dokonci, zrus: __dj.zrus} : null})"""
chybyJs = []


def stranka(b, padni=0, most=True, bezMp4=False, stahovani=False):
    ctx = b.new_context(viewport={'width': 915, 'height': 412}, accept_downloads=stahovani)
    ctx.add_init_script((MOST % padni if most else '') + (BEZ_MP4 if bezMp4 else ''))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(URL)
    pg.wait_for_timeout(800)
    return ctx, pg


def hraj(pg, sekund):
    pady = pg.locator('#sampler .sPad')
    n = pady.count()
    kroku = 8
    for i in range(kroku):
        if n:
            pady.nth(i % min(n, 5)).click()
        pg.wait_for_timeout(int(sekund * 1000 / kroku))


def nahraj(pg, sekund=2.5):
    pg.click('#recBtn')
    pg.wait_for_timeout(300)
    zap = pg.evaluate(STAV)
    hraj(pg, sekund)
    pg.click('#recBtn')
    return zap


def cekej_ulozeni(pg, ms=10000):
    s = None
    for _ in range(int(ms / 100)):
        s = pg.evaluate(STAV)
        if s['tl'].startswith('SAVED') or s['tl'].startswith('SAVE FAILED'):
            return s
        pg.wait_for_timeout(100)
    return s


def soubor(pg, jm):
    b64 = pg.evaluate("""() => { const b = __dj.bajty; let s = ''; for (let i = 0; i < b.length; i += 8192)
        s += String.fromCharCode.apply(null, b.slice(i, i + 8192)); return btoa(s); }""")
    cesta = os.path.join(PRAC, jm)
    open(cesta, 'wb').write(base64.b64decode(b64))
    return cesta


def rozbor(cesta):
    pr = subprocess.run(['ffprobe', '-v', 'error', '-show_entries', 'format=format_name,duration:stream=codec_name', '-of', 'json', cesta],
                        capture_output=True, text=True)
    j = json.loads(pr.stdout or '{}')
    dh = j.get('format', {}).get('duration')
    dek = subprocess.run(['ffmpeg', '-v', 'error', '-i', cesta, '-ac', '1', '-ar', '48000', '-f', 'f32le', '-'], capture_output=True)
    return (j.get('format', {}).get('format_name', ''), ','.join(s.get('codec_name', '') for s in j.get('streams', [])),
            float(dh) if dh else None, len(dek.stdout) / 4 / 48000.0, dek.stderr.decode().strip())


with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files', '--autoplay-policy=no-user-gesture-required'])

    # ---- 1) appka: M4A ----
    ctx, pg = stranka(b)
    zap = nahraj(pg, 4.0)
    over(zap['on'] and zap['tl'] == 'RECORD', '1) RECORD zapnuto (tlacitko sviti)')
    s = cekej_ulozeni(pg)
    jm = s['dj']['zacni'][0][1] if s['dj']['zacni'] else ''
    over(len(s['dj']['zacni']) == 1 and s['dj']['zacni'][0][0] == 'DJ_mixy' and re.match(r'^NP_VISION_mix_\d{8}_\d{6}\.m4a$', jm)
         and s['dj']['zacni'][0][2] == 'audio/mp4' and s['tl'] == 'SAVED ✓' and s['dj']['dokonci'] == 1 and s['dj']['kusu'] >= 2,
         '1) souborZacni(DJ_mixy, %s, %s), %d kousku, SAVED' % (jm, s['dj']['zacni'][0][2] if s['dj']['zacni'] else '-', s['dj']['kusu']))
    over(s['hl'].startswith('MIX SAVED') and ('Download/AtariHelp/DJ_mixy/' + jm) in s['hl'], '1) hlaska: "%s"' % s['hl'])
    fmt, kod, dh, dd, ch = rozbor(soubor(pg, 'dj_mix.m4a'))
    over('mp4' in fmt and kod in ('opus', 'aac') and dh is not None and 3.6 <= dh <= 5.2 and abs(dh - dd) < 0.3 and not ch,
         '1) soubor: MP4 (%s), delka %.2f s v hlavicce = %.2f s dekodovano, ffmpeg bez chyby' % (kod, dh or -1, dd))
    pg.wait_for_timeout(3800)
    over(pg.evaluate(STAV)['tl'] == 'RECORD', '1) za chvili tlacitko zase RECORD')
    ctx.close()

    # ---- 2) WebView bez M4A: WebM s delkou ----
    ctx, pg = stranka(b, bezMp4=True)
    nahraj(pg, 3.0)
    s = cekej_ulozeni(pg)
    jm = s['dj']['zacni'][0][1] if s['dj']['zacni'] else ''
    fmt, kod, dh, dd, ch = rozbor(soubor(pg, 'dj_mix.webm'))
    over(re.match(r'^NP_VISION_mix_\d{8}_\d{6}\.webm$', jm) and s['dj']['zacni'][0][2] == 'audio/webm' and 'webm' in fmt and kod == 'opus'
         and dh is not None and abs(dh - dd) < 0.3 and 2.6 <= dd <= 4.2 and not ch,
         '2) bez M4A: %s, WebM / Opus, delka v hlavicce %.2f s = dekodovano %.2f s (drive v hlavicce zadna)' % (jm, dh or -1, dd))
    ctx.close()

    # ---- 3) chyba zapisu ----
    ctx, pg = stranka(b, padni=1)
    nahraj(pg, 1.2)
    s = cekej_ulozeni(pg)
    over(s['tl'] == 'SAVE FAILED' and s['dj']['zrus'] == 1 and s['dj']['dokonci'] == 0 and 'MIX NOT SAVED · write error' in s['hl'],
         '3) chyba zapisu v Jave: souborZrus 1x, SAVE FAILED, hlaska "%s"' % s['hl'])
    pg.wait_for_timeout(3800)
    zap = nahraj(pg, 0.8)
    over(zap['on'], '3) po chybe jde nahravat znovu')
    ctx.close()

    # ---- 4) prohlizec na PC ----
    ctx, pg = stranka(b, most=False, stahovani=True)
    with pg.expect_download(timeout=8000) as d:
        nahraj(pg, 1.5)
    dl = d.value
    cesta = os.path.join(PRAC, dl.suggested_filename)
    dl.save_as(cesta)
    fmt, kod, dh, dd, ch = rozbor(cesta)
    over(re.match(r'^NP_VISION_mix_\d{8}_\d{6}\.m4a$', dl.suggested_filename) and 'mp4' in fmt and dd > 1.0 and not ch,
         '4) prohlizec na PC: stazeni %s (%d B, %.2f s)' % (dl.suggested_filename, os.path.getsize(cesta), dd))
    ctx.close()

    # ---- 5) BACK behem nahravani, ZPET na telefonu ----
    ctx, pg = stranka(b)
    pg.click('#recBtn')
    hraj(pg, 1.5)
    pg.click('#backBtn')
    pg.wait_for_timeout(250)
    url_hned = pg.url
    ulozeno = None
    for _ in range(80):
        try:
            if pg.url.endswith('/assets/index.html'):
                break
            ulozeno = max(ulozeno or 0, pg.evaluate('() => __dj.dokonci'))  # (po navigaci uz bezi v nove strance)
        except Exception:
            pass
        pg.wait_for_timeout(100)
    over(url_hned.endswith('/dj/index.html') and ulozeno == 1 and pg.url.endswith('/assets/index.html'),
         '5) BACK behem nahravani: pult pocka, mix ulozi (souborDokonci %s) a teprve pak jde do menu (drive se nahravka zahodila)' % ulozeno)
    ctx.close()
    ctx, pg = stranka(b)
    z0 = pg.evaluate('() => napZpet()')
    pg.click('#navHelp')
    pg.wait_for_timeout(200)
    z1 = pg.evaluate("() => [napZpet(), document.getElementById('helpWrap').classList.contains('open')]")
    pg.click('#recBtn')
    hraj(pg, 1.2)
    z2 = pg.evaluate('() => napZpet()')
    ulozeno = None
    for _ in range(80):
        try:
            if pg.url.endswith('/assets/index.html'):
                break
            ulozeno = max(ulozeno or 0, pg.evaluate('() => __dj.dokonci'))  # (po navigaci uz bezi v nove strance)
        except Exception:
            pass
        pg.wait_for_timeout(100)
    over(z0 == 0 and z1 == [1, False] and z2 == 1 and ulozeno == 1 and pg.url.endswith('/assets/index.html'),
         '5) ZPET na telefonu (napZpet): nic -> 0 (appka jde zpet), napoveda -> zavre (1), nahravani -> ulozi a pak menu (1)')
    ctx.close()
    b.close()

over(not chybyJs, '6) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b312_dj.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
