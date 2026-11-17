#!/usr/bin/env python3
"""B335: ZVUK Z YOUTUBE V APCE NA TV - SKUTECNY SKRIPT MOSTU Z MainActivity V CHROMIU.

Rene (log B333): "yt nebere zvuk z apky na tv web". V logu vsude YOUTUBE_AUDIO_BRIDGE_LIVE_EQ
result="controls_ready_ORIGINAL_SOUND_untouched" a proud stranky "zvuk 0 vzorku/s": zvuk na TV jde jen
z audio grafu (odbocka -> AHTVWEB.pushYoutubePcm16), ale graf se stavel jen tlacitkem EQ (BUILD2SA14).

  - skript = PRESNE ten, ktery appka vlozi do stranky YouTube (vytahne se z
    MainActivity.napTvWebInjectYoutubeAudioBridge, hodnoty EQ jako v appce),
  - stranka "jako YouTube": <video> se zdrojem blob: (YouTube = MSE, blob:), ton 440 Hz; AHTVWEB = nahrada
    mostu Javy (status, tvPosloucha, pushYoutubePcm16 - pocita a rozbali PCM, setSoundState),
  - Chromium s --autoplay-policy=no-user-gesture-required (jako WebView: setMediaPlaybackRequiresUserGesture(false)).
Kontroly:
  1) skript je ES5 (starsi WebView) a vlozeni 3x za sebou (900 / 2200 / 5200 ms) nic nezdvoji
  2) TV se nediva: video hraje, ZADNY graf (puvodni zvuk nedotceny - ORIGINAL FIRST), zadne PCM
  3) TV se diva: pri prehravani se graf postavi sam, PCM jde do AHTVWEB.pushYoutubePcm16: ton 440 Hz
     (frekvence z pruchodu nulou), stereo, vzorkovani AudioContextu, log "[NAP PLAYER] B335 ..."
  4) TV se pripoji, az kdyz video uz hraje: do 2 s se graf postavi (kontrola kazdych 1,5 s)
  5) dalsi video (Shorts, reklama, dalsi klip): pripoji se taky, zvuk jde dal
  6) cizi zdroj bez CORS (WebAudio by dal ticho): nepripoji se - telefon hraje dal original
  7) tlacitko EQ bez TV dal funguje jako drive (graf az na pozadani)
  8) zadna chyba JavaScriptu
Pouziti: python3 test_b335_youtube.py   -> VYSTUP_test_b335_youtube.txt
"""
import base64
import http.server
import io
import os
import re
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
    """String js = ... ve funkci napTvWebInjectYoutubeAudioBridge: retezce + promenne EQ jako v appce (vychozi)."""
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
                    n = src[i + 1]
                    b.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}[n])
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


JS = skript_z_javy(open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read())


def ton_wav(f=440.0, s=6.0, fs=48000):
    t = np.arange(int(s * fs)) / fs
    x = (0.5 * np.sin(2 * np.pi * f * t) * 32767).astype('<i2')
    st = np.stack([x, x], -1).reshape(-1)
    b = io.BytesIO()
    w = wave.open(b, 'wb')
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(fs)
    w.writeframes(st.tobytes())
    w.close()
    return b.getvalue()


