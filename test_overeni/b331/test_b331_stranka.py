"""B331: OPTIONS -> HERNI OVLADAC -> PRIPOJIT OVLADAC (BLUETOOTH) v Chromiu (skutecna stranka appky
assets/index.html, falesny most AHBT misto MainActivity.AHBluetooth - co stranka zavola, se zapise):
  1) po otevreni OPTIONS: tlacitko PRIPOJIT OVLADAC, panel hledani schovany, zapamatovane ovladace (vypnuty)
  2) klepnuti -> AHBT.hledej(), panel s textem z Javy, v seznamu jen ovladace, UKAZAT VSE (pocet ostatnich)
  3) UKAZAT VSE -> i ostatni zarizeni, tlacitko JEN OVLADACE
  4) klepnuti na ovladac -> AHBT.sparuj(adresa), text sparovano
  5) ovladac se pripojil -> v seznamu "pripojeny", zapamatovane "(pripojeny)"
  6) NASTAVENI BLUETOOTH -> AHBT.nastaveni(); ZAPNOUT POLOHU jen pri POLOHA_VYPNUTA -> AHBT.poloha()
  7) jmeno zarizeni s HTML znackami se ukaze jako text (zadny vlozeny prvek)
  8) zavreni OPTIONS -> AHBT.zastav(), stranka se uz na stav nepta
  9) anglicky: CONNECT CONTROLLER (BLUETOOTH), BLUETOOTH SETTINGS, SHOW ALL, Remembered ... (off), bez cestiny
 10) starsi appka bez mostu AHBT: klepnuti bez chyby
 11) zadna chyba JavaScriptu
Spusteni z test_overeni/b331: python3 test_b331_stranka.py -> VYSTUP_test_b331_stranka.txt
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
window.__bt = {volani: [], stavu: 0, f: 'NIC', t: '', z: [], m: [{j: 'Pro Controller', p: 0}]};
window.AHBT = {
  hledej: function () {
    __bt.volani.push('hledej'); __bt.f = 'HLEDAM'; __bt.t = 'TEXT_Z_JAVY hledam (12 s)';
    __bt.z = [{a: 'AA:BB:CC:DD:EE:01', j: 'Pro Controller', o: 1, s: 0, p: 0},
              {a: 'AA:BB:CC:DD:EE:02', j: 'JBL Flip 5', o: 0, s: 1, p: 0},
              {a: 'AA:BB:CC:DD:EE:03', j: '<b id="vlozeno">Jana</b> iPhone', o: 0, s: 0, p: 0}];
  },
  sparuj: function (a) { __bt.volani.push('sparuj:' + a); __bt.f = 'SPAROVANO'; __bt.t = 'TEXT_Z_JAVY sparovano'; },
  zastav: function () { __bt.volani.push('zastav'); },
  stav: function () { __bt.stavu++; return JSON.stringify({f: __bt.f, t: __bt.t, z: __bt.z, m: __bt.m}); },
  nastaveni: function () { __bt.volani.push('nastaveni'); },
  poloha: function () { __bt.volani.push('poloha'); }
};
'''


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


def seznam(pg):
    return pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#stBtSeznam button'), function (b) { return b.textContent; })")


def volani(pg):
    return pg.evaluate('() => window.__bt.volani.slice()')


def klik(pg, sel):
    pg.click(sel)
    pg.wait_for_timeout(900)


def main():
    srv, zaklad = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        ctx, pg = novy(b, zaklad, chyby, FALESNY_AHBT)
        t1 = text(pg, 'stBtHledej')
        over(videt(pg, 'stBtHledej') and t1.strip() == 'PŘIPOJIT OVLADAČ (BLUETOOTH)' and not videt(pg, 'stBt')
             and text(pg, 'stBtZapam') == 'Zapamatované: Pro Controller (vypnutý)',
             '1) OPTIONS: tlacitko "' + t1.strip() + '", panel hledani schovany, "' + text(pg, 'stBtZapam') + '"')

        klik(pg, '#stBtHledej')
        s2 = seznam(pg)
        over(volani(pg) == ['hledej'] and videt(pg, 'stBt') and text(pg, 'stBtStav') == 'TEXT_Z_JAVY hledam (12 s)'
             and s2 == ['\U0001F3AE Pro Controller'] and text(pg, 'stBtVse') == 'UKÁZAT VŠE (2)',
             '2) klepnuti -> AHBT.hledej(), text z Javy, v seznamu jen ovladac ' + str(s2) + ', "' + text(pg, 'stBtVse') + '"')

        klik(pg, '#stBtVse')
        s3 = seznam(pg)
        over(len(s3) == 3 and s3[1] == 'JBL Flip 5 – spárovaný' and text(pg, 'stBtVse') == 'JEN OVLADAČE',
             '3) UKAZAT VSE -> i ostatni zarizeni ' + str(s3) + ', tlacitko "' + text(pg, 'stBtVse') + '"')
        vlozeno = pg.evaluate("() => !!document.getElementById('vlozeno')")
        over(not vlozeno and s3[2] == '<b id="vlozeno">Jana</b> iPhone',
             '7) jmeno zarizeni s HTML znackami je jen text (zadny vlozeny prvek): "' + s3[2] + '"')
        klik(pg, '#stBtVse')

        pg.click('#stBtSeznam button')
        pg.wait_for_timeout(900)
        over(volani(pg) == ['hledej', 'sparuj:AA:BB:CC:DD:EE:01'] and text(pg, 'stBtStav') == 'TEXT_Z_JAVY sparovano',
             '4) klepnuti na ovladac -> AHBT.sparuj("AA:BB:CC:DD:EE:01"), text "' + text(pg, 'stBtStav') + '"')

        pg.evaluate("() => { __bt.f = 'PRIPOJENO'; __bt.t = 'TEXT_Z_JAVY pripojeno'; __bt.z[0].p = 1; __bt.m = [{j: 'Pro Controller', p: 1}]; }")
        pg.wait_for_timeout(1000)
        s5 = seznam(pg)
        over(s5 == ['\U0001F3AE Pro Controller – připojený ✔'] and text(pg, 'stBtZapam') == 'Zapamatované: Pro Controller (připojený)'
             and text(pg, 'stBtStav') == 'TEXT_Z_JAVY pripojeno',
             '5) pripojeno: seznam ' + str(s5) + ', "' + text(pg, 'stBtZapam') + '" (stranka se pta sama kazdych 0,7 s)')

        klik(pg, '#stBtNastaveni')
        pol0 = videt(pg, 'stBtPoloha')
        pg.evaluate("() => { __bt.f = 'POLOHA_VYPNUTA'; }")
        pg.wait_for_timeout(1000)
        pol1 = videt(pg, 'stBtPoloha')
        klik(pg, '#stBtPoloha')
        v6 = volani(pg)
        over(v6[-2:] == ['nastaveni', 'poloha'] and not pol0 and pol1,
             '6) NASTAVENI BLUETOOTH -> AHBT.nastaveni(); ZAPNOUT POLOHU jen pri POLOHA_VYPNUTA -> AHBT.poloha()')

        pg.evaluate('() => window.napZavriNastaveni()')
        pg.wait_for_timeout(600)
        n0 = pg.evaluate('() => window.__bt.stavu')
        pg.wait_for_timeout(2100)
        n1 = pg.evaluate('() => window.__bt.stavu')
        over(volani(pg)[-1] == 'zastav' and n1 == n0 and not videt(pg, 'stBt'),
             '8) zavreni OPTIONS -> AHBT.zastav(), stranka se uz na stav nepta (' + str(n0) + ' -> ' + str(n1) + ')')
        ctx.close()

        # ---- anglicky ----
        ctx, pg = novy(b, zaklad, chyby, FALESNY_AHBT, jazyk='en')
        klik(pg, '#stBtHledej')
        e = [text(pg, 'stBtHledej').strip(), text(pg, 'stBtNastaveni').strip(), text(pg, 'stBtVse'), text(pg, 'stBtZapam'),
             text(pg, 'stBtPoloha').strip()]
        print('anglicky:', e)
        cest = H.ceska_slova(' '.join(e), {'pro', 'controller'})
        over(e == ['CONNECT CONTROLLER (BLUETOOTH)', 'BLUETOOTH SETTINGS', 'SHOW ALL (2)', 'Remembered: Pro Controller (off)', 'TURN ON LOCATION']
             and not cest, '9) anglicky: ' + ' | '.join(e) + ', bez cestiny ' + str(cest))
        ctx.close()

        # ---- starsi appka bez mostu AHBT ----
        ctx, pg = novy(b, zaklad, chyby, None)
        klik(pg, '#stBtHledej')
        over(videt(pg, 'stBt') and seznam(pg) == [] and text(pg, 'stBtZapam') == '',
             '10) bez mostu AHBT (starsi appka): klepnuti bez chyby, prazdny seznam')
        ctx.close()
        b.close()
    srv.shutdown()
    over(not chyby, '11) zadna chyba JavaScriptu na strance' + (': ' + '; '.join(chyby[:4]) if chyby else ''))
    vystup.append('')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYB' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    with open(os.path.join(ZDE, 'VYSTUP_test_b331_stranka.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
