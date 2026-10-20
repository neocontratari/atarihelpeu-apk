#!/usr/bin/env python3
# B311: HLAVNI MENU - stranka assets/index.html v Chromiu (PRESNE soubor z appky).
# Na telefonu menu kresli C++ (napmenu) NAD touto strankou; stranka pod nim drzi OPTIONS
# a zalohu - karusel v HTML, kdyby C++ menu neslo. Most AHINTRO / AHTVWEB (Java) je
# napodoben a zapisuje, co stranka volala.
#  A) ZALOHA (C++ menu nejde): karty z menu/karty.txt (XHR jako ve WebView), poradi, obrazky,
#     pismo z appky, zamky podle postupu (Sega 45 radku), sipky, klepnuti na zamcenou kartu,
#     tecky, HELP -> napoveda/index.html, ATARI -> emu_atari_cpp/index.html
#  B) C++ MENU (AHINTRO.menuBude): karusel v HTML schovany hned (zadne probliknuti),
#     karty z AHINTRO.menuKarty, kolecko C++ -> napOtevriNastaveni, ZAVRIT -> AHINTRO.menuZpet,
#     napMenuNativni(false) -> karusel v HTML zpet
#  C) OPTIONS - SPUSTIT WEB TV: adresa (jen http://...:port/, ne cela odpoved Javy) zustane
#     v OPTIONS, tlacitko WEB TV BEZI; pri dalsim otevreni OPTIONS je adresa zase videt;
#     kdyz WEB TV nejde -> zprava, zadna adresa
#  D) zadna chyba JavaScriptu (menu, navod, stranka Atari 130XE s kroky testu B311)
# WebView v appce ma setAllowUniversalAccessFromFileURLs -> Chromium s --allow-file-access-from-files.
#   python3 test_b311_menu.py [PRAC]            -> VYSTUP_test_b311_menu.txt
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


def most(nativni, karty_js='null', tv='ok'):
    return """
window.__volani = [];
window.AHINTRO = {
  postupStav: function(){ __volani.push('postupStav'); return JSON.stringify({sega:false, ps1:false, zbyvaMinut:30, zbyvaRadku:45}); },
  menuBude: function(){ __volani.push('menuBude'); return %s; },
  menuKarty: function(){ __volani.push('menuKarty'); return %s; },
  menuZpet: function(){ __volani.push('menuZpet'); },
  znovu: function(){ __volani.push('znovu'); },
  tvPokus: function(n){ __volani.push('tvPokus:'+n); return 'OK'; }
};
window.__tvBezi = false;
window.AHTVWEB = {
  start: function(){ __volani.push('tvStart');
    if ('%s' !== 'ok') return 'TV_WEB_CAST_FAIL java.net.BindException';
    window.__tvBezi = true; return 'TV_WEB_CAST_OK http://192.168.1.23:8765/ APP'; },
  status: function(){ return window.__tvBezi ? 'http://192.168.1.23:8765/' : 'TV_WEB_CAST_OFF'; },
  getQualityTier: function(){ return 2; },
  setQualityTier: function(t){ return t; }
};
""" % ('true' if nativni else 'false', karty_js, tv)


MENU = 'file://' + os.path.join(A, 'index.html')
POPIS_ATARI = 'Atari 130XE s grafikou VBXE'

