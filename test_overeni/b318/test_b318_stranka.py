#!/usr/bin/env python3
"""B318: STRANKA PS1 - JEMNEJSI MYS (Chromium, stranka presne z appky: assets/emu_ps1/index.html).

Rene (B317): "mys sice funguje ale je dost neomalena i pri nastaveni citlivosti na 300 je to
divny". Dotyky jsou SKUTECNE dotykove udalosti Chromia (CDP Input.dispatchTouchEvent - jako prst
na displeji, i vic prstu naraz). Most Javy (AHPS1) je falesny: co stranka posle, se zapise i s casem
(performance.now()). Zaznam dotyku pak prehraje prehraj_dotyky (SKUTECNA logika mysi B318 z appky,
nap_ps1_mys.h) - co by z toho videla hra: posun kurzoru, kliky.

Kontroly:
  1) gesta pres skutecne dotyky -> logika mysi B318: tah (bez skoku, za lanem 1,5 %), stejny tah se
     zrychlenim dal, tuknuti, dva prsty, podrzeni 0,8 s + tah, prst na KRIZKU a druhy na obraze
  2) panel MYS PLAYSTATION: tlacitko ZRYCHLENI (stav z C++ - bit5), prepnuti -> AHPS1.ps1MysZrychleni,
     hlaska; zvyraznena volba portu se s tlacitkem zrychleni neplete; odstavec o mysi pocitace
  3) zbytek jako B317 (tlacitko MYS, schovana tlacitka, drzeni + vibrace, zruseni dotyku)
  4) cesky = stranka B317 + jen texty B318, anglicky bez cestiny, zadna chyba JavaScriptu
Pouziti: python3 test_b318_stranka.py PRAC     -> VYSTUP_test_b318_stranka.txt, VYSTUP_texty_ps1_mys_en.txt
"""
import os
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402
from stranky_b314 import STRANKY               # noqa: E402

B317 = os.environ.get('NAP_B317', '2f8fff4')
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


# falesny most: rezim / citlivost / zrychleni si pamatuje stranka (jako C++), dotyky se zapisuji s casem
STAV = "((window.__mys||[0,100])[0]>0?1:0)|(window.__mysDrzi?2:0)|(window.__mysZr===false?0:32)"
KLIC = "(window.__mysKlic!==undefined?window.__mysKlic:'SLUS00973')"
MOCK = {
    'AHPS1.ps1MysNastav': "fn: window.__mys=[args[0]|0,args[1]|0]; (window.__nastav=window.__nastav||[]).push(args[0]+','+args[1]);"
                          " return window.__mys[0]+'|'+window.__mys[1]+'|'+(" + STAV + ")+'|'+" + KLIC + ";",
    'AHPS1.ps1MysStav': "fn: var m=window.__mys||[0,100]; return m[0]+'|'+m[1]+'|'+(" + STAV + ")+'|'+" + KLIC + ";",
    'AHPS1.ps1MysZrychleni': "fn: window.__mysZr=!!(args[0]|0); (window.__zr=window.__zr||[]).push(args[0]|0);"
                             " var m=window.__mys||[0,100]; return m[0]+'|'+m[1]+'|'+(" + STAV + ")+'|'+" + KLIC + ";",
    'AHPS1.ps1MysDotyk': "fn: (window.__dotyky=window.__dotyky||[]).push([args[0],args[1],args[2],args[3],args[4],args[5],performance.now()]);"
                         " var m=window.__mys||[0]; return (m[0]>0?1:0)|(window.__mysZr===false?0:32)|(window.__mysVratit&&args[0]==1?window.__mysVratit:0);",
    'AHPS1.ps1Vibrate': "fn: (window.__vibrace=window.__vibrace||[]).push(args[0]); return '';",
    'AHPS1.ps1Input': "fn: (window.__vstup=window.__vstup||[]).push(args[0]+(args[1]?'+':'-')); return '';",
}
NA_SIRKU = {'width': 846, 'height': 412}


