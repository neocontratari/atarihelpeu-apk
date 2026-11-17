#!/usr/bin/env python3
"""B336: CHRAPLAVY ZVUK YOUTUBE NA TV - SKUTECNY SKRIPT MOSTU Z MainActivity V CHROMIU, ZADRHAVAJICI STRANKA.

Rene (log B335): "zvuk je z yt ale je chraplavy". V logu proud stranky "zvuk 39615 vzorku/s" (ma byt 44100)
a prohlizec TV "podteceni 12 / 18 / 10" za 30 s. Odbocka B335 byla ScriptProcessor 2048 - jeho udalost bezi
v hlavnim vlakne stranky; YouTube ma tezky JavaScript a hlavni vlakno se zadrhava. Prohlizec ma pro
ScriptProcessor jen dva buffery - pri zadrhnuti zvuk zmizi nebo se zopakuje (chrapot) a TV dojde zvuk.

  - skript = PRESNE ten z MainActivity (B336) a pro srovnani z gitu (B335, HEAD~ / soubor zadany 1. argumentem),
  - stranka "jako YouTube": <video> se zdrojem blob: (MSE YouTube = blob:), cisty ton 440 Hz 20 s,
    AHTVWEB = nahrada mostu Javy (tvPosloucha = ano, pushYoutubePcm16 si PCM schova),
  - ZADRHAVANI: hlavni vlakno stranky se kazdych 700 ms na 250 ms zastavi (tezky JavaScript YouTube),
  - rozbor PCM, ktere by slo na TV: kolik zvuku doslo (proti dobe prehravani) a kolik "lupnuti" (v cistem
    tonu je kazde preruseni / opakovani ostry skok - zbytek predpovedi sinusovky).
Kontroly:
  1) B335 se zadrhavanim: zvuk chybi nebo luska (pricina chrapotu potvrzena)
  2) B336 se zadrhavanim: zachyt MediaStreamTrackProcessor, doslo >= 98 % zvuku, 0 lupnuti
  3) B336 bez zadrhavani: stejne cisty (nic se nerozbilo)
  4) B336 kdyz MediaStreamTrackProcessor neni (WebView starsi nez Chrome 94): zalozni ScriptProcessor 4096 funguje
  5) skript B336 je ES5, zadna chyba JavaScriptu
Pouziti: python3 test_b336_youtube_zvuk.py [MainActivity_B335.java]   -> VYSTUP_test_b336_youtube_zvuk.txt
"""
import base64
import http.server
import io
import os
import re
import subprocess
import sys
import threading
import time
import wave

import numpy as np
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
vystup = []
kontrol = chyb = 0


def tisk(s):
    print(s, flush=True)
    vystup.append(s)


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    tisk(('OK     ' if ok else 'CHYBA  ') + co)


def skript_z_javy(src):
    """String js = ... ve funkci napTvWebInjectYoutubeAudioBridge (hodnoty EQ jako v appce - vychozi)."""
    f = src.index('private void napTvWebInjectYoutubeAudioBridge(String reason)')
    i = src.index('String js =', f) + len('String js =')
    hodnoty = {'eqJs': '[0,0,0,0,0]', 'bassJs': '0', 'trebleJs': '0', 'balanceJs': '0', 'volumeJs': '1'}
    out = []
    while True:
        while src[i] in ' \t\r\n+':
            i += 1
        if src.startswith('//', i):
            i = src.index('\n', i)
            continue
        if src[i] == ';':
            break
        if src[i] == '"':
            i += 1
            b = []
            while src[i] != '"':
                if src[i] == '\\':
                    b.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}[src[i + 1]])
                    i += 2
                else:
                    b.append(src[i])
                    i += 1
            i += 1
            out.append(''.join(b))
            continue
        m = re.match(r'[A-Za-z_][A-Za-z0-9_]*', src[i:])
        if not m or m.group(0) not in hodnoty:
            raise RuntimeError('neocekavany vyraz ve skriptu: ' + src[i:i + 60])
        out.append(hodnoty[m.group(0)])
        i += len(m.group(0))
    return ''.join(out)


