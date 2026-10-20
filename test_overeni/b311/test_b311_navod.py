#!/usr/bin/env python3
# B311: UPLNY NAVOD (assets/napoveda/index.html) v Chromiu - PRESNE soubor z appky.
#  - nacte se bez chyby JavaScriptu, pismo Chakra Petch z appky (pisma/*.woff2) se nacte,
#  - obsah: 13 dlazdic, kazda vede na existujici kapitolu, kazda kapitola ma ZPET DO MENU
#    (../index.html, soubor existuje), vsechny obrazky se nactou,
#  - klepnuti v obsahu posune stranku a NEZAPISE se do historie (ZPET na telefonu = menu),
#  - hledani bez diakritiky ("sipky" najde "šipky"), schova kapitoly bez shody, otevre
#    shodne otazky, "nic" hlasi, smazani vrati vse,
#  - plovouci tlacitko nahoru, zadne vodorovne posouvani na 360 px, na sirku,
#  - snimky pro kontrolu (jen pro me).
# Okno WebView v appce ma pristup k souborum (setAllowUniversalAccessFromFileURLs) -
# Chromium se proto spousti s --allow-file-access-from-files.
#   python3 test_b311_navod.py [PRAC]            -> VYSTUP_test_b311_navod.txt
import os
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b311'
os.makedirs(PRAC, exist_ok=True)
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


URL = 'file://' + os.path.join(A, 'napoveda/index.html')

