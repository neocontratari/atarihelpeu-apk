#!/usr/bin/env python3
"""B314: HLAVNI NABIDKA A OPTIONS (index.html) CESKY / ANGLICKY - Chromium.

Rene: "slo by dat vyber jazyka - volbu Cze a Ang? ... po volbe na ang by byla apka
v anglictine? a naopak?"

Kontroluje (stranka PRESNE z appky, mosty Javy falesne):
  1) cesky: stranka vypada jako v B313 - vsechny texty stejne (jen pribyl radek JAZYK · LANGUAGE)
  2) anglicky: zadny zbytek cestiny (texty, atributy, titulek) ve vsech stavech OPTIONS
     (rucni nastaveni, zvuk, WEB TV, servis - spatny kod, pockej, otevreno, odemknout, zamknout,
     TV pokus) i v kartach (zamcena SEGA a PS1: duvod anglicky)
  3) prepnuti v OPTIONS za chodu: CZ -> EN (Java dostane "en", stranka anglicky bez nacteni),
     EN -> CZ (zpet presne ceske texty)
  4) zadna chyba JavaScriptu
Pouziti: python3 test_b314_jazyk_menu.py            -> VYSTUP_test_b314_jazyk_menu.txt
"""
import os
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

import jazyk_harness as H

ZDE = H.ZDE
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


MOCK = {
    'AHINTRO.menuBude': False,
    'AHINTRO.postupStav': '{"sega":false,"ps1":false,"zbyvaMinut":30,"zbyvaRadku":47}',
    'AHINTRO.servisZbyva': 'fn:return window.__servis||0;',
    'AHINTRO.servisKod': 'fn:var k=String(args[0]); if(k==="SPATNY") return "SPATNE"; if(k==="RYCHLE") return "POCKEJ:30"; window.__servis=600; return "OK";',
    'AHINTRO.odemkniDalsi': 'SEGA',
    'AHINTRO.zamkniVse': 'OK',
    'AHTVWEB.getQualityTier': 2,
    'AHTVWEB.setQualityTier': 'OK',
    'AHTVWEB.start': 'TV_WEB_CAST_OK http://192.168.1.50:8765/ APP',
    'AHTVWEB.status': '',
}

# B340: co se v cestine hlavni nabidky ZAMERNE zmenilo od B313 (srovnani "cesky beze zmeny" plati pro vsechno
# ostatni). Novy text v OPTIONS / kartach = pridat sem s cislem buildu, jinak kontrola 1 spadne (oci na cestinu).
# cele nove radky OPTIONS (souvisly blok textu; AUTOMATICKY / LOW / HIGH uz v B313 byly u jinych radku -
# jako samostatne texty by se lisil jen jejich pocet a poradi)
BLOKY_PO_B313 = [
    ('B320', ['HERNÍ OVLADAČ', 'AUTOMATICKY', 'VŽDY DOTYK', 'SKRÝT DOTYK']),
    ('B331', ['PŘIPOJIT OVLADAČ (BLUETOOTH)', 'NASTAVENÍ BLUETOOTH', 'ZAPNOUT POLOHU']),
    ('B338', ['VÝKON ZAŘÍZENÍ', 'AUTO', 'LOW', 'HIGH']),
    ('B339', ['ÚLOŽIŠTĚ']),
]
PRIBYLO_PO_B313 = {
    'Žádný ovladač – po připojení (Retroid, Bluetooth, USB) se dotyková tlačítka ve hře schovají': 'B320',
    'Atari 130XE s grafikou VBXE, celé v C++. Klávesnice, joystick, kazety, diskety a hry ze serveru N&P.': 'B321',
    '🔓 ODEMKNOUT': 'B322',
    'ZAMČENO · Napiš v Atari ještě 47 řádků programu – nebo odemkni hned.': 'B322',
    'ZAMČENO · Nejdřív odemkni Segu, pak 120 hodin hraní na Seze.': 'B322',
    'LOG APPKY PRO PODPORU': 'B337',
}
UBYLO_PO_B313 = {
    'Atari 130XE s grafikou VBXE, celé v C++. Klávesnice, joystick, kazety, diskety a hry z atarihelp.eu.': 'B321',
    '🔒 ZAMČENO': 'B322',
    'ZAMČENO · Napiš v Atari ještě 47 řádků programu (řádky s číslem, počítají se při RUN nebo LIST).': 'B322',
    'ZAMČENO · Nejdřív odemkni Segu, pak 30 minut hraní na Seze.': 'B322',
}

