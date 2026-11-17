#!/usr/bin/env python3
"""B338: ANDROID TV - MENU KONZOLE SEGA / PS1 A VYKON ZARIZENI (Chromium, skutecne stranky z assets, falesne mosty).

Rene (TV TCL, B337): "furt stejny co se tyce vyberu her primo v sega a ps1 ale slo to obejit pres atari ... primo na tv
nehratelny kouse se obraz i zvuk uz i u sonica ... budeme muset do nastaveni pridat neco jako low pro slabe procesory".

  T1) SEGA na TV (1280x720 na sirku): dalkovym ovladacem (zlaty ramecek B334) jde jen SAVE / LOAD GAME / ⚙ -
      zadna hra (SBIRKA, HRY jsou na sirku neviditelna mista na obrazku konzole) a "LOAD GAME" nacita POZICI,
      ne hru = proc se ze Segy na TV k hrам nedalo dostat (B338: menu konzole na ZPET - Java),
  T2) PS1 na TV: ramecek jen ⚙ / MYS / MONITOR - LOAD GAME, CD ISO, SAVE GAME na sirku nejsou,
  T3) menu konzole klepe na tlacitka stranky presne JavaScriptem z NapPravidla.tvKlik: SEGA ulozit / nacist pozici
      -> AHSega.segaSaveState / segaLoadState, PS1 SAVE GAME / nacist / RESET -> AHPS1.ps1QuickSave / ps1QuickLoad /
      ps1Reset (i kdyz jsou tlacitka na sirku schovana), chybejici tlacitko -> 'chybi',
  T4) OPTIONS -> VYKON ZARIZENI: AUTO / LOW / HIGH, veta z Javy (AHRENDER.vykon), klepnuti -> AHRENDER.nastavVykon('l'),
      vybrane tlacitko a nova veta; EN popisek "DEVICE PERFORMANCE",
  T5) zadna chyba JavaScriptu.
Pouziti (z test_overeni/b338): python3 test_b338_tv_stranky.py   -> VYSTUP_test_b338_tv_stranky.txt
"""
import json
import os
import re
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
PRAV = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/NapPravidla.java'), encoding='utf-8').read()
# presne JavaScript z Javy (NapPravidla.tvKlik): "...getElementById('" + id + "')..."
m = re.search(r'static String tvKlik\(String id\) \{\s*return "(.*?)" \+ id \+ "(.*?)";', PRAV, re.S)
JS_A, JS_B = m.group(1).replace('\\"', '"'), m.group(2).replace('\\"', '"')


def tv_klik(id_):
    return JS_A + id_ + JS_B


vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


KAND = ("() => NapTvOvladani.kandidati().map(function(f){return (f.id||f.tagName)+' \"'+(f.textContent||'').trim().slice(0,12)+'\"';})")


