#!/usr/bin/env python3
"""B337: STRANKY APPKY NA TELEVIZI (Chromium, SKUTECNE stranky z assets, falesne mosty Javy).

Rene (TV TCL, Android 9): "u segy a ps1 to vyskoci zpet do hlavni nabidky - mozna spatne odemceno".
Zamcena karta SEGA / PS1 otevre stranku ODEMKNOUT (hry/odemknout.html). Pri nacteni je na ni videt jen
"◀ DO MENU" - appka zapne ramecek dalkoveho ovladace (NapTvOvladani.zapni) prave na nem. Server pak rekne
"zaplaceno" a stranka ukaze SPUSTIT SEGU, ale ramecek zustal na DO MENU: OK = zpet do hlavniho menu.

  T1) B336 (stranka z gitu ac42c22): server zaplaceno -> ramecek porad na DO MENU, OK = AHHRY.menu()
      (presne Reneho "vyskoci zpet do hlavni nabidky"),
  T2) B337: server zaplaceno -> ramecek na SPUSTIT SEGU, OK = AHHRY.spustKonzoli('sega'),
  T3) B337: PS1 (Sega otevrena, PS1 zaplacena na serveru) -> SPUSTIT PS1, OK = spustKonzoli('ps1'),
  T4) B337: zamceno, platebni udaje -> ramecek na ZAPLATIL JSEM (ne DO MENU); hlidani platby (stejna
      odpoved znovu) cloveku ramecek nebere (sel sipkou na DO MENU - zustane),
  T5) telefon (bez NapTvOvladani.zapni): zadny ramecek, stranka jako drive,
  T6) OPTIONS: SPUSTIT WEB TV -> pod adresou i adresa logu .../log (CZ i EN popisek),
  T7) N&P HRY: BIOS na serveru (zaloha pro STAHNOUT HRY A BIOS) neni v seznamu her PS1.
Pouziti (z test_overeni/b337): python3 test_b337_tv_stranky.py   -> VYSTUP_test_b337_tv_stranky.txt
"""
import http.server
import os
import subprocess
import sys
import threading

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
B336 = os.environ.get('NAP_B336', 'ac42c22')
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


STARA = subprocess.run(['git', '-C', KOREN, 'show', B336 + ':app/src/main/assets/hry/odemknout.html'],
                       capture_output=True, check=True).stdout


class Obsluha(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def do_GET(self):
        if self.path.split('?')[0] == '/hry/odemknout_b336.html':   # stara stranka vedle nove (stejne ../ cesty)
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(STARA)))
            self.end_headers()
            self.wfile.write(STARA)
            return
        return super().do_GET()


def server():
    port = H.volny_port()
    srv = http.server.ThreadingHTTPServer(('127.0.0.1', port), lambda *a, **k: Obsluha(*a, directory=H.ASSETS, **k))
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv, 'http://127.0.0.1:%d/' % port


# falesny most AHHRY (MainActivity.AHHry): odemknuti() hned, platba(co) za 300 ms do napHry.platba
MOST = r'''
(function(){
  window.__volani = [];
  window.__scenar = window.__scenar || {};
  function zapis(x){ window.__volani.push(x); }
  window.AHHRY = {
    odemknuti: function(){ zapis('odemknuti'); return JSON.stringify(__scenar.mistni || {}); },
    platba: function(co){ zapis('platba:' + co);
      setTimeout(function(){ if (window.napHry) napHry.platba(__scenar.platba(co)); }, 300); },
    spustKonzoli: function(co){ zapis('spustKonzoli:' + co); },
    menu: function(){ zapis('menu'); },
    zaplatil: function(co){ zapis('zaplatil:' + co); },
    zkontroluj: function(){ zapis('zkontroluj'); },
    odemknout: function(co){ zapis('odemknout:' + co); },
    kopiruj: function(){ zapis('kopiruj'); },
    ulozQr: function(){ zapis('ulozQr'); return ''; },
    stav: function(){ return JSON.stringify({server: 'http://192.0.2.10:8080', sparovano: true, zablokovano: false}); },
    seznam: function(){ zapis('seznam');
      setTimeout(function(){ if (window.napHry) napHry.seznam(__scenar.seznam); }, 200); },
    spust: function(s, id, f){ zapis('spust:' + s + ':' + f); },
    zpet: function(s){ zapis('zpet:' + s); },
    web: function(){ zapis('web'); }
  };
})();
'''

