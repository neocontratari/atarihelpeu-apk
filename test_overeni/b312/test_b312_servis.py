#!/usr/bin/env python3
# B312: SERVIS v OPTIONS (assets/index.html v Chromiu) - Reneho ODEMKNOUT DALSI / ZAMKNOUT VSE
# uz nejsou videt; objevi se az po servisnim kodu. Java (AHINTRO) je napodobena jako NapServis:
# zkusebni kod TESTKOD234 (NE skutecny servisni kod - ten v repozitari neni), 3 spatne pokusy =
# minuta pauza, servis 10 minut, bez servisu odemkniDalsi / zamkniVse vrati ZAMCENO.
# Skutecne overeni kodu (PBKDF2) testuje TestB312Java v JVM.
#   python3 test_b312_servis.py [PRAC]      -> VYSTUP_test_b312_servis.txt
import os
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b312/servis'
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


MOST = """
window.__volani = [];
window.__servis = {do: 0, spatnych: 0, pauzaDo: 0, odemceno: 0, kody: []};
window.AHINTRO = {
  postupStav: function(){ return JSON.stringify({sega: __servis.odemceno > 0, ps1: __servis.odemceno > 1, zbyvaMinut: 30, zbyvaRadku: 45}); },
  menuBude: function(){ return true; },
  menuKarty: function(){ return null; },
  menuZpet: function(){ __volani.push('menuZpet'); },
  servisKod: function(kod){
    __servis.kody.push(kod); __volani.push('servisKod');
    var ted = Date.now();
    if (ted < __servis.pauzaDo) return 'POCKEJ:' + Math.ceil((__servis.pauzaDo - ted) / 1000);
    if (String(kod).toUpperCase().replace(/[ -]/g, '') === 'TESTKOD234') { __servis.spatnych = 0; __servis.do = ted + 600000; return 'OK'; }
    if (++__servis.spatnych >= 3) { __servis.spatnych = 0; __servis.pauzaDo = ted + 60000; return 'POCKEJ:60'; }
    return 'SPATNE'; },
  servisZbyva: function(){ var z = __servis.do - Date.now(); return z > 0 ? Math.ceil(z / 1000) : 0; },
  odemkniDalsi: function(){ __volani.push('odemkniDalsi');
    if (Date.now() >= __servis.do) return 'ZAMCENO';
    __servis.odemceno++; return __servis.odemceno === 1 ? 'SEGA' : __servis.odemceno === 2 ? 'PS1' : 'UZ JE VSE OTEVRENE'; },
  zamkniVse: function(){ __volani.push('zamkniVse');
    if (Date.now() >= __servis.do) return 'ZAMCENO';
    __servis.odemceno = 0; return 'OK'; }
};
window.AHTVWEB = { status: function(){ return 'TV_WEB_CAST_OFF'; }, getQualityTier: function(){ return 1; } };
"""
URL = 'file://' + os.path.join(A, 'index.html')
STAV = """() => { const v = id => { const e = document.getElementById(id); return !!(e && e.offsetParent !== null); };
  return {otevreno: document.getElementById('settingsPanel').classList.contains('open'),
    pole: v('stServisKod'), servis: v('stServis'), odemkni: v('stOdemkni'), zamkni: v('stZamkni'),
    hlaska: document.getElementById('stKodHlaska').textContent, hint: document.getElementById('stServisHint').textContent,
    toast: document.getElementById('toast').classList.contains('show') ? document.getElementById('toast').textContent : '',
    kod: document.getElementById('stKod').value, fokus: document.activeElement && document.activeElement.id,
    volani: __volani.slice(), viditelnyText: document.querySelector('#settingsPanel .list').innerText}; }"""

chybyJs = []
konzole = []

