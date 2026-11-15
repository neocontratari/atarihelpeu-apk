# B326 - obrazovka volby jazyka pri prvnim spusteni (assets/hry/jazyk.html) v Chromiu.
# Rene: "vyber jazyka hned na zacatku pri spusteni aplikace ... jazyk prioritni by mela byt ang".
#
# Pouziti: python3 test_b326_jazyk_start.py   -> VYSTUP_test_b326_jazyk_start.txt
# Falesny most AHJAZYK (prvniVolba zapisuje do window.__volby) jako Java v MainActivity.AHJazyk.
import os, re, sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.abspath(os.path.join(ZDE, '..', '..'))
STRANKA = 'file://' + os.path.join(R, 'app', 'src', 'main', 'assets', 'hry', 'jazyk.html')
MOST = "window.__volby = []; window.AHJAZYK = { prvniVolba: function (k) { window.__volby.push(String(k)); } };"
vystup, chyb = [], 0


def kontrola(nazev, ok, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '  -> ' + str(detail)[:300]))


def otevri(b, w, h, most=True, telefon=True):
    ctx = b.new_context(viewport={'width': w, 'height': h}, device_scale_factor=1, has_touch=telefon, is_mobile=telefon and w < h)
    if most:
        ctx.add_init_script(MOST)
    s = ctx.new_page()
    chyby = []
    s.on('pageerror', lambda e: chyby.append(str(e)))
    s.on('requestfailed', lambda r: chyby.append('chybi ' + r.url))
    s.goto(STRANKA, wait_until='load')
    s.wait_for_timeout(250)
    return ctx, s, chyby


def rozlozeni(s):
    return s.evaluate('''() => {
        const r = id => { const b = document.getElementById(id).getBoundingClientRect(); return [b.left, b.top, b.right, b.bottom]; };
        const st = id => getComputedStyle(document.getElementById(id));
        return { sirka: document.documentElement.scrollWidth, vyska: document.documentElement.scrollHeight,
                 vw: innerWidth, vh: innerHeight, en: r('en'), cs: r('cs'), fokus: (document.activeElement || {}).id || '',
                 enText: document.getElementById('en').textContent, csText: document.getElementById('cs').textContent,
                 enPozadi: st('en').backgroundImage, csPozadi: st('cs').backgroundImage, csRamecek: st('cs').borderTopWidth,
                 skripty: [...document.scripts].map(x => x.src).filter(Boolean),
                 text: document.body.innerText };
    }''')


with sync_playwright() as p:
    b = p.chromium.launch()
    vystup.append('B326 - volba jazyka pri prvnim spusteni (hry/jazyk.html), Chromium, falesny most AHJAZYK')
    vystup.append('')

    # J1 telefon na vysku
    ctx, s, chyby = otevri(b, 412, 892)
    m = rozlozeni(s)
    kontrola('J1 telefon 412 px: ENGLISH nahore, CESTINA pod ni, obe cele na obrazovce bez posouvani',
             m['enText'] == 'ENGLISH' and m['csText'] == 'ČEŠTINA' and m['en'][3] <= m['cs'][1]
             and m['cs'][3] <= m['vh'] and m['en'][1] >= 0 and m['sirka'] <= m['vw'], m)
    kontrola('J1 ENGLISH je vybrana (fokus) a hlavni (zlate pozadi), CESTINA vedlejsi (jen ramecek)',
             m['fokus'] == 'en' and 'gradient' in m['enPozadi'] and m['csPozadi'] == 'none' and m['csRamecek'] == '1px', m)
    kontrola('J1 dvojjazycne: "Choose the app language" + "Vyber jazyk aplikace", rada OPTIONS -> JAZYK · LANGUAGE v obou',
             'Choose the app language' in m['text'] and 'Vyber jazyk aplikace' in m['text']
             and m['text'].count('OPTIONS → JAZYK · LANGUAGE') == 2, m['text'])
    kontrola('J1 stranka se neprekladata (bez nap_jazyk.js a slovniku)', not any('nap_jazyk' in x or '/jazyk/' in x for x in m['skripty']), m['skripty'])
    s.screenshot(path=os.path.join('/tmp', 'b326_jazyk_412.png'))
    s.click('#en')
    s.wait_for_timeout(150)
    s.click('#cs', force=True)
    s.click('#en', force=True)
    v = s.evaluate('window.__volby')
    dis = s.evaluate("[document.getElementById('en').disabled, document.getElementById('cs').disabled]")
    kontrola('J2 klepnuti ENGLISH -> AHJAZYK.prvniVolba("en") jednou; dalsi klepnuti uz nic (obe tlacitka zakazana)',
             v == ['en'] and dis == [True, True], (v, dis))
    kontrola('J2 zadna chyba JavaScriptu, zadny chybejici soubor (pismo)', chyby == [], chyby)
    ctx.close()

    # J3 cestina
    ctx, s, chyby = otevri(b, 412, 892)
    s.click('#cs')
    s.wait_for_timeout(150)
    kontrola('J3 klepnuti CESTINA -> prvniVolba("cs")', s.evaluate('window.__volby') == ['cs'] and chyby == [], (s.evaluate('window.__volby'), chyby))
    ctx.close()

    # J4 herni ovladac / dalkovy ovladac TV: sipky + Enter
    ctx, s, chyby = otevri(b, 1280, 720, telefon=False)
    m = rozlozeni(s)
    kontrola('J4 TV 1280x720: obe tlacitka videt, nic nepreteka', m['cs'][3] <= m['vh'] and m['sirka'] <= m['vw'], m)
    s.keyboard.press('ArrowDown')
    f1 = s.evaluate('document.activeElement.id')
    s.keyboard.press('ArrowUp')
    f2 = s.evaluate('document.activeElement.id')
    s.keyboard.press('ArrowRight')
    s.keyboard.press('Enter')
    s.wait_for_timeout(150)
    kontrola('J4 sipka dolu = CESTINA, nahoru = ENGLISH, doprava + Enter = vybrana CESTINA', f1 == 'cs' and f2 == 'en'
             and s.evaluate('window.__volby') == ['cs'], (f1, f2, s.evaluate('window.__volby')))
    ctx.close()
    ctx, s, chyby = otevri(b, 1280, 720, telefon=False)
    s.keyboard.press('Enter')
    s.wait_for_timeout(150)
    kontrola('J4 hned Enter (OK na ovladaci) = ENGLISH (vybrana)', s.evaluate('window.__volby') == ['en'], s.evaluate('window.__volby'))
    ctx.close()

    # J5 telefon na sirku
    ctx, s, chyby = otevri(b, 892, 412)
    m = rozlozeni(s)
    kontrola('J5 telefon na sirku 892x412: ENGLISH cela videt bez posouvani, do strany nic nepreteka',
             m['en'][3] <= m['vh'] and m['sirka'] <= m['vw'], m)
    ctx.close()

    # J6 bez mostu (prohlizec): nic nespadne, tlacitka se zase povoli
    ctx, s, chyby = otevri(b, 412, 892, most=False)
    s.click('#en')
    s.wait_for_timeout(1000)
    dis = s.evaluate("[document.getElementById('en').disabled, document.getElementById('cs').disabled]")
    kontrola('J6 bez mostu: zadna chyba JS, tlacitka se po chvili zase povoli', chyby == [] and dis == [False, False], (chyby, dis))
    ctx.close()
    b.close()

vystup.append('')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
open(os.path.join(ZDE, 'VYSTUP_test_b326_jazyk_start.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