MISTNI_ZAMCENO = {'sega': False, 'ps1': False, 'segaKoupena': False, 'ps1Koupena': False, 'radku': 50, 'potrebaRadku': 1500,
                  'segaMs': 0, 'potrebaMs': 432000000, 'ceka': False, 'sparovano': True, 'jazyk': 'cs'}
# testovaci platebni udaje (vymyslene - zadny skutecny ucet)
PLATBA = {'nastaveno': True, 'koupeno': False, 'zadost': False, 'vs': '10061', 'kc': '199', 'eur': '8.00',
          'ucet_cz': '', 'iban': 'CZ0000000000000000000000', 'iban_hezky': 'CZ00 0000 0000 0000 0000 0000', 'bic': '',
          'jmeno': 'N&P TEST', 'zprava': 'N&P SEGA', 'spd': 'SPD*1.0*ACC:CZ0000000000000000000000*AM:199*CC:CZK*X-VS:10061',
          'epc': 'BCD\n002\n1\nSCT\n\nN&P TEST\nCZ0000000000000000000000\nEUR8.00'}


def scenar_js(nazev):
    import json
    m = json.dumps(MISTNI_ZAMCENO)
    p = json.dumps(PLATBA)
    if nazev == 'zaplaceno':      # spravce odemkl v okne serveru, appka o tom nevedela (Java to ted zapamatuje)
        return ('window.__scenar={mistni:%s,platba:function(co){var mi=JSON.parse(JSON.stringify(window.__scenar.mistni));'
                'mi.sega=true;mi.segaKoupena=true;if(co==="ps1"){mi.ps1=true;mi.ps1Koupena=true;}'
                'return {ok:true,kod:200,data:{koupeno:true,nastaveno:true},mistni:mi};}};' % m)
    if nazev == 'ps1zaplaceno':
        mi = dict(MISTNI_ZAMCENO, sega=True, radku=1500)
        return ('window.__scenar={mistni:%s,platba:function(co){var mi=JSON.parse(JSON.stringify(window.__scenar.mistni));'
                'mi.ps1=true;mi.ps1Koupena=true;return {ok:true,kod:200,data:{koupeno:true,nastaveno:true},mistni:mi};}};'
                % json.dumps(mi))
    if nazev == 'zamceno':
        return ('window.__scenar={mistni:%s,platba:function(co){return {ok:true,kod:200,data:%s,mistni:window.__scenar.mistni};}};'
                % (m, p))
    raise ValueError(nazev)


FOKUS = "() => { var f = window.NapTvOvladani && NapTvOvladani.fokus(); return f ? (f.id || f.tagName) : null; }"


def stranka(b, url, scenar, tv=True, jazyk='cs', chyby=None):
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    ctx.add_init_script(H.init_skript(jazyk, {}))
    ctx.add_init_script(scenar_js(scenar) if scenar else '')
    ctx.add_init_script(MOST)
    pg = ctx.new_page()
    if chyby is not None:
        pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(url)
    pg.wait_for_load_state('load')
    if tv:   # appka: onPageFinished -> napTvStrankaNactena -> NapTvOvladani.zapni()
        pg.evaluate('() => NapTvOvladani.zapni()')
    return ctx, pg


