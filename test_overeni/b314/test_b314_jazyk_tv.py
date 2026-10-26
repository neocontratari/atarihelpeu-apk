#!/usr/bin/env python3
"""B314: STRANKA WEB TV (prohlizec na TV / PC) CESKY / ANGLICKY.

  - stranka = PRESNE ta z appky (vytahne se z MainActivity.napTvWebWriteHtml), anglicky do ni
    test vlozi prekladac a slovnik PRESNE jako Java (MainActivity.napTvJazykStranka: za </title>
    <script>window.NAP_JAZYK='en'</script> + nap_jazyk.js + en_tv.js, "</script" -> "<\\/script"),
  - Python server = co dela Java: /status (menu, Atari psani / hrani, Sega, PS1), /vloz (odpovedi
    C++ devVlozNative a Javy), proudy drzi otevrene, /clientlog.
Kontroly:
  a) cesky: stranka je stejna jako v B313 (git HEAD) - Java ji v cestine nemeni,
  b) anglicky: v zadnem stavu (menu, Atari psani, Atari hrani, Sega, PS1, panel OBRAZ a jeho
     prepinace, hlasky vlozeni Ctrl+V) nezustala cestina - texty, atributy, titulek,
  c) anglicke hlasky jsou spravne (napr. PASTED: 3 PROGRAM LINES ..., TYPED: 1 CHARACTER),
  d) zadna chyba JavaScriptu (cesky ani anglicky).
Pouziti: python3 test_b314_jazyk_tv.py      -> VYSTUP_test_b314_jazyk_tv.txt, VYSTUP_texty_tv_en.txt
"""
import http.server
import os
import subprocess
import sys
import threading
import time

from playwright.sync_api import sync_playwright

import jazyk_harness as H

ZDE = H.ZDE
KOREN = H.KOREN
JAVA_CESTA = os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
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


def stranka_z_javy(src):
    """HTML prohlizece presne tak, jak ho sklada MainActivity.napTvWebWriteHtml (String body = ...;)."""
    i = src.index('String body = ', src.index('private void napTvWebWriteHtml(OutputStream out)')) + len('String body = ')
    out = []
    while True:
        while src[i] in ' \t\r\n+':
            i += 1
        if src.startswith('//', i):
            i = src.index('\n', i)
            continue
        if src[i] == ';':
            break
        if src[i] != '"':
            raise RuntimeError('neocekavany vyraz v HTML: ' + src[i:i + 60])
        i += 1
        b = []
        while src[i] != '"':
            if src[i] == '\\':
                n = src[i + 1]
                if n == 'u':
                    b.append(chr(int(src[i + 2:i + 6], 16)))
                    i += 6
                    continue
                b.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}[n])
                i += 2
            else:
                b.append(src[i])
                i += 1
        i += 1
        out.append(''.join(b))
    return ''.join(out)


def anglicky_jako_java(body):
    """MainActivity.napTvJazykStranka pro NapJazyk.en == true."""
    js = open(os.path.join(H.ASSETS, 'spolecne/nap_jazyk.js'), encoding='utf-8').read() + '\n' + \
        open(os.path.join(H.ASSETS, 'jazyk/en_tv.js'), encoding='utf-8').read()
    i = body.index('</title>')
    vlozit = "<script>window.NAP_JAZYK='en';</script><script>" + js.replace('</script', '<\\/script') + '</script>'
    return body[:i + 8] + vlozit + body[i + 8:]


JAVA = open(JAVA_CESTA, encoding='utf-8').read()
CZ = stranka_z_javy(JAVA)
EN = anglicky_jako_java(CZ)
# Java opravdu tohle dela (kdyby se napTvJazykStranka zmenila, test to musi vedet)
assert 'body = napTvJazykStranka(body);' in JAVA and "window.NAP_JAZYK='en';" in JAVA and \
    'atariCppAsset("spolecne/nap_jazyk.js")' in JAVA and 'atariCppAsset("jazyk/en_tv.js")' in JAVA

