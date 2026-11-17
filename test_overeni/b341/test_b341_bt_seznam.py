#!/usr/bin/env python3
"""B341: OPTIONS -> PRIPOJIT OVLADAC (BLUETOOTH) - SEZNAM SE NEPREKRESLUJE PORAD DOKOLA (Chromium, skutecna stranka).

Rene (TV, B340): "na tv v options nejde najet na najity ovladac controler - proste ten radek preskakuje".
Seznam nalezenych ovladacu se kazdych 0,7 s smazal a nakreslil znovu (btObnov) - zlaty ramecek dalkoveho
(B334) mel tlacitko, ktere za chvili zmizelo. B341: prekresli se jen pri zmene; pri zmene zustane ramecek
na tomtez zarizeni (data-a).

  T0) B340 (stara stranka z gitu): tlacitko nalezeneho ovladace je po 1 s jine (smazane a nove) = chyba,
  T1) B341 na TV (1280x720): ramecek sipkou dolu na "Pro Controller", po 2 s porad totez tlacitko a ramecek
      na nem; OK (Enter) -> AHBT.sparuj(adresa),
  T2) seznam se zmeni (prisel dalsi ovladac, prvni sparovany): tlacitka nova, ramecek na tomtez zarizeni,
      text "sparovany",
  T3) zapamatovany ovladac (PRIPOJIT "..."): po 2 s totez tlacitko,
  T4) telefon (dotyk): totez tlacitko po 2 s, klepnuti -> AHBT.sparuj,
  T5) jmeno zarizeni s HTML se nevlozi jako HTML (textContent - jako B332),
  T6) zadna chyba JavaScriptu.
Pouziti (z test_overeni/b341): python3 test_b341_bt_seznam.py   -> VYSTUP_test_b341_bt_seznam.txt
"""
import os
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

B340 = '802a087'   # commit B340 (stara stranka - prekreslovani kazdych 0,7 s)

FALESNY_AHBT = r'''
window.__bt = {volani: [], stavu: 0, f: 'HLEDAM', t: 'Hledam ovladace...',
  z: [{a: 'AA:BB:CC:DD:EE:11', j: 'Pro Controller', o: true, s: false, p: false},
      {a: 'AA:BB:CC:DD:EE:12', j: 'Telefon Pepa', o: false, s: false, p: false}],
  m: [{j: 'Battletron', p: 0, a: 'AA:BB:CC:DD:EE:21', h: 0},
      {j: '<b id="vlozeno">Pad</b>', p: 0, a: 'AA:BB:CC:DD:EE:22', h: -1}]};
window.AHBT = {
  hledej: function () { __bt.volani.push('hledej'); },
  sparuj: function (a) { __bt.volani.push('sparuj:' + a); },
  pripoj: function (a) { __bt.volani.push('pripoj:' + a); },
  zastav: function () { __bt.volani.push('zastav'); },
  stav: function () { __bt.stavu++; return JSON.stringify({f: __bt.f, t: __bt.t, z: __bt.z, m: __bt.m}); },
  nastaveni: function () { __bt.volani.push('nastaveni'); },
  poloha: function () { __bt.volani.push('poloha'); }
};
'''

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def otevri(b, zaklad, chyby, tv=True):
    if tv:
        ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    else:
        ctx = b.new_context(viewport={'width': 412, 'height': 846}, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript('cs', {}))
    ctx.add_init_script(FALESNY_AHBT)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(zaklad + 'index.html')
    pg.wait_for_timeout(900)
    pg.evaluate('() => window.napOtevriNastaveni()')
    pg.wait_for_timeout(400)
    pg.evaluate("() => document.getElementById('stBtHledej').click()")
    pg.wait_for_timeout(900)
    return ctx, pg


OZNAC = "(sel) => { window.__el = document.querySelector(sel); return window.__el ? window.__el.textContent : null; }"
STEJNE = "() => !!window.__el && document.body.contains(window.__el)"


