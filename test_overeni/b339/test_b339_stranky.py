#!/usr/bin/env python3
"""B339: ULOZISTE V OPTIONS A PS1 HRY V TELEVIZI (Chromium, skutecne stranky z assets, falesne mosty).

Rene (TV TCL, B338): "Jinak PS1 jsem nezkousel - nevim jakou ma kapacitu disk v tv".

  T1) OPTIONS -> ULOZISTE (cesky): popisek, veta z Javy (AHRENDER.uloziste) - a pri DALSIM otevreni OPTIONS
      nova hodnota (po stazeni / smazani hry se volno meni),
  T2) anglicky: popisek "STORAGE", veta z Javy uz anglicky (prekladac stranky na ni nesaha),
  T3) most nic nevrati (starsi appka / chyba) -> "Uloziste nejde zjistit" / "Storage cannot be read",
  T4) PS1 na TV (1280x720 na sirku): HRY V TELEVIZI -> SPUSTIT vola z Javy primo ps1LibraryLaunchByKey (jako ▶
      v panelu CD ISO) - funkce stranky jsou uvnitr (function(){...})(), JavaScriptem zvenku nejdou zavolat.
      Nahravaci obrazovku si stranka zapne sama podle stavu z Javy "PS1_REMOTE_BOOTING #cislo jmeno"
      (AHPS1.ps1RemoteStatus) a po startu (AHPS1.ps1StartStav = 1) ji schova,
  T5) telefon na vysku: panel CD ISO s daty v podobe B339 (nazev bez pripony / z katalogu N&P, velikost cele
      slozky hry) - nazev a velikost vypsane, ▶ spusti hru mostem AHPS1.ps1LibraryLaunch s klicem,
  T6) zadna chyba JavaScriptu.
Pouziti (z test_overeni/b339): python3 test_b339_stranky.py   -> VYSTUP_test_b339_stranky.txt
"""
import json
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