STAVY = {
    'menu': 'running=true atari=0 atariCpp=0 atariProud=0 proud=menu sega=0 ps1=0 menu=1 seq=1 h264Seq=0 url=x\n',
    'atari': 'running=true atari=1 atariCpp=1 atariProud=1 proud=atari sega=0 ps1=0 menu=0 klavRezim=PSANI seq=1 h264Seq=0 url=x\n',
    'atari_hrani': 'running=true atari=1 atariCpp=1 atariProud=1 proud=atari sega=0 ps1=0 menu=0 klavRezim=HRANI seq=1 h264Seq=0 url=x\n',
    'sega': 'running=true atari=0 atariCpp=0 atariProud=0 proud=sega sega=1 ps1=0 menu=0 seq=1 h264Seq=0 url=x\n',
    'ps1': 'running=true atari=0 atariCpp=0 atariProud=0 proud=ps1 sega=0 ps1=1 menu=0 seq=1 h264Seq=0 url=x\n',
}
# odpovedi appky na vlozeni (C++ devVlozNative / Java) -> co ma byt anglicky
VLOZ = [
    ('VLOZENO: 3 RADKU PROGRAMU (BEZ NEW) - PISU', 'PASTED: 3 PROGRAM LINES (NO NEW) - TYPING'),
    ('VLOZENO: 1 RADKU PROGRAMU (BEZ NEW) - PISU AZ BUDE READY (NEJVYS 45 S) (BEZ CISLA PRESKOCENO 2)',
     'PASTED: 1 PROGRAM LINE (NO NEW) - TYPING WHEN READY (45 S AT MOST) (SKIPPED 2 WITHOUT A NUMBER)'),
    ('NAPSANO: 1 ZNAKU', 'TYPED: 1 CHARACTER'),
    ('NAPSANO: 12 ZNAKU (2 NA ATARI NENI)', 'TYPED: 12 CHARACTERS (2 NOT ON THE ATARI)'),
    ('VLOZENI: ATARI JE VYPNUTE', 'PASTE: THE ATARI IS OFF'),
    ('VLOZENI: APPKA PRAVE PISE JINY PROGRAM - POCKEJ', 'PASTE: THE APP IS TYPING ANOTHER PROGRAM - WAIT'),
    ('VLOZENI: TO NENI TEXT (XEX / ATR / BAS NAHRAJ V APPCE)', 'PASTE: NOT TEXT (LOAD XEX / ATR / BAS IN THE APP)'),
    ('VLOZENI: ATARI 130XE NENI NA OBRAZOVCE', 'PASTE: THE ATARI 130XE IS NOT ON SCREEN'),
    ('VLOZENI: KNIHOVNA NENI NACTENA', 'PASTE: THE LIBRARY IS NOT LOADED'),
    ('VLOZENI: PRAZDNE', 'PASTE: EMPTY'),
    ('', 'PASTE: NO RESPONSE'),
]


def server(html):
    STAV = {'status': STAVY['menu'], 'vloz': 'VLOZENI: PRAZDNE', 'klavesa': 'OK:KeyA=63', 'konec': False}

    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def posli(self, kod, typ, data):
            self.send_response(kod)
            self.send_header('Content-Type', typ)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            p = self.path.split('?')[0]
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', html.encode('utf-8'))
            if p == '/status':
                return self.posli(200, 'text/plain', STAV['status'].encode())
            if p.endswith('.stream') or p in ('/stream.mjpg', '/stream.h264'):
                # jako Java bez snimku: spojeni zustane otevrene
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                    self.wfile.flush()
                    t0 = time.time()
                    while not STAV['konec'] and time.time() - t0 < 60:
                        time.sleep(0.1)
                except Exception:
                    pass
                return
            if p in ('/klavesa', '/klavesy'):
                return self.posli(200, 'text/plain', (STAV['klavesa'] + '\n').encode())
            if p == '/frame.jpg':
                return self.posli(503, 'text/plain; charset=utf-8', b'frame not ready\n')
            return self.posli(200, 'text/plain', b'')

        def do_POST(self):
            n = int(self.headers.get('Content-Length') or 0)
            if n:
                self.rfile.read(n)
            p = self.path.split('?')[0]
            if p == '/vloz':
                return self.posli(200, 'text/plain; charset=utf-8', STAV['vloz'].encode())
            return self.posli(200, 'text/plain', b'ok')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv, 'http://127.0.0.1:%d/' % srv.server_address[1], STAV