def prehraj(prac, rezim, citlivost, zrychleni, dotyky):
    """zaznam dotyku -> skutecna logika mysi B318 (prehraj_dotyky) -> co uvidi hra"""
    vstup = '%d %d %d\n' % (rezim, citlivost, zrychleni) + ''.join('%d %d %.3f %.3f %.3f %.3f %.3f\n' % tuple(d) for d in dotyky)
    r = subprocess.run([os.path.join(prac, 'prehraj_dotyky_b318')], input=vstup, capture_output=True, text=True, check=True)
    sx, sy, kL, kP, sL, sP, vrL, vrP, mk = (int(v) for v in r.stdout.split())
    return dict(sx=sx, sy=sy, kL=kL, kP=kP, sL=sL, sP=sP, vrL=vrL, vrP=vrP, maxKrok=mk)


class Prsty:
    """skutecne dotyky pres CDP: aktivni prsty se posilaji vzdy vsechny (jako displej)"""

    def __init__(self, pg):
        self.pg = pg
        self.cdp = pg.context.new_cdp_session(pg)
        self.aktivni = {}

    def _posli(self, typ):
        self.cdp.send('Input.dispatchTouchEvent', {'type': typ, 'touchPoints': [
            {'x': x, 'y': y, 'id': i, 'radiusX': 4, 'radiusY': 4, 'force': 1} for i, (x, y) in self.aktivni.items()]})

    def dolu(self, i, x, y):
        self.aktivni[i] = (x, y)
        self._posli('touchStart')

    def pohyb(self, i, x, y):
        self.aktivni[i] = (x, y)
        self._posli('touchMove')

    def nahoru(self, i):
        self.aktivni.pop(i, None)
        self._posli('touchEnd')

    def zrus(self):
        self.aktivni = {}
        self._posli('touchCancel')


def stred(pg, sel):
    b = pg.locator(sel).bounding_box()
    return b['x'] + b['width'] / 2, b['y'] + b['height'] / 2, b


def viditelne(pg, sel):
    return pg.evaluate("(s) => { var e=document.querySelector(s); if(!e) return false; var c=getComputedStyle(e);"
                       " var r=e.getBoundingClientRect(); return c.display!=='none' && c.visibility!=='hidden' && r.width>0 && r.height>0; }", sel)


def novy(b, zaklad, jazyk, okno, chyby):
    ctx = b.new_context(viewport=okno, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, MOCK))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(zaklad + 'emu_ps1/index.html')
    pg.wait_for_timeout(700)
    return ctx, pg


def zaznam(pg):
    return pg.evaluate("() => { var d=window.__dotyky||[]; window.__dotyky=[]; return d; }")