def main():
    srv, z = server()
    chyby = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()

        # ---- T1: B336 - ramecek zustane na DO MENU, OK = zpet do menu ----
        ctx, pg = stranka(b, z + 'hry/odemknout_b336.html?co=sega', 'zaplaceno', chyby=chyby)
        f0 = pg.evaluate(FOKUS)
        pg.wait_for_timeout(900)
        spustit = pg.evaluate("() => !document.getElementById('spustit').classList.contains('skryte')")
        f1 = pg.evaluate(FOKUS)
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(150)
        v = pg.evaluate('() => __volani')
        over(f0 == 'doMenu' and spustit and f1 == 'doMenu' and 'menu' in v and not any(x.startswith('spustKonzoli') for x in v),
             'T1) B336: pri nacteni ramecek na "%s", server zaplaceno -> SPUSTIT SEGU %s, ramecek porad na "%s", OK -> %s'
             ' (= Reneho "vyskoci zpet do hlavni nabidky")' % (f0, 'videt' if spustit else 'NEVIDET', f1, [x for x in v if x in ('menu',) or x.startswith('spust')]))
        ctx.close()

        # ---- T2: B337 - ramecek na SPUSTIT SEGU, OK = spustit Segu ----
        ctx, pg = stranka(b, z + 'hry/odemknout.html?co=sega', 'zaplaceno', chyby=chyby)
        f0 = pg.evaluate(FOKUS)
        pg.wait_for_timeout(900)
        f1 = pg.evaluate(FOKUS)
        txt = pg.evaluate("() => document.getElementById('spustit').textContent")
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(150)
        v = pg.evaluate('() => __volani')
        over(f1 == 'spustit' and 'spustKonzoli:sega' in v and 'menu' not in v,
             'T2) B337: pri nacteni "%s", server zaplaceno -> ramecek na "%s" (%s), OK -> %s'
             % (f0, f1, txt, [x for x in v if x == 'menu' or x.startswith('spust')]))
        ctx.close()

        # ---- T3: B337 - PS1 ----
        ctx, pg = stranka(b, z + 'hry/odemknout.html?co=ps1', 'ps1zaplaceno', chyby=chyby)
        pg.wait_for_timeout(900)
        f1 = pg.evaluate(FOKUS)
        txt = pg.evaluate("() => document.getElementById('spustit').textContent")
        pg.keyboard.press('Enter')
        pg.wait_for_timeout(150)
        v = pg.evaluate('() => __volani')
        over(f1 == 'spustit' and 'spustKonzoli:ps1' in v and 'menu' not in v,
             'T3) B337: PS1 zaplacena na serveru -> ramecek na "%s" (%s), OK -> %s' % (f1, txt, [x for x in v if x == 'menu' or x.startswith('spust')]))
        ctx.close()

        # ---- T4: B337 - zamceno: ZAPLATIL JSEM; hlidani platby ramecek nebere ----
        ctx, pg = stranka(b, z + 'hry/odemknout.html?co=sega', 'zamceno', chyby=chyby)
        pg.wait_for_timeout(900)
        f1 = pg.evaluate(FOKUS)
        cesta = [f1]
        for _ in range(12):           # sipkou dolu az na DO MENU (posledni)
            pg.keyboard.press('ArrowDown')
            pg.wait_for_timeout(40)
            cesta.append(pg.evaluate(FOKUS))
        f2 = pg.evaluate(FOKUS)
        pg.evaluate("() => napHry.platba(__scenar.platba('sega'))")   # hlidani platby: stejna odpoved znovu
        pg.wait_for_timeout(300)
        f3 = pg.evaluate(FOKUS)
        over(f1 == 'zaplatil' and f2 == 'doMenu' and f3 == 'doMenu',
             'T4) B337: zamceno (platebni udaje) -> ramecek na "%s"; sipkou dolu na "%s", hlidani platby (stejna odpoved) -> porad "%s"'
             % (f1, f2, f3))
        ctx.close()

        # ---- T5: telefon - zadny ramecek ----
        ctx, pg = stranka(b, z + 'hry/odemknout.html?co=sega', 'zaplaceno', tv=False, chyby=chyby)
        pg.wait_for_timeout(900)
        f1 = pg.evaluate(FOKUS)
        krouzek = pg.evaluate("() => document.querySelectorAll('.nap-tv-fokus').length")
        spustit = pg.evaluate("() => !document.getElementById('spustit').classList.contains('skryte')")
        over(f1 is None and krouzek == 0 and spustit,
             'T5) telefon (dotyk, bez zapni): SPUSTIT SEGU videt, ramecek zadny (%s, %d prvku s ramcem)' % (f1, krouzek))
        ctx.close()

        # ---- T6: OPTIONS - adresa logu pod adresou WEB TV ----
        vysl = []
        for jaz in ('cs', 'en'):
            ctx = b.new_context(viewport={'width': 1280, 'height': 720})
            ctx.add_init_script(H.init_skript(jaz, {'AHTVWEB.start': 'TV_WEB_CAST_OK http://192.0.2.10:8765/ APP',
                                                   'AHTVWEB.status': ''}))
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
            pg.goto(z + 'index.html')
            pg.wait_for_timeout(800)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            pg.click('#stStartTv')
            pg.wait_for_timeout(300)
            vysl.append(pg.evaluate("() => [document.querySelector('#stTvAdresa b').textContent, document.getElementById('stTvLog').textContent,"
                                    " document.querySelector('#stTvAdresa .stTvLogPopis').textContent, getComputedStyle(document.getElementById('stTvAdresa')).display]"))
            ctx.close()
        cs, en = vysl
        over(cs[0] == 'http://192.0.2.10:8765/' and cs[1] == 'http://192.0.2.10:8765/log' and cs[2] == 'LOG APPKY PRO PODPORU'
             and cs[3] == 'block' and en[1] == 'http://192.0.2.10:8765/log' and en[2] == 'APP LOG FOR SUPPORT',
             'T6) OPTIONS -> SPUSTIT WEB TV: adresa %s, log %s ("%s" / EN "%s")' % (cs[0], cs[1], cs[2], en[2]))

        # ---- T7: N&P HRY - BIOS neni v seznamu her ----
        ctx = b.new_context(viewport={'width': 1280, 'height': 720})
        ctx.add_init_script(H.init_skript('cs', {}))
        ctx.add_init_script('window.__scenar={seznam:{ok:true,offline:false,kod:200,data:{hry:['
                            '{id:"ps1-0000000000000001",system:"ps1",nazev:"PS1-BIOS",soubor:"PS1-BIOS_.zip",velikost:500000},'
                            '{id:"ps1-0000000000000002",system:"ps1",nazev:"Crash Bandicoot",soubor:"Crash Bandicoot.chd",velikost:300000000},'
                            '{id:"sega-0000000000000003",system:"sega",nazev:"Sonic-The-Hedgehog-USA-Europe",soubor:"Sonic-The-Hedgehog-USA-Europe.zip",velikost:400000}'
                            ']}}};')
        ctx.add_init_script(MOST)
        pg = ctx.new_page()
        pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
        pg.goto(z + 'hry/index.html?system=ps1')
        pg.wait_for_timeout(900)
        ps1 = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mrizka .hra'), function(e){return e.textContent.trim();})")
        pg.goto(z + 'hry/index.html?system=sega')
        pg.wait_for_timeout(900)
        sega = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#mrizka .hra'), function(e){return e.textContent.trim();})")
        ctx.close()
        over(len(ps1) == 1 and 'Crash' in ps1[0] and len(sega) == 1 and 'Sonic' in sega[0],
             'T7) N&P HRY: PS1 %s (BIOS skryty), SEGA %s' % (ps1, sega))

        over(not chyby, 'T8) zadna chyba JavaScriptu%s' % ('' if not chyby else ': ' + '; '.join(chyby[:3])))
        b.close()
    srv.shutdown()
    vystup.append('VYSLEDEK: %d testu, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b337_tv_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