def main():
    srv, z = H.server()
    chyby = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()

        def stranka(url, mock=None, jazyk='cs'):
            ctx = b.new_context(viewport={'width': 1280, 'height': 720})
            ctx.add_init_script(H.init_skript(jazyk, mock or {}))
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chyby.append(url + ': ' + str(e).split('\n')[0][:160]))
            pg.goto(z + url)
            pg.wait_for_timeout(1300)
            return ctx, pg

        # ---- T1: SEGA na TV ----
        ctx, pg = stranka('emu_sega/index.html', {'AHSega.segaSaveState': 'OK', 'AHSega.segaLoadState': 'OK'})
        # TV: hraje dalkovy / ovladac -> dotykove ovladani schovane (B320 napOvladacStav), pak ramecek (B334)
        pg.evaluate("() => { try { window.napOvladacStav && napOvladacStav(2, ''); } catch (e) {} }")
        pg.evaluate('() => NapTvOvladani.zapni()')
        kand = pg.evaluate(KAND)
        hry = [k for k in kand if re.search(r'btnCollection|btnGames|HRY|SBÍRKA', k)]
        load = [k for k in kand if 'segaBottomLoad' in k]
        over(not hry and load,
             'T1) SEGA na TV: ramecek jen %s - zadna hra (SBIRKA / HRY na sirku neviditelne), LOAD GAME = nacist POZICI' % kand)

        # ---- T3a: SEGA ulozit / nacist pozici z menu konzole ----
        r1 = pg.evaluate(tv_klik('segaBtnSaveState'))
        r2 = pg.evaluate(tv_klik('segaBtnLoadState'))
        r3 = pg.evaluate(tv_klik('neexistuje'))
        v = pg.evaluate('() => __napVolani.filter(function(x){return /AHSega\\.sega(Save|Load)State/.test(x);})')
        ctx.close()

        # ---- T2: PS1 na TV ----
        ctx, pg = stranka('emu_ps1/index.html', {'AHPS1.ps1QuickSave': 'OK', 'AHPS1.ps1QuickLoad': 'OK', 'AHPS1.ps1Reset': 'OK'})
        pg.evaluate("() => { try { window.napOvladacStav && napOvladacStav(2, ''); } catch (e) {} }")
        pg.evaluate('() => NapTvOvladani.zapni()')
        kand2 = pg.evaluate(KAND)
        hry2 = [k for k in kand2 if re.search(r'btnLoad|btnCd|btnSave|LOAD GAME|CD ISO', k)]
        over(not hry2, 'T2) PS1 na TV: ramecek jen %s - LOAD GAME / CD ISO / SAVE GAME na sirku nejsou' % kand2)

        # ---- T3b: PS1 z menu konzole ----
        p1 = pg.evaluate(tv_klik('btnSave'))
        p2 = pg.evaluate(tv_klik('btnQuickLoad'))
        p3 = pg.evaluate(tv_klik('btnReset'))
        pg.wait_for_timeout(200)
        v2 = pg.evaluate('() => __napVolani.filter(function(x){return /AHPS1\\.ps1(QuickSave|QuickLoad|Reset)/.test(x);})')
        ctx.close()
        over(r1 == 'ok' and r2 == 'ok' and r3 == 'chybi' and v == ['AHSega.segaSaveState', 'AHSega.segaLoadState']
             and p1 == 'ok' and p2 == 'ok' and p3 == 'ok'
             and v2 == ['AHPS1.ps1QuickSave', 'AHPS1.ps1QuickLoad', 'AHPS1.ps1Reset'],
             'T3) menu konzole (NapPravidla.tvKlik): SEGA %s, PS1 %s, chybejici tlacitko -> %s' % (v, v2, r3))

        # ---- T4: OPTIONS -> VYKON ZARIZENI ----
        auto = json.dumps({'rezim': 'a', 'low': True, 'slabe': True,
                           'popis': 'AUTO: tohle zařízení je SLABÉ (jen 32 bitu, 4 jader, max 1100 MHz) – LOW · SEGA kreslí grafikou, bez úvodního dema (projeví se u další hry)'})
        nastav = ("fn:window.__vykonArg=args[0];return JSON.stringify({rezim:args[0],low:args[0]!=='h',slabe:true,"
                  "popis:(args[0]==='l'?'LOW: vždy (slabý procesor)':'HIGH: vždy (plná kvalita)')+' (projeví se u další hry)'});")
        vysl = []
        for jaz in ('cs', 'en'):
            ctx, pg = stranka('index.html', {'AHRENDER.vykon': auto, 'AHRENDER.nastavVykon': nastav}, jaz)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            stav = lambda: pg.evaluate("() => [['stVykonAuto','stVykonLow','stVykonHigh'].filter(function(i){return document.getElementById(i).classList.contains('active');}),"
                                       " document.getElementById('stVykonHint').textContent, document.querySelector('#stVykon').previousElementSibling.textContent]")
            pred = stav()
            pg.click('#stVykonLow')
            pg.wait_for_timeout(150)
            po = stav()
            arg = pg.evaluate('() => window.__vykonArg')
            vysl.append((pred, po, arg))
            ctx.close()
        (cp, cpo, carg), (ep, epo, earg) = vysl
        over(cp[0] == ['stVykonAuto'] and 'SLABÉ' in cp[1] and cp[2].strip() == 'VÝKON ZAŘÍZENÍ'
             and carg == 'l' and cpo[0] == ['stVykonLow'] and cpo[1].startswith('LOW: vždy')
             and ep[2].strip() == 'DEVICE PERFORMANCE' and earg == 'l',
             'T4) OPTIONS VYKON ZARIZENI: pred %s "%s" -> LOW: nastavVykon(%s), %s "%s"; EN "%s"'
             % (cp[0], cp[1][:46], carg, cpo[0], cpo[1][:30], ep[2].strip()))

        over(not chyby, 'T5) zadna chyba JavaScriptu%s' % ('' if not chyby else ': ' + '; '.join(chyby[:3])))
        b.close()
    srv.shutdown()
    vystup.append('VYSLEDEK: %d testu, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b338_tv_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
