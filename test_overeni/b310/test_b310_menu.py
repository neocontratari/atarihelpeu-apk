#!/usr/bin/env python3
# B310: hlavni nabidka appky (assets/index.html) v Chromiu - PRESNE soubor z appky.
#  - ATARI 130XE EMULATOR vede na Atari 130XE v C++ (emu_atari_cpp/index.html),
#  - HELP (dole vpravo) i MENU (vlevo nahore) otevrou NAPOVEDU, nic se nenacita,
#  - OPTIONS - TV POKUS se toci jen OBOJI / JEN ZVUK / JEN OBRAZ (KRESLENIM bylo
#    jen pro smazane stare Atari),
#  - zamcena Sega rekne, kolik radku jeste napsat v Atari (AHINTRO.postupStav),
#  - stranka Atari 130XE (log a testy) se nacte bez chyby JavaScriptu a ma kroky B310,
#  - zadna chyba JavaScriptu.
# Most AHINTRO (Java) je napodoben a zapisuje, co stranka volala.
#   python3 test_b310_menu.py [PRAC]            -> VYSTUP_test_b310_menu.txt
import os
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b310'
os.makedirs(PRAC, exist_ok=True)
vystup = []
kontrol = chyb = 0


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    r = ('OK     ' if ok else 'CHYBA  ') + co
    print(r)
    vystup.append(r)


MOST = """
window.__volani = [];
window.AHINTRO = {
  postupStav: function(){ __volani.push('postupStav'); return JSON.stringify({sega:false, ps1:false, zbyvaMinut:30, zbyvaRadku:45}); },
  tvPokus: function(n){ __volani.push('tvPokus:'+n); return 'OK'; },
  znovu: function(){ __volani.push('znovu'); }
};
"""

with sync_playwright() as p:
    b = p.chromium.launch()
    ctx = b.new_context(viewport={'width': 412, 'height': 732}, device_scale_factor=2)
    ctx.add_init_script(MOST)
    pg = ctx.new_page()
    chybyJs = []
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    dialogy = []
    def na_dialog(d):
        dialogy.append(d.message)
        d.accept()
    pg.on('dialog', na_dialog)
    menu = 'file://' + os.path.join(A, 'index.html')
    pg.goto(menu)
    pg.wait_for_timeout(600)

    # 1) ATARI 130XE EMULATOR
    href = pg.eval_on_selector('#btnAtari', 'a => a.href')
    print('btnAtari ->', href)
    cil = href.replace('file://', '')
    over(href.endswith('/emu_atari_cpp/index.html') and os.path.exists(cil),
         'ATARI 130XE EMULATOR vede na Atari 130XE v C++ (emu_atari_cpp/index.html, soubor existuje)')
    over(not os.path.exists(os.path.join(A, 'emu_vbxe')), 'stare Atari v JavaScriptu (emu_vbxe) v appce neni')

    # 2) HELP = napoveda
    pg.click('#btnHelp')
    pg.wait_for_timeout(300)
    otevreno = pg.eval_on_selector('#helpPanel', 'p => p.classList.contains("open")')
    text = pg.inner_text('#helpPanel')
    over(otevreno and pg.url == menu, 'HELP otevre napovedu a nikam se neodchazi (adresa stejna)')
    over('NÁPOVĚDA' in text and 'ATARI 130XE EMULATOR' in text and 'NET HRY' in text and 'LOG/CHYBA' in text,
         'napoveda popisuje Atari 130XE a tlacitka na pristroji')
    over('50 řádků programu' in text and 'RUN nebo LIST' in text and '30 minutách' in text,
         'napoveda rika, jak se otevre Sega (50 radku, RUN / LIST) a PS1 (30 min Segy)')
    pg.screenshot(path=os.path.join(PRAC, 'menu_napoveda.png'))
    # cela napoveda jde precist (panel roluje, tlacitko ZAVRIT je dosazitelne)
    pg.eval_on_selector('#helpPanel', 'p => p.scrollTop = p.scrollHeight')
    pg.wait_for_timeout(150)
    pg.screenshot(path=os.path.join(PRAC, 'menu_napoveda_konec.png'))
    pg.click('#helpClose')
    pg.wait_for_timeout(200)
    over(not pg.eval_on_selector('#helpPanel', 'p => p.classList.contains("open")'), 'ZAVRIT NAPOVEDU ji zavre')

    # 3) MENU vlevo nahore taky napoveda
    pg.click('#btnMenu')
    pg.wait_for_timeout(200)
    over(pg.eval_on_selector('#helpPanel', 'p => p.classList.contains("open")'), 'MENU (vlevo nahore) otevre napovedu')
    over(pg.eval_on_selector('#helpPanel', 'p => p.scrollTop') == 0, 'napoveda se otevre od zacatku')
    pg.click('#helpClose')
    pg.wait_for_timeout(200)

    # 4) TV POKUS bez KRESLENIM
    pg.click('#btnSettings')
    pg.wait_for_timeout(200)
    texty = []
    for i in range(4):
        pg.click('#stTvPokus')
        pg.wait_for_timeout(100)
        texty.append(pg.inner_text('#stTvPokus'))
    vol = pg.evaluate('__volani.filter(function(x){return x.indexOf("tvPokus")===0})')
    print('TV POKUS:', texty, vol)
    over(texty == ['TV POKUS: JEN ZVUK', 'TV POKUS: JEN OBRAZ', 'TV POKUS: OBOJI', 'TV POKUS: JEN ZVUK']
         and vol == ['tvPokus:1', 'tvPokus:2', 'tvPokus:0', 'tvPokus:1'],
         'TV POKUS se toci OBOJI / JEN ZVUK / JEN OBRAZ (bez KRESLENIM ze stareho Atari)')
    pg.mouse.click(5, 700)            # zavrit OPTIONS klikem vedle
    pg.wait_for_timeout(200)

    # 5) zamcena Sega
    pg.click('#btnSega')
    pg.wait_for_timeout(300)
    print('dialogy:', dialogy)
    over(any('NAPIS JESTE 45 RADKU KODU V ATARI' in d for d in dialogy) and pg.url == menu,
         'zamcena Sega: ZAMCENO - NAPIS JESTE 45 RADKU KODU V ATARI (stav z Javy), nikam se nejde')
    over(not chybyJs, 'hlavni nabidka bez chyby JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))

    # 6) stranka Atari 130XE (log a testy pod pristrojem)
    chybyJs.clear()
    pg.click('#btnAtari')
    pg.wait_for_timeout(800)
    over(pg.url.endswith('/emu_atari_cpp/index.html') and pg.title() == 'ATARI 130XE — log a testy',
         'klik na ATARI 130XE EMULATOR otevre stranku Atari 130XE (v appce nad ni C++ kresli pristroj)')
    kroky = pg.eval_on_selector_all('#kroky .krok', 'k => k.length')
    t = pg.inner_text('body')
    over(kroky == 8 and 'B310: RADIKALNI REZ' in t and 'ODEMCENI SEGY' in t, 'stranka ma 8 kroku testu B310 a poznamku B310')
    over(not chybyJs, 'stranka Atari 130XE bez chyby JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
    b.close()

vystup.append('')
vystup.append('VYSLEDEK: %d kontrol, %d chyb' % (kontrol, chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b310_menu.txt'), 'w', encoding='utf-8') as f:
    f.write('B310 - hlavni nabidka appky (assets/index.html) v Chromiu, most AHINTRO napodoben\n\n')
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
