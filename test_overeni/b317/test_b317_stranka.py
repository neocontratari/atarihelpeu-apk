#!/usr/bin/env python3
"""B317: STRANKA PS1 S MYSI (Chromium, stranka presne z appky: assets/emu_ps1/index.html).

Dotyky jsou SKUTECNE dotykove udalosti Chromia (CDP Input.dispatchTouchEvent - jako prst na
displeji, i vic prstu naraz). Most Javy (AHPS1) je falesny: co stranka posle, se zapise i s casem
(performance.now()). Zaznam dotyku pak prehraje prehraj_dotyky (SKUTECNA logika mysi z appky,
nap_ps1_mys.h) - co by z toho videla hra: posun kurzoru, kliky.

Kontroly:
  1) na sirku je tlacitko MYS, na vysku ne; s vypnutou mysi dotyk na obraze do mysi nejde
  2) MYS zapne mys (port 1), poprve se otevre napoveda; schovaji se tlacitka, ktera hra s mysi
     samotnou nevidi (trojuhelnik, ctverec, ramena, SELECT, START), KRIZEK / KOLECKO / D-pad zustanou
  3) gesta pres skutecne dotyky -> skutecna logika mysi: tah, tuknuti, dva prsty, podrzeni + tah,
     prst na KRIZKU a druhy na obraze naraz
  4) leve tlacitko drzi -> ukazatel DRZIM LEVE TLACITKO + vibrace; klik -> vibrace
  5) panel MYS: 4 rezimy (aktivni zvyrazneny), rychlost, hra a rezim; vypnuti / zapnuti tlacitkem
  6) zruseni dotyku (appka do pozadi)
  7) cesky beze zmeny proti B316 krome textu mysi (vsechny stavy stranky z testu B314 + mys),
     anglicky bez cestiny, zadna chyba JavaScriptu
Pouziti: python3 test_b317_stranka.py PRAC     -> VYSTUP_test_b317_stranka.txt, VYSTUP_texty_ps1_mys_en.txt
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

B316 = os.environ.get('NAP_B316', 'd7c85b9')   # B316 = verze pred mysi
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


# falesny most: rezim / citlivost si pamatuje stranka (jako C++), dotyky se zapisuji s casem
MOCK = {
    'AHPS1.ps1MysNastav': "fn: window.__mys=[args[0]|0,args[1]|0]; (window.__nastav=window.__nastav||[]).push(args[0]+','+args[1]);"
                          " return window.__mys[0]+'|'+window.__mys[1]+'|'+(window.__mys[0]>0?1:0)+'|'+(window.__mysKlic!==undefined?window.__mysKlic:'SLES02250');",
    'AHPS1.ps1MysStav': "fn: var m=window.__mys||[0,100]; return m[0]+'|'+m[1]+'|'+((m[0]>0?1:0)|(window.__mysDrzi?2:0))+'|'+(window.__mysKlic!==undefined?window.__mysKlic:'SLES02250');",
    'AHPS1.ps1MysDotyk': "fn: (window.__dotyky=window.__dotyky||[]).push([args[0],args[1],args[2],args[3],args[4],args[5],performance.now()]);"
                         " var m=window.__mys||[0]; return (m[0]>0?1:0)|(window.__mysVratit&&args[0]==1?window.__mysVratit:0);",
    'AHPS1.ps1Vibrate': "fn: (window.__vibrace=window.__vibrace||[]).push(args[0]); return '';",
    'AHPS1.ps1Input': "fn: (window.__vstup=window.__vstup||[]).push(args[0]+(args[1]?'+':'-')); return '';",
}
NA_SIRKU = {'width': 846, 'height': 412}


def prehraj(prac, rezim, citlivost, dotyky):
    """zaznam dotyku -> skutecna logika mysi (prehraj_dotyky) -> co uvidi hra"""
    vstup = '%d %d\n' % (rezim, citlivost) + ''.join('%d %d %.3f %.3f %.3f %.3f %.3f\n' % tuple(d) for d in dotyky)
    r = subprocess.run([os.path.join(prac, 'prehraj_dotyky')], input=vstup, capture_output=True, text=True, check=True)
    sx, sy, kL, kP, sL, sP, vrL, vrP = (int(v) for v in r.stdout.split())
    return dict(sx=sx, sy=sy, kL=kL, kP=kP, sL=sL, sP=sP, vrL=vrL, vrP=vrP)


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
    # 1) mys vypnuta: dotyk na obraze do mysi nejde
    p.dolu(1, mx, my); pg.wait_for_timeout(50); p.pohyb(1, mx + 60, my); p.nahoru(1)
    pg.wait_for_timeout(100)
    over(len(zaznam(pg)) == 0, 'mys vypnuta: dotyk na obraze se mysi neposila (jako B316)')
    # 2) zapnout tlacitkem MYS (dotykem, jako prst)
    bx, by, _ = stred(pg, '#btnMys')
    p.dolu(5, bx, by); pg.wait_for_timeout(60); p.nahoru(5)
    pg.wait_for_timeout(300)
    nast = pg.evaluate('() => window.__nastav||[]')
    trida = pg.evaluate('() => document.body.className')
    over(nast == ['1,100'] and 'mysZap' in trida and 'mysPort1' in trida,
         'MYŠ: ps1MysNastav(1, 100) - mys v portu 1, stranka v rezimu mysi (%s)' % nast)
    over(pg.evaluate("() => document.getElementById('mysPanel').classList.contains('open')"),
         'poprve se rovnou otevre napoveda MYŠ PLAYSTATION (gesta)')
    pg.click('#mysZavrit')
    pg.wait_for_timeout(200)
    schovane = [s for s in ('#btnTriangle', '#btnSquare', '#btnL1', '#btnL2', '#btnR1', '#btnR2', '#btnSelect', '#btnStart') if viditelne(pg, s)]
    zustava = [s for s in ('#btnCross', '#btnCircle', '#dpad', '#btnSettings', '#btnMys', '#psMonitor') if viditelne(pg, s)]
    over(not schovane and len(zustava) == 6,
         'mys v portu 1: schovane trojuhelnik, ctverec, L1, L2, R1, R2, SELECT, START; KRIZEK, KOLECKO, D-pad zustavaji')
    mx, my, mb = stred(pg, '#psMonitor')
    w = mb['width']
    # 3) gesta -> skutecna logika mysi
    zaznam(pg)
    p.dolu(1, mx - 150, my)
    for k in range(1, 16):
        pg.wait_for_timeout(16)
        p.pohyb(1, mx - 150 + 10 * k, my + 4 * k)
    p.nahoru(1)
    pg.wait_for_timeout(50)
    d = zaznam(pg)
    r = prehraj(prac, 1, 100, d)
    ocek_x, ocek_y = 150 * 320 / w, 60 * 320 / w
    akce = [x[0] for x in d]
    over(akce[0] == 0 and akce[-1] == 1 and akce.count(2) == 15 and all(abs(x[4] - w) < 0.5 for x in d),
         'tah: stranka poslala dolu, 15x pohyb, nahoru; sirka obrazu %.0f bodu (w v kazde zprave)' % w)
    over(abs(r['sx'] - ocek_x) <= 1 and abs(r['sy'] - ocek_y) <= 1 and r['kL'] == 0 and r['kP'] == 0,
         'tah prstem 150 x 60 bodu stranky: hra +%d / +%d (vypocet %.1f / %.1f), zadny klik' % (r['sx'], r['sy'], ocek_x, ocek_y))
    x0 = d[0][2]
    over(abs(x0 - (mx - 150 - mb['x'])) < 0.6 and abs(d[0][3] - (my - mb['y'])) < 0.6,
         'souradnice prstu se posilaji vuci obrazu hry (x %.1f = prst - levy okraj obrazu)' % x0)

    p.dolu(2, mx, my); pg.wait_for_timeout(70); p.nahoru(2)
    pg.wait_for_timeout(50)
    r = prehraj(prac, 1, 100, zaznam(pg))
    over(r['kL'] == 1 and r['sL'] == 5 and r['sx'] == 0 and r['kP'] == 0, 'tuknuti: hra 1 klik levym (5 snimku), kurzor stoji')

    p.dolu(3, mx - 60, my); pg.wait_for_timeout(30); p.dolu(4, mx + 60, my + 10); pg.wait_for_timeout(80)
    p.nahoru(4); p.nahoru(3)
    pg.wait_for_timeout(50)
    d = zaznam(pg)
    r = prehraj(prac, 1, 100, d)
    over(r['kP'] == 1 and r['kL'] == 0 and sorted(set(x[1] for x in d)) == [3, 4],
         'tuknuti dvema prsty: hra 1 klik pravym, levym ne (oba prsty s vlastnim cislem)')

    p.dolu(6, mx - 100, my)
    pg.wait_for_timeout(700)
    for k in range(1, 9):
        p.pohyb(6, mx - 100 + 10 * k, my)
        pg.wait_for_timeout(16)
    p.nahoru(6)
    pg.wait_for_timeout(50)
    r = prehraj(prac, 1, 100, zaznam(pg))
    # leve drzi od 0,5 s do zvednuti prstu (~0,85 s) = asi 20-30 snimku
    over(r['kL'] == 1 and 15 <= r['sL'] <= 45 and abs(r['sx'] - 80 * 320 / w) <= 1,
         'podrzeni 0,7 s a tah: leve tlacitko 1x stisknuto, drzeno %d snimku (od 0,5 s do zvednuti), posun +%d (vyber ramem)' % (r['sL'], r['sx']))

    # prst na KRIZKU (tlacitko) a druhy prst tahne po obrazu naraz
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

    # 4) drzeni a vibrace
    pg.evaluate('() => { window.__vibrace=[]; window.__mysDrzi=true; }')
    p.dolu(9, mx, my)
    pg.wait_for_timeout(350)
    drzi = pg.evaluate("() => document.body.classList.contains('mysLeveDrzi')") and viditelne(pg, '#mysUkazDrzi')
    text = pg.inner_text('#mysUkazDrzi')
    pg.evaluate('() => { window.__mysDrzi=false; }')
    p.nahoru(9)
    pg.wait_for_timeout(200)
    vib = pg.evaluate('() => window.__vibrace')
    over(drzi and text == 'DRŽÍM LEVÉ TLAČÍTKO' and vib.count(30) == 1 and not viditelne(pg, '#mysUkazDrzi'),
         'C++ hlasi leve drzi: nahore DRŽÍM LEVÉ TLAČÍTKO + vibrace 30 ms (1x), po zvednuti prstu zmizi')
    pg.evaluate('() => { window.__vibrace=[]; window.__mysVratit=8; }')
    p.dolu(10, mx, my); pg.wait_for_timeout(40); p.nahoru(10)
    pg.evaluate('() => { window.__mysVratit=16; }')
    p.dolu(11, mx, my); pg.wait_for_timeout(40); p.nahoru(11)
    pg.wait_for_timeout(50)
    pg.evaluate('() => { window.__mysVratit=0; }')
    vib = pg.evaluate('() => window.__vibrace')
    over(vib == [12, 28], 'C++ hlasi klik levym / pravym: kratka vibrace 12 / 28 ms (%s)' % vib)
    zaznam(pg)

    # 6) zruseni dotyku (appka do pozadi)
    p.dolu(12, mx, my)
    pg.wait_for_timeout(30)
    p.zrus()
    pg.wait_for_timeout(50)
    d = zaznam(pg)
    over([x[0] for x in d] == [0, 3], 'system dotyk zrusil: stranka posle ZRUSENO (akce 3) - nic nekliknout, vse pustit')
    p.dolu(13, mx, my)
    pg.wait_for_timeout(30)
    pg.evaluate("() => window.dispatchEvent(new Event('blur'))")
    pg.wait_for_timeout(30)
    p.nahoru(13)
    d = zaznam(pg)
    over([x[0] for x in d] == [0, 3], 'okno ztratilo zamereni (appka do pozadi) s prstem na obraze: ZRUSENO')

    # 5) panel a rezimy
    pg.evaluate('() => { window.__nastav=[]; }')
    pg.click('#btnSettings')
    pg.wait_for_timeout(200)
    over(viditelne(pg, '#btnMysNastaveni'), 'kolecko (na sirku D-PAD A OVLÁDÁNÍ): tlacitko MYŠ PLAYSTATION (DUNE 2000 …)')
    pg.click('#btnMysNastaveni')
    pg.wait_for_timeout(200)
    aktivni = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mysPanel .mysRezim.aktivni'), function(e){return e.id;})")
    over(aktivni == ['mysRezim1'] and pg.inner_text('#mysHra') == 'Hra: SLES02250 – MYŠ V PORTU 1' and
         pg.inner_text('#mysCitlivostVal') == 'Rychlost: 100 %',
         'panel: zvyraznena volba MYŠ V PORTU 1, "Hra: SLES02250 – MYŠ V PORTU 1", "Rychlost: 100 %"')
    pg.click('#mysRezim3')
    pg.wait_for_timeout(150)
    aktivni = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mysPanel .mysRezim.aktivni'), function(e){return e.id;})")
    over(pg.evaluate('() => window.__nastav') == ['3,100'] and aktivni == ['mysRezim3'] and
         'mysPort1' not in pg.evaluate('() => document.body.className'),
         'OVLADAČ PORT 1 + MYŠ PORT 2: ps1MysNastav(3, 100), zvyraznena, ovladac cely (nic schovaneho)')
    pg.evaluate("() => { var s=document.getElementById('mysCitlivost'); s.value=180;"
                " s.dispatchEvent(new Event('input')); s.dispatchEvent(new Event('change')); }")
    pg.wait_for_timeout(150)
    over(pg.evaluate('() => window.__nastav')[-1] == '3,180' and pg.inner_text('#mysCitlivostVal') == 'Rychlost: 180 %',
         'rychlost 180 %: ps1MysNastav(3, 180), "Rychlost: 180 %"')
    pg.click('#mysZavrit')
    pg.wait_for_timeout(150)
    vse = all(viditelne(pg, s) for s in ('#btnTriangle', '#btnSquare', '#btnL1', '#btnR2', '#btnSelect', '#btnStart'))
    over(vse, 'mys v portu 2 + ovladac v portu 1: vsechna tlacitka ovladace videt')
    # vypnout a zase zapnout tlacitkem MYS
    pg.evaluate('() => { window.__nastav=[]; }')
    pg.click('#btnMys')
    pg.wait_for_timeout(150)
    t1 = pg.inner_text('#toast')
    pg.click('#btnMys')
    pg.wait_for_timeout(150)
    t2 = pg.inner_text('#toast')
    otevreno = pg.evaluate("() => document.getElementById('mysPanel').classList.contains('open')")
    over(pg.evaluate('() => window.__nastav') == ['0,180', '3,180'] and t1 == 'MYŠ VYPNUTÁ – zase ovladač' and
         t2.startswith('MYŠ ZAPNUTÁ (OVLADAČ PORT 1 + MYŠ PORT 2)') and not otevreno,
         'MYŠ vypne (0) a zapne znovu naposledy zvoleny port (3) - uz bez napovedy, jen hlaska: "%s"' % t2)
    ctx.close()


def test_na_vysku(b, zaklad, chyby):
    ctx, pg = novy(b, zaklad, 'cs', {'width': 412, 'height': 846}, chyby)
    over(not viditelne(pg, '#btnMys'), 'na vysku (uvodni obrazovka, nehraje se) tlacitko MYŠ neni')
    pg.click('#btnSettings')
    pg.wait_for_timeout(200)
    pg.click('#btnDpadMenuFromHelp')
    pg.wait_for_timeout(200)
    over(viditelne(pg, '#btnMysNastaveni'), 'na vysku: nastaveni mysi je v D-PAD A OVLÁDÁNÍ')
    ctx.close()


# ---------------- jazyk: cesky jako B316 (+ mys), anglicky bez cestiny ----------------
KROKY_MYS = [
    ('MYS zapnuta (poprve napoveda)', "document.getElementById('btnMys').click()", 300),
    ('drzim leve', "window.__mysDrzi=true", 1300),
    ('pusteno', "window.__mysDrzi=false", 1300),
    ('panel zavrit', "document.getElementById('mysZavrit').click()", 200),
    ('MYS vypnuta', "document.getElementById('btnMys').click()", 200),
    ('MYS zapnuta znovu', "document.getElementById('btnMys').click()", 200),
    ('panel z kolecka', "document.getElementById('btnSettings').click(); document.getElementById('btnMysNastaveni').click()", 300),
    ('port 2', "document.getElementById('mysRezim3').click()", 200),
    ('port 1 + ovladac', "document.getElementById('mysRezim2').click()", 200),
    ('rychlost', "var s=document.getElementById('mysCitlivost'); s.value=55; s.dispatchEvent(new Event('input')); s.dispatchEvent(new Event('change'))", 200),
    ('vypnuta v panelu', "document.getElementById('mysRezim0').click()", 200),
    ('hra nebezi', "window.__mysKlic=''; document.getElementById('mysRezim1').click()", 300),
    ('jadro neodpovida', "NAP_MOCK['AHPS1.ps1MysNastav']=''; document.getElementById('mysRezim2').click()", 300),
]

# texty, ktere v B317 na strance PS1 pribyly (mys) - jinak musi byt cestina presne jako B316
NOVE_CESKY = (
    'MYŠ', 'DRŽÍM LEVÉ TLAČÍTKO', 'MYŠ PLAYSTATION', 'Pro hry s myší PlayStation', 'VYPNUTÁ – JEN OVLADAČ',
    'MYŠ V PORTU 1 (DUNE 2000)', 'MYŠ PORT 1 + OVLADAČ PORT 2', 'OVLADAČ PORT 1 + MYŠ PORT 2', 'Hry hledají myš',
    'Rychlost kurzoru', '— jak daleko kurzor dojede', 'Táhni prstem po obrazu', '= pohyb kurzoru.', 'Ťukni',
    '= levé tlačítko (vybrat, potvrdit).', 'Ťukni dvěma prsty', '= pravé tlačítko (zrušit).', 'Podrž prst na místě',
    '(zavibruje, nahoře se ukáže', 'Myš v portu 1:', '✕ = levé a ○ = pravé tlačítko', 'Volba se pamatuje',
    'Rychlost: ', 'Hra: ', 'Hra zatím neběží', 'MYŠ ZAPNUTÁ', 'MYŠ VYPNUTÁ', 'MYŠ: jádro PS1 neodpovídá',
)


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
        chyby_cz, chyby_en, chyby_316 = [], [], []
        cz = projdi(b, zaklad, okno, kroky, 'cs', mock, chyby_cz)
        en = projdi(b, zaklad, okno, kroky, 'en', mock, chyby_en)
        puv = projdi(b, zaklad0, okno, kroky, 'cs', mock, chyby_316) if jm.startswith('na vysku') else None
        if puv:
            rozdil = []
            for (popis, a), (_, a0) in zip(cz, puv):
                for druh in ('texty', 'atributy'):
                    sx, sy = set(a[druh]), set(a0[druh])
                    navic = [t for t in sx - sy if not any(t.startswith(z) or z in t for z in NOVE_CESKY)]
                    chybi = sorted(sy - sx)
                    if navic or chybi:
                        rozdil.append('%s/%s: +%s -%s' % (popis, druh, navic[:3], chybi[:3]))
            over(not rozdil, 'cesky %s: %d stavu stranky - texty jako B316, navic jen texty mysi%s' % (
                jm, len(cz), '' if not rozdil else ' - ROZDIL: ' + ' | '.join(rozdil[:4])))
            nove = sorted(set(chyby_cz + chyby_en) - set(chyby_316))
            over(not nove, 'cesky i anglicky %s: zadna nova chyba JavaScriptu (B316: %d)%s' % (
                jm, len(set(chyby_316)), '' if not nove else ' - NOVE: ' + ' | '.join(nove[:4])))
        else:
            vse_chyby = sorted(set(chyby_cz + chyby_en))
            over(not vse_chyby, 'cesky i anglicky %s: zadna chyba JavaScriptu%s' % (jm, '' if not vse_chyby else ' - ' + ' | '.join(vse_chyby[:4])))
        zbytky = []
        for popis, s in en:
            for druh, t, sl in H.zbytky_cestiny(s, ps1.get('povoleno_en', ()), ps1.get('slova_en', ())):
                zbytky.append('%s / %s: "%s" (%s)' % (popis, druh, t[:90], ','.join(sl[:4])))
            vse_en += s['texty'] + s['atributy'] + ['[dialog] ' + d for d in s['dialogy']]
        zbytky = list(dict.fromkeys(zbytky))
        over(not zbytky, 'anglicky %s: %d stavu bez cestiny%s' % (jm, len(en), '' if not zbytky else ' - ZBYTKY: ' + ' | '.join(zbytky[:8])))
        if not jm.startswith('na vysku'):
            texty = set(t for _, s in en for t in s['texty'])
            hledane = ('MOUSE', 'PLAYSTATION MOUSE', 'HOLDING THE LEFT BUTTON', 'MOUSE IN PORT 1 (DUNE 2000)',
                       'Game: SLES02250 – MOUSE PORT 1 + CONTROLLER PORT 2', 'Speed: 55 %', 'No game running yet – MOUSE IN PORT 1',
                       'MOUSE OFF – controller again', 'MOUSE: the PS1 core does not answer')
            chybi = [h for h in hledane if h not in texty]
            zap = [t for t in texty if t == 'MOUSE ON (MOUSE IN PORT 1) – drag your finger over the picture, tap = click. Settings: gear → PLAYSTATION MOUSE']
            over(not chybi and zap, 'anglicky: tlacitko MOUSE, panel, ukazatel, hlasky (%s)' % (', '.join(chybi) if chybi else 'vse prelozeno'))
    with open(os.path.join(ZDE, 'VYSTUP_texty_ps1_mys_en.txt'), 'w', encoding='utf-8') as f:
        f.write('B317 - anglicke texty stranky PS1 (vsechny stavy, i mys), pro kontrolu ocima\n')
        for t in dict.fromkeys(vse_en):
            f.write(t + '\n')


def main():
    prac = sys.argv[1]
    subprocess.run(['g++', '-std=c++17', '-O2', '-o', os.path.join(prac, 'prehraj_dotyky'), os.path.join(ZDE, 'prehraj_dotyky.cpp')], check=True)
    srv, zaklad = H.server()
    tmp = tempfile.mkdtemp(prefix='b317_str_')
    subprocess.run('git -C "%s" archive %s app/src/main/assets | tar -x -C "%s"' % (H.KOREN, B316, tmp), shell=True, check=True)
    srv0, zaklad0 = H.server(os.path.join(tmp, 'app/src/main/assets'))
    chyby = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        test_mys(b, zaklad, prac, chyby)
        test_na_vysku(b, zaklad, chyby)
        over(not chyby, 'gesta a panel: zadna chyba JavaScriptu%s' % ('' if not chyby else ' - ' + ' | '.join(chyby[:4])))
        test_jazyk(b, zaklad, zaklad0)
        b.close()
    srv.shutdown(); srv0.shutdown()
    vystup.insert(0, 'B317 - stranka PS1 s mysi (Chromium, skutecne dotyky, logika mysi z appky)')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b317_stranka.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