WAV = ton_wav()
# stranka "jako YouTube": nahrada mostu Javy + video se zdrojem blob: (jako MSE YouTube)
STRANKA = """<!doctype html><html><head><meta charset="utf-8"><title>yt</title></head><body>
<div id="feed">video</div>
<script>
window.__tv = false; window.__pcm = []; window.__logy = []; window.__stav = [];
var _log = console.log; console.log = function(m){ window.__logy.push(String(m)); _log.apply(console, arguments); };
window.AHTVWEB = {
  status: function(){ return 'http://192.0.2.10:8765/'; },
  tvPosloucha: function(){ return window.__tv; },
  setSoundState: function(j){ window.__stav.push(j); return 'ok'; },
  pushYoutubePcm16: function(b64, rate, frames, ch){ window.__pcm.push([b64, rate, frames, ch]); return 'TV_WEB_AUDIO_YOUTUBE_OK'; }
};
window.novyVideo = function(zdroj){
  var v = document.createElement('video'); v.playsInline = true; v.loop = true;
  document.body.appendChild(v);
  if (zdroj === 'blob') return fetch('/ton.wav').then(function(r){ return r.blob(); }).then(function(b){ v.src = URL.createObjectURL(b); return v; });
  v.src = zdroj; return Promise.resolve(v);
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


def vloz(pg):
    return pg.evaluate('(js) => eval(js)', JS)


def pcm_rozbor(pg, od=0):
    """PCM od indexu od: (kusu, vzorkovani, kanalu, frekvence z pruchodu nulou, RMS)"""
    kusy = pg.evaluate('(od) => window.__pcm.slice(od)', od)
    if not kusy:
        return 0, 0, 0, 0.0, 0.0
    data = b''.join(base64.b64decode(k[0]) for k in kusy)
    x = np.frombuffer(data, dtype='<i2').reshape(-1, 2).astype(np.float64)
    rate = kusy[-1][1]
    l = x[:, 0]
    pruchody = np.sum((l[:-1] < 0) & (l[1:] >= 0))
    f = pruchody / (len(l) / rate) if len(l) else 0
    rms = float(np.sqrt(np.mean(l ** 2)) / 32768)
    return len(kusy), rate, kusy[-1][3], float(f), rms


def main():
    tisk('B335 - zvuk z YouTube v apce na TV: skript mostu z MainActivity v Chromiu (%d znaku)' % len(JS))
    # 1a) ES5
    try:
        import subprocess
        r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", JS],
                           capture_output=True, text=True, timeout=60)
        es5 = r.stdout.strip() == 'ok'
        es5chyba = r.stderr.strip().splitlines()[-1] if r.stderr.strip() else ''
    except Exception as e:
        es5, es5chyba = False, str(e)
    srv = server()
    srv2 = server()      # "cizi" web (jiny port = jiny puvod, bez CORS)
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]
    cizi = 'http://localhost:%d/ton.wav' % srv2.server_address[1]
    chybyJs = []
    with sync_playwright() as p:
        b = p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required'])
        ctx = b.new_context()

        def nova():
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chybyJs.append(str(e)))
            pg.goto(url)
            return pg

        # ===== 1) 2) TV se nediva =====
        pg = nova()
        v1 = [vloz(pg), vloz(pg), vloz(pg)]
        over(es5 and all(v == 'controls_ready_ORIGINAL_SOUND_untouched' for v in v1) and pg.evaluate('document.querySelectorAll("#ahtvEqBtn").length') == 1,
             '1) skript ES5%s, vlozen 3x: %s, tlacitko EQ jedno' % ('' if es5 else ' NE: ' + es5chyba, v1[0]))
        pg.evaluate("novyVideo('blob').then(function(v){ window.__v1 = v; return v.play(); })")
        time.sleep(2.5)
        st = pg.evaluate("[!window.__v1.paused, window.__v1.currentTime, !!window.__AHTV_YT_AUDIO_BRIDGE, !!window.__v1.__ahtvSrc, window.__pcm.length]")
        over(st[0] and st[1] > 1.0 and not st[2] and not st[3] and st[4] == 0,
             '2) TV se nediva: video hraje (%.1f s), audio graf NENI, video nepripojene (original), PCM na TV 0 kusu' % st[1])
        # ===== 4) TV se pripoji, az kdyz video uz hraje =====
        pg.evaluate("window.__tv = true")
        t0 = time.time()
        while time.time() - t0 < 4 and not pg.evaluate("!!(window.__v1.__ahtvSrc)"):
            time.sleep(0.1)
        dt = time.time() - t0
        time.sleep(1.5)
        n, rate, ch, f, rms = pcm_rozbor(pg)
        logy = pg.evaluate("window.__logy.filter(function(m){return m.indexOf('B335')>=0;})")
        over(dt < 2.2 and n >= 5 and 400 < f < 480 and rms > 0.2,
             '4) TV se pripojila pri hrajicim videu: graf za %.1f s, PCM %d kusu, %d Hz, %d kanaly, ton %.0f Hz, RMS %.2f; log: %s'
             % (dt, n, rate, ch, f, rms, logy[-1] if logy else '-'))
        # ===== 5) dalsi video =====
        pred = pg.evaluate("window.__pcm.length")
        pg.evaluate("window.__v1.pause(); novyVideo('blob').then(function(v){ window.__v2 = v; return v.play(); })")
        time.sleep(2.0)
        n2, rate2, ch2, f2, rms2 = pcm_rozbor(pg, pred + 3)
        st = pg.evaluate("[!!window.__v2.__ahtvSrc, window.__AHTV_YT_AUDIO_BRIDGE.prvku]")
        over(st[0] and st[1] == 2 and n2 >= 5 and 400 < f2 < 480,
             '5) dalsi video (Shorts / dalsi klip): pripojeno taky (prvku v grafu %d), zvuk dal: %d kusu, ton %.0f Hz' % (st[1], n2, f2))
        # ===== 6) cizi zdroj bez CORS =====
        pg.evaluate("window.__v2.pause(); novyVideo(%r).then(function(v){ window.__v3 = v; return v.play(); })" % cizi)
        time.sleep(2.0)
        st = pg.evaluate("[!window.__v3.paused, window.__v3.currentTime, !!window.__v3.__ahtvSrc, window.__v3.__ahtvPosl || '']")
        over(st[0] and st[1] > 0.5 and not st[2] and st[3] == 'skip_cizi_zdroj_original',
             '6) cizi zdroj bez CORS (WebAudio by dal ticho): video hraje (%.1f s), NEpripojene -> telefon hraje original (%s)' % (st[1], st[3]))
        pg.close()

        # ===== 3) TV se diva uz pri vlozeni =====
        pg = nova()
        pg.evaluate("window.__tv = true")
        r3 = vloz(pg)
        pg.evaluate("novyVideo('blob').then(function(v){ window.__v1 = v; return v.play(); })")
        t0 = time.time()
        while time.time() - t0 < 3 and not pg.evaluate("!!(window.__v1 && window.__v1.__ahtvSrc)"):
            time.sleep(0.05)
        dt = time.time() - t0
        time.sleep(2.0)
        n, rate, ch, f, rms = pcm_rozbor(pg)
        ctxRate = pg.evaluate("window.__AHTV_YT_AUDIO_CTX.sampleRate")
        logy = pg.evaluate("window.__logy.filter(function(m){return m.indexOf('B335')>=0;})")
        r3b = vloz(pg)
        over(r3 == 'B335_TV_SE_DIVA_zvuk_do_TV:ceka_na_prehravani' and dt < 0.8 and n >= 10 and rate == ctxRate and ch == 2 and 400 < f < 480 and rms > 0.2
             and logy and 'prehravani VIDEO a TV se diva -> ok_live_eq' in logy[0] and r3b == 'B335_TV_SE_DIVA_zvuk_do_TV:graph_ready',
             '3) TV se diva: vlozeni "%s", pri prehravani graf za %.2f s, PCM %d kusu po %d Hz (= AudioContext), %d kanaly, ton %.0f Hz, RMS %.2f;'
             ' log "%s"; dalsi vlozeni "%s"' % (r3, dt, n, rate, ch, f, rms, logy[0] if logy else '-', r3b))
        pg.close()

        # ===== 7) EQ bez TV =====
        pg = nova()
        vloz(pg)
        pg.evaluate("novyVideo('blob').then(function(v){ window.__v1 = v; return v.play(); })")
        time.sleep(1.0)
        pred = pg.evaluate("!!window.__AHTV_YT_AUDIO_BRIDGE")
        pg.click('#ahtvEqBtn')
        time.sleep(1.0)
        st = pg.evaluate("[!!window.__AHTV_YT_AUDIO_BRIDGE, !!window.__v1.__ahtvSrc, document.getElementById('ahtvEqPanel').style.display]")
        over(not pred and st[0] and st[1] and st[2] == 'block',
             '7) bez TV: graf az po otevreni EQ (pred %s, po %s, video pripojeno %s, panel %s)' % (pred, st[0], st[1], st[2]))
        pg.close()
        b.close()
    over(not chybyJs, '8) zadna chyba JavaScriptu (%s)' % (chybyJs[:3] if chybyJs else 'zadna'))
    srv.shutdown()
    srv2.shutdown()
    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b335_youtube.txt'), 'w').write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
