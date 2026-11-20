#!/usr/bin/env python3
"""B357: STRANKA SEGY - HRÁT ONLINE (2 TELEFONY) (Chromium, stranka presne z appky - assets/emu_sega/index.html).

Most Javy AHONLINE (MainActivity.AHOnline) je falesny: stav (radky klic=hodnota jako z C++) nastavuje test,
co stranka zavola (seznam / zaloz / pripoj / konec), se zapise. Seznam her z enginu posila test do
window.napOnline.vysledek({...}) presne ve tvaru, v jakem ho posila Java (JSON enginu v12.4).
Kontroly:
  1) na sirku: kolecko -> D-PAD A OVLÁDÁNÍ -> HRÁT ONLINE (2 TELEFONY) otevre panel, nacte seznam her, jmeno z telefonu
  2) seznam: jen hry Segy, u kazde co je potreba (mas stejnou hru / stahne se ze serveru / potrebujes stejnou),
     PŘIPOJIT -> AHONLINE.pripoj(kod, otisk, nastaveni, hra_id, jmeno)
  3) ZALOŽIT HRU bez hry v konzoli -> vysvetleni, nic se nezaklada; s hrou -> AHONLINE.zaloz(jmeno), jmeno se pamatuje
  4) stav: cekam na hrace 2 (cislo hry) v panelu i v odznaku, ZALOŽIT nejde
  5) hra zacala -> panel se zavre sam, odznak "ONLINE · jmeno · odezva"
  6) konec (druhy hrac skoncil) -> vysvetleni cesky s diakritikou, odznak ONLINE SKONČILO
  7) chyby enginu: nesparovany telefon (401), stary engine bez online (404), odpoved enginu pri pripojeni (JSON)
  8) na vysku: MOJE HRY -> HRÁT ONLINE (2 TELEFONY), odznak dole
  9) anglicky: tlacitka, panel i stav anglicky, zadna cestina v panelu
 10) starsi appka bez AHONLINE: panel rekne proc nejde, zadna chyba
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b357_stranka.py      -> VYSTUP_test_b357_stranka.txt
"""
import json
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


NA_SIRKU = {'width': 846, 'height': 412}
NA_VYSKU = {'width': 412, 'height': 846}
OTISK = '891e5c3ab4c57347c9dcdc6a263580c860b8ef3e'

AHONLINE = '''
(function(){
  window.__onl = {stav: 'krok=\\nsparovano=1\\nfaze=NIC', hra: {otisk: '%s', nastaveni: '16', nazev: 'Sonic', hra_id: '', jmeno: 'Galaxy S8'}, volani: []};
  window.AHONLINE = {
    stav: function () { return window.__onl.stav; },
    hra: function () { return JSON.stringify(window.__onl.hra); },
    seznam: function () { window.__onl.volani.push('seznam'); },
    zaloz: function (j) { window.__onl.volani.push('zaloz|' + j); },
    pripoj: function (k, o, n, id, j) { window.__onl.volani.push(['pripoj', k, o, n, id, j].join('|')); },
    konec: function () { window.__onl.volani.push('konec'); }
  };
})();
''' % OTISK

SEZNAM = {'akce': 'seznam', 'ok': True, 'kod': 200, 'duvod': '', 'zprava': '', 'data': {'ok': True, 'mistnosti': [
    {'kod': '0427', 'system': 'sega', 'hra': 'Sonic', 'hra_id': '', 'otisk': OTISK, 'nastaveni': '16', 'hrac': 'bracha', 'ceka_s': 15},
    {'kod': '8812', 'system': 'sega', 'hra': 'Streets of Rage 2', 'hra_id': 'sega-0123456789abcdef', 'otisk': 'e33fe5db01a322a6d8c42763049d889c8b2f5cc4',
     'nastaveni': '16', 'hrac': 'tester', 'ceka_s': 190},
    {'kod': '5555', 'system': 'ps1', 'hra': 'Tekken 3', 'hra_id': '', 'otisk': '00' * 20, 'nastaveni': '0', 'hrac': 'nekdo', 'ceka_s': 5}]}}