def test_mys(b, zaklad, prac, chyby):
    ctx, pg = novy(b, zaklad, 'cs', NA_SIRKU, chyby)
    p = Prsty(pg)
    over(viditelne(pg, '#btnMys') and pg.inner_text('#btnMys') == 'MYŠ', 'na sirku je tlacitko MYŠ (vlevo nahore)')
    mx, my, mb = stred(pg, '#psMonitor')
    p.dolu(1, mx, my); pg.wait_for_timeout(50); p.pohyb(1, mx + 60, my); p.nahoru(1)
    pg.wait_for_timeout(100)
    over(len(zaznam(pg)) == 0, 'mys vypnuta: dotyk na obraze se mysi neposila (jako B316 / B317)')
    bx, by, _ = stred(pg, '#btnMys')
    p.dolu(5, bx, by); pg.wait_for_timeout(60); p.nahoru(5)
    pg.wait_for_timeout(300)
    nast = pg.evaluate('() => window.__nastav||[]')
    trida = pg.evaluate('() => document.body.className')
    over(nast == ['1,100'] and 'mysZap' in trida and 'mysPort1' in trida, 'MYŠ: ps1MysNastav(1, 100) - mys v portu 1 (%s)' % nast)
    over(pg.evaluate("() => document.getElementById('mysPanel').classList.contains('open')"), 'poprve se rovnou otevre napoveda MYŠ PLAYSTATION')
    zr = pg.inner_text('#mysZrychleni')
    zrA = pg.evaluate("() => document.getElementById('mysZrychleni').classList.contains('aktivni')")
    aktivni = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mysPanel .mysRezim[data-rezim].aktivni'), function(e){return e.id;})")
    over(zr == 'ZRYCHLENÍ: ZAPNUTO' and zrA and aktivni == ['mysRezim1'],
         'B318 panel: tlacitko "%s" (zvyraznene - stav z C++, bit5), volba portu zvyraznena zvlast: %s' % (zr, aktivni))
    over(viditelne(pg, '#mysZrychleni') and 'Myš počítače:' in pg.inner_text('#mysPanel') and
         'pomalý tah prstem = jemný přesný posun, rychlý tah = kurzor dojede dál' in pg.inner_text('#mysPanel'),
         'B318 panel: vysvetleni zrychleni a odstavec "Myš počítače: v prohlížeči na PC nebo TV (WEB TV) klikni do obrazu ..."')
    pg.click('#mysZrychleni')
    pg.wait_for_timeout(200)
    zr2, t2 = pg.inner_text('#mysZrychleni'), pg.inner_text('#toast')
    zrA2 = pg.evaluate("() => document.getElementById('mysZrychleni').classList.contains('aktivni')")
    pg.click('#mysZrychleni')
    pg.wait_for_timeout(200)
    zr3, t3 = pg.inner_text('#mysZrychleni'), pg.inner_text('#toast')
    volani = pg.evaluate('() => window.__zr||[]')
    over(volani == [0, 1] and zr2 == 'ZRYCHLENÍ: VYPNUTO' and not zrA2 and t2 == 'ZRYCHLENÍ VYPNUTO – kurzor jede přesně jako prst' and
         zr3 == 'ZRYCHLENÍ: ZAPNUTO' and t3 == 'ZRYCHLENÍ ZAPNUTO – pomalý tah jemně, rychlý dál',
         'ZRYCHLENÍ: klik -> ps1MysZrychleni(0) "%s" + hlaska "%s", znovu -> (1) "%s" (%s)' % (zr2, t2, zr3, volani))
    aktivni = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mysPanel .mysRezim[data-rezim].aktivni'), function(e){return e.id;})")
    over(aktivni == ['mysRezim1'], 'prepnuti zrychleni nezmenilo zvyraznenou volbu portu (%s)' % aktivni)
    pg.click('#mysZavrit')
    pg.wait_for_timeout(200)
    schovane = [s for s in ('#btnTriangle', '#btnSquare', '#btnL1', '#btnL2', '#btnR1', '#btnR2', '#btnSelect', '#btnStart') if viditelne(pg, s)]
    zustava = [s for s in ('#btnCross', '#btnCircle', '#dpad', '#btnSettings', '#btnMys', '#psMonitor') if viditelne(pg, s)]
    over(not schovane and len(zustava) == 6, 'mys v portu 1: schovane tlacitka, ktera hra s mysi nevidi; KRIZEK, KOLECKO, D-pad zustavaji')
    mx, my, mb = stred(pg, '#psMonitor')
    w = mb['width']
    lano = 0.015 * w
    # 1) tah: 15 x (10, 4) bodu stranky po 16 ms
    zaznam(pg)
    p.dolu(1, mx - 150, my)
    for k in range(1, 16):
        pg.wait_for_timeout(16)
        p.pohyb(1, mx - 150 + 10 * k, my + 4 * k)
    p.nahoru(1)
    pg.wait_for_timeout(50)
    d = zaznam(pg)
    r = prehraj(prac, 1, 100, 0, d)
    delka = (150 ** 2 + 60 ** 2) ** 0.5
    ocek_x, ocek_y = (150 - lano * 150 / delka) * 320 / w, (60 - lano * 60 / delka) * 320 / w
    akce = [x[0] for x in d]
    over(akce[0] == 0 and akce[-1] == 1 and akce.count(2) == 15 and all(abs(x[4] - w) < 0.5 for x in d),
         'tah: stranka poslala dolu, 15x pohyb, nahoru; sirka obrazu %.0f bodu (w v kazde zprave)' % w)
    over(abs(r['sx'] - ocek_x) <= 1 and abs(r['sy'] - ocek_y) <= 1 and r['kL'] == 0 and r['kP'] == 0 and r['maxKrok'] <= 6,
         'tah 150 x 60 bodu stranky, zrychleni vypnuto: hra +%d / +%d (vypocet za lanem 1,5 %%: %.1f / %.1f), nejvic %d bodu za snimek - zadny skok'
         % (r['sx'], r['sy'], ocek_x, ocek_y, r['maxKrok']))
    rz = prehraj(prac, 1, 100, 1, d)
    # skutecna rychlost prstu ze zaznamu (Chromium posila dotyky pomaleji nez po 16 ms)
    pohyby = [x for x in d if x[0] == 2]
    v = (delka / w) / max(0.001, (pohyby[-1][6] - pohyby[0][6]) / 1000.0) * len(pohyby) / max(1, len(pohyby) - 1)
    g = min(2.5, max(0.4, (v / 0.7) ** 0.5))
    over(rz['kL'] == 0 and abs(rz['sx'] / max(1, r['sx']) - g) <= 0.2 * g,
         'stejny tah se zrychlenim: prst %.2f sirky obrazu za s -> nasobek %.2f, hra +%d (bez zrychleni +%d, pomer %.2f)'
         % (v, g, rz['sx'], r['sx'], rz['sx'] / max(1, r['sx'])))
    p.dolu(2, mx, my); pg.wait_for_timeout(70); p.nahoru(2)
    pg.wait_for_timeout(50)
    r = prehraj(prac, 1, 100, 1, zaznam(pg))
    over(r['kL'] == 1 and r['sL'] == 5 and r['sx'] == 0 and r['kP'] == 0, 'tuknuti: hra 1 klik levym (5 snimku), kurzor stoji')
    p.dolu(3, mx - 60, my); pg.wait_for_timeout(30); p.dolu(4, mx + 60, my + 10); pg.wait_for_timeout(80)
    p.nahoru(4); p.nahoru(3)
    pg.wait_for_timeout(50)
    d = zaznam(pg)
    r = prehraj(prac, 1, 100, 1, d)
    over(r['kP'] == 1 and r['kL'] == 0 and sorted(set(x[1] for x in d)) == [3, 4], 'tuknuti dvema prsty: hra 1 klik pravym, levym ne')
    # podrzeni 0,8 s (B318: drzi az od 0,65 s) a tah
    p.dolu(6, mx - 100, my)
    pg.wait_for_timeout(800)
    for k in range(1, 9):
        p.pohyb(6, mx - 100 + 10 * k, my)
        pg.wait_for_timeout(16)
    p.nahoru(6)
    pg.wait_for_timeout(50)
    r = prehraj(prac, 1, 100, 0, zaznam(pg))
    over(r['kL'] == 1 and 10 <= r['sL'] <= 40 and abs(r['sx'] - (80 - lano) * 320 / w) <= 1,
         'podrzeni 0,8 s a tah: leve tlacitko 1x stisknuto, drzeno %d snimku (od 0,65 s do zvednuti), posun +%d (vyber ramem)' % (r['sL'], r['sx']))
    pg.evaluate('() => { window.__vstup=[]; }')
    cx, cy, _ = stred(pg, '#btnCross')
    p.dolu(7, cx, cy)
    p.dolu(8, mx - 50, my)
    for k in range(1, 6):
        pg.wait_for_timeout(16)
        p.pohyb(8, mx - 50 + 12 * k, my)
    p.nahoru(8)
    p.nahoru(7)
    pg.wait_for_timeout(80)
    d = zaznam(pg)
    vstup = pg.evaluate('() => window.__vstup')
    over(vstup == ['CROSS+', 'CROSS-'] and set(x[1] for x in d) == {8},
         'prst na KRIZKU + druhy po obrazu: KRIZEK jde jako tlacitko (%s), mysi jen prst z obrazu' % vstup)
    # drzeni a vibrace (stav bit1 z C++), klik L / P
    pg.evaluate('() => { window.__vibrace=[]; window.__mysDrzi=true; }')
    p.dolu(9, mx, my)
    pg.wait_for_timeout(350)
    drzi = pg.evaluate("() => document.body.classList.contains('mysLeveDrzi')") and viditelne(pg, '#mysUkazDrzi')
    pg.evaluate('() => { window.__mysDrzi=false; }')
    p.nahoru(9)
    pg.wait_for_timeout(200)
    vib = pg.evaluate('() => window.__vibrace')
    over(drzi and vib.count(30) == 1 and not viditelne(pg, '#mysUkazDrzi'), 'C++ hlasi leve drzi: DRŽÍM LEVÉ TLAČÍTKO + vibrace 30 ms, po zvednuti zmizi')
    pg.evaluate('() => { window.__vibrace=[]; window.__mysVratit=8; }')
    p.dolu(10, mx, my); pg.wait_for_timeout(40); p.nahoru(10)
    pg.evaluate('() => { window.__mysVratit=16; }')
    p.dolu(11, mx, my); pg.wait_for_timeout(40); p.nahoru(11)
    pg.wait_for_timeout(50)
    pg.evaluate('() => { window.__mysVratit=0; }')
    vib = pg.evaluate('() => window.__vibrace')
    over(vib == [12, 28], 'C++ hlasi klik levym / pravym: vibrace 12 / 28 ms (%s) - bit5 (zrychleni) v odpovedi nevadi' % vib)
    zaznam(pg)
    p.dolu(12, mx, my)
    pg.wait_for_timeout(30)
    p.zrus()
    pg.wait_for_timeout(50)
    over([x[0] for x in zaznam(pg)] == [0, 3], 'system dotyk zrusil: stranka posle ZRUSENO (akce 3)')
    # panel z kolecka: rezimy, rychlost, zrychleni vypnute zobrazene
    pg.evaluate('() => { window.__nastav=[]; window.__mysZr=false; }')
    pg.click('#btnSettings')
    pg.wait_for_timeout(200)
    pg.click('#btnMysNastaveni')
    pg.wait_for_timeout(250)
    over(pg.inner_text('#mysZrychleni') == 'ZRYCHLENÍ: VYPNUTO' and pg.inner_text('#mysHra') == 'Hra: SLUS00973 – MYŠ V PORTU 1' and
         pg.inner_text('#mysCitlivostVal') == 'Rychlost: 100 %',
         'panel z kolecka: zrychleni podle C++ (VYPNUTO), "Hra: SLUS00973 – MYŠ V PORTU 1", "Rychlost: 100 %"')
    pg.click('#mysRezim2')
    pg.wait_for_timeout(150)
    pg.evaluate("() => { var s=document.getElementById('mysCitlivost'); s.value=295;"
                " s.dispatchEvent(new Event('input')); s.dispatchEvent(new Event('change')); }")
    pg.wait_for_timeout(150)
    aktivni = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mysPanel .mysRezim.aktivni'), function(e){return e.id;})")
    over(pg.evaluate('() => window.__nastav') == ['2,100', '2,295'] and aktivni == ['mysRezim2'] and
         pg.inner_text('#mysCitlivostVal') == 'Rychlost: 295 %',
         'Reneho nastaveni Dune 2000 (port 1 + ovladac 2, 295 %%): ps1MysNastav jako B317, zvyraznena jen volba portu (%s)' % aktivni)
    pg.click('#mysZavrit')
    ctx.close()