def main():
    srv, zaklad = H.server()
    tmp = tempfile.mkdtemp(prefix='b341_b340_')
    subprocess.run('git -C "%s" archive %s app/src/main/assets | tar -x -C "%s"' % (H.KOREN, B340, tmp), shell=True, check=True)
    srv0, zaklad0 = H.server(os.path.join(tmp, 'app/src/main/assets'))
    chyby, chyby0 = [], []
    with sync_playwright() as p:
        b = p.chromium.launch()

        # ---- T0: B340 - tlacitko se kazdych 0,7 s nahradi ----
        ctx, pg = otevri(b, zaklad0, chyby0)
        t0 = pg.evaluate(OZNAC, '#stBtSeznam button')
        pg.wait_for_timeout(1000)
        stejne0 = pg.evaluate(STEJNE)
        ctx.close()
        over(t0 and not stejne0, 'T0) B340: tlacitko "%s" po 1 s smazane a nakreslene znovu (ramecek dalkoveho ho ztratil)' % t0)

        # ---- T1: B341 na TV ----
        ctx, pg = otevri(b, zaklad, chyby)
        pg.evaluate('() => NapTvOvladani.zapni()')
        cesta = []
        for _ in range(40):
            f = pg.evaluate("() => { var f = NapTvOvladani.fokus(); return f ? (f.getAttribute('data-a') || f.id || f.tagName) : null; }")
            cesta.append(f)
            if f == 'AA:BB:CC:DD:EE:11':
                break
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(60)
        pg.evaluate("() => { window.__el = NapTvOvladani.fokus(); }")
        pg.wait_for_timeout(2100)
        drzi = pg.evaluate("() => !!window.__el && document.body.contains(window.__el) && NapTvOvladani.fokus() === window.__el")
        stavu = pg.evaluate('() => __bt.stavu')
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(300)
        vol = pg.evaluate('() => __bt.volani.slice()')
        over(cesta[-1] == 'AA:BB:CC:DD:EE:11' and drzi and stavu >= 3 and 'sparuj:AA:BB:CC:DD:EE:11' in vol,
             'T1) TV: ramecek na "Pro Controller" (sipka dolu %dx), po 2 s (%d dotazu na stav) porad totez tlacitko a ramecek'
             ' na nem; OK -> %s' % (len(cesta) - 1, stavu, [v for v in vol if v.startswith('sparuj')]))

        # ---- T2: seznam se zmeni ----
        pg.evaluate("() => { __bt.z = [{a: 'AA:BB:CC:DD:EE:11', j: 'Pro Controller', o: true, s: true, p: false},"
                    " {a: 'AA:BB:CC:DD:EE:13', j: 'Battletron', o: true, s: false, p: false}]; }")
        pg.wait_for_timeout(1000)
        t2 = pg.evaluate("() => { var f = NapTvOvladani.fokus(); return {jine: f !== window.__el, a: f ? f.getAttribute('data-a') : null,"
                         " t: f ? f.textContent : '', n: document.querySelectorAll('#stBtSeznam button').length,"
                         " v: !!f && document.body.contains(f)}; }")
        over(t2['jine'] and t2['v'] and t2['a'] == 'AA:BB:CC:DD:EE:11' and 'spárovaný' in t2['t'] and t2['n'] == 2,
             'T2) seznam se zmenil (2 ovladace, prvni sparovany): tlacitka nova, ramecek porad na "%s"' % t2['t'])

        # ---- T3: zapamatovany ovladac ----
        t3 = pg.evaluate(OZNAC, '#stBtZapamSeznam button')
        pg.wait_for_timeout(2100)
        stejne3 = pg.evaluate(STEJNE)
        vlozeno = pg.evaluate("() => !!document.getElementById('vlozeno')")
        texty = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#stBtZapamSeznam button'), function (b) { return b.textContent; })")
        ctx.close()
        over(t3 and stejne3, 'T3) zapamatovany "%s": po 2 s totez tlacitko' % t3)
        over(not vlozeno and any('<b id="vlozeno">' in t for t in texty),
             'T5) jmeno s HTML jako text (zadny vlozeny prvek): %s' % texty[-1:])

        # ---- T4: telefon ----
        ctx, pg = otevri(b, zaklad, chyby, tv=False)
        t4 = pg.evaluate(OZNAC, '#stBtSeznam button')
        pg.wait_for_timeout(2100)
        stejne4 = pg.evaluate(STEJNE)
        pg.tap('#stBtSeznam button')
        pg.wait_for_timeout(300)
        vol4 = pg.evaluate('() => __bt.volani.slice()')
        ctx.close()
        over(t4 and stejne4 and 'sparuj:AA:BB:CC:DD:EE:11' in vol4,
             'T4) telefon: "%s" po 2 s totez tlacitko, klepnuti -> sparuj' % t4)

        over(not chyby, 'T6) zadna chyba JavaScriptu%s' % ('' if not chyby else ': ' + '; '.join(chyby[:3])))
        b.close()
    srv.shutdown()
    srv0.shutdown()
    vystup.append('VYSLEDEK: %d testu, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b341_bt_seznam.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