# kroky v OPTIONS: (popis, javascript) - po kazdem se sbira text
KROKY = [
    ('menu (karusel v HTML)', "1", 300),
    ('karta SEGA (zamcena)', "document.querySelectorAll('.karta')[1].click()", 900),
    # B340: SPUSTIT u zamcene SEGY uz neni hlaska - od B322 otevre stranku ODEMKNOUT (hry/odemknout.html),
    # stranka menu by odesla a dalsi kroky by nemely karty -> zvlast kontrola 1c
    ('karta PS1 (zamcena)', "document.querySelectorAll('.karta')[2].click()", 900),
    ('OPTIONS otevrene', "window.napOtevriNastaveni()", 150),
    ('RUCNE + zvuk LOW', "document.getElementById('stModeMan').click(); document.getElementById('stPLow').click()", 150),
    ('zvuk HIGH', "document.getElementById('stPHigh').click()", 150),
    ('kvalita MED', "document.getElementById('stQMed').click()", 150),
    ('WEB TV spustena', "document.getElementById('stStartTv').click()", 150),
    ('TV pokus 1', "document.getElementById('stTvPokus').click()", 150),
    ('TV pokus 2', "document.getElementById('stTvPokus').click()", 150),
    ('servis: 5x nadpis', "for (var i=0;i<5;i++) document.getElementById('stTitul').click()", 150),
    ('servis: spatny kod', "document.getElementById('stKod').value='SPATNY'; document.getElementById('stKodOk').click()", 150),
    ('servis: moc pokusu', "document.getElementById('stKod').value='RYCHLE'; document.getElementById('stKodOk').click()", 150),
    ('servis: spravny kod', "document.getElementById('stKod').value='DOBRY'; document.getElementById('stKodOk').click()", 150),
    ('servis otevreny', "window.napOtevriNastaveni()", 150),
    ('ODEMKNOUT DALSI', "document.getElementById('stOdemkni').click()", 150),
    ('OPTIONS znovu', "window.napOtevriNastaveni()", 150),
    ('ZAMKNOUT VSE', "document.getElementById('stZamkni').click()", 150),
    ('OPTIONS a AUTOMATICKY', "window.napOtevriNastaveni(); document.getElementById('stModeAuto').click()", 150),
]


def bez_bloku(texty):
    """B340: z textu stranky vyjme nove radky OPTIONS (BLOKY_PO_B313) - kazdy blok jednou, jen souvisly."""
    t = list(texty)
    for _, blok in BLOKY_PO_B313:
        for i in range(len(t) - len(blok) + 1):
            if t[i:i + len(blok)] == blok:
                del t[i:i + len(blok)]
                break
    return t


def projdi(page, url, jazyk, chyby_js, sbirej=True):
    page.goto(url)
    page.wait_for_timeout(400)
    sbery = []
    for popis, js, cekej in KROKY:
        page.evaluate("() => { %s }" % js)
        page.wait_for_timeout(cekej)
        if sbirej:
            sbery.append((popis, page.evaluate(H.SBER_JS)))
    return sbery