# ---------------- jazyk: cesky jako B317 (+ B318), anglicky bez cestiny ----------------
KROKY_MYS = [
    ('MYS zapnuta (poprve napoveda)', "document.getElementById('btnMys').click()", 300),
    ('zrychleni vypnout', "document.getElementById('mysZrychleni').click()", 300),
    ('zrychleni zapnout', "document.getElementById('mysZrychleni').click()", 300),
    ('drzim leve', "window.__mysDrzi=true", 1300),
    ('pusteno', "window.__mysDrzi=false", 1300),
    ('panel zavrit', "document.getElementById('mysZavrit').click()", 200),
    ('MYS vypnuta', "document.getElementById('btnMys').click()", 200),
    ('MYS zapnuta znovu', "document.getElementById('btnMys').click()", 200),
    ('panel z kolecka', "document.getElementById('btnSettings').click(); document.getElementById('btnMysNastaveni').click()", 300),
    ('port 2', "document.getElementById('mysRezim3').click()", 200),
    ('rychlost', "var s=document.getElementById('mysCitlivost'); s.value=55; s.dispatchEvent(new Event('input')); s.dispatchEvent(new Event('change'))", 200),
    ('zrychleni z C++ vypnute', "window.__mysZr=false; document.getElementById('mysRezim2').click()", 300),
    ('hra nebezi', "window.__mysKlic=''; document.getElementById('mysRezim1').click()", 300),
    ('jadro neodpovida', "NAP_MOCK['AHPS1.ps1MysZrychleni']=''; document.getElementById('mysZrychleni').click()", 300),
]
# texty, ktere v B318 na strance PS1 pribyly - jinak musi byt cestina presne jako B317
NOVE_CESKY = ('ZRYCHLENÍ: ZAPNUTO', 'ZRYCHLENÍ: VYPNUTO', 'Zrychlení', '— jako u myši počítače', 'Myš počítače:',
              'v prohlížeči na PC nebo TV (WEB TV)', 'ZRYCHLENÍ ZAPNUTO', 'ZRYCHLENÍ VYPNUTO')


