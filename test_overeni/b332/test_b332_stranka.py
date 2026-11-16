"""B332: OPTIONS -> HERNI OVLADAC -> zapamatovany ovladac, ktery neni pripojeny: tlacitko PRIPOJIT „jmeno“
v Chromiu (skutecna stranka appky assets/index.html, falesny most AHBT misto MainActivity.AHBluetooth):
  1) po otevreni OPTIONS: zapamatovane (vypnuty / pripojeny), PRIPOJIT jen u nepripojeneho, panel schovany
  2) klepnuti na PRIPOJIT -> AHBT.pripoj(adresa), panel s textem z Javy, stranka se pta na stav sama
  3) Android ho pripojuje -> "(pripojuje se…)"
  4) pripojeno -> tlacitko PRIPOJIT zmizi, "(pripojeny)", text z Javy
  5) zavreni OPTIONS -> AHBT.zastav(), stranka se uz na stav nepta
  6) jmeno s HTML znackami jen jako text (zadny vlozeny prvek)
  7) anglicky: CONNECT “Pro Controller”, Remembered: ... (off), ... (connected), bez cestiny
  8) starsi appka (most AHBT bez pripoj): klepnuti bez chyby
  9) zadna chyba JavaScriptu
Spusteni z test_overeni/b332: python3 test_b332_stranka.py -> VYSTUP_test_b332_stranka.txt
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
    print(vystup[-1])
    if not ok:
        chyb += 1


FALESNY_AHBT = r'''
window.__bt = {volani: [], stavu: 0, f: 'NIC', t: '', z: [],
  m: [{j: 'Pro Controller', p: 0, a: 'AA:BB:CC:DD:EE:01', h: 0},
      {j: 'Xbox Wireless Controller', p: 1, a: 'AA:BB:CC:DD:EE:02', h: 2},
      {j: '<b id="vlozeno">Pad</b>', p: 0, a: 'AA:BB:CC:DD:EE:03', h: -1}]};
window.AHBT = {
  hledej: function () { __bt.volani.push('hledej'); },
  sparuj: function (a) { __bt.volani.push('sparuj:' + a); },
  pripoj: function (a) { __bt.volani.push('pripoj:' + a); __bt.f = 'PRIPOJUJI'; __bt.t = 'TEXT_Z_JAVY cekam (30 s)'; },
  zastav: function () { __bt.volani.push('zastav'); },
  stav: function () { __bt.stavu++; return JSON.stringify({f: __bt.f, t: __bt.t, z: __bt.z, m: __bt.m}); },
  nastaveni: function () { __bt.volani.push('nastaveni'); },
  poloha: function () { __bt.volani.push('poloha'); }
};
'''
STARY_AHBT = FALESNY_AHBT.replace("  pripoj: function (a) { __bt.volani.push('pripoj:' + a); __bt.f = 'PRIPOJUJI'; __bt.t = 'TEXT_Z_JAVY cekam (30 s)'; },\n", '')


def novy(b, adresa, chyby, init, jazyk='cs'):
    ctx = b.new_context(viewport={'width': 412, 'height': 846}, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    if init:
        ctx.add_init_script(init)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append('index.html: ' + str(e).split('\n')[0][:200]))
    pg.goto(adresa + 'index.html')
    pg.wait_for_timeout(900)
    pg.evaluate('() => window.napOtevriNastaveni()')
    pg.wait_for_timeout(500)
    return ctx, pg


def text(pg, id_):
    return pg.evaluate("(i) => { var e = document.getElementById(i); return e ? e.textContent : null; }", id_)


def videt(pg, id_):
    return pg.evaluate("(i) => { var e = document.getElementById(i); return !!e && getComputedStyle(e).display !== 'none' && e.getBoundingClientRect().height > 0; }", id_)


def pripojit(pg):
    return pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#stBtZapamSeznam button'), function (b) { return b.textContent; })")


def volani(pg):
    return pg.evaluate('() => window.__bt.volani.slice()')


def main():
    srv, zaklad = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        ctx, pg = novy(b, zaklad, chyby, FALESNY_AHBT)
        z1 = text(pg, 'stBtZapam')
        t1 = pripojit(pg)
        over(z1 == 'Zapamatované: Pro Controller (vypnutý), Xbox Wireless Controller (připojený), <b id="vlozeno">Pad</b> (vypnutý)'
             and t1 == ['🎮 PŘIPOJIT „Pro Controller“', '🎮 PŘIPOJIT „<b id="vlozeno">Pad</b>“'] and not videt(pg, 'stBt'),
             '1) OPTIONS: "' + z1 + '", PRIPOJIT jen u nepripojenych ' + str(t1) + ', panel hledani schovany')
        vlozeno = pg.evaluate("() => !!document.getElementById('vlozeno')")
        over(not vlozeno, '6) jmeno s HTML znackami je jen text (zadny vlozeny prvek)')

        pg.click('#stBtZapamSeznam button')
        pg.wait_for_timeout(900)
        n0 = pg.evaluate('() => window.__bt.stavu')
        pg.wait_for_timeout(1500)
        n1 = pg.evaluate('() => window.__bt.stavu')
        over(volani(pg) == ['pripoj:AA:BB:CC:DD:EE:01'] and videt(pg, 'stBt') and text(pg, 'stBtStav') == 'TEXT_Z_JAVY cekam (30 s)' and n1 > n0,
             '2) klepnuti -> AHBT.pripoj("AA:BB:CC:DD:EE:01"), panel s textem "' + str(text(pg, 'stBtStav')) + '", stranka se pta sama (' +
             str(n0) + ' -> ' + str(n1) + ')')

        pg.evaluate("() => { __bt.m[0].h = 1; }")
        pg.wait_for_timeout(1000)
        z3 = text(pg, 'stBtZapam')
        over(z3.startswith('Zapamatované: Pro Controller (připojuje se…)'), '3) Android ho pripojuje: "' + z3 + '"')

        pg.evaluate("() => { __bt.m[0].p = 1; __bt.m[0].h = 2; __bt.f = 'PRIPOJENO'; __bt.t = 'TEXT_Z_JAVY pripojeno'; }")
        pg.wait_for_timeout(1000)
        t4 = pripojit(pg)
        z4 = text(pg, 'stBtZapam')
        over(t4 == ['🎮 PŘIPOJIT „<b id="vlozeno">Pad</b>“'] and z4.startswith('Zapamatované: Pro Controller (připojený)')
             and text(pg, 'stBtStav') == 'TEXT_Z_JAVY pripojeno',
             '4) pripojeno: PRIPOJIT u nej zmizel ' + str(t4) + ', "' + z4[:45] + '...", text "' + str(text(pg, 'stBtStav')) + '"')

        pg.evaluate('() => window.napZavriNastaveni()')
        pg.wait_for_timeout(600)
        s0 = pg.evaluate('() => window.__bt.stavu')
        pg.wait_for_timeout(2100)
        s1 = pg.evaluate('() => window.__bt.stavu')
        over(volani(pg)[-1] == 'zastav' and s1 == s0 and not videt(pg, 'stBt'),
             '5) zavreni OPTIONS -> AHBT.zastav(), stranka se uz na stav nepta (' + str(s0) + ' -> ' + str(s1) + ')')
        ctx.close()

        # ---- anglicky ----
        ctx, pg = novy(b, zaklad, chyby, FALESNY_AHBT, jazyk='en')
        e = [text(pg, 'stBtZapam')] + pripojit(pg)
        print('anglicky:', e)
        cest = H.ceska_slova(' '.join(e), {'pro', 'controller', 'xbox', 'wireless', 'pad'})
        over(e == ['Remembered: Pro Controller (off), Xbox Wireless Controller (connected), <b id="vlozeno">Pad</b> (off)',
                   '🎮 CONNECT “Pro Controller”', '🎮 CONNECT “<b id="vlozeno">Pad</b>”'] and not cest,
             '7) anglicky: ' + ' | '.join(e) + ', bez cestiny ' + str(cest))
        ctx.close()

        # ---- starsi appka: most AHBT bez pripoj ----
        ctx, pg = novy(b, zaklad, chyby, STARY_AHBT)
        pg.click('#stBtZapamSeznam button')
        pg.wait_for_timeout(900)
        over(videt(pg, 'stBt') and volani(pg) == [], '8) starsi appka (AHBT bez pripoj): klepnuti bez chyby, nic se nevolalo')
        ctx.close()
        b.close()
    srv.shutdown()
    over(not chyby, '9) zadna chyba JavaScriptu na strance' + (': ' + '; '.join(chyby[:4]) if chyby else ''))
    vystup.append('')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYB' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    with open(os.path.join(ZDE, 'VYSTUP_test_b332_stranka.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
