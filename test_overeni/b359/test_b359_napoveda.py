#!/usr/bin/env python3
"""B359: NAPOVEDA - sekce CO JE NOVEHO nahore (Chromium, stranky presne z appky: assets/napoveda/index.html a en.html).
Texty bere z assets/novinky/novinky.js (stejny soubor jako okno po aktualizaci v appce - nic se nepise dvakrat).
Kontroly: sekce je prvni pod hlavickou (pred RYCHLY START), nadpis, vsechny zaznamy se stejnymi vetami jako v souboru,
nejnovejsi zvyrazneny; anglicky en.html totez z "en"; pri hledani v navodu se schova (jako RYCHLY START); bez souboru
novinek sekce neni videt (a nic nespadne); zadna chyba JavaScriptu.
Pouziti: python3 test_b359_napoveda.py [slozka pro snimky]   -> VYSTUP_test_b359_napoveda.txt
"""
import json
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

SNIMKY = sys.argv[1] if len(sys.argv) > 1 else ''
ASSETS = os.path.normpath(os.path.join(ZDE, '..', '..', 'app', 'src', 'main', 'assets'))
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


def novinky():
    js = open(os.path.join(ASSETS, 'novinky', 'novinky.js'), encoding='utf-8').read()
    i = js.index('NAP_NOVINKY')
    return json.loads(js[js.index('[', i):js.rindex(']') + 1])


def sekce(pg):
    return pg.evaluate('''() => {
        var s = document.getElementById("novinky"); if (!s) return null;
        var r = s.getBoundingClientRect(), q = document.querySelector("section.rychle:not(.novinky)");
        return {vidi: !s.hidden && getComputedStyle(s).display !== "none" && r.height > 0,
                nadpis: s.querySelector("h2").textContent,
                pred_rychlym: !!q && (s.compareDocumentPosition(q) & Node.DOCUMENT_POSITION_FOLLOWING) > 0,
                hlavy: Array.prototype.map.call(s.querySelectorAll(".novinka-hlava"), function (e) { return e.textContent; }),
                prvni: (s.querySelector(".novinka") || {}).className,
                body: Array.prototype.map.call(s.querySelectorAll(".novinka"), function (n) {
                    return Array.prototype.map.call(n.querySelectorAll("li"), function (l) { return l.textContent; }); })};
    }''')


def main():
    srv, url = H.server()
    data = novinky()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        for jazyk, stranka, nadpis in (('cs', 'napoveda/index.html', 'CO JE NOVÉHO'), ('en', 'napoveda/en.html', "WHAT'S NEW")):
            ctx = b.new_context(viewport={'width': 412, 'height': 846}, is_mobile=True, has_touch=True)
            ctx.add_init_script(H.init_skript(jazyk, {}))
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
            pg.goto(url + stranka)
            pg.wait_for_timeout(800)
            s = sekce(pg)
            ocek_hlavy = ['%s · %s' % (z['build'], z['datum']) for z in data[:6]]
            ocek_body = [z[jazyk] for z in data[:6]]
            over(s is not None and s['vidi'] and s['nadpis'] == nadpis and s['pred_rychlym'] and s['hlavy'] == ocek_hlavy
                 and s['body'] == ocek_body and 'nejnovejsi' in (s['prvni'] or ''),
                 '%s: sekce "%s" nahore (pred RYCHLY START: %s), %d verzi %s, vety presne z novinky.js, nejnovejsi zvyrazneny'
                 % (stranka, s and s['nadpis'], s and s['pred_rychlym'], len(s['hlavy']) if s else 0, s and s['hlavy']))
            if SNIMKY:
                os.makedirs(SNIMKY, exist_ok=True)
                pg.screenshot(path=os.path.join(SNIMKY, 'napoveda_%s.png' % jazyk))
            pg.fill('#hledat', 'online')
            pg.wait_for_timeout(500)
            skryto = pg.evaluate('() => getComputedStyle(document.getElementById("novinky")).display === "none"')
            over(skryto, '   pri hledani v navodu se sekce schova (jako RYCHLY START)')
            ctx.close()
        # bez souboru novinek: sekce neni videt, nic nespadne
        ctx = b.new_context(viewport={'width': 412, 'height': 846})
        ctx.add_init_script(H.init_skript('cs', {}))
        pg = ctx.new_page()
        pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
        pg.route('**/novinky/novinky.js', lambda r: r.fulfill(status=404, body=''))
        pg.goto(url + 'napoveda/index.html')
        pg.wait_for_timeout(600)
        s = sekce(pg)
        over(s is not None and not s['vidi'], 'bez novinky.js: sekce neni videt, navod jinak beze zmeny')
        ctx.close()
        b.close()
    over(not chyby, 'zadna chyba JavaScriptu' + ('' if not chyby else ': ' + ' | '.join(chyby[:5])))
    srv.shutdown()
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b359_napoveda.txt'), 'w', encoding='utf-8') as f:
        f.write('B359 - napoveda: sekce CO JE NOVEHO (Chromium, stranky z appky)\n' + '\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