def projdi(b, html, popis_jazyka):
    """Vsechny stavy stranky -> [(popis, sber)], hlasky vlozeni, chyby JS."""
    srv, url, STAV = server(html)
    chyby = []
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(url)
    pg.wait_for_timeout(1500)
    sbery = [('nacteno (menu)', pg.evaluate(H.SBER_JS))]
    for jm in ('atari', 'atari_hrani', 'sega', 'ps1', 'menu'):
        STAV['status'] = STAVY[jm]
        pg.wait_for_timeout(2300)
        pg.mouse.move(400 + len(sbery) * 7, 300)       # panel OBRAZ + napoveda klavesnice
        pg.wait_for_timeout(500)
        sbery.append((jm, pg.evaluate(H.SBER_JS)))
    # panel OBRAZ: prepinace
    for tl in ('#sF', '#sV', '#sR'):
        try:
            pg.evaluate("(s) => document.querySelector(s).click()", tl)
        except Exception as e:
            chyby.append('klik %s: %s' % (tl, str(e)[:100]))
        pg.wait_for_timeout(300)
    sbery.append(('panel OBRAZ po prepnuti', pg.evaluate(H.SBER_JS)))
    # vlozeni Ctrl+V v Atari: odpovedi appky
    STAV['status'] = STAVY['atari']
    pg.wait_for_timeout(2300)
    hlasky = []
    for odp, _ in VLOZ:
        STAV['vloz'] = odp
        pg.evaluate("""() => { const dt = new DataTransfer(); dt.setData('text/plain', '10 PRINT 1\\n20 GOTO 10\\n');
            document.dispatchEvent(new ClipboardEvent('paste', {clipboardData: dt, bubbles: true, cancelable: true})); }""")
        pg.wait_for_timeout(700)
        hlasky.append(pg.evaluate("() => { const o = document.getElementById('vlozmsg'); return o ? o.textContent : ''; }"))
    sbery.append(('po vlozeni', pg.evaluate(H.SBER_JS)))
    # posledni klavesa za napovedou [..] (odpoved appky na /klavesa): Atari hrani, PS1
    napovedy = []
    for stav, odp, klav in (('atari_hrani', 'SKOK', 'k'), ('atari_hrani', 'VYSTREL', 'l'), ('atari_hrani', 'SMER:KeyW', 'w'),
                            ('atari', 'NEZNAMA:F12', 'F12'), ('ps1', 'PS1:KRIZEK', 'k'), ('ps1', 'PS1:TROJUHELNIK', 'i'),
                            ('ps1', 'PS1:NAHORU', 'ArrowUp')):
        STAV['status'] = STAVY[stav]
        STAV['klavesa'] = odp
        pg.wait_for_timeout(1600)
        pg.keyboard.press(klav)
        pg.wait_for_timeout(1700)
        napovedy.append(pg.evaluate("""() => { const d = [...document.querySelectorAll('body > div')].filter(e => /\\[.*\\]$/.test(e.textContent));
            return d.length ? d[d.length - 1].textContent : ''; }"""))
    sbery.append(('napoveda s klavesou', pg.evaluate(H.SBER_JS)))
    # vlozeni mimo Atari
    STAV['status'] = STAVY['sega']
    pg.wait_for_timeout(2300)
    pg.evaluate("""() => { const dt = new DataTransfer(); dt.setData('text/plain', 'x');
        document.dispatchEvent(new ClipboardEvent('paste', {clipboardData: dt, bubbles: true, cancelable: true})); }""")
    pg.wait_for_timeout(500)
    STAV['konec'] = True
    ctx.close()
    srv.shutdown()
    return sbery, hlasky + napovedy, chyby