with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files'])
    ctx = b.new_context(viewport={'width': 412, 'height': 915}, device_scale_factor=2, is_mobile=True, has_touch=True)
    pg = ctx.new_page()
    chybyJs, konzole = [], []
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.on('console', lambda m: konzole.append(m.type + ': ' + m.text) if m.type in ('error', 'warning') else None)
    neslo = []
    pg.on('requestfailed', lambda r: neslo.append(r.url))
    pg.goto(URL)
    pg.wait_for_timeout(900)

    # ---- 1) pismo z appky ----
    pismo = pg.evaluate("""async () => { await document.fonts.ready;
        return [document.fonts.check('700 20px NapChakra'), document.fonts.check('600 20px NapChakra'),
                [...document.fonts].filter(f => f.status === 'loaded').map(f => f.family + ' ' + f.weight)]; }""")
    over(pismo[0] and pismo[1] and len(pismo[2]) >= 2, '1) pismo Chakra Petch z appky (pisma/chakra_bold + semibold.woff2) nacteno: ' + ', '.join(pismo[2]))

    # ---- 2) obsah a kapitoly ----
    dl = pg.evaluate("""() => [...document.querySelectorAll('#obsah a')].map(a => [a.getAttribute('href'), a.innerText.replace(/\\s+/g,' ').trim()])""")
    cile = [h for h, _ in dl]
    existuji = pg.evaluate("ids => ids.map(h => !!document.getElementById(h.slice(1)))", cile)
    over(len(dl) == 13 and all(existuji), '2) obsah: 13 dlazdic, kazda vede na existujici kapitolu (%s)' % ', '.join(c[1:] for c in cile))
    kap = pg.evaluate("""() => [...document.querySelectorAll('section.kap')].map(s => ({id: s.id,
        nadpis: s.querySelector('h2').innerText, menu: [...s.querySelectorAll('.kap-paticka a')].map(a => a.getAttribute('href'))}))""")
    menuOk = all('../index.html' in k['menu'] and '#obsah' in k['menu'] for k in kap)
    over(len(kap) == 13 and menuOk and os.path.exists(os.path.join(A, 'index.html')),
         '2) 13 kapitol, kazda konci tlacitky OBSAH a ZPET DO MENU (../index.html existuje)')
    over([k['id'] for k in kap] == [c[1:] for c in cile], '2) poradi kapitol = poradi v obsahu')
    vnitrni = pg.evaluate("""() => [...document.querySelectorAll('main a[href^="#"]')].map(a => a.getAttribute('href').slice(1))
        .filter(id => !document.getElementById(id))""")
    over(not vnitrni, '2) vsechny odkazy uvnitr navodu vedou na existujici misto' + (' (chybi: %s)' % vnitrni if vnitrni else ''))
    obr = pg.evaluate("""async () => { const im = [...document.images]; for (const i of im) { i.loading = 'eager'; }
        await new Promise(r => setTimeout(r, 400)); await Promise.all(im.map(i => i.decode().catch(() => null)));
        return im.map(i => [i.getAttribute('src'), i.naturalWidth]); }""")
    spatne = [s for s, w in obr if w <= 0]
    over(len(obr) == 12 and not spatne, '2) vsech %d obrazku (6 karet z menu/ v obsahu a 6 v nadpisech kapitol) se nacte' % len(obr) + (' - NEJDOU: %s' % spatne if spatne else ''))
    logo = pg.evaluate("""() => { const b = getComputedStyle(document.querySelector('.minca')).backgroundImage; return b; }""")
    over('nap_logo_original.png' in logo and os.path.exists(os.path.join(A, 'design/nap_logo_original.png')), '2) logo N&P v liste (design/nap_logo_original.png)')
    pg.screenshot(path=os.path.join(PRAC, 'navod_1_nahore.png'))

    # ---- 3) klepnuti v obsahu: posun, bez historie ----
    hist0 = pg.evaluate('history.length')
    url0 = pg.url
    pg.click('#obsah a[href="#tv"]')
    pg.wait_for_timeout(1200)
    st = pg.evaluate("""() => { const r = document.getElementById('tv').getBoundingClientRect();
        return {y: scrollY, top: r.top, lista: document.querySelector('.lista').offsetHeight, hist: history.length, url: location.href}; }""")
    over(st['y'] > 1000 and 0 <= st['top'] - st['lista'] <= 30 and st['hist'] == hist0 and st['url'] == url0,
         '3) klepnuti v obsahu na WEB TV: stranka dojede na kapitolu tesne pod listu (top %.0f, lista %d), historie %d -> %d, adresa beze zmeny'
         % (st['top'], st['lista'], hist0, st['hist']))
    pg.screenshot(path=os.path.join(PRAC, 'navod_2_webtv.png'))
    nah = pg.evaluate("document.getElementById('nahoru').classList.contains('ukaz')")
    over(nah, '3) po odjeti z obsahu se ukaze plovouci tlacitko nahoru')
    pg.click('#nahoru')
    pg.wait_for_timeout(1200)
    st2 = pg.evaluate("""() => ({top: document.getElementById('obsah').getBoundingClientRect().top, hist: history.length})""")
    over(0 < st2['top'] < 140 and st2['hist'] == hist0, '3) tlacitko nahoru vrati na obsah (bez historie)')
    # odkaz v textu kapitoly (Odemykani)
    pg.evaluate("document.querySelector('#sega .uvod a[href=\"#odemykani\"]').scrollIntoView()")
    pg.click('#sega .uvod a[href="#odemykani"]')
    pg.wait_for_timeout(1200)
    st3 = pg.evaluate("""() => ({top: document.getElementById('odemykani').getBoundingClientRect().top, hist: history.length})""")
    over(0 <= st3['top'] < 100 and st3['hist'] == hist0, '3) odkaz v textu (Sega -> Odemykani) posune na kapitolu, bez historie')
    pg.screenshot(path=os.path.join(PRAC, 'navod_3_odemykani.png'))

    # ---- 4) hledani ----
    pg.evaluate('scrollTo(0,0)')
    pg.fill('#hledat', 'sipky')
    pg.wait_for_timeout(500)
    h = pg.evaluate("""() => ({info: document.getElementById('hledatInfo').innerText,
        marky: [...document.querySelectorAll('main mark')].map(m => m.textContent),
        videt: [...document.querySelectorAll('section.kap')].filter(s => !s.classList.contains('skryto')).map(s => s.id),
        obsah: getComputedStyle(document.getElementById('obsah')).display})""")
    over(h['marky'] and all(m.lower() == 'šipk' for m in h['marky']) and 'NALEZENO' in h['info'] and h['obsah'] == 'none',
         '4) hledani "sipky" (bez hacku) najde "šipky" i "šipkami" (koncovka se nehleda): %s, %s' % (h['info'], sorted(set(h['marky']))))
    over(set(h['videt']) >= {'menu', 'ps1', 'tv'} and 'dzubox' not in h['videt'] and 'soubory' not in h['videt'],
         '4) kapitoly bez shody se schovaji (videt: %s)' % ', '.join(h['videt']))
    pg.screenshot(path=os.path.join(PRAC, 'navod_4_hledani.png'))
    pg.fill('#hledat', 'BIOS')
    pg.wait_for_timeout(500)
    h2 = pg.evaluate("""() => ({info: document.getElementById('hledatInfo').innerText,
        otevrene: [...document.querySelectorAll('details[open]')].map(d => d.querySelector('summary').innerText),
        videt: [...document.querySelectorAll('section.kap')].filter(s => !s.classList.contains('skryto')).map(s => s.id)})""")
    over(any('PS1 hra' in o for o in h2['otevrene']) and 'ps1' in h2['videt'] and 'options' in h2['videt'],
         '4) hledani "BIOS" otevre shodnou otazku (%s) a ukaze PS1 i OPTIONS' % '; '.join(h2['otevrene']))
    pg.fill('#hledat', 'xyzqw')
    pg.wait_for_timeout(500)
    h3 = pg.evaluate("""() => ({info: document.getElementById('hledatInfo').innerText,
        videt: [...document.querySelectorAll('section.kap')].filter(s => !s.classList.contains('skryto')).length})""")
    over('NIC NENALEZENO' in h3['info'] and h3['videt'] == 0, '4) nesmysl: "%s", zadna kapitola' % h3['info'])
    pg.click('#hledatZrus')
    pg.wait_for_timeout(400)
    h4 = pg.evaluate("""() => ({skryto: document.querySelectorAll('.skryto').length, marky: document.querySelectorAll('mark').length,
        otevrene: document.querySelectorAll('details[open]').length, info: document.getElementById('hledatInfo').hidden,
        obsah: getComputedStyle(document.getElementById('obsah')).display, hodnota: document.getElementById('hledat').value})""")
    over(h4['skryto'] == 0 and h4['marky'] == 0 and h4['otevrene'] == 0 and h4['info'] and h4['obsah'] != 'none' and h4['hodnota'] == '',
         '4) smazani hledani vrati vse (zadne schovane kapitoly, znacky ani otevrene otazky)')
    # hledani s diakritikou v dotazu a v textu ("řádků")
    pg.fill('#hledat', 'řádků')
    pg.wait_for_timeout(500)
    h5 = pg.evaluate("() => [...document.querySelectorAll('main mark')].map(m => m.textContent)")
    over(len(h5) >= 7 and all(m.lower() == 'řádk' for m in h5), '4) hledani "řádků" (s diakritikou) najde %d mist (řádky, řádků, řádku ...)' % len(h5))
    # klepnuti na odkaz pri hledani (kapitola schovana) - hledani se zrusi a stranka dojede
    pg.fill('#hledat', 'Odemykání')
    pg.wait_for_timeout(500)
    pg.fill('#hledat', 'Ctrl')
    pg.wait_for_timeout(500)
    pg.click('#hledatZrus')
    pg.wait_for_timeout(300)

    # ---- 5) otazky se rozbaluji, rozvrzeni ----
    pg.evaluate("document.getElementById('otazky').scrollIntoView()")
    pg.click('#otazky details:nth-of-type(2) summary')
    pg.wait_for_timeout(300)
    otevr = pg.evaluate("document.querySelector('#otazky details:nth-of-type(2)').open")
    over(otevr, '5) casta otazka se klepnutim rozbali')
    pg.screenshot(path=os.path.join(PRAC, 'navod_5_otazky.png'))
    pg.goto('about:blank')
    ctx.close()

    # ---- 6) nejuzsi telefon 320 px, 360 px a na sirku: nic nepreteka ----
    for (w, hh, jm) in ((320, 640, 'navod_6b_320.png'), (360, 740, 'navod_6_360.png'), (915, 412, 'navod_7_sirka.png')):
        c2 = b.new_context(viewport={'width': w, 'height': hh}, device_scale_factor=2, is_mobile=True, has_touch=True)
        p2 = c2.new_page()
        p2.on('pageerror', lambda e: chybyJs.append(str(e)))
        p2.goto(URL)
        p2.wait_for_timeout(700)
        sirka = p2.evaluate("""() => { const sw = document.documentElement.scrollWidth, cw = document.documentElement.clientWidth;
            const siroke = [...document.querySelectorAll('main *')].filter(e => e.getBoundingClientRect().right > cw + 1).map(e => e.tagName + '.' + e.className).slice(0, 5);
            return [sw, cw, siroke]; }""")
        over(sirka[0] <= sirka[1] and not sirka[2], '6) %dx%d: nic nepreteka do strany (scrollWidth %d, sirka %d)%s' % (w, hh, sirka[0], sirka[1], ' ' + str(sirka[2]) if sirka[2] else ''))
        p2.evaluate("document.getElementById('atari').scrollIntoView()")
        p2.wait_for_timeout(300)
        p2.screenshot(path=os.path.join(PRAC, jm))
        c2.close()
    b.close()

over(not chybyJs, '7) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
over(not neslo, '7) zadny soubor, ktery stranka chce, nechybi' + (': ' + ', '.join(neslo) if neslo else ''))
if konzole:
    print('konzole:', konzole)
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b311_navod.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