with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files'])
    ctx = b.new_context(viewport={'width': 360, 'height': 780}, device_scale_factor=2)
    ctx.add_init_script(MOST)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.on('console', lambda m: konzole.append(m.text))
    pg.goto(URL)
    pg.wait_for_timeout(600)

    def otevri():
        pg.evaluate('() => napOtevriNastaveni()')
        pg.wait_for_timeout(150)

    def titul(kolikrat, mezera=80):
        for _ in range(kolikrat):
            pg.click('#stTitul')
            pg.wait_for_timeout(mezera)

    otevri()
    s = pg.evaluate(STAV)
    over(s['otevreno'] and not s['odemkni'] and not s['zamkni'] and not s['pole'] and not s['servis']
         and 'ODEMKNOUT' not in s['viditelnyText'] and 'ZAMKNOUT' not in s['viditelnyText'] and 'SERVIS' not in s['viditelnyText'],
         '1) OPTIONS: ODEMKNOUT DALSI ani ZAMKNOUT VSE nejsou videt (ani slovo SERVIS)')
    pg.screenshot(path=os.path.join(PRAC, 'options_bez_servisu.png'))
    titul(4)
    s4 = pg.evaluate(STAV)
    pg.wait_for_timeout(3100)
    titul(5, mezera=800)
    s5p = pg.evaluate(STAV)
    over(not s4['pole'] and not s5p['pole'], '1) 4 klepnuti na nadpis nic, 5 pomalych (pres 3 s) taky nic')
    pg.wait_for_timeout(3100)
    titul(5)
    s = pg.evaluate(STAV)
    over(s['pole'] and s['fokus'] == 'stKod' and not s['servis'], '2) 5 rychlych klepnuti na nadpis OPTIONS: pole pro servisni kod (kurzor v nem)')
    box_p = pg.locator('#stKod').bounding_box()
    box_l = pg.locator('#settingsPanel .list').bounding_box()
    over(box_p['x'] >= box_l['x'] and box_p['x'] + box_p['width'] <= box_l['x'] + box_l['width'] + 0.5 and box_p['height'] >= 40,
         '2) pole se vejde do OPTIONS i na sirku 360 px (%dx%d)' % (box_p['width'], box_p['height']))
    pg.fill('#stKod', 'SPATNY1')
    pg.click('#stKodOk')
    pg.wait_for_timeout(100)
    s = pg.evaluate(STAV)
    over(s['hlaska'] == 'Špatný kód.' and s['kod'] == '' and not s['servis'], '3) spatny kod: "%s", pole se vymaze, servis zavreny' % s['hlaska'])
    pg.fill('#stKod', 'SPATNY2')
    pg.press('#stKod', 'Enter')
    pg.wait_for_timeout(100)
    pg.fill('#stKod', 'SPATNY3')
    pg.click('#stKodOk')
    pg.wait_for_timeout(100)
    s = pg.evaluate(STAV)
    over(s['hlaska'] == 'Moc pokusů – počkej 60 s.' and s['volani'].count('servisKod') == 3,
         '3) Enter taky potvrdi; 3. spatny pokus: "%s"' % s['hlaska'])
    pg.fill('#stKod', 'testkod-234')
    pg.click('#stKodOk')
    pg.wait_for_timeout(100)
    s = pg.evaluate(STAV)
    over(s['hlaska'].startswith('Moc pokusů – počkej') and not s['servis'], '3) behem pauzy nepomuze ani spravny kod: "%s"' % s['hlaska'])
    pg.evaluate('() => { __servis.pauzaDo = 0; }')     # minuta uplynula
    pg.fill('#stKod', 'testkod-234')
    pg.click('#stKodOk')
    pg.wait_for_timeout(150)
    s = pg.evaluate(STAV)
    over(s['servis'] and s['odemkni'] and s['zamkni'] and not s['pole'] and s['toast'] == 'SERVIS OTEVŘEN NA 10 MINUT'
         and s['hint'] == 'Servis je otevřený ještě 10 min.',
         '4) spravny kod (i malymi pismeny s pomlckou): SERVIS s ODEMKNOUT DALSI a ZAMKNOUT VSE, "%s", "%s"' % (s['toast'], s['hint']))
    pg.screenshot(path=os.path.join(PRAC, 'options_servis.png'))
    pg.click('#stOdemkni')
    pg.wait_for_timeout(150)
    s = pg.evaluate(STAV)
    over(s['toast'] == 'ODEMČENO: SEGA' and not s['otevreno'], '4) ODEMKNOUT DALSI: "%s", OPTIONS se zavre' % s['toast'])
    pg.wait_for_timeout(2600)
    over('menuZpet' in pg.evaluate(STAV)['volani'], '4) az zprava zhasne, vrati se C++ menu (AHINTRO.menuZpet)')
    otevri()
    s = pg.evaluate(STAV)
    over(s['servis'] and not s['pole'], '5) OPTIONS znovu: servis je porad otevreny (10 minut) - tlacitka rovnou videt')
    pg.click('#stOdemkni')
    pg.wait_for_timeout(150)
    t2 = pg.evaluate(STAV)['toast']
    pg.wait_for_timeout(2600)
    otevri()
    pg.click('#stZamkni')
    pg.wait_for_timeout(150)
    t3 = pg.evaluate(STAV)['toast']
    over(t2 == 'ODEMČENO: PS1' and t3 == 'ZAMČENO VŠE – pohled nového hráče', '5) dalsi ODEMKNOUT: "%s"; ZAMKNOUT VSE: "%s"' % (t2, t3))
    pg.wait_for_timeout(2600)
    pg.evaluate('() => { __servis.do = Date.now() - 1; }')     # 10 minut uplynulo
    otevri()
    s = pg.evaluate(STAV)
    over(not s['servis'] and not s['odemkni'], '6) po 10 minutach: OPTIONS zase bez servisnich tlacitek')
    # servis vyprsel, kdyz byla tlacitka otevrena: Java odmitne, stranka je schova
    pg.evaluate('() => { __servis.do = Date.now() + 600000; }')
    pg.evaluate('() => napZavriNastaveni()')
    pg.wait_for_timeout(2700)
    otevri()
    pg.evaluate('() => { __servis.do = Date.now() - 1; }')
    pg.click('#stOdemkni')
    pg.wait_for_timeout(150)
    s = pg.evaluate(STAV)
    over(s['toast'] == 'SERVIS VYPRŠEL – zadej kód znovu' and not s['servis'] and s['otevreno'],
         '6) servis vyprsel s otevrenym OPTIONS: Java odmitne (ZAMCENO), "%s", tlacitka zmizi' % s['toast'])
    over(not any('TESTKOD234' in k.upper().replace('-', '') or 'SPATNY' in k for k in konzole),
         '7) napsany kod neni v logu stranky (%d radku konzole)' % len(konzole))
    ctx.close()

    # ---- 9) stranka Atari 130XE (log a testy pod pristrojem): kroky testu B312 pro Reneho ----
    ctx = b.new_context(viewport={'width': 412, 'height': 915})
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto('file://' + os.path.join(A, 'emu_atari_cpp/index.html'))
    pg.wait_for_timeout(500)
    t = pg.inner_text('body')
    kroky = pg.evaluate("() => [...document.querySelectorAll('#kroky .krok')].map(k => k.innerText)")
    vse = ' '.join(kroky)
    over(pg.title() == 'ATARI 130XE — log a testy' and len(kroky) == 8 and 'B312: MP3 PREHRAVAC - RECORD' in t
         and 'B311: HLAVNI MENU' in t and 'RECORD' in kroky[0] and 'FAVORITES' in kroky[1] and 'SERVIS' in kroky[3]
         and 'ZAMKY Z WEBU' in kroky[4],
         '9) stranka Atari 130XE: 8 kroku testu B312 pro Reneho (RECORD, FAVORITES, ZPET, SERVIS, zamky z webu, DZUBOX, VOLUME, regrese) a poznamka B312')
    over('5x' not in vse and 'nadpis OPTIONS' not in vse and 'klepni na nadpis' not in vse.lower() and 'TESTKOD' not in t,
         '9) kroky ani poznamka neprozrazuji, jak se servis otevira (jen "jak, vis z me zpravy")')
    ctx.close()
    b.close()

over(not chybyJs, '8) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b312_servis.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