JS336 = skript_z_javy(open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read())
if len(sys.argv) > 1:
    JS335 = skript_z_javy(open(sys.argv[1], encoding='utf-8').read())
else:
    JS335 = skript_z_javy(subprocess.run(['git', '-C', KOREN, 'show', 'ee83fa1:app/src/main/java/eu/atarihelp/emu10/MainActivity.java'],
                                         capture_output=True, text=True, check=True).stdout)

F = 440.0


def ton_wav(s=20.0, fs=48000):
    t = np.arange(int(s * fs)) / fs
    x = (0.5 * np.sin(2 * np.pi * F * t) * 32767).astype('<i2')
    b = io.BytesIO()
    w = wave.open(b, 'wb')
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(fs)
    w.writeframes(np.stack([x, x], -1).reshape(-1).tobytes())
    w.close()
    return b.getvalue()


WAV = ton_wav()
STRANKA = """<!doctype html><html><head><meta charset="utf-8"><title>yt</title></head><body>
<script>
window.__pcm = []; window.__logy = []; window.__zadrhnuti = 0;
var _log = console.log; console.log = function(m){ window.__logy.push(String(m)); _log.apply(console, arguments); };
window.AHTVWEB = {
  status: function(){ return 'http://192.0.2.10:8765/'; },
  tvPosloucha: function(){ return true; },
  setSoundState: function(){ return 'ok'; },
  pushYoutubePcm16: function(b64, rate, frames, ch){ window.__pcm.push([b64, rate, frames, ch, performance.now()]); return 'TV_WEB_AUDIO_YOUTUBE_OK'; }
};
window.zadrhavej = function(kazdych, naMs){
  window.__zt = setInterval(function(){ var t = performance.now(); while (performance.now() - t < naMs) {} window.__zadrhnuti++; }, kazdych);
};
window.hraj = function(){
  return fetch('/ton.wav').then(function(r){ return r.blob(); }).then(function(b){
    var v = document.createElement('video'); v.playsInline = true; document.body.appendChild(v);
    v.src = URL.createObjectURL(b); window.__v = v; return v.play(); });
};
</script></body></html>"""


def server():
    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def do_GET(self):
            p = self.path.split('?')[0]
            if p == '/':
                d, t = STRANKA.encode(), 'text/html; charset=utf-8'
            elif p == '/ton.wav':
                d, t = WAV, 'audio/wav'
            else:
                self.send_response(404)
                self.end_headers()
                return
            self.send_response(200)
            self.send_header('Content-Type', t)
            self.send_header('Content-Length', str(len(d)))
            self.end_headers()
            self.wfile.write(d)

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def rozbor(kusy, hral_s):
    """(dodano %, lupnuti, vzorkovani, kusu)"""
    if not kusy:
        return 0.0, 0, 0, 0
    rate = kusy[-1][1]
    data = b''.join(base64.b64decode(k[0]) for k in kusy)
    x = np.frombuffer(data, dtype='<i2').reshape(-1, 2)[:, 0].astype(np.float64) / 32768.0
    dodano = 100.0 * len(x) / (hral_s * rate)
    w = 2 * np.pi * F / rate
    e = np.abs(x[2:] - 2 * np.cos(w) * x[1:-1] + x[:-2])
    e[: int(0.3 * rate)] = 0                       # zacatek tonu
    spatne = np.nonzero(e > 0.02)[0]
    lup = 0
    posl = -10 ** 9
    for i in spatne:                               # shluky (jedno lupnuti = vic vzorku za sebou)
        if i - posl > 200:
            lup += 1
        posl = i
    return dodano, lup, rate, len(kusy)


