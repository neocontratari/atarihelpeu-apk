#!/usr/bin/env python3
"""B343: NAPOVEDA a navody v appce v Chromiu (skutecne stranky, falesne mosty Javy jako ../b314/jazyk_harness).

  Z1-Z3) zapati navodu: s mostem AHJAZYK.verze() "ATARIHELP.EU · B343 (386)" (CZ i EN), bez verze jen
         "ATARIHELP.EU" (zadne "undefined" ani stare "B322")
  Z4)    nove caste otazky (otoceni, dalkovy ovladac): odkazy dojedou na Obrazovka / WEB TV
  Z5)    hledani "úložiště" / "storage" najde novy blok v OPTIONS, "L3" najde PS1 i WEB TV
  P1-P3) PS1 na vysku: kolecko -> panel s texty B343, tlacitko HRY ZE SERVERU N&P -> AHNET.openPs1Games;
         anglicky bez zbytku cestiny, GAMES FROM THE N&P SERVER
  S1-S2) SEGA na vysku (360x800, 412x915, 393x873): NAVODY -> kratky navod na obrazovce konzole, CELY videt
         (do B342 useknuty), anglicky prelozeny a taky cely
  S3)    SEGA menu vykonu (podrzene SBÍRKA): poznamka "pri VÝKON ZAŘÍZENÍ LOW vzdy LOW" videt, CZ i EN
  J)     zadna chyba JavaScriptu
Pouziti (z test_overeni/b343): python3 test_b343_napoveda.py [PRAC]  -> VYSTUP_test_b343_napoveda.txt (+ obrazky v PRAC)
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b343')
os.makedirs(PRAC, exist_ok=True)

VERZE = "window.AHJAZYK.verze = function () { return 'EMU10-B343-HELP-KONTROLA|386'; };"

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def pockej_na_klid(pg, max_ms=6000):
    pg.wait_for_timeout(150)
    pg.evaluate("() => { window.__klidY = -1; window.__klidN = 0; }")
    pg.wait_for_function("""() => { const y = window.scrollY;
        if (window.__klidY === y) window.__klidN++; else { window.__klidY = y; window.__klidN = 0; }
        return window.__klidN >= 6; }""", polling=50, timeout=max_ms)


chybyJs = []


def otevri(b, zaklad, stranka, jazyk='cs', w=412, h=915, verze=True, dotyk=True, mock=None):
    ctx = b.new_context(viewport={'width': w, 'height': h}, has_touch=dotyk, is_mobile=dotyk, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, mock or {}))
    if verze:
        ctx.add_init_script(VERZE)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(stranka + ': ' + str(e).split('\n')[0][:200]))
    pg.goto(zaklad + stranka)
    pg.wait_for_timeout(1200)
    return ctx, pg


srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()

    # ---------------- Z) navod ----------------
    for jazyk, st in (('cs', 'napoveda/index.html'), ('en', 'napoveda/en.html')):
        ctx, pg = otevri(b, zaklad, st, jazyk)
        zap = pg.evaluate("document.querySelector('footer.zapati').innerText.replace(/\\s+/g,' ').trim()")
        over(pg.url.endswith(st) and zap.endswith('ATARIHELP.EU · B343 (386)') and 'B322' not in zap,
             'Z1) %s zapati s verzi z appky: "%s"' % (jazyk.upper(), zap))
        pg.evaluate("document.querySelector('footer.zapati').scrollIntoView()")
        pg.wait_for_timeout(300)
        pg.screenshot(path=os.path.join(PRAC, 'navod_%s_zapati.png' % jazyk))
        # Z4 nove caste otazky
        q = 'Jak otočím obrazovku na šířku nebo na výšku?' if jazyk == 'cs' else 'How do I turn the screen to landscape or portrait?'
        pg.evaluate("q => { const s = [...document.querySelectorAll('summary')].find(x => x.textContent === q); s.scrollIntoView(); s.click(); }", q)
        pg.wait_for_timeout(300)
        pg.screenshot(path=os.path.join(PRAC, 'navod_%s_otazka_otoceni.png' % jazyk))
        pg.evaluate("q => [...document.querySelectorAll('summary')].find(x => x.textContent === q).parentNode.querySelector('a[href=\"#obrazovka\"]').click()", q)
        pockej_na_klid(pg)
        top = pg.evaluate("document.getElementById('obrazovka').getBoundingClientRect().top")
        lista = pg.evaluate("document.querySelector('.lista').offsetHeight")
        over(0 <= top - lista <= 30, 'Z4) %s otazka "%s" -> odkaz dojede na Obrazovka na vysku a na sirku (top %.0f, lista %d)' % (jazyk.upper(), q, top, lista))
        q2 = 'Jak hraju na televizi jen dálkovým ovladačem?' if jazyk == 'cs' else 'How do I play on a TV with just the remote?'
        pg.evaluate("q => { const s = [...document.querySelectorAll('summary')].find(x => x.textContent === q); s.scrollIntoView(); s.click(); }", q2)
        pg.wait_for_timeout(300)
        pg.evaluate("q => [...document.querySelectorAll('summary')].find(x => x.textContent === q).parentNode.querySelector('a[href=\"#tv\"]').click()", q2)
        pockej_na_klid(pg)
        top = pg.evaluate("document.getElementById('tv').getBoundingClientRect().top")
        over(0 <= top - lista <= 30, 'Z4) %s otazka "%s" -> odkaz dojede na WEB TV (top %.0f)' % (jazyk.upper(), q2, top))
        # Z5 hledani
        for slovo, kap in ((('úložiště', 'options'), ('L3', 'ps1')) if jazyk == 'cs' else (('storage', 'options'), ('L3', 'ps1'))):
            pg.fill('#hledat', slovo)
            pg.wait_for_timeout(500)
            vid = pg.evaluate("[...document.querySelectorAll('section.kap')].filter(s => !s.classList.contains('skryto')).map(s => s.id)")
            over(kap in vid and (slovo != 'L3' or 'tv' in vid), 'Z5) %s hledani "%s" najde %s' % (jazyk.upper(), slovo, ', '.join(vid)))
        pg.fill('#hledat', '')
        pg.wait_for_timeout(300)
        # snimky novych bloku
        for co, sel in (('vykon', "#options .blok:nth-last-of-type(3)"), ('tv_menu', '#tv')):
            pg.evaluate("s => { const e = document.querySelector(s); if (e) e.scrollIntoView(); }", sel)
            pg.wait_for_timeout(250)
            pg.screenshot(path=os.path.join(PRAC, 'navod_%s_%s.png' % (jazyk, co)))
        ctx.close()
    # Z2 bez verze v mostu (starsi appka / stranka mimo appku)
    for jazyk, st in (('cs', 'napoveda/index.html'), ('en', 'napoveda/en.html')):
        ctx, pg = otevri(b, zaklad, st, jazyk, verze=False)
        zap = pg.evaluate("document.querySelector('footer.zapati').innerText.replace(/\\s+/g,' ').trim()")
        over(zap.endswith('ATARIHELP.EU') and 'undefined' not in zap, 'Z2) %s bez AHJAZYK.verze: zapati "%s"' % (jazyk.upper(), zap))
        ctx.close()

    # ---------------- P) PS1 panel ----------------
    for jazyk in ('cs', 'en'):
        ctx, pg = otevri(b, zaklad, 'emu_ps1/index.html', jazyk, w=412, h=846)
        pg.click('#btnSettings')
        pg.wait_for_timeout(500)
        otevren = pg.evaluate("document.getElementById('helpPanel').classList.contains('open')")
        txt = pg.evaluate("document.querySelector('#helpPanel .box').innerText")
        tl = pg.evaluate("document.getElementById('psGooglePage').innerText.trim()")
        pg.screenshot(path=os.path.join(PRAC, 'ps1_panel_%s.png' % jazyk))
        if jazyk == 'cs':
            over(otevren and 'hry ze serveru N&P' in txt and 'Google' not in txt and 'Drive' not in txt and 'Úplný návod' in txt,
                 'P1) PS1 na vysku: kolecko -> panel, LOAD GAME = hry ze serveru N&P, zadny Google Drive, odkaz na HELP')
            over(tl == 'HRY ZE SERVERU N&P', 'P1) tlacitko "%s" (do B342 PS1 GOOGLE DRIVE)' % tl)
            pg.click('#psGooglePage')
            pg.wait_for_timeout(300)
            vol = pg.evaluate("window.__napVolani.slice()")
            over('AHNET.openPs1Games' in vol, 'P2) klik -> AHNET.openPs1Games (seznam her N&P pro PS1)')
        else:
            panel = txt.split('PS1 TEST LOG')[0] + ' ' + tl
            zb = H.ceska_slova(panel)
            over(otevren and not zb and 'games from the N&P server' in txt and tl == 'GAMES FROM THE N&P SERVER',
                 'P3) PS1 anglicky: panel bez cestiny, GAMES FROM THE N&P SERVER%s' % ((' - CESKY: %s' % zb) if zb else ''))
        ctx.close()

    # ---------------- S) SEGA NAVODY ----------------
    for jazyk in ('cs', 'en'):
        for (w, h) in ((360, 800), (412, 915), (393, 873)):
            ctx, pg = otevri(b, zaklad, 'emu_sega/index.html', jazyk, w=w, h=h)
            pg.click('#btnManuals')
            pg.wait_for_timeout(500)
            m = pg.evaluate("""() => { const t = document.getElementById('monitorText'), o = document.getElementById('monitorOverlay');
                return {vid: o.classList.contains('show') && getComputedStyle(o).display !== 'none', text: t.textContent,
                        sh: t.scrollHeight, ch: t.clientHeight, sw: t.scrollWidth, cw: t.clientWidth}; }""")
            if (w, h) == (412, 915):
                pg.screenshot(path=os.path.join(PRAC, 'sega_navody_%s.png' % jazyk))
            cely = m['sh'] <= m['ch'] + 1 and m['sw'] <= m['cw'] + 1
            if jazyk == 'cs':
                over(m['vid'] and m['text'].startswith('SEGA – KRÁTKÝ NÁVOD') and cely,
                     'S1) %dx%d NAVODY: kratky navod, cely videt (text %d px / okno %d px)' % (w, h, m['sh'], m['ch']))
            else:
                zb = H.ceska_slova(m['text'], povolena=('sbirka', 'hry', 'ulozene', 'navody'))
                over(m['vid'] and m['text'].startswith('SEGA – SHORT GUIDE') and cely and not zb,
                     'S2) %dx%d anglicky: SHORT GUIDE, cely videt (%d / %d px)%s' % (w, h, m['sh'], m['ch'], (' - CESKY: %s' % zb) if zb else ''))
            ctx.close()

    # S4) obrazovka konzole hned po otevreni (bez hry): kratky prehled tlacitek, cely videt
    for jazyk, zac in (('cs', 'SEGA MEGA DRIVE – PŘIPRAVENO'), ('en', 'SEGA MEGA DRIVE – READY')):
        ctx, pg = otevri(b, zaklad, 'emu_sega/index.html', jazyk, w=360, h=800)
        m = pg.evaluate("""() => { const t = document.getElementById('monitorText'), o = document.getElementById('monitorOverlay');
            return {vid: o.classList.contains('show'), text: t.textContent, sh: t.scrollHeight, ch: t.clientHeight}; }""")
        zb = H.ceska_slova(m['text']) if jazyk == 'en' else []
        over(m['vid'] and m['text'].startswith(zac) and 'SBIRKA=vykon' not in m['text'] and m['sh'] <= m['ch'] + 1 and not zb,
             'S4) %s obrazovka Segy bez hry: "%s" (cely videt%s)' % (jazyk.upper(), m['text'].replace('\n', ' | '), (' - CESKY: %s' % zb) if zb else ''))
        ctx.close()

    # S3) menu vykonu Segy (podrzene SBÍRKA): poznamka, ze VÝKON ZAŘÍZENÍ LOW drzi Segu na LOW
    for jazyk, ocek in (('cs', 'Podržené SBÍRKA = výkon. Při OPTIONS → VÝKON ZAŘÍZENÍ LOW hraje Sega vždy LOW.'),
                        ('en', 'Hold CATALOG = performance. With OPTIONS → DEVICE PERFORMANCE LOW the Sega always plays LOW.')):
        ctx, pg = otevri(b, zaklad, 'emu_sega/index.html', jazyk)
        pg.evaluate("(() => { const m = document.getElementById('perfMenu'); m.classList.add('show'); m.setAttribute('aria-hidden','false'); })()")
        pg.wait_for_timeout(300)
        r = pg.evaluate("""() => { const n = document.querySelector('#perfMenu .pmNote'), r = n.getBoundingClientRect();
            return {t: n.textContent, vid: r.width > 0 && r.height > 0 && getComputedStyle(n).visibility !== 'hidden',
                    cely: n.scrollWidth <= n.clientWidth + 1 && r.right <= innerWidth + 1}; }""")
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'sega_menu_vykonu.png'))
        over(r['t'] == ocek and r['vid'] and r['cely'], 'S3) %s menu vykonu: "%s"' % (jazyk.upper(), r['t']))
        ctx.close()

    b.close()
srv.shutdown()

over(not chybyJs, 'J) zadna chyba JavaScriptu' + (': %s' % chybyJs[:3] if chybyJs else ''))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b343_napoveda.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