def projdi(b, zaklad, okno, kroky, jazyk, mock, chyby):
    ctx = b.new_context(viewport=okno, has_touch=True)
    ctx.add_init_script(H.init_skript(jazyk, mock))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(zaklad + 'emu_ps1/index.html')
    pg.wait_for_timeout(600)
    sbery = [('nacteno', pg.evaluate(H.SBER_JS))]
    for popis, js, cekej in kroky:
        try:
            pg.evaluate('() => { %s }' % js)
        except Exception as e:
            chyby.append('krok "%s": %s' % (popis, str(e).split('\n')[0][:160]))
        pg.wait_for_timeout(cekej)
        sbery.append((popis, pg.evaluate(H.SBER_JS)))
    ctx.close()
    return sbery


def test_jazyk(b, zaklad, zaklad0):
    ps1 = STRANKY['ps1']
    mock = dict(ps1['mock'])
    mock.update(MOCK)
    sady = [('na vysku (kroky testu B314)', {'width': 412, 'height': 846}, ps1['kroky']),
            ('na sirku (mys)', NA_SIRKU, KROKY_MYS)]
    vse_en = []
    for jm, okno, kroky in sady:
        chyby_cz, chyby_en, chyby_317 = [], [], []
        cz = projdi(b, zaklad, okno, kroky, 'cs', mock, chyby_cz)
        en = projdi(b, zaklad, okno, kroky, 'en', mock, chyby_en)
        # B317 nema tlacitko zrychleni - jeho kroky tam nic neudelaji (porovnani jen textu, ktere tam jsou)
        puv = projdi(b, zaklad0, okno, kroky, 'cs', mock, chyby_317)
        rozdil = []
        for (popis, a), (_, a0) in zip(cz, puv):
            for druh in ('texty', 'atributy'):
                sx, sy = set(a[druh]), set(a0[druh])
                navic = [t for t in sx - sy if not any(t.startswith(z) or z in t for z in NOVE_CESKY)]
                chybi = sorted(t for t in sy - sx if not t.startswith(('Rychlost: ', 'Hra: ', 'MYŠ ZAPNUTÁ', 'MYŠ VYPNUTÁ')))
                if navic or chybi:
                    rozdil.append('%s/%s: +%s -%s' % (popis, druh, navic[:3], chybi[:3]))
        if jm.startswith('na vysku'):
            over(not rozdil, 'cesky %s: %d stavu stranky - texty jako B317, navic jen texty B318%s' % (
                jm, len(cz), '' if not rozdil else ' - ROZDIL: ' + ' | '.join(rozdil[:4])))
        else:
            texty = set(t for _, s in cz for t in s['texty'])
            nove = [z for z in NOVE_CESKY if any(t.startswith(z) or z in t for t in texty)]
            over(len(nove) == len(NOVE_CESKY), 'cesky %s: vsechny nove texty B318 na strance (%d / %d)' % (jm, len(nove), len(NOVE_CESKY)))
        nove_chyby = sorted(set(chyby_cz + chyby_en) - set(chyby_317))
        over(not nove_chyby, 'cesky i anglicky %s: zadna nova chyba JavaScriptu (B317: %d)%s' % (
            jm, len(set(chyby_317)), '' if not nove_chyby else ' - NOVE: ' + ' | '.join(nove_chyby[:4])))
        zbytky = []
        for popis, s in en:
            for druh, t, sl in H.zbytky_cestiny(s, ps1.get('povoleno_en', ()), ps1.get('slova_en', ())):
                zbytky.append('%s / %s: "%s" (%s)' % (popis, druh, t[:90], ','.join(sl[:4])))
            vse_en += s['texty'] + s['atributy'] + ['[dialog] ' + d for d in s['dialogy']]
        zbytky = list(dict.fromkeys(zbytky))
        over(not zbytky, 'anglicky %s: %d stavu bez cestiny%s' % (jm, len(en), '' if not zbytky else ' - ZBYTKY: ' + ' | '.join(zbytky[:8])))
        if not jm.startswith('na vysku'):
            texty = set(t for _, s in en for t in s['texty'])
            hledane = ('ACCELERATION: ON', 'ACCELERATION: OFF', 'Acceleration', 'Computer mouse:',
                       'ACCELERATION OFF – the cursor moves exactly like your finger', 'ACCELERATION ON – slow drag fine, fast drag further',
                       'PLAYSTATION MOUSE', 'Speed: 55 %', 'MOUSE: the PS1 core does not answer')
            chybi = [h for h in hledane if h not in texty]
            over(not chybi, 'anglicky: zrychleni, mys pocitace, hlasky (%s)' % (', '.join(chybi) if chybi else 'vse prelozeno'))
    with open(os.path.join(ZDE, 'VYSTUP_texty_ps1_mys_en.txt'), 'w', encoding='utf-8') as f:
        f.write('B318 - anglicke texty stranky PS1 (vsechny stavy, i mys a zrychleni), pro kontrolu ocima\n')
        for t in dict.fromkeys(vse_en):
            f.write(t + '\n')


def main():
    prac = sys.argv[1]
    subprocess.run(['g++', '-std=c++17', '-O2', '-o', os.path.join(prac, 'prehraj_dotyky_b318'), os.path.join(ZDE, 'prehraj_dotyky.cpp')], check=True)
    srv, zaklad = H.server()
    tmp = tempfile.mkdtemp(prefix='b318_str_')
    subprocess.run('git -C "%s" archive %s app/src/main/assets | tar -x -C "%s"' % (H.KOREN, B317, tmp), shell=True, check=True)
    srv0, zaklad0 = H.server(os.path.join(tmp, 'app/src/main/assets'))
    chyby = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        test_mys(b, zaklad, prac, chyby)
        over(not chyby, 'gesta a panel: zadna chyba JavaScriptu%s' % ('' if not chyby else ' - ' + ' | '.join(chyby[:4])))
        test_jazyk(b, zaklad, zaklad0)
        b.close()
    srv.shutdown(); srv0.shutdown()
    vystup.insert(0, 'B318 - stranka PS1: jemnejsi mys (Chromium, skutecne dotyky, logika mysi B318 z appky)')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b318_stranka.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