def main():
    global chyb
    srv, zaklad = H.server()
    # B313 (puvodni stranka) do docasneho adresare - pro srovnani cestiny
    tmp = tempfile.mkdtemp(prefix='b314_menu_')
    subprocess.run('git -C "%s" archive %s app/src/main/assets | tar -x -C "%s"' % (H.KOREN, H.B313, tmp), shell=True, check=True)
    srv0, zaklad0 = H.server(os.path.join(tmp, 'app/src/main/assets'))
    with sync_playwright() as p:
        b = p.chromium.launch()
        chyby_js = []

        def stranka(jazyk, mock=MOCK):
            ctx = b.new_context(viewport={'width': 412, 'height': 846})
            ctx.add_init_script(H.init_skript(jazyk, mock))
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chyby_js.append('%s: %s' % (jazyk, e)))
            return ctx, pg

        # --- 1) cesky = B313 ---
        c1, p1 = stranka('cs')
        cz = projdi(p1, zaklad + 'index.html', 'cs', chyby_js)
        c0, p0 = stranka('cs')
        puv = projdi(p0, zaklad0 + 'index.html', 'cs', chyby_js)
        rozdil = []
        for (popis, a), (_, b0) in zip(cz, puv):
            ta = [t for t in bez_bloku(a['texty']) if t not in ('ČEŠTINA', 'ENGLISH', 'JAZYK · LANGUAGE') and t not in PRIBYLO_PO_B313]
            tb = [t for t in b0['texty'] if t not in UBYLO_PO_B313]
            if ta != tb:
                rozdil.append('%s: %s' % (popis, sorted(set(ta) ^ set(tb))[:6]))
            if a['atributy'] != b0['atributy']:
                rozdil.append('%s atributy: %s' % (popis, sorted(set(a['atributy']) ^ set(b0['atributy']))[:6]))
        over(not rozdil, '1) cesky: vsech %d stavu stranky ma stejne texty jako B313 (jen radek JAZYK · LANGUAGE a %d zamernych zmen'
             ' B320-B339 - BLOKY / PRIBYLO / UBYLO_PO_B313)%s'
             % (len(KROKY), len(BLOKY_PO_B313) + len(PRIBYLO_PO_B313) + len(UBYLO_PO_B313), '' if not rozdil else ' - ROZDIL: ' + ' | '.join(rozdil[:4])))
        jaz = p1.evaluate("() => [document.getElementById('stJazCs').className, document.getElementById('stJazEn').className, document.documentElement.lang]")
        over(jaz[0] == 'active' and jaz[1] == '', '1b) cesky: v OPTIONS je oznacena CESTINA (%s)' % jaz)
        c1.close(); c0.close()

        # --- 1c) B340: SPUSTIT u zamcene SEGY = stranka ODEMKNOUT (od B322; drive hlaska - krok v KROKY odesel ze stranky) ---
        c5, p5 = stranka('cs')
        p5.goto(zaklad + 'index.html')
        p5.wait_for_timeout(400)
        p5.evaluate("() => document.querySelectorAll('.karta')[1].click()")
        p5.wait_for_timeout(900)
        p5.evaluate("() => document.getElementById('spustit').click()")
        p5.wait_for_timeout(700)
        over(p5.url.endswith('hry/odemknout.html?co=sega'),
             '1c) cesky: zamcena SEGA, SPUSTIT -> stranka ODEMKNOUT (%s)' % p5.url.replace(zaklad, ''))
        c5.close()

        # --- 2) anglicky: zadny zbytek cestiny ---
        c2, p2 = stranka('en')
        en = projdi(p2, zaklad + 'index.html', 'en', chyby_js)
        zbytky = []
        for popis, s in en:
            for druh, t, sl in H.zbytky_cestiny(s):
                zbytky.append('%s / %s: "%s" (%s)' % (popis, druh, t, ','.join(sl)))
        zbytky = list(dict.fromkeys(zbytky))
        over(not zbytky, '2) anglicky: ve vsech %d stavech (karusel, zamky, OPTIONS, WEB TV, servis, TV pokus) zadna cestina%s'
             % (len(KROKY), '' if not zbytky else ' - ZBYTKY: ' + ' | '.join(zbytky[:8])))
        vsechno = []
        for popis, s in en:
            vsechno += s['texty'] + s['atributy']
        vsechno = list(dict.fromkeys(vsechno))
        prvni = en[0][1]
        over(prvni['lang'] == 'en' and 'LOCKED' in ' '.join(prvni['texty']) and 'START WEB TV' in vsechno
             and any('more program lines on the Atari' in t for t in vsechno),
             '2b) anglicky: <html lang=en>, karta LOCKED s duvodem "Type 47 more program lines on the Atari ...", START WEB TV')
        with open(os.path.join(ZDE, 'VYSTUP_texty_menu_en.txt'), 'w', encoding='utf-8') as f:
            f.write('B314 - vsechny texty hlavni nabidky a OPTIONS anglicky (pro kontrolu ocima)\n')
            for t in vsechno:
                f.write(t + '\n')
        c2.close()

        # --- 3) prepnuti za chodu ---
        c3, p3 = stranka('cs')
        p3.goto(zaklad + 'index.html')
        p3.wait_for_timeout(300)
        p3.evaluate("() => window.napOtevriNastaveni()")
        p3.wait_for_timeout(100)
        cz0 = p3.evaluate(H.SBER_JS)
        p3.evaluate("() => document.getElementById('stJazEn').click()")
        p3.wait_for_timeout(200)
        en1 = p3.evaluate(H.SBER_JS)
        volani = p3.evaluate("() => window.__napVolani.filter(function(v){return v.indexOf('AHJAZYK.nastav')===0;})")
        zb = [t for d, t, s in H.zbytky_cestiny(en1) if t not in ('LANGUAGE: ENGLISH',)]
        over(volani == ['AHJAZYK.nastav:en'] and not zb and en1['lang'] == 'en',
             '3) CZ -> EN za chodu: Java dostala "en" (%s), stranka hned anglicky (zbytky: %s)' % (volani, zb[:4]))
        p3.wait_for_timeout(1800)       # toast zhasne
        p3.evaluate("() => document.getElementById('stJazCs').click()")
        p3.wait_for_timeout(1800)
        cz1 = p3.evaluate(H.SBER_JS)
        # posledni text je hlaska (toast) - ta zustava i schovana: neporovnava se
        a = cz0['texty'][:-1]
        bb = cz1['texty'][:-1]
        over(a == bb and cz1['atributy'] == cz0['atributy'] and cz1['lang'] == 'cs',
             '3b) EN -> CZ za chodu: presne puvodni ceske texty (%d textu)%s'
             % (len(a), '' if a == bb else ' - ROZDIL: %s' % sorted(set(a) ^ set(bb))[:6]))
        c3.close()

        # --- 3c) zalozni karusel: karta HELP anglicky = obrazek knihy GUIDE / CONTROLS a navod en.html ---
        for jazyk, obr, cil in (('cs', 'menu/karta_help.jpg', 'napoveda/index.html'), ('en', 'menu/karta_help_en.jpg', 'napoveda/en.html')):
            c4, p4 = stranka(jazyk)
            p4.goto(zaklad + 'index.html')
            p4.wait_for_timeout(500)
            im = p4.evaluate("""async () => { const im = [...document.querySelectorAll('#karusel .karta img')];
                for (const i of im) i.loading = 'eager';
                await Promise.all(im.map(i => i.decode().catch(() => null)));
                const i = im[im.length - 1]; return i ? [i.getAttribute('src'), i.naturalWidth] : ['', 0]; }""")
            n = p4.evaluate("() => document.querySelectorAll('#karusel .karta').length")
            p4.evaluate("(n) => { const k = document.querySelectorAll('#karusel .karta')[n - 1]; k.click(); }", n)
            p4.wait_for_timeout(1200)
            p4.evaluate("(n) => { const k = document.querySelectorAll('#karusel .karta')[n - 1]; k.click(); }", n)
            p4.wait_for_timeout(1500)
            url = p4.url
            over(im[0] == obr and im[1] > 0 and url.endswith(cil),
                 '3c) %s: karta HELP v zaloznim karuselu - obrazek %s (%d bodu siroky), otevre %s' % (
                     'cesky' if jazyk == 'cs' else 'anglicky', im[0], im[1], url.split('/assets/')[-1].replace(zaklad, '')))
            c4.close()

        # --- 4) chyby JS ---
        over(not chyby_js, '4) zadna chyba JavaScriptu (%s)' % (chyby_js[:3] if chyby_js else 'cesky, anglicky, prepinani'))
        b.close()
    srv.shutdown(); srv0.shutdown()
    vystup.insert(0, 'B314 - hlavni nabidka a OPTIONS (index.html) cesky / anglicky, Chromium')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b314_jazyk_menu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