with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files'])
    chybyJs = []

    def nova(nativni, karty_js='null', tv='ok', w=412, h=915):
        ctx = b.new_context(viewport={'width': w, 'height': h}, device_scale_factor=2, is_mobile=True, has_touch=True)
        ctx.add_init_script(most(nativni, karty_js, tv))
        pg = ctx.new_page()
        pg.on('pageerror', lambda e, pg=pg: chybyJs.append((pg.url, str(e))))
        return ctx, pg

    def vybrana(pg):
        return pg.evaluate("""() => { const t = [...document.querySelectorAll('#tecky i')]; return t.findIndex(x => x.className === 'on'); }""")

    # ================= A) ZALOHA - karusel v HTML =================
    ctx, pg = nova(False)
    pg.goto(MENU)
    pg.wait_for_timeout(900)
    k = pg.evaluate("""() => [...document.querySelectorAll('#karusel .karta')].map(d => ({
        nazev: d.querySelector('.stitek b').textContent, pod: d.querySelector('.stitek i').textContent,
        zamcena: d.classList.contains('zamcena'), obr: d.querySelector('img').getAttribute('src')}))""")
    nazvy = [x['nazev'] for x in k]
    over(nazvy == ['ATARI 130XE', 'SEGA MEGA DRIVE', 'PS1', 'MP3 PŘEHRÁVAČ', 'DŽUBOX', 'ATARIHELP.EU', 'HELP'],
         'A1) zaloha: 7 karet z menu/karty.txt ve spravnem poradi: ' + ' | '.join(nazvy))
    over(pg.evaluate("getComputedStyle(document.getElementById('menu')).display") != 'none' and
         'nativni' not in pg.evaluate('document.documentElement.className'), 'A1) bez C++ menu je karusel v HTML videt')
    zam = [x['nazev'] for x in k if x['zamcena']]
    over(zam == ['SEGA MEGA DRIVE', 'PS1'], 'A2) zamcene podle postupu hrace: ' + ', '.join(zam))
    obr = pg.evaluate("""async () => { const im = [...document.querySelectorAll('#karusel img')]; im.forEach(i => i.loading = 'eager');
        await new Promise(r => setTimeout(r, 500)); await Promise.all(im.map(i => i.decode().catch(() => null)));
        return im.map(i => [i.getAttribute('src'), i.naturalWidth]); }""")
    over(len(obr) == 7 and all(w > 0 for _, w in obr), 'A3) vsech 7 obrazku karet se nacte (' + ', '.join(s for s, _ in obr) + ')')
    pismo = pg.evaluate("""async () => { await document.fonts.ready; return [document.fonts.check('800 20px "Chakra Petch"'),
        [...document.fonts].filter(f => f.status === 'loaded').map(f => f.family + ' ' + f.weight)]; }""")
    over(pismo[0] and len(pismo[1]) >= 1, 'A4) pismo Chakra Petch z appky (pisma/*.woff2): ' + ', '.join(pismo[1]))
    st = pg.evaluate("() => [document.getElementById('popis').textContent, document.getElementById('spustit').textContent]")
    over(vybrana(pg) == 0 and st[0].startswith(POPIS_ATARI) and 'SPUSTIT' in st[1], 'A5) na zacatku ATARI 130XE: "%s" / %s' % (st[0][:40], st[1]))
    pg.screenshot(path=os.path.join(PRAC, 'menu_zaloha_1.png'))
    pg.keyboard.press('ArrowRight')
    pg.wait_for_timeout(900)
    st = pg.evaluate("() => [document.getElementById('popis').textContent, document.getElementById('spustit').textContent]")
    over(vybrana(pg) == 1 and st[0].startswith('ZAMČENO · Napiš v Atari ještě 45 řádků programu') and 'ZAMČENO' in st[1],
         'A6) sipka vpravo -> SEGA zamcena: "%s" / %s' % (st[0][:70], st[1]))
    pg.screenshot(path=os.path.join(PRAC, 'menu_zaloha_2_sega.png'))
    url0 = pg.url
    pg.click('#spustit')
    pg.wait_for_timeout(400)
    t = pg.evaluate("() => [document.getElementById('toast').classList.contains('show'), document.getElementById('toast').innerText]")
    over(t[0] and t[1].startswith('ZAMČENO') and '45 řádků' in t[1] and pg.url == url0, 'A7) klepnuti na zamcenou SEGU: nic se nespusti, zprava "%s"' % t[1].replace('\n', ' / ')[:80])
    # tecka HELP (posledni) -> spustit -> navod
    pg.click('#tecky i:nth-child(7)')
    pg.wait_for_timeout(1100)
    st = pg.evaluate("() => [document.getElementById('popis').textContent, document.getElementById('spustit').textContent]")
    over(vybrana(pg) == 6 and 'OTEVŘÍT NÁVOD' in st[1], 'A8) tecka HELP -> karta HELP (%s)' % st[1])
    pg.click('#spustit')
    pg.wait_for_url('**/napoveda/index.html', timeout=5000)
    pg.wait_for_timeout(500)
    over(pg.url.endswith('/napoveda/index.html') and pg.title().startswith('N&P VISION') and pg.locator('#obsah a').count() == 13,
         'A9) HELP otevre uplny navod (napoveda/index.html, 13 kapitol)')
    pg.go_back()
    pg.wait_for_timeout(900)
    # klepnuti na kartu vedle = dojet na ni, klepnuti na prostredni = spustit
    pg.click('#tecky i:nth-child(1)')
    pg.wait_for_timeout(1100)
    pg.click('#karusel .karta:nth-child(1)')
    pg.wait_for_timeout(1200)
    over(pg.url.endswith('/emu_atari_cpp/index.html'), 'A10) klepnuti na prostredni kartu ATARI 130XE -> emu_atari_cpp/index.html')
    t = pg.inner_text('body')
    over(pg.title() == 'ATARI 130XE — log a testy' and pg.locator('#kroky .krok').count() == 10 and 'B311: HLAVNI MENU' in t and 'ZPET na telefonu primo v hlavnim menu' in t,
         'A11) stranka Atari 130XE (log a testy pod pristrojem): 10 kroku testu B311 pro Reneho a poznamka B311')
    ctx.close()

    # ================= B) C++ MENU nad strankou =================
    vlastni = "'# test\\nx1 | PRVNI | P1 | Popis jedna. | menu/karta_atari.jpg | emu_atari_cpp/index.html | #F2B640 | | fx:x | JEDNA\\nx2 | DRUHA | P2 | Popis dva. | menu/karta_sega.jpg | napoveda/index.html | #3FA9FF | | | DVA\\n'"
    ctx, pg = nova(True, vlastni)
    pg.goto(MENU)
    pg.wait_for_timeout(700)
    st = pg.evaluate("""() => ({cls: document.documentElement.className, menu: getComputedStyle(document.getElementById('menu')).display,
        karet: document.querySelectorAll('#karusel .karta').length, nazvy: [...document.querySelectorAll('#karusel .stitek b')].map(x => x.textContent),
        volani: window.__volani.slice()})""")
    over('nativni' in st['cls'] and st['menu'] == 'none', 'B1) C++ menu: karusel v HTML schovany hned od zacatku (class nativni, #menu display none)')
    over(st['nazvy'] == ['PRVNI', 'DRUHA'] and 'menuKarty' in st['volani'], 'B2) karty z AHINTRO.menuKarty (Java cte assets/menu/karty.txt): ' + ', '.join(st['nazvy']))
    pg.keyboard.press('ArrowRight')
    pg.wait_for_timeout(300)
    over(vybrana(pg) == 0, 'B3) pod C++ menu stranka sipky nebere (listuje C++)')
    pg.evaluate('window.napOtevriNastaveni()')
    pg.wait_for_timeout(200)
    otevr = pg.evaluate("document.getElementById('settingsPanel').classList.contains('open')")
    over(otevr, 'B4) kolecko v C++ menu -> napOtevriNastaveni otevre OPTIONS')
    pg.screenshot(path=os.path.join(PRAC, 'menu_options.png'))
    pg.click('#stZavrit')
    pg.wait_for_timeout(300)
    st = pg.evaluate("() => [document.getElementById('settingsPanel').classList.contains('open'), window.__volani.filter(v => v === 'menuZpet').length]")
    over(not st[0] and st[1] == 1, 'B5) ZAVRIT zavre OPTIONS a vrati C++ menu (AHINTRO.menuZpet 1x)')
    pg.evaluate('window.napZavriNastaveni()')
    pg.wait_for_timeout(200)
    st = pg.evaluate("() => window.__volani.filter(v => v === 'menuZpet').length")
    over(st == 1, 'B6) napZavriNastaveni nad zavrenym OPTIONS uz menuZpet nevola (zadne dvojite vraceni)')
    pg.evaluate('window.napMenuNativni(false)')
    pg.wait_for_timeout(500)
    st = pg.evaluate("() => [document.documentElement.className, getComputedStyle(document.getElementById('menu')).display]")
    over('nativni' not in st[0] and st[1] != 'none', 'B7) C++ menu nejde (napMenuNativni(false)) -> karusel v HTML zpet')
    ctx.close()

    # ================= C) OPTIONS - WEB TV =================
    ctx, pg = nova(True)
    pg.goto(MENU)
    pg.wait_for_timeout(600)
    pg.evaluate('window.napOtevriNastaveni()')
    pg.wait_for_timeout(200)
    st = pg.evaluate("() => [getComputedStyle(document.getElementById('stTvAdresa')).display, document.getElementById('stStartTv').textContent]")
    over(st[0] == 'none' and st[1] == 'SPUSTIT WEB TV', 'C1) WEB TV nebezi: adresa schovana, tlacitko SPUSTIT WEB TV')
    pg.click('#stStartTv')
    pg.wait_for_timeout(300)
    st = pg.evaluate("""() => ({otevreno: document.getElementById('settingsPanel').classList.contains('open'),
        adr: document.querySelector('#stTvAdresa b').textContent, videt: getComputedStyle(document.getElementById('stTvAdresa')).display,
        tl: document.getElementById('stStartTv').textContent, toast: document.getElementById('toast').classList.contains('show'),
        zpet: window.__volani.filter(v => v === 'menuZpet').length})""")
    over(st['otevreno'] and st['videt'] == 'block' and st['adr'] == 'http://192.168.1.23:8765/' and st['tl'] == 'WEB TV BĚŽÍ' and st['zpet'] == 0,
         'C2) SPUSTIT WEB TV: adresa "%s" zustane v OPTIONS (ne "TV_WEB_CAST_OK ... APP"), tlacitko %s, menu se nevraci' % (st['adr'], st['tl']))
    pg.screenshot(path=os.path.join(PRAC, 'menu_options_webtv.png'))
    pg.click('#stZavrit')
    pg.wait_for_timeout(200)
    pg.evaluate('window.napOtevriNastaveni()')
    pg.wait_for_timeout(200)
    st = pg.evaluate("() => [getComputedStyle(document.getElementById('stTvAdresa')).display, document.querySelector('#stTvAdresa b').textContent]")
    over(st[0] == 'block' and st[1] == 'http://192.168.1.23:8765/', 'C3) dalsi otevreni OPTIONS: adresa bezici WEB TV je zase videt (AHTVWEB.status)')
    ctx.close()
    ctx, pg = nova(True, tv='fail')
    pg.goto(MENU)
    pg.wait_for_timeout(500)
    pg.evaluate('window.napOtevriNastaveni()')
    pg.click('#stStartTv')
    pg.wait_for_timeout(300)
    st = pg.evaluate("""() => [getComputedStyle(document.getElementById('stTvAdresa')).display, document.getElementById('toast').innerText,
        document.getElementById('stStartTv').textContent]""")
    over(st[0] == 'none' and st[1].startswith('WEB TV nejde spustit') and st[2] == 'SPUSTIT WEB TV',
         'C4) WEB TV nejde: zadna adresa, zprava "%s"' % st[1].replace('\n', ' / '))
    ctx.close()
    b.close()

over(not chybyJs, 'D) zadna chyba JavaScriptu (menu, navod, stranka Atari 130XE)' + (': ' + '; '.join('%s %s' % e for e in chybyJs) if chybyJs else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b311_menu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