def beh(b, url, js, zadrhavani, bez_mstp=False, sekund=8.0):
    chybyJs = []
    ctx = b.new_context()
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(url)
    if bez_mstp:
        pg.evaluate('delete window.MediaStreamTrackProcessor; window.MediaStreamTrackProcessor = undefined;')
    pg.evaluate('(js) => eval(js)', js)
    pg.evaluate('hraj()')
    t0 = time.time()
    while time.time() - t0 < 5 and not pg.evaluate('!!(window.__v && window.__v.__ahtvSrc)'):
        time.sleep(0.05)
    if zadrhavani:
        pg.evaluate('zadrhavej(700, 250)')
    zac = pg.evaluate('window.__v.currentTime')
    time.sleep(sekund)
    if zadrhavani:
        pg.evaluate('clearInterval(window.__zt)')
    time.sleep(1.5)                                 # dobehne fronta
    pg.evaluate('window.__v.pause()')
    time.sleep(0.6)
    kon = pg.evaluate('window.__v.currentTime')
    kusy = pg.evaluate('window.__pcm.map(function(k){return [k[0],k[1],k[2],k[3]];})')
    info = pg.evaluate("[window.__AHTV_YT_AUDIO_BRIDGE?window.__AHTV_YT_AUDIO_BRIDGE.zachyt||'sp2048':'', window.__zadrhnuti,"
                       " window.__logy.filter(function(m){return m.indexOf('YOUTUBE ZVUK NA TV')>=0;})]")
    ctx.close()
    return kusy, zac, kon, info, chybyJs


def main():
    es5 = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", JS336],
                         capture_output=True, text=True, timeout=60).stdout.strip() == 'ok'
    tisk('B336 - zvuk YouTube na TV pri zadrhavajici strance (skript mostu B335 a B336 z MainActivity, Chromium)')
    srv = server()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]
    vys = {}
    chybyJs = []
    with sync_playwright() as p:
        b = p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required'])
        for jm, js, zadrh, bez in (('B335 zadrhava', JS335, True, False), ('B336 zadrhava', JS336, True, False),
                                   ('B336 plynule', JS336, False, False), ('B336 bez MSTP', JS336, False, True)):
            kusy, zac, kon, info, cj = beh(b, url, js, zadrh, bez)
            chybyJs += cj
            # PCM jde od pripojeni (hned pri prehrani) do pauzy: doba = cas videa pri pauze
            d, lup, rate, n = rozbor(kusy, kon)
            vys[jm] = (d, lup, rate, n, info)
            tisk('   %-14s: zachyt %-6s, zadrhnuti %2d x 250 ms, PCM %3d kusu po %d Hz, doslo %.1f %% zvuku, lupnuti %d'
                 % (jm, info[0] or '-', info[1], n, rate, d, lup))
        b.close()
    srv.shutdown()
    a = vys['B335 zadrhava']
    over(a[0] < 97.0 or a[1] >= 3,
         '1) B335 se zadrhavanim (250 ms kazdych 700 ms): doslo %.1f %% zvuku, %d lupnuti - pricina chrapotu potvrzena' % (a[0], a[1]))
    c = vys['B336 zadrhava']
    over(c[4][0] == 'mstp' and c[0] >= 98.0 and c[1] == 0,
         '2) B336 se zadrhavanim: zachyt %s, doslo %.1f %% zvuku, %d lupnuti' % (c[4][0], c[0], c[1]))
    d = vys['B336 plynule']
    over(d[4][0] == 'mstp' and d[0] >= 98.0 and d[1] == 0, '3) B336 bez zadrhavani: doslo %.1f %%, %d lupnuti' % (d[0], d[1]))
    e = vys['B336 bez MSTP']
    over(e[4][0] == 'sp4096' and e[0] >= 98.0,
         '4) B336 bez MediaStreamTrackProcessor (WebView starsi nez Chrome 94): zalozni %s funguje, doslo %.1f %% (lupnuti %d - ScriptProcessor'
         ' luska i bez zadrhavani, proto je jen zaloha)' % (e[4][0], e[0], e[1]))
    logy = c[4][2]
    over(es5 and not chybyJs and any('MediaStreamTrackProcessor' in l for l in logy),
         '5) skript B336 ES5%s, zadna chyba JavaScriptu (%s), log "%s"' % ('' if es5 else ' NE', chybyJs[:2] or 'zadna',
                                                                            next((l for l in logy if 'zachyt zvuku' in l), '-')))
    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b336_youtube_zvuk.txt'), 'w').write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
