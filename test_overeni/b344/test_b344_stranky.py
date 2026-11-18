#!/usr/bin/env python3
"""B344 v Chromiu - skutecne stranky appky + falesne mosty Javy (../b314/jazyk_harness):

  O1-O3) OPTIONS -> PRIPOJIT OVLADAC (BLUETOOTH): sparovany, ale nepripojeny ovladac v seznamu hledani ma
         "spárovaný, klepni = spárovat znovu" (reproduktor jen "spárovaný", pripojeny "připojený ✔"), klepnuti ->
         AHBT.sparuj(adresa) a text faze ZNOVU z Javy; anglicky "paired, tap = pair again"
  A1-A3) zadni strana Atari (LOG / CHYBA): bez sekce CO TESTOVAT a bez starych poznamek k buildum B296-B312,
         ZAPSAT -> AHATARICPP.atariPoznamka, ZPET NA ATARI 130XE -> zobrazZarizeni, test jadra je tam;
         anglicky bez cestiny
  N1)    napoveda CZ / EN: polozka "Spárovat znovu" / "Pair it again" a jeji odkazy
  J)     zadna chyba JavaScriptu
Pouziti (z test_overeni/b344): python3 test_b344_stranky.py [PRAC] -> VYSTUP_test_b344_stranky.txt
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b344')
os.makedirs(PRAC, exist_ok=True)

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


FALESNY_AHBT = r'''
window.__bt = {volani: [], f: 'NIC', t: '', z: [], m: [{j: 'Pro Controller', p: 0, a: 'AA:BB:CC:DD:EE:01', h: 0}]};
window.AHBT = {
  hledej: function () {
    __bt.volani.push('hledej'); __bt.f = 'HOTOVO'; __bt.t = 'TEXT_Z_JAVY hledani skoncilo';
    __bt.z = [{a: 'AA:BB:CC:DD:EE:01', j: 'Pro Controller', o: 1, s: 1, p: 0},
              {a: 'AA:BB:CC:DD:EE:02', j: 'JBL Flip 5', o: 0, s: 1, p: 0},
              {a: 'AA:BB:CC:DD:EE:03', j: '8BitDo Pro 2', o: 1, s: 1, p: 1}];
  },
  sparuj: function (a) { __bt.volani.push('sparuj:' + a); __bt.f = 'ZNOVU'; __bt.t = 'TEXT_Z_JAVY paruji znovu'; },
  pripoj: function (a) { __bt.volani.push('pripoj:' + a); },
  zastav: function () { __bt.volani.push('zastav'); },
  stav: function () { return JSON.stringify({f: __bt.f, t: __bt.t, z: __bt.z, m: __bt.m}); },
  nastaveni: function () { __bt.volani.push('nastaveni'); },
  poloha: function () { __bt.volani.push('poloha'); }
};
'''

chybyJs = []


def otevri(b, zaklad, stranka, jazyk='cs', init=None, w=412, h=846):
    ctx = b.new_context(viewport={'width': w, 'height': h}, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    if init:
        ctx.add_init_script(init)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(stranka + ': ' + str(e).split('\n')[0][:200]))
    pg.goto(zaklad + stranka)
    pg.wait_for_timeout(1000)
    return ctx, pg


def seznam(pg):
    return pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#stBtSeznam button'), function (b) { return b.textContent; })")


srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()

    # ---------------- O) OPTIONS -> Bluetooth ----------------
    for jazyk, ocek in (('cs', ['🎮 Pro Controller – spárovaný, klepni = spárovat znovu', '🎮 8BitDo Pro 2 – připojený ✔',
                                'JBL Flip 5 – spárovaný']),
                        ('en', ['🎮 Pro Controller – paired, tap = pair again', '🎮 8BitDo Pro 2 – connected ✔',
                                'JBL Flip 5 – paired'])):
        ctx, pg = otevri(b, zaklad, 'index.html', jazyk, FALESNY_AHBT)
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(500)
        pg.click('#stBtHledej')
        pg.wait_for_timeout(900)
        pg.click('#stBtVse')                      # UKAZAT VSE - i reproduktor
        pg.wait_for_timeout(900)
        s = seznam(pg)
        over(sorted(s) == sorted(ocek), 'O1) %s seznam hledani: %s' % (jazyk.upper(), s))
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'options_bt_seznam.png'))
            pg.click('#stBtSeznam button[data-a="AA:BB:CC:DD:EE:01"]')
            pg.wait_for_timeout(900)
            vol = pg.evaluate('() => window.__bt.volani.slice()')
            st = pg.evaluate("() => document.getElementById('stBtStav').textContent")
            over(vol == ['hledej', 'sparuj:AA:BB:CC:DD:EE:01'] and st == 'TEXT_Z_JAVY paruji znovu',
                 'O2) klepnuti na sparovany ovladac -> AHBT.sparuj (Java ho sparuje znovu), text faze z Javy "%s"' % st)
            zap = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#stBtZapamSeznam button'), function (b) { return b.textContent; })")
            over(zap == ['🎮 PŘIPOJIT „Pro Controller“'], 'O3) zapamatovany nepripojeny ovladac ma dal PRIPOJIT: %s' % zap)
        ctx.close()

    # ---------------- A) zadni strana Atari ----------------
    for jazyk in ('cs', 'en'):
        ctx, pg = otevri(b, zaklad, 'emu_atari_cpp/index.html', jazyk)
        t = pg.evaluate('() => document.body.innerText')
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'atari_zadni_strana.png'), full_page=True)
            over('CO TESTOVAT' not in t and 'B312' not in t and 'B296' not in t and 'TEST JADRA' in t and 'Jak mi log poslat' in t
                 and pg.evaluate("() => !document.getElementById('kroky') && !document.getElementById('souhrn')"),
                 'A1) CZ: bez CO TESTOVAT a starych poznamek B296-B312, test jadra a "Jak mi log poslat" zustaly (%d znaku textu)' % len(t))
            pg.fill('#poznamkaText', 'zkouska B344')
            pg.click('#btnPoznamka')
            pg.wait_for_timeout(300)
            pg.click('#btnZpet')
            pg.wait_for_timeout(300)
            vol = pg.evaluate('() => window.__napVolani.slice()')
            over('AHATARICPP.atariPoznamka' in vol and 'AHATARICPP.zobrazZarizeni' in vol and 'AHATARICPP.zapisKrok' not in vol,
                 'A2) ZAPSAT -> atariPoznamka, ZPET NA ATARI 130XE -> zobrazZarizeni (zapisKrok uz nikdo nevola)')
        else:
            zb = H.ceska_slova(t, povolena=('atari', 'xe', 'turbo', 'basic', 'zapis', 'funguje'))   # zapis_funguje = jmeno radku logu
            over('WHAT TO TEST' not in t and 'CORE TEST' in t and 'B312' not in t and not zb,
                 'A3) EN: bez WHAT TO TEST a poznamek, bez cestiny%s' % ((' - CESKY: %s' % zb) if zb else ''))
        ctx.close()

    # ---------------- N) napoveda ----------------
    for jazyk, st, nadpis, odkazy in (('cs', 'napoveda/index.html', 'Spárovat znovu:', 'PŘIPOJIT OVLADAČ (BLUETOOTH)'),
                                      ('en', 'napoveda/en.html', 'Pair it again:', 'CONNECT CONTROLLER (BLUETOOTH)')):
        ctx, pg = otevri(b, zaklad, st, jazyk)
        li = pg.evaluate("n => { const b = [...document.querySelectorAll('li > b')].find(x => x.textContent === n); return b ? b.parentNode.innerText : ''; }", nadpis)
        over(li.startswith(nadpis) and odkazy in li and ('telefonem a televizí' in li or 'phone and the TV' in li),
             'N1) %s napoveda: "%s"' % (jazyk.upper(), li[:150]))
        ctx.close()
    b.close()
srv.shutdown()

over(not chybyJs, 'J) zadna chyba JavaScriptu' + (': %s' % chybyJs[:3] if chybyJs else ''))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b344_stranky.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
