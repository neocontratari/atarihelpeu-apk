#!/usr/bin/env python3
"""B334: STRANKY APPKY DALKOVYM OVLADACEM TV / GAMEPADEM (Chromium, SKUTECNE stranky z assets).

Rene: "aby se to dalo ovladat ovladacem od TV nebo gamepadem" (appka primo na Android TV, bez dotyku).
Dalkovy ovladac posle do WebView sipky a OK (Enter) - stranky je resi spolecne/nap_tv_ovladani.js:
  T1) OPTIONS na TV (okno 1280 x 720, appka zavola NapTvOvladani.zapni()): zlaty ramecek hned na prvni
      polozce PANELU, sipka dolu prochazi polozky shora dolu, nikdy mimo panel (stranka pod nim),
      tri tlacitka vedle sebe (AUTOMATICKY / VZDY DOTYK / SKRYT DOTYK) - vlevo / vpravo,
  T2) OK na polozce = klepnuti: CESTINA -> vpravo ENGLISH -> OK -> AHJAZYK.nastav('en'); ZAVRIT -> panel zavren,
  T3) karusel karet (HTML menu, kdyz neni C++ menu) si sipky resi sam - ramecek mu nic nebere,
  T4) volba jazyka pri prvnim spusteni (stranka si sipky resi sama, presouva focus): ramecek jde za
      focusem stranky, OK stiskne to, co je opravdu vybrane (jedno klepnuti, ne dve),
  T5) napoveda (dlouha stranka): sipky prochazi dlazdice kapitol, OK skoci na kapitolu, kdyz uz dole
      nic neni, stranka se posouva,
  T6) telefon (dotyk): ramecek az po prvni sipce, dotyk ho schova,
  T7) vsech 16 stranek appky: skript je na strance PRVNI, NapTvOvladani existuje, zapni() bez chyby,
      zadna chyba JavaScriptu ze skriptu ovladani.
Pouziti (z test_overeni/b334): python3 test_b334_tv_ovladani.py   -> VYSTUP_test_b334_tv_ovladani.txt
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


FOKUS = "() => { var f = NapTvOvladani.fokus(); if (!f) return null; var p = document.getElementById('settingsPanel'); " \
        "return {id: f.id || f.tagName, t: (f.textContent || '').trim().slice(0, 30), vPanelu: !!(p && p.contains(f)), " \
        "kruh: f.classList.contains('nap-tv-fokus'), y: Math.round(f.getBoundingClientRect().top)}; }"
STRANKY = ['index.html', 'hry/index.html', 'hry/aktivace.html', 'hry/odemknout.html', 'hry/jazyk.html',
           'napoveda/index.html', 'napoveda/en.html', 'player/index.html', 'dj/index.html',
           'emu_ps1/index.html', 'emu_sega/index.html', 'emu_atari_cpp/index.html',
           'intro/index.html', 'intro/etapa1.html', 'intro/etapa2.html', 'intro/etapa5.html']


def tv(b, chyby, mock=None):
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    ctx.add_init_script(H.init_skript('cs', mock or {}))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    return ctx, pg


def main():
    srv, z = H.server()
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        # ---- T1 + T2: OPTIONS ----
        ch1 = []
        ctx, pg = tv(b, ch1)
        pg.goto(z + 'index.html')
        pg.wait_for_timeout(900)
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(500)
        prvni = pg.evaluate('() => NapTvOvladani.zapni()')
        f0 = pg.evaluate(FOKUS)
        cesta = [f0]
        for _ in range(16):
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(60)
            cesta.append(pg.evaluate(FOKUS))
        ids = [c['id'] for c in cesta if c]
        vse_v_panelu = all(c and c['vPanelu'] and c['kruh'] for c in cesta)
        over(f0 and f0['vPanelu'] and f0['kruh'] and vse_v_panelu and 'stZavrit' in ids and ids.index('stZavrit') > 5,
             'T1) OPTIONS na TV: ramecek hned na "%s" (v panelu), sipka dolu: %s - vse v panelu, nic ze stranky pod nim'
             % (prvni, ' > '.join(dict.fromkeys(ids))))
        # vlevo / vpravo ve trojici tlacitek
        pg.evaluate('() => NapTvOvladani.zapni()')
        while pg.evaluate(FOKUS)['id'] != 'stOvlDotyk':
            pg.keyboard.press('ArrowUp')
            pg.wait_for_timeout(40)
            if pg.evaluate(FOKUS)['id'] == 'stTitul':
                break
        pg.keyboard.press('ArrowRight')
        vpravo = pg.evaluate(FOKUS)['id']
        pg.keyboard.press('ArrowLeft')
        pg.keyboard.press('ArrowLeft')
        vlevo = pg.evaluate(FOKUS)['id']
        over(vpravo == 'stOvlSkryt' and vlevo == 'stOvlAuto', 'T1) vedle sebe: VZDY DOTYK -> vpravo %s -> 2x vlevo %s' % (vpravo, vlevo))
        # T2: CESTINA -> ENGLISH -> OK
        for _ in range(12):
            pg.keyboard.press('ArrowUp')
            pg.wait_for_timeout(30)
        pg.keyboard.press('ArrowDown')
        na = pg.evaluate(FOKUS)['id']
        pg.keyboard.press('ArrowRight')
        na2 = pg.evaluate(FOKUS)['id']
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(300)
        volani = pg.evaluate('() => window.__napVolani.slice()')
        over(na == 'stJazCs' and na2 == 'stJazEn' and volani.count('AHJAZYK.nastav:en') == 1,
             'T2) OK = klepnuti: %s -> vpravo %s -> OK -> AHJAZYK.nastav(en) %dx' % (na, na2, volani.count('AHJAZYK.nastav:en')))
        for _ in range(20):
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(30)
        posl = pg.evaluate(FOKUS)['id']
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(400)
        zavreno = pg.evaluate("() => !document.getElementById('settingsPanel').classList.contains('open')")
        over(posl == 'stZavrit' and zavreno, 'T2) dolu az na %s -> OK -> panel OPTIONS zavren: %s' % (posl, zavreno))
        # ---- T3: karusel karet (panel zavreny, C++ menu neni) ----
        kar0 = pg.evaluate("() => document.getElementById('karusel').scrollLeft")
        f_pred = pg.evaluate(FOKUS)
        pg.keyboard.press('ArrowRight')
        pg.wait_for_timeout(700)
        kar1 = pg.evaluate("() => document.getElementById('karusel').scrollLeft")
        f_po = pg.evaluate(FOKUS)
        over(kar1 != kar0 and f_po == f_pred, 'T3) karusel karet si sipky resi sam: posun %d -> %d, ramecek se nehnul' % (kar0, kar1))
        over(not ch1, 'T1-T3) zadna chyba JavaScriptu%s' % ((': ' + '; '.join(ch1)) if ch1 else ''))
        ctx.close()

        # ---- T4: volba jazyka ----
        ch4 = []
        ctx, pg = tv(b, ch4, {'AHJAZYK.prvniVolba': 'fn:window.__volba=(window.__volba||[]).concat(args);return "ok";'})
        pg.add_init_script("window.AHJAZYK.prvniVolba = function (k) { window.__volba = (window.__volba || []).concat([k]); };")
        pg.goto(z + 'hry/jazyk.html')
        pg.wait_for_timeout(400)
        pg.evaluate('() => NapTvOvladani.zapni()')
        z0 = pg.evaluate(FOKUS)
        pg.keyboard.press('ArrowDown')
        pg.wait_for_timeout(100)
        z1 = pg.evaluate(FOKUS)
        akt = pg.evaluate('() => document.activeElement.id')
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(300)
        volba = pg.evaluate('() => window.__volba || []')
        over(z0 and z0['id'] == 'en' and z1 and z1['id'] == 'cs' and akt == 'cs' and volba == ['cs'],
             'T4) volba jazyka: ramecek %s -> sipka dolu (stranka presune focus) -> ramecek %s (focus %s) -> OK -> vybrano %s'
             % (z0 and z0['id'], z1 and z1['id'], akt, volba))
        over(not ch4, 'T4) zadna chyba JavaScriptu%s' % ((': ' + '; '.join(ch4)) if ch4 else ''))
        ctx.close()

        # ---- T5: napoveda ----
        ch5 = []
        ctx, pg = tv(b, ch5)
        pg.goto(z + 'napoveda/index.html')
        pg.wait_for_timeout(800)
        pg.evaluate('() => NapTvOvladani.zapni()')
        dl = []
        for _ in range(6):
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(60)
            f = pg.evaluate("() => { var f = NapTvOvladani.fokus(); return f ? (f.getAttribute('href') || f.id || f.tagName) : null; }")
            dl.append(f)
        y0 = pg.evaluate('() => Math.round(window.scrollY)')
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(900)
        y1 = pg.evaluate('() => Math.round(window.scrollY)')
        h = pg.evaluate('() => location.hash')
        over(any(d and d.startswith('#') for d in dl) and (y1 != y0 or h), 'T5) napoveda: sipky po dlazdicich %s, OK -> kapitola (posun %d -> %d, %s)'
             % (' > '.join([str(d) for d in dl]), y0, y1, h or '-'))
        for _ in range(60):
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(20)
        y2 = pg.evaluate('() => Math.round(window.scrollY)')
        over(y2 > y1, 'T5) dlouha stranka: sipkou dolu az na konec (posun %d -> %d)' % (y1, y2))
        over(not ch5, 'T5) zadna chyba JavaScriptu%s' % ((': ' + '; '.join(ch5)) if ch5 else ''))
        ctx.close()

        # ---- T6: telefon ----
        ch6 = []
        ctx = b.new_context(viewport={'width': 412, 'height': 846}, has_touch=True, is_mobile=True)
        ctx.add_init_script(H.init_skript('cs', {}))
        pg = ctx.new_page()
        pg.on('pageerror', lambda e: ch6.append(str(e)[:200]))
        pg.goto(z + 'index.html')
        pg.wait_for_timeout(900)
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(400)
        bez = pg.evaluate(FOKUS)
        pg.keyboard.press('ArrowDown')
        po = pg.evaluate(FOKUS)
        pg.touchscreen.tap(200, 300)
        pg.wait_for_timeout(200)
        po_dotyku = pg.evaluate("() => document.querySelectorAll('.nap-tv-fokus').length")
        over(bez is None and po and po['kruh'] and po_dotyku == 0, 'T6) telefon: bez ramecku, po sipce ramecek na %s, dotyk ho schova (%d)' % (po and po['id'], po_dotyku))
        ctx.close()

        # ---- T7: vsechny stranky ----
        vys = []
        spatne = []
        for s in STRANKY:
            ch = []
            ctx, pg = tv(b, ch)
            try:
                pg.goto(z + s, wait_until='load', timeout=15000)
                pg.wait_for_timeout(400)
                r = pg.evaluate("""() => { var sk = document.getElementsByTagName('script'); var prvni = sk.length ? sk[0].getAttribute('src') || '' : '';
                    return {je: !!window.NapTvOvladani, prvni: prvni.indexOf('nap_tv_ovladani.js') >= 0, zap: window.NapTvOvladani ? NapTvOvladani.zapni() : null,
                            n: window.NapTvOvladani ? NapTvOvladani.kandidati().length : -1}; }""")
                pg.keyboard.press('ArrowDown')
                pg.keyboard.press('ArrowUp')
                pg.wait_for_timeout(100)
            except Exception as e:
                r = {'je': False, 'chyba': str(e)[:100]}
            nase = [c for c in ch if 'nap_tv_ovladani' in c or 'NapTvOvladani' in c]
            ok = r.get('je') and r.get('prvni') and not nase
            vys.append('%s(%s)' % (s, r.get('n')))
            if not ok:
                spatne.append('%s %s %s' % (s, r, nase))
            ctx.close()
        over(not spatne, 'T7) %d stranek: skript ovladani je PRVNI, NapTvOvladani + zapni() bez chyby (polozek: %s)%s'
             % (len(STRANKY), ', '.join(vys), (' | SPATNE: ' + '; '.join(spatne)) if spatne else ''))
        b.close()
    srv.shutdown()
    v = 'VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb)
    print(v)
    vystup.append(v)
    open(os.path.join(ZDE, 'VYSTUP_test_b334_tv_ovladani.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


main()
