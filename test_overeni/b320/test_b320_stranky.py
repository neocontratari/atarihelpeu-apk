#!/usr/bin/env python3
"""B320: STRANKY PS1, SEGA A OPTIONS S HERNIM OVLADACEM (Chromium, stranky presne z appky - assets).

Uzivatel z fora (Retroid Pocket 6): "po pripojeni by se na obrazovce stahnul nas d-pad a najelo to na
origo tlacitka". Java (NapOvladac) rekne strance stav pres window.napOvladacStav(stav, jmeno) a pri
nacteni ho stranka zjisti sama (AHOVL.stav()): 0 = dotykove ovladani videt, 1 = schovane (hraje
ovladac - dotyk na misto tlacitka ho ukaze a tlacitko zaroven funguje, Java se to dozvi pres
AHOVL.dotyk), 2 = schovane vzdy (OPTIONS -> HERNI OVLADAC: SKRYT DOTYK).
Dotyky jsou SKUTECNE dotykove udalosti Chromia (CDP Input.dispatchTouchEvent), mosty Javy falesne
(zapisuji, co stranka zavolala - jako ../b318/test_b318_stranka.py).

Kontroly (na sirku jako telefon / Retroid):
  PS1   1) stav 1 pri nacteni: D-pad, KRIZEK ..., L1 L2 R1 R2, SELECT START pruhledne; MYS, kolecko a obraz ne
        2) dotyk na misto schovaneho D-padu: ovladani zpet, AHOVL.dotyk('PS1'), D-pad zaroven poslal smer
        3) stav 2: ovladani neni (display none), dotyk na jeho misto nic nedela
        4) uprava rozlozeni (layoutEditEnter): tlacitka videt i pri schovani
        5) stav 0 = vse presne jako bez ovladace (vypocitane styly tlacitek stejne jako stranka bez AHOVL)
        6) starsi appka bez mostu AHOVL: stranka jako drive, zadna chyba
  SEGA  7) stav 1: D-pad, A, B, C, START pruhledne; dotyk ho ukaze (AHOVL.dotyk('Sega')) a D-pad zaroven jede
        8) stav 2 + 0 a bez AHOVL jako u PS1
  OPTIONS (hlavni stranka) 9) HERNI OVLADAC: rezim z AHOVL.rezim(), napoveda se jmenem ovladace, kliknuti
        -> AHOVL.nastavRezim(n); 10) anglicky: GAME CONTROLLER / AUTOMATIC / ALWAYS TOUCH / HIDE TOUCH,
        napoveda bez cestiny
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b320_stranky.py      -> VYSTUP_test_b320_stranky.txt
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
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


NA_SIRKU = {'width': 846, 'height': 412}
JMENO = 'Retroid Pocket Controller'


def ahovl(stav, rezim=0, jmeno=JMENO):
    """falesny most AHOVL (MainActivity.AHOvladac) - co stranka zavola, se zapise"""
    return '''
(function(){
  window.__ovl = {stav: %s, rezim: %d, jmeno: %s, dotyky: [], nastav: []};
  window.AHOVL = {
    stav: function () { return window.__ovl.stav; },
    dotyk: function (k) { window.__ovl.dotyky.push(String(k)); },
    rezim: function () { return window.__ovl.rezim; },
    nastavRezim: function (r) { window.__ovl.rezim = r | 0; window.__ovl.nastav.push(r | 0); },
    jmeno: function () { return window.__ovl.jmeno; }
  };
})();
''' % (repr(stav), rezim, repr(jmeno))


class Prsty:
    def __init__(self, pg):
        self.cdp = pg.context.new_cdp_session(pg)

    def tukni(self, x, y, pg, drz=120):
        self.cdp.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [{'x': x, 'y': y, 'id': 1, 'radiusX': 4, 'radiusY': 4, 'force': 1}]})
        pg.wait_for_timeout(drz)
        self.cdp.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})


def novy(b, adresa, stranka, okno, chyby, init=None, jazyk='cs'):
    ctx = b.new_context(viewport=okno, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    if init:
        ctx.add_init_script(init)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(stranka + ': ' + str(e).split('\n')[0][:200]))
    pg.goto(adresa + stranka)
    pg.wait_for_timeout(900)
    return ctx, pg


STYL_JS = '''(ids) => ids.map(function (id) {
  var e = document.getElementById(id); if (!e) return id + ':chybi';
  var c = getComputedStyle(e); return id + ':' + c.display + ':' + c.opacity + ':' + c.visibility + ':' + Math.round(e.getBoundingClientRect().width);
})'''


def styly(pg, ids):
    return pg.evaluate(STYL_JS, ids)


def pruhledne(pg, ids):
    return all(s.split(':')[2] == '0' for s in styly(pg, ids))


VYCHOZI = {}      # pruhlednost tlacitek na strance bez ovladace (PS1 MYS 0,9, Sega ovladani 0,55 - 0,6 ...)


def zaklad(pg, stranka, ids):
    for s in styly(pg, ids):
        VYCHOZI[(stranka, s.split(':')[0])] = float(s.split(':')[2])


def videt(pg, ids, stranka='ps1'):
    return all(s.split(':')[1] != 'none' and abs(float(s.split(':')[2]) - VYCHOZI.get((stranka, s.split(':')[0]), 1.0)) < 0.02
               for s in styly(pg, ids))


def nic(pg, ids):
    return all(s.split(':')[1] == 'none' for s in styly(pg, ids))


def stred(pg, sel):
    b = pg.locator(sel).bounding_box()
    return b['x'] + b['width'] / 2, b['y'] + b['height'] / 2


def volani(pg, predpona):
    return [v for v in pg.evaluate('() => window.__napVolani || []') if v.startswith(predpona)]


def main():
    srv, zaklad_url = H.server()
    chyby = []
    PS1_OVL = ['dpad', 'btnTriangle', 'btnCircle', 'btnCross', 'btnSquare', 'btnL1', 'btnL2', 'btnR1', 'btnR2', 'btnSelect', 'btnStart']
    PS1_ZUSTAVA = ['btnMys', 'btnSettings']
    SEGA_OVL = ['dpad', 'btnA', 'btnB', 'btnC', 'btnStart']
    with sync_playwright() as p:
        b = p.chromium.launch()
        # vychozi vzhled tlacitek (stranka bez mostu AHOVL = jako B319)
        for stranka, ids in (('ps1', PS1_OVL + PS1_ZUSTAVA), ('sega', SEGA_OVL)):
            ctx, pg = novy(b, zaklad_url, 'emu_%s/index.html' % stranka, NA_SIRKU, chyby, None)
            pg.wait_for_timeout(500)
            zaklad(pg, stranka, ids)
            ctx.close()
        print('vychozi pruhlednost:', VYCHOZI)
        # ================= PS1 =================
        ctx, pg = novy(b, zaklad_url, 'emu_ps1/index.html', NA_SIRKU, chyby, ahovl('1|' + JMENO))
        pg.wait_for_timeout(500)
        tridy = pg.evaluate('() => document.body.className')
        over('napOvladac' in tridy.split() and pruhledne(pg, PS1_OVL) and videt(pg, PS1_ZUSTAVA),
             'PS1 1) AHOVL.stav() = "1|' + JMENO + '" pri nacteni: D-pad, KRIZEK, KOLECKO, CTVEREC, TROJUHELNIK, L1 L2 R1 R2, SELECT, START '
             'pruhledne; MYS a kolecko nastaveni zustavaji')
        pr = Prsty(pg)
        x, y = stred(pg, '#dpad')
        pg.evaluate('() => { window.__napVolani = []; }')
        pr.tukni(x, y - 25, pg)
        pg.wait_for_timeout(600)
        vstup = volani(pg, 'AHPS1.ps1Input')
        over(pg.evaluate('() => window.__ovl.dotyky.join(",")') == 'PS1' and 'napOvladac' not in pg.evaluate('() => document.body.className').split()
             and videt(pg, PS1_OVL) and len(vstup) >= 1,
             'PS1 2) dotyk na misto schovaneho D-padu: ovladani zase videt, AHOVL.dotyk("PS1") (Java: dalsi stisk na ovladaci ho schova), '
             'D-pad zaroven poslal smer (' + str(len(vstup)) + 'x AHPS1.ps1Input)')
        pg.evaluate('() => window.napOvladacStav(2, "' + JMENO + '")')
        pg.wait_for_timeout(500)
        nic2 = nic(pg, PS1_OVL)
        pg.evaluate('() => { window.__napVolani = []; window.__ovl.dotyky = []; }')
        pr.tukni(x, y - 25, pg)
        pg.wait_for_timeout(300)
        over(nic2 and not volani(pg, 'AHPS1.ps1Input') and pg.evaluate('() => window.__ovl.dotyky.length') == 0 and videt(pg, PS1_ZUSTAVA),
             'PS1 3) stav 2 (SKRYT DOTYK): dotykove ovladani neni (display none), dotyk na jeho misto nic neposle a neukaze ho; MYS a kolecko zustavaji')
        pg.evaluate('() => window.napOvladacStav(1, "' + JMENO + '")')
        pg.evaluate("() => document.getElementById('btnEditLayout').click()")      # menu D-PAD -> UPRAVIT ROZLOZENI
        pg.wait_for_timeout(600)
        edit = pg.evaluate("() => document.getElementById('dpad').classList.contains('layoutEdit')") and \
            all(s.split(':')[1] != 'none' and float(s.split(':')[2]) > 0.5 for s in styly(pg, ['dpad', 'btnCross', 'btnStart']))
        pg.evaluate("() => document.getElementById('layoutEditDone').click()")
        pg.wait_for_timeout(600)
        over(edit and pruhledne(pg, ['dpad', 'btnCross']), 'PS1 4) uprava rozlozeni tlacitek: ovladani videt i pri schovani; po ni zase schovane')
        pg.evaluate('() => window.napOvladacStav(0, "")')
        pg.wait_for_timeout(600)
        s0 = styly(pg, PS1_OVL + PS1_ZUSTAVA)
        ctx.close()
        ctx, pg = novy(b, zaklad_url, 'emu_ps1/index.html', NA_SIRKU, chyby, None)       # starsi appka: bez AHOVL
        pg.wait_for_timeout(500)
        bez = styly(pg, PS1_OVL + PS1_ZUSTAVA)
        over(s0 == bez, 'PS1 5) stav 0 = presne jako bez ovladace (vypocitane styly ' + str(len(s0)) + ' tlacitek stejne jako stranka bez AHOVL)')
        x, y = stred(pg, '#dpad')
        Prsty(pg).tukni(x, y - 25, pg)
        pg.wait_for_timeout(300)
        over(len(volani(pg, 'AHPS1.ps1Input')) >= 1 and not pg.evaluate('() => !!window.__ovl'),
             'PS1 6) starsi appka bez mostu AHOVL: stranka jako drive (D-pad posila smer), zadna chyba')
        ctx.close()
        # na vysku: ovladani se neschovava (pravidla jen na sirku - na vysku je pod obrazem, nic nezakryva)
        ctx, pg = novy(b, zaklad_url, 'emu_ps1/index.html', {'width': 412, 'height': 846}, chyby, ahovl('1|' + JMENO))
        pg.wait_for_timeout(500)
        over(not pruhledne(pg, ['dpad']), 'PS1 6b) na vysku se dotykove ovladani neschovava (je pod obrazem hry)')
        ctx.close()

        # ================= SEGA =================
        ctx, pg = novy(b, zaklad_url, 'emu_sega/index.html', NA_SIRKU, chyby, ahovl('1|' + JMENO))
        pg.wait_for_timeout(500)
        over('napOvladac' in pg.evaluate('() => document.body.className').split() and pruhledne(pg, SEGA_OVL),
             'SEGA 7a) stav 1 pri nacteni: D-pad, A, B, C, START pruhledne')
        x, y = stred(pg, '#dpad')
        pg.evaluate('() => { window.__napVolani = []; }')
        Prsty(pg).tukni(x, y - 25, pg)
        pg.wait_for_timeout(600)
        segaVstup = [v for v in pg.evaluate('() => window.__napVolani || []') if v.startswith('AHSega.') or v.startswith('AHNATIVE.')]
        over(pg.evaluate('() => window.__ovl.dotyky.join(",")') == 'Sega' and videt(pg, SEGA_OVL, 'sega') and len(segaVstup) >= 1,
             'SEGA 7b) dotyk na misto schovaneho D-padu: ovladani zpet, AHOVL.dotyk("Sega"), D-pad zaroven jede (' + ', '.join(sorted(set(segaVstup))[:3]) + ')')
        pg.evaluate('() => window.napOvladacStav(2, "")')
        pg.wait_for_timeout(500)
        nic7 = nic(pg, SEGA_OVL)
        pg.evaluate('() => window.napOvladacStav(0, "")')
        pg.wait_for_timeout(600)
        s0 = styly(pg, SEGA_OVL)
        ctx.close()
        ctx, pg = novy(b, zaklad_url, 'emu_sega/index.html', NA_SIRKU, chyby, None)
        pg.wait_for_timeout(500)
        over(nic7 and s0 == styly(pg, SEGA_OVL), 'SEGA 8) stav 2 = ovladani neni; stav 0 = presne jako bez ovladace (a bez AHOVL jako drive)')
        ctx.close()

        # ================= OPTIONS =================
        ctx, pg = novy(b, zaklad_url, 'index.html', {'width': 412, 'height': 846}, chyby, ahovl('0|', 0))
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(400)
        akt = pg.evaluate("() => ['stOvlAuto','stOvlDotyk','stOvlSkryt'].filter(function(i){return document.getElementById(i).classList.contains('active');}).join(',')")
        h0 = pg.evaluate("() => document.getElementById('stOvlHint').textContent")
        pg.click('#stOvlDotyk')
        pg.wait_for_timeout(400)
        akt1 = pg.evaluate("() => ['stOvlAuto','stOvlDotyk','stOvlSkryt'].filter(function(i){return document.getElementById(i).classList.contains('active');}).join(',')")
        h1 = pg.evaluate("() => document.getElementById('stOvlHint').textContent")
        pg.click('#stOvlSkryt')
        pg.wait_for_timeout(400)
        h2 = pg.evaluate("() => document.getElementById('stOvlHint').textContent")
        pg.evaluate("() => { window.__ovl.rezim = 0; window.__ovl.jmeno = ''; }")
        pg.click('#stOvlAuto')
        pg.wait_for_timeout(400)
        h3 = pg.evaluate("() => document.getElementById('stOvlHint').textContent")
        nast = pg.evaluate('() => window.__ovl.nastav.join(",")')
        print('napovedy:', h0, '|', h1, '|', h2, '|', h3)
        over(akt == 'stOvlAuto' and JMENO in h0 and akt1 == 'stOvlDotyk' and 'vždy' in h1 and 'jen ovladač' in h2 and 'Žádný ovladač' in h3 and nast == '1,2,0',
             'OPTIONS 9) HERNI OVLADAC: rezim z AHOVL.rezim() (AUTOMATICKY), napoveda se jmenem ovladace; VZDY DOTYK / SKRYT DOTYK / AUTOMATICKY '
             '-> AHOVL.nastavRezim(1, 2, 0), napoveda podle rezimu ("' + h3 + '")')
        ctx.close()
        ctx, pg = novy(b, zaklad_url, 'index.html', {'width': 412, 'height': 846}, chyby, ahovl('1|' + JMENO, 0), jazyk='en')
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(600)
        texty = pg.evaluate("() => { var s = document.getElementById('stOvladac'); var l = s.previousElementSibling;"
                            " return [l ? l.textContent : '', s.textContent.replace(/\\s+/g, ' ').trim(), document.getElementById('stOvlHint').textContent]; }")
        cest = H.ceska_slova(' '.join(texty), {'retroid', 'pocket', 'controller'})
        print('anglicky:', texty)
        over(texty[0].strip() == 'GAME CONTROLLER' and 'AUTOMATIC' in texty[1] and 'ALWAYS TOUCH' in texty[1] and 'HIDE TOUCH' in texty[1]
             and 'Controller: ' + JMENO in texty[2] and not cest,
             'OPTIONS 10) anglicky: GAME CONTROLLER - AUTOMATIC / ALWAYS TOUCH / HIDE TOUCH, napoveda "' + texty[2] + '", bez cestiny ' + str(cest))
        ctx.close()
        b.close()
    srv.shutdown()
    over(not chyby, 'zadna chyba JavaScriptu na strankach PS1, Sega a OPTIONS' + (': ' + '; '.join(chyby[:4]) if chyby else ''))
    vystup.append('')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYB' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b320_stranky.txt'), 'w', encoding='utf-8') as f:
        f.write('B320 - stranky PS1, Sega a OPTIONS s hernim ovladacem (Chromium, skutecne dotyky)\n')
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