KLIC = 'nap_ps1-0123456789abcdef'
STITEK = KLIC + '__Crash Bandicoot.cue'     # bootPs1FileOnCurrentThread: slozka hry + "__" + soubor

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def main():
    srv, z = H.server()
    chyby = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()

        def stranka(url, mock=None, jazyk='cs', sirka=1280, vyska=720):
            ctx = b.new_context(viewport={'width': sirka, 'height': vyska})
            ctx.add_init_script(H.init_skript(jazyk, mock or {}))
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chyby.append(url + ': ' + str(e).split('\n')[0][:160]))
            pg.goto(z + url)
            pg.wait_for_timeout(1300)
            return ctx, pg

        STAV = ("() => { var h=document.getElementById('stUloziste'); return [h.previousElementSibling.textContent.trim(), h.textContent,"
                " getComputedStyle(h).display!=='none' && h.getBoundingClientRect().height>0]; }")

        # ---- T1 + T2: OPTIONS -> ULOZISTE ----
        vety = {
            'cs': ['Volno 2,1 GB z 4,5 GB · PS1 hry v televizi: 0 · stažení PS1 hry chce volno asi 2× její velikost + 200 MB',
                   'Volno 1,4 GB z 4,5 GB · PS1 hry v televizi: 1 (650 MB) · stažení PS1 hry chce volno asi 2× její velikost + 200 MB'],
            'en': ['2.1 GB free of 4.5 GB · PS1 games on this TV: 0 · downloading a PS1 game needs about 2× its size + 200 MB free',
                   '1.4 GB free of 4.5 GB · PS1 games on this TV: 1 (650 MB) · downloading a PS1 game needs about 2× its size + 200 MB free'],
        }
        vysl = {}
        for jaz in ('cs', 'en'):
            fn = ("fn:window.__ulN=(window.__ulN||0)+1;var v=%s;return JSON.stringify({volno:1,celkem:2,ps1Her:0,ps1Bajtu:0,"
                  "popis:v[Math.min(window.__ulN,2)-1]});" % json.dumps(vety[jaz]))
            ctx, pg = stranka('index.html', {'AHRENDER.uloziste': fn}, jaz)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            prvni = pg.evaluate(STAV)
            pg.evaluate('() => window.napZavriNastaveni()')
            pg.wait_for_timeout(200)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            druhe = pg.evaluate(STAV)
            vysl[jaz] = (prvni, druhe)
            ctx.close()
        (c1, c2), (e1, e2) = vysl['cs'], vysl['en']
        over(c1[0] == 'ÚLOŽIŠTĚ' and c1[1] == vety['cs'][0] and c1[2] and c2[1] == vety['cs'][1],
             'T1) OPTIONS %s: "%s" -> po dalsim otevreni "%s"' % (c1[0], c1[1][:40], c2[1][:40]))
        over(e1[0] == 'STORAGE' and e1[1] == vety['en'][0] and e2[1] == vety['en'][1],
             'T2) EN %s: "%s" (veta z Javy, prekladac na ni nesaha)' % (e1[0], e1[1][:44]))

        # ---- T3: most nic nevrati ----
        nic = []
        for jaz in ('cs', 'en'):
            ctx, pg = stranka('index.html', {}, jaz)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            nic.append(pg.evaluate(STAV)[1])
            ctx.close()
        over(nic == ['Úložiště nejde zjistit', 'Storage cannot be read'], 'T3) bez udaju z Javy: %s' % nic)

        # ---- T4: PS1 na TV - HRY V TELEVIZI -> SPUSTIT (Java) -> nahravaci obrazovka podle stavu ----
        mock_ps1 = {'AHPS1.ps1LibraryLaunch': 'fn:window.__klic=args[0];return "";', 'AHPS1.ps1StartStav': '0|',
                    'AHPS1.ps1RemoteStatus': '-'}
        NAHR = "() => { var o=document.getElementById('psMonitorLoading'); return o ? [o.style.display, o.textContent] : null; }"
        ctx, pg = stranka('emu_ps1/index.html', mock_ps1)
        globalni = pg.evaluate("() => [typeof window.nahravaniStart, typeof window.ensurePs1ScreenAndPoll]")
        pred = pg.evaluate(NAHR)
        # Java: ps1LibraryLaunchByKey -> bootPs1FileOnCurrentThread -> stav "PS1_REMOTE_BOOTING #1 <stitek>"
        pg.evaluate("(s) => { NAP_MOCK['AHPS1.ps1RemoteStatus'] = s; }", 'PS1_REMOTE_BOOTING #1 ' + STITEK)
        pg.wait_for_timeout(900)
        behem = pg.evaluate(NAHR)
        # start dokoncen (ps1StartDokoncen: pocet startu 1)
        pg.evaluate("(s) => { NAP_MOCK['AHPS1.ps1StartStav'] = '1|OK ' + s; NAP_MOCK['AHPS1.ps1RemoteStatus'] = 'PS1_REMOTE_BOOT_OK ' + s; }", STITEK)
        pg.wait_for_timeout(1200)
        po = pg.evaluate(NAHR)
        ctx.close()
        over(globalni == ['undefined', 'undefined'] and (pred is None or pred[0] == 'none')
             and behem and behem[0] == 'flex' and 'NAHRÁVÁM HRU' in behem[1] and 'Crash Bandicoot' in behem[1]
             and po and po[0] == 'none',
             'T4) TV SPUSTIT z Javy: funkce stranky zvenku %s (proto Java, ne JavaScript); stav BOOTING -> nahravani "%s" %s,'
             ' po startu %s' % (globalni, (behem[1][:44] if behem else None), behem and behem[0], po and po[0]))

        # ---- T5: telefon - panel CD ISO s daty B339 ----
        data = json.dumps([{'key': KLIC, 'nazev': 'Crash Bandicoot', 'bytes': 650 * 1024 * 1024, 'ma_ulozenou_hru': True},
                           {'key': 'url_abc', 'nazev': 'Wipeout XL (Track 01)', 'bytes': 420 * 1024 * 1024, 'ma_ulozenou_hru': False}])
        mock_tel = dict(mock_ps1)
        mock_tel['AHPS1.ps1LibraryList'] = data
        ctx, pg = stranka('emu_ps1/index.html', mock_tel, 'cs', 412, 915)
        pg.evaluate("() => document.getElementById('btnCd').click()")
        pg.wait_for_timeout(500)
        radky = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#libraryList .libGame'), function(r){"
                            "return r.querySelector('.libName').textContent+' | '+r.querySelector('.libMeta').textContent+"
                            "(r.querySelector('.libSaveTag')?' | pozice':'');})")
        pg.evaluate("() => document.querySelector('#libraryList .libPlay').click()")
        pg.wait_for_timeout(300)
        klic2 = pg.evaluate('() => window.__klic')
        ctx.close()
        over(radky == ['Crash Bandicoot | 650.0 MB | pozice', 'Wipeout XL (Track 01) | 420.0 MB'] and klic2 == KLIC,
             'T5) telefon CD ISO: %s; ▶ -> AHPS1.ps1LibraryLaunch(%s) - stejne jako TV' % (radky, klic2))

        over(not chyby, 'T6) zadna chyba JavaScriptu%s' % ('' if not chyby else ': ' + '; '.join(chyby[:3])))
        b.close()
    srv.shutdown()
    vystup.append('VYSLEDEK: %d testu, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b339_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