def main():
    # a) cesky = B313
    stary = subprocess.run(['git', '-C', KOREN, 'show', 'HEAD:app/src/main/java/eu/atarihelp/emu10/MainActivity.java'],
                           capture_output=True, text=True, check=True).stdout
    CZ0 = stranka_z_javy(stary)
    # B314 zmenil jen napovedu klavesnice: pise ji prelozenou (napTr - cesky bez NapJazyk = beze zmeny)
    zmeny = [("var kn=document.createElement('div');",
              "function napTr(s){ try{ return (window.NapJazyk&&NapJazyk.en())?NapJazyk.tr(s):s; }catch(e){ return s; } }var kn=document.createElement('div');"),
             ("var tx=zap?napPopisKlaves():'';", "var tx=zap?napTr(napPopisKlaves()):'';"),
             ("var t=napPopisKlaves();", "var t=napTr(napPopisKlaves());"),
             ("t+='   ['+window.napPosledni+']';", "t+='   ['+napTr(window.napPosledni)+']';")]
    CZ0b = CZ0
    for a, b in zmeny:
        CZ0b = CZ0b.replace(a, b, 1)
    over(CZ == CZ0b and CZ != CZ0, 'a) cesky: stranka WEB TV z MainActivity = B313 + jen napTr u napovedy klavesnice (%d znaku)' % len(CZ))
    over(EN.count('<script>') == CZ.count('<script>') + 2 and EN.index("window.NAP_JAZYK='en'") < EN.index('<body') if '<body' in EN else True,
         'a) anglicky: Java vlozi za </title> 2 skripty (NAP_JAZYK + prekladac se slovnikem), %d znaku navic' % (len(EN) - len(CZ)))
    with sync_playwright() as p:
        b = p.chromium.launch()
        cz, hl_cz, ch_cz = projdi(b, CZ, 'cs')
        cz0, hl_cz0, ch_cz0 = projdi(b, CZ0, 'cs')
        en, hl_en, ch_en = projdi(b, EN, 'en')
        b.close()
    # a2) cesky se stranka chova jako v B313: stejne texty ve vsech stavech, stejne hlasky
    rozdil = []
    import re
    def bez_casu(v):   # stavovy radek ma hodiny a fps - ty se mezi behy lisi
        return [re.sub(r'\d{1,2}:\d{2}:\d{2}( [AP]M)?', 'HH:MM:SS', re.sub(r'\d+fps', 'Nfps', t)) for t in v]
    for (popis, x), (_, y) in zip(cz, cz0):
        tx, ty = bez_casu(x['texty']), bez_casu(y['texty'])
        if tx != ty or x['atributy'] != y['atributy'] or x['titulek'] != y['titulek']:
            rozdil.append('%s: +%s -%s' % (popis, [t for t in tx if t not in ty][:2], [t for t in ty if t not in tx][:2]))
    over(not rozdil and hl_cz == hl_cz0 and len(cz) == len(cz0), 'a) cesky: %d stavu stranky + %d hlasek stejne jako B313%s' % (
        len(cz), len(hl_cz), '' if not rozdil and hl_cz == hl_cz0 else ' - ROZDIL: %s %s' % (rozdil[:3], [h for h in hl_cz if h not in hl_cz0][:3])))
    # b) anglicky bez cestiny
    zbytky, vse = [], []
    for popis, s in en:
        for druh, t, sl in H.zbytky_cestiny(s):
            zbytky.append('%s / %s: "%s" (%s)' % (popis, druh, t[:90], ','.join(sl[:4])))
        vse += s['texty'] + s['atributy'] + ['[titulek] ' + s['titulek']]
    for t in hl_en:
        sl = H.ceska_slova(t)
        if sl:
            zbytky.append('hlaska vlozeni: "%s" (%s)' % (t, ','.join(sl[:4])))
    zbytky = list(dict.fromkeys(zbytky))
    over(not zbytky, 'b) anglicky: %d stavu stranky (menu, Atari psani / hrani, Sega, PS1, panel OBRAZ, vlozeni) bez cestiny (%d ruznych textu)%s' % (
        len(en), len(set(vse)), '' if not zbytky else ' - ZBYTKY: ' + ' | '.join(zbytky[:8])))
    with open(os.path.join(ZDE, 'VYSTUP_texty_tv_en.txt'), 'w', encoding='utf-8') as f:
        f.write('B314 - vsechny anglicke texty stranky WEB TV (z MainActivity.java + nap_jazyk.js + en_tv.js), pro kontrolu ocima\n')
        for t in dict.fromkeys(vse + ['[vlozeni] ' + h for h in hl_en]):
            f.write(t + '\n')
    # napoveda klavesnice v kazdem stavu anglicky
    nap = {popis: [t for t in s['texty'] if ' - ' in t and ('F9' in t or 'WASD' in t or 'Enter' in t)] for popis, s in en}
    ocek = {'atari': 'ATARI - type normally', 'atari_hrani': 'ATARI GAME MODE', 'sega': 'SEGA - WASD move', 'ps1': 'PS1 - arrows/WASD',
            'menu': 'MENU - arrows'}
    for k, v in ocek.items():
        ok = any(t.startswith(v) for t in nap.get(k, []))
        over(ok, 'c) napoveda klavesnice %-11s anglicky: %s' % (k, (nap.get(k) or ['(neni)'])[0][:110]))
    # c) hlasky vlozeni
    for (odp, cil), cz_h, en_h in zip(VLOZ, hl_cz, hl_en):
        cz_ok = cz_h == (odp or 'VLOZENI: BEZ ODPOVEDI')
        over(en_h == cil and cz_ok, 'c) vlozeni: appka "%s" -> cesky "%s", anglicky "%s"' % (odp or '(prazdna odpoved)', cz_h, en_h))
    # posledni klavesa za napovedou
    KLAV = [('SKOK', '[JUMP]'), ('VYSTREL', '[FIRE]'), ('SMER:KeyW', '[DIRECTION:KeyW]'), ('NEZNAMA:F12', '[UNKNOWN:F12]'),
            ('PS1:KRIZEK', '[CROSS]'), ('PS1:TROJUHELNIK', '[TRIANGLE]'), ('PS1:NAHORU', '[UP]')]
    for (odp, cil), cz_h, en_h in zip(KLAV, hl_cz[len(VLOZ):], hl_en[len(VLOZ):]):
        ok = en_h.endswith('   ' + cil) and not H.ceska_slova(en_h) and cz_h.endswith('[' + odp.replace('PS1:', '') + ']')
        over(ok, 'c) napoveda + klavesa: appka "%s" -> anglicky "%s"' % (odp, en_h[-95:]))
    over(len(hl_en) == len(VLOZ) + len(KLAV), 'c) vsechny hlasky zachyceny (%d)' % len(hl_en))
    # d) chyby JS
    over(not ch_cz and not ch_en and not ch_cz0, 'd) zadna chyba JavaScriptu (B313 %d, cesky %d, anglicky %d)%s' % (
        len(ch_cz0), len(ch_cz), len(ch_en), '' if not (ch_cz or ch_en) else ': ' + ' | '.join((ch_cz + ch_en)[:4])))
    vystup.insert(0, 'B314 - stranka WEB TV cesky / anglicky (Chromium, stranka z MainActivity.java, server jako Java)')
    vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
    vystup.append(vysl)
    print(vysl)
    open(os.path.join(ZDE, 'VYSTUP_test_b314_jazyk_tv.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