def novy(b, adresa, okno, chyby, jazyk='cs', online=True):
    ctx = b.new_context(viewport=okno, has_touch=True, is_mobile=True, device_scale_factor=1)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    if online:
        ctx.add_init_script(AHONLINE)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(adresa + 'emu_sega/index.html')
    pg.wait_for_timeout(900)
    return ctx, pg


def stav(pg, radky):
    pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=\nsparovano=1\n' + radky)
    pg.wait_for_timeout(900)   # stranka se pta kazdych 600 ms


def volani(pg):
    return pg.evaluate('() => window.__onl ? window.__onl.volani.slice() : []')


def text(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); return e ? e.textContent : null; }', sel)


def otevreno(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); return !!e && e.classList.contains("open"); }', sel)


def odznak(pg):
    return pg.evaluate('() => { var e = document.getElementById("segaOnlineOdznak"); var c = getComputedStyle(e);'
                       ' return [c.display, e.textContent, e.className, Math.round(e.getBoundingClientRect().top)]; }')


def klik(pg, sel):
    pg.evaluate('(s) => document.querySelector(s).click()', sel)
    pg.wait_for_timeout(250)


def main():
    srv, url = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        # ---------------------------------------------------------------- na sirku, cesky
        ctx, pg = novy(b, url, NA_SIRKU, chyby)
        klik(pg, '#btnSegaSettings')
        pg.wait_for_timeout(200)
        vidi = pg.evaluate('() => { var e = document.getElementById("segaBtnOnline"); return !!e && e.offsetParent !== null; }')
        klik(pg, '#segaBtnOnline')
        pg.wait_for_timeout(300)
        jm = pg.evaluate('() => document.getElementById("segaOnlineJmeno").value')
        over(vidi and otevreno(pg, '#segaOnlinePanel') and not otevreno(pg, '#segaDpadMenuPanel') and volani(pg) == ['seznam'] and jm == 'Galaxy S8'
             and text(pg, '#segaBtnOnline') == 'HRÁT ONLINE (2 TELEFONY)',
             '1) kolecko -> HRÁT ONLINE (2 TELEFONY): panel otevren, seznam her nacten (%s), jmeno z telefonu "%s"' % (volani(pg), jm))

        pg.evaluate('(r) => window.napOnline.vysledek(r)', SEZNAM)
        pg.wait_for_timeout(200)
        radky = pg.evaluate('() => Array.prototype.map.call(document.querySelectorAll("#segaOnlineSeznam .onlineHra"), function (r) { return r.textContent; })')
        over(len(radky) == 2 and 'Sonic · bracha' in radky[0] and 'máš stejnou hru' in radky[0] and 'čeká chvilku' in radky[0]
             and 'Streets of Rage 2 · tester' in radky[1] and 'hra se stáhne ze serveru N&P' in radky[1] and 'čeká 3 min' in radky[1]
             and not any('Tekken' in r for r in radky), '2) seznam: jen hry Segy (%d) - %s' % (len(radky), ' | '.join(radky)))
        pg.evaluate('() => { document.getElementById("segaOnlineJmeno").value = "René"; }')
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(2) .onlinePripoj')
        v = volani(pg)
        zavren = not otevreno(pg, '#segaOnlinePanel')
        over(v[-1] == 'pripoj|8812|e33fe5db01a322a6d8c42763049d889c8b2f5cc4|16|sega-0123456789abcdef|René' and zavren,
             '   PŘIPOJIT -> AHONLINE.pripoj(%s); hra se stahne -> panel pryc (obraz videt pro hlidac obrazu appky)' % v[-1])
        pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=STAHUJI\nsparovano=1\nfaze=NIC')
        pg.wait_for_timeout(900)
        o = odznak(pg)
        pg.evaluate('() => window.napOnline.vysledek({akce: "pripoj", ok: false, zprava: "Hru hráče 1 se nepodařilo stáhnout."})')
        pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=\nsparovano=1\nfaze=NIC')
        pg.wait_for_timeout(900)
        over(o[1] == 'ONLINE · STAHUJI HRU' and o[0] == 'block' and otevreno(pg, '#segaOnlinePanel') and text(pg, '#segaOnlineStav') == 'Hru hráče 1 se nepodařilo stáhnout.',
             '   stahovani: odznak "%s", chyba panel zase otevre: "%s"' % (o[1], text(pg, '#segaOnlineStav')))
        pg.evaluate('(r) => window.napOnline.vysledek(r)', SEZNAM)   # panel si seznam nacetl znovu
        pg.wait_for_timeout(200)
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(1) .onlinePripoj')
        v = volani(pg)
        over(v[-1] == 'pripoj|0427|%s|16||René' % OTISK and otevreno(pg, '#segaOnlinePanel'),
             '   PŘIPOJIT ke hre se stejnou hrou: panel zustava (nic se nenahrava) - %s' % v[-1])

        pg.evaluate('() => { window.__onl.hra.otisk = ""; }')
        klik(pg, '#segaOnlineZaloz')
        pg.wait_for_timeout(300)
        t = text(pg, '#segaOnlineStav')
        bez = [x for x in volani(pg) if x.startswith('zaloz')]
        pg.evaluate('() => { window.__onl.hra.otisk = "%s"; }' % OTISK)
        klik(pg, '#segaOnlineZaloz')
        s_jm = pg.evaluate('() => { try { return localStorage.getItem("napOnlineJmeno"); } catch (e) { return "?"; } }')
        over(not bez and t.startswith('Nejdřív spusť hru') and volani(pg)[-1] == 'zaloz|René' and s_jm == 'René',
             '3) ZALOŽIT HRU bez hry: "%s"; s hrou -> %s, jmeno zapamatovano (%s)' % (t[:60], volani(pg)[-1], s_jm))

        stav(pg, 'faze=CEKA_NA_HRACE\nkod=0427\naktivni=1\nhraje=0')
        o = odznak(pg)
        zal = pg.evaluate('() => document.getElementById("segaOnlineZaloz").disabled')
        over(text(pg, '#segaOnlineStav') == 'Čekám na hráče 2 – číslo hry 0427' and o[0] == 'block' and o[1] == 'ONLINE · ČÍSLO HRY 0427' and zal,
             '4) ceka na hrace 2: "%s", odznak "%s" (%s), ZALOŽIT nejde' % (text(pg, '#segaOnlineStav'), o[1], o[0]))

        stav(pg, 'faze=HRAJE\nkod=0427\ndruhy=bracha\nodezva=23\nzpozdeni=3\naktivni=1\nhraje=1')
        o = odznak(pg)
        over(not otevreno(pg, '#segaOnlinePanel') and o[0] == 'block' and o[1] == 'ONLINE · bracha · 23 MS' and o[3] < 40,
             '5) hra zacala: panel se zavrel sam, odznak nahore "%s" (y %d)' % (o[1], o[3]))

        stav(pg, 'faze=KONEC\nkod=0427\ndruhy=bracha\nchyba=druhy hrac hru ukoncil\nkonec=druhy hrac hru ukoncil\naktivni=0\nhraje=0')
        o = odznak(pg)
        pg.evaluate('() => document.getElementById("segaBtnOnline").click()')
        pg.wait_for_timeout(400)
        t = text(pg, '#segaOnlineStav')
        over(o[1] == 'ONLINE SKONČILO' and 'chyba' in o[2] and t == 'Online skončilo: druhý hráč hru ukončil',
             '6) konec: odznak "%s", v panelu "%s"' % (o[1], t))

        pg.evaluate('() => window.napOnline.vysledek({akce: "seznam", ok: false, kod: 401, duvod: "nesparovano", zprava: "", data: null})')
        e401 = text(pg, '#segaOnlineSeznam')
        pg.evaluate('() => window.napOnline.vysledek({akce: "seznam", ok: false, kod: 404, duvod: "neznamy_dotaz", zprava: "Neznámý dotaz.", data: null})')
        e404 = text(pg, '#segaOnlineSeznam')
        stav(pg, 'faze=NIC')
        pg.evaluate('''() => window.napOnline.vysledek({akce: "pripoj", ok: false, zprava: 'CHYBA: engine odmitl online (HTTP 404): {"ok":false,"duvod":"mistnost_neni","zprava":"Tahle hra už nečeká – hráč ji zrušil, nebo už hraje s někým jiným."}'})''')
        pg.wait_for_timeout(700)
        ep = text(pg, '#segaOnlineStav')
        over(e401.startswith('Telefon není spárovaný') and 'v12.4' in e404 and ep.startswith('Tahle hra už nečeká'),
             '7) chyby: 401 "%s" | 404 "%s" | pripojeni "%s"' % (e401[:45], e404[:50], ep[:45]))
        klik(pg, '#segaOnlineKonec')
        over(volani(pg)[-1] == 'konec', '   UKONČIT ONLINE HRU -> AHONLINE.konec()')
        klik(pg, '#segaOnlineClose')
        over(not otevreno(pg, '#segaOnlinePanel'), '   ZAVŘÍT zavre panel')
        ctx.close()

        # ---------------------------------------------------------------- na vysku: MOJE HRY
        ctx, pg = novy(b, url, NA_VYSKU, chyby)
        pg.evaluate('() => { var p = document.getElementById("segaLibraryPanel"); p.classList.add("open"); p.setAttribute("aria-hidden", "false"); }')
        klik(pg, '#segaLibraryOnline')
        pg.wait_for_timeout(300)
        stav(pg, 'faze=CEKA_NA_HRACE\nkod=1234\naktivni=1\nhraje=0')
        klik(pg, '#segaOnlineClose')
        o = odznak(pg)
        over(otevreno(pg, '#segaOnlinePanel') is False and text(pg, '#segaLibraryOnline') == 'HRÁT ONLINE (2 TELEFONY)' and o[0] == 'block' and o[3] > 700,
             '8) na vysku: MOJE HRY -> HRÁT ONLINE (2 TELEFONY), odznak dole (y %d): "%s"' % (o[3], o[1]))
        ctx.close()

        # ---------------------------------------------------------------- anglicky
        ctx, pg = novy(b, url, NA_SIRKU, chyby, jazyk='en')
        klik(pg, '#btnSegaSettings')
        btn = text(pg, '#segaBtnOnline')
        klik(pg, '#segaBtnOnline')
        pg.wait_for_timeout(300)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', SEZNAM)
        stav(pg, 'faze=CEKA_NA_HRACE\nkod=0427\naktivni=1\nhraje=0')
        panel = pg.evaluate('() => { var b = document.querySelector("#segaOnlinePanel .box"); var t = [];'
                            ' var w = document.createTreeWalker(b, NodeFilter.SHOW_TEXT); var n; while ((n = w.nextNode())) { var x = n.nodeValue.trim(); if (x) t.push(x); }'
                            ' return t; }')
        cesky = [x for x in panel if any(c in x for c in 'áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ') and 'René' not in x]
        o = odznak(pg)
        over(btn == 'PLAY ONLINE (2 PHONES)' and 'PLAY ONLINE' in panel and 'CREATE A GAME (PLAYER 1)' in panel and 'GAMES WAITING FOR PLAYER 2' in panel
             and 'Waiting for player 2 – game number 0427' in panel and o[1] == 'ONLINE · GAME NUMBER 0427' and not cesky,
             '9) anglicky: "%s", panel bez cestiny%s, odznak "%s"' % (btn, '' if not cesky else ' - CESKY: ' + ' | '.join(cesky), o[1]))
        ctx.close()

        # ---------------------------------------------------------------- starsi appka bez AHONLINE
        ctx, pg = novy(b, url, NA_SIRKU, chyby, online=False)
        klik(pg, '#btnSegaSettings')
        klik(pg, '#segaBtnOnline')
        pg.wait_for_timeout(300)
        t = text(pg, '#segaOnlineSeznam')
        klik(pg, '#segaOnlineZaloz')
        over(t.startswith('Online tady nejde'), '10) bez mostu AHONLINE: "%s"' % t)
        ctx.close()
        b.close()
    over(not chyby, 'zadna chyba JavaScriptu' + ('' if not chyby else ': ' + ' | '.join(chyby[:5])))
    srv.shutdown()
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b357_stranka.txt'), 'w', encoding='utf-8') as f:
        f.write('B357 - stranka Segy: HRÁT ONLINE (2 TELEFONY) - Chromium, stranka z appky, falesny most AHONLINE\n')
        f.write('\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
