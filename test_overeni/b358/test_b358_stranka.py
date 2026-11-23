#!/usr/bin/env python3
"""B358: STRANKA SEGY - ONLINE JEDNODUSE (Chromium, stranka presne z appky - assets/emu_sega/index.html).
Rene (23. 11., po prvnim testu online): "online musi byt vice intuitivni".

Most Javy AHONLINE (MainActivity.AHOnline) je falesny: stav (radky klic=hodnota jako z C++) nastavuje test, co
stranka zavola (seznam / knihovna / zaloz / pripojHru / konec), se zapise. Seznam her z enginu posila test do
window.napOnline.vysledek({...}) presne ve tvaru, v jakem ho posila Java (JSON enginu v12.4).
Kontroly:
  1) na vysku: viditelne tlacitko 🌐 ONLINE na mrizce mezi 16-BIT a cartridgi (nic neprekryva), otevre panel
  2) panel bez hry v konzoli: karta JAK NA TO (3 kroky), VYBRAT HRU (HRY) misto ZALOŽIT, prazdny seznam "objeví se sama"
  3) seznam se obnovuje sam (4 s) a pri obnove neblika "Načítám…"
  4) VYBRAT HRU -> MOJE HRY, ▶ u hry -> hra se spusti a panel ONLINE se sam vrati
  5) s hrou v konzoli: "Hra v konzoli: …", ZALOŽIT HRU -> AHONLINE.zaloz(jmeno)
  6) cekani na hrace 2: velke cislo hry + navod pro druhy telefon, seznam a zalozeni schovane, jen ZRUŠIT ČEKÁNÍ,
     tlacitko ONLINE zelene, odznak; ZRUŠIT ČEKÁNÍ -> AHONLINE.konec()
  7) seznam: u kazde hry, co je potreba (máš stejnou / stáhne se / máš ji v HRY - spustí se sama / nemáš), jen Sega
  8) PŘIPOJIT SE: stejna hra (panel zustane), hra z HRY a ze serveru (panel pryc, odznak SPOUŠTÍM / STAHUJI),
     hra, kterou telefon nema -> chyba z Javy v karte (cervene)
  9) konec: chyba site -> rada "zkontroluj engine a internet"; odmitnuti enginu (JSON) -> jeho zprava bez rady
 10) hra zacala -> panel se zavre sam, odznak "ONLINE · jmeno · odezva"
 11) na sirku: tlacitko ONLINE dole vpravo vedle LOAD GAME - neprekryva D-pad, A B C, START, SAVE / LOAD,
     kolecko, otoceni ani neviditelne zony nahore
 12) anglicky: tlacitko, panel (sekce, karta JAK NA TO, cekani) bez cestiny
 13) starsi appka: bez AHONLINE (panel rekne proc nejde), most bez pripojHru (B357 Java) -> pripoj()
 14) klic nazvu hry v JS = Java NapOnlineHra.klicNazvu (stejne vzorky jako jvm/TestB358Online)
 15) odznak se pri stejnem stavu neprepisuje (prekresleni nad obrazem hry)
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b358_stranka.py [slozka pro snimky]   -> VYSTUP_test_b358_stranka.txt
"""
import json
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

SNIMKY = sys.argv[1] if len(sys.argv) > 1 else ''
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
OTISK2 = 'e33fe5db01a322a6d8c42763049d889c8b2f5cc4'
OTISK3 = '31f12a21af01aa00bb11cc22dd33ee44ff556677'
OTISK4 = '0123456789abcdef0123456789abcdef01234567'

# klic nazvu - stejne vzorky jako jvm/TestB358Online (Java NapOnlineHra.klicNazvu)
KLICE = [
    ('Golden Axe II  World', 'golden axe ii world'),
    ('Golden_Axe_II__World_', 'golden axe ii world'),
    ('Golden Axe II (World)', 'golden axe ii world'),
    ('  Streets of Rage 2 ', 'streets of rage 2'),
    ('Žluťoučký kůň', 'lu ou k k'),
    ('_lu_ou_k__k__', 'lu ou k k'),
    ('Sonic The Hedgehog (USA, Europe)', 'sonic the hedgehog usa europe'),
    ('Sonic-The.Hedgehog\u3000(USA,_Europe)', 'sonic the hedgehog usa europe'),
]

AHONLINE = '''
(function(){
  window.__onl = {stav: 'krok=\\nsparovano=1\\nfaze=NIC', hra: {otisk: '', nastaveni: '16', nazev: '', hra_id: '', jmeno: 'Galaxy S8'},
                  knihovna: [{soubor: 'Golden_Axe_II__World_.gen', klic: 'golden axe ii world'}, {soubor: 'Sonic.gen', klic: 'sonic'}], volani: []};
  window.AHONLINE = {
    stav: function () { return window.__onl.stav; },
    hra: function () { return JSON.stringify(window.__onl.hra); },
    knihovna: function () { window.__onl.volani.push('knihovna'); return JSON.stringify(window.__onl.knihovna); },
    seznam: function () { window.__onl.volani.push('seznam'); },
    zaloz: function (j) { window.__onl.volani.push('zaloz|' + j); },
    pripojHru: function (k, o, n, id, h, j) { window.__onl.volani.push(['pripojHru', k, o, n, id, h, j].join('|')); },
    pripoj: function (k, o, n, id, j) { window.__onl.volani.push(['pripoj', k, o, n, id, j].join('|')); },
    konec: function () { window.__onl.volani.push('konec'); }
  };
})();
'''

MOCK = {
    'AHSega.segaLibraryList': '[{"key":"Golden_Axe_II__World_.gen","nazev":"Golden_Axe_II__World_.gen","bytes":524288,"ma_ulozenou_hru":false},'
                              '{"key":"Sonic.gen","nazev":"Sonic.gen","bytes":524288,"ma_ulozenou_hru":true}]',
    'AHSega.segaLibraryLaunch': 'SEGA_LIBRARY_LAUNCH_OK',
}


def seznam(mistnosti):
    return {'akce': 'seznam', 'ok': True, 'kod': 200, 'duvod': '', 'zprava': '', 'data': {'ok': True, 'mistnosti': mistnosti}}


MISTNOSTI = [
    {'kod': '0427', 'system': 'sega', 'hra': 'Sonic', 'hra_id': '', 'otisk': OTISK, 'nastaveni': '16', 'hrac': 'bracha', 'ceka_s': 15},
    {'kod': '8812', 'system': 'sega', 'hra': 'Streets of Rage 2', 'hra_id': 'sega-0123456789abcdef', 'otisk': OTISK2,
     'nastaveni': '16', 'hrac': 'tester', 'ceka_s': 190},
    {'kod': '3141', 'system': 'sega', 'hra': 'Golden Axe II  World', 'hra_id': '', 'otisk': OTISK3, 'nastaveni': '16', 'hrac': 'René', 'ceka_s': 40},
    {'kod': '2718', 'system': 'sega', 'hra': 'Aladdin', 'hra_id': '', 'otisk': OTISK4, 'nastaveni': '16', 'hrac': 'kamarad', 'ceka_s': 70},
    {'kod': '5555', 'system': 'ps1', 'hra': 'Tekken 3', 'hra_id': '', 'otisk': '00' * 20, 'nastaveni': '0', 'hrac': 'nekdo', 'ceka_s': 5}]


def novy(b, adresa, okno, chyby, jazyk='cs', online=AHONLINE):
    ctx = b.new_context(viewport=okno, has_touch=True, is_mobile=True, device_scale_factor=1)
    ctx.add_init_script(H.init_skript(jazyk, MOCK))
    if online:
        ctx.add_init_script(online)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(adresa + 'emu_sega/index.html')
    pg.wait_for_timeout(900)
    return ctx, pg


def stav(pg, radky, cekej=900):
    pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=\nsparovano=1\n' + radky)
    pg.wait_for_timeout(cekej)   # stranka se pta kazdych 600 ms


def volani(pg):
    return pg.evaluate('() => window.__onl ? window.__onl.volani.slice() : []')


def text(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); return e ? e.textContent : null; }', sel)


def vidi(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); if (!e) return false; var r = e.getBoundingClientRect();'
                       ' return e.offsetParent !== null && getComputedStyle(e).visibility !== "hidden" && r.width > 0 && r.height > 0; }', sel)


def otevreno(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); return !!e && e.classList.contains("open"); }', sel)


def odznak(pg):
    return pg.evaluate('() => { var e = document.getElementById("segaOnlineOdznak"); var c = getComputedStyle(e);'
                       ' return [c.display, e.textContent, e.className]; }')


def klik(pg, sel, cekej=250):
    pg.evaluate('(s) => document.querySelector(s).click()', sel)
    pg.wait_for_timeout(cekej)


def obdelnik(pg, sel):
    return pg.evaluate('(s) => { var e = document.querySelector(s); if (!e || e.offsetParent === null) return null;'
                       ' var r = e.getBoundingClientRect(); return [r.left, r.top, r.right, r.bottom]; }', sel)


def prekryv(a, b):
    return a and b and a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def snimek(pg, jmeno):
    if SNIMKY:
        os.makedirs(SNIMKY, exist_ok=True)
        pg.screenshot(path=os.path.join(SNIMKY, jmeno + '.png'))


def radky_seznamu(pg):
    return pg.evaluate('() => Array.prototype.map.call(document.querySelectorAll("#segaOnlineSeznam .onlineHra"), function (r) { return r.textContent; })')


def main():
    srv, url = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        # ================================================================ na vysku, cesky
        ctx, pg = novy(b, url, NA_VYSKU, chyby)
        snimek(pg, '1_na_vysku_tlacitko_online')
        r = obdelnik(pg, '#segaOnlineTlacitko')
        st = obdelnik(pg, '#stage')
        jine = [s for s in ('#cartSlot', '#cartCenter', '#btnCollection', '#btnSaves', '#btnGames', '#btnManuals', '#btnWeb', '#btnMenu',
                            '#dpad', '#btnA', '#btnB', '#btnC', '#btnStart', '#btnSegaOtoc') if prekryv(r, obdelnik(pg, s))]
        # mrizka mezi destickou 16-BIT (vlevo, do ~31 % sirky) a cartridgi (od 68 %), pod ovladacem (od ~80 % vysky)
        v_mrizce = r and st and r[0] > st[0] + 0.31 * (st[2] - st[0]) and r[2] < st[0] + 0.66 * (st[2] - st[0]) \
            and r[1] > st[1] + 0.80 * (st[3] - st[1]) and r[3] < st[1] + 0.88 * (st[3] - st[1])
        over(vidi(pg, '#segaOnlineTlacitko') and text(pg, '#segaOnlineTlacitko').strip() == '🌐 ONLINE' and v_mrizce and not jine,
             '1) na vysku: tlacitko "%s" na mrizce mezi 16-BIT a cartridgi %s, nic neprekryva%s'
             % (text(pg, '#segaOnlineTlacitko').strip(), [round(x) for x in r] if r else None, '' if not jine else ' - PREKRYVA ' + ', '.join(jine)))
        klik(pg, '#segaOnlineTlacitko', 400)
        v = volani(pg)
        over(otevreno(pg, '#segaOnlinePanel') and 'seznam' in v and 'knihovna' in v,
             '   tlacitko ONLINE otevre panel, nacte seznam her a HRY (%s)' % v)

        # ---------------------------------------------------------------- 2) bez hry
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam([]))
        pg.wait_for_timeout(200)
        karta = text(pg, '#segaOnlineStav')
        prazdny = text(pg, '#segaOnlineSeznam')
        over(karta.startswith('JAK NA TO') and 'Hráč 1 spustí hru a dá ZALOŽIT HRU.' in karta and 'otevře 🌐 ONLINE a u hry klepne PŘIPOJIT SE' in karta
             and 'Hra začne u obou najednou od začátku.' in karta and not vidi(pg, '#segaOnlineZaloz') and vidi(pg, '#segaOnlineVybrat')
             and text(pg, '#segaOnlineVybrat') == 'VYBRAT HRU (HRY)' and text(pg, '#segaOnlineMojeHra').startswith('V konzoli zatím není žádná hra')
             and not vidi(pg, '#segaOnlineKonec') and prazdny.startswith('Zatím nikdo nečeká. Až druhý hráč dá ZALOŽIT HRU, objeví se tady sama.'),
             '2) bez hry: karta "%s…", VYBRAT HRU misto ZALOŽIT, seznam "%s"' % (karta[:40], prazdny[:45]))
        snimek(pg, '2_panel_bez_hry')

        # ---------------------------------------------------------------- 3) obnova seznamu
        n0 = volani(pg).count('seznam')
        pg.evaluate('() => { window.__nacitam = 0; var h = document.getElementById("segaOnlineSeznam");'
                    ' new MutationObserver(function () { if (/Načítám/.test(h.textContent)) window.__nacitam++; }).observe(h, {childList: true, subtree: true, characterData: true}); }')
        pg.wait_for_timeout(4500)
        n1 = volani(pg).count('seznam')
        nac = pg.evaluate('() => window.__nacitam')
        over(n1 == n0 + 1 and nac == 0, '3) seznam se obnovil sam po 4 s (%d -> %d), bez blikani "Načítám…" (%d)' % (n0, n1, nac))

        # ---------------------------------------------------------------- 4) VYBRAT HRU -> HRY -> zpet
        klik(pg, '#segaOnlineVybrat', 900)
        knih = otevreno(pg, '#segaLibraryPanel')
        zavren = not otevreno(pg, '#segaOnlinePanel')
        klik(pg, '#segaLibraryList .libGame .libPlay', 300)
        spust = [x for x in pg.evaluate('() => window.__napVolani.slice()') if x == 'AHSega.segaLibraryLaunch']
        pg.evaluate('() => { window.__onl.hra = {otisk: "%s", nastaveni: "16", nazev: "Golden Axe II  World", hra_id: "", jmeno: "Galaxy S8"}; }' % OTISK3)
        pg.wait_for_timeout(3400)
        zpet = otevreno(pg, '#segaOnlinePanel')
        over(knih and zavren and spust and zpet and text(pg, '#segaOnlineMojeHra') == 'Hra v konzoli: Golden Axe II  World',
             '4) VYBRAT HRU -> MOJE HRY (%s), ▶ spusti hru (%s), panel ONLINE se sam vratil (%s): "%s"'
             % (knih, len(spust), zpet, text(pg, '#segaOnlineMojeHra')))
        klik(pg, '#segaOnlineClose')
        klik(pg, '#segaOnlineTlacitko', 400)

        # ---------------------------------------------------------------- 5) s hrou -> ZALOŽIT
        pg.evaluate('() => { document.getElementById("segaOnlineJmeno").value = "René"; }')
        klik(pg, '#segaOnlineZaloz')
        over(vidi(pg, '#segaOnlineZaloz') and text(pg, '#segaOnlineVybrat') == 'VYBRAT JINOU HRU (HRY)' and volani(pg)[-1] == 'zaloz|René',
             '5) s hrou v konzoli: ZALOŽIT HRU -> %s, druhe tlacitko "%s"' % (volani(pg)[-1], text(pg, '#segaOnlineVybrat')))

        # ---------------------------------------------------------------- 6) ceka na hrace 2
        stav(pg, 'faze=CEKA_NA_HRACE\nkod=0427\naktivni=1\nhraje=0\nstart=1')
        karta = text(pg, '#segaOnlineStav')
        cislo = pg.evaluate('() => { var e = document.querySelector("#segaOnlineStav .onlineCislo"); return e ? [e.textContent, parseFloat(getComputedStyle(e).fontSize)] : null; }')
        o = odznak(pg)
        tl = pg.evaluate('() => document.getElementById("segaOnlineTlacitko").className')
        over(karta.startswith('ČEKÁM NA HRÁČE 2') and cislo == ['0427', 46.0] and 'Na druhém telefonu: SEGA → 🌐 ONLINE → u hry 0427 klepni PŘIPOJIT SE.' in karta
             and 'Hra ti zatím běží dál – panel můžeš zavřít.' in karta and not vidi(pg, '#segaOnlineSeznamBlok') and not vidi(pg, '#segaOnlineZalozBlok')
             and vidi(pg, '#segaOnlineKonec') and text(pg, '#segaOnlineKonec') == 'ZRUŠIT ČEKÁNÍ' and 'aktivni' in tl
             and o[0] == 'block' and o[1] == 'ONLINE · ČÍSLO HRY 0427',
             '6) cekani: velke cislo %s, navod pro druhy telefon, jen ZRUŠIT ČEKÁNÍ, tlacitko ONLINE "%s", odznak "%s"' % (cislo, tl, o[1]))
        snimek(pg, '3_ceka_na_hrace_2')
        klik(pg, '#segaOnlineKonec')
        over(volani(pg)[-1] == 'konec', '   ZRUŠIT ČEKÁNÍ -> AHONLINE.konec()')
        stav(pg, 'faze=KONEC\nchyba=online hru jsi ukoncil\nkonec=online hru jsi ukoncil\naktivni=0\nhraje=0\nstart=1')

        # ---------------------------------------------------------------- 7) seznam s hrami
        stav(pg, 'faze=NIC\nstart=1', 300)
        pg.evaluate('() => { window.__onl.hra = {otisk: "%s", nastaveni: "16", nazev: "Sonic", hra_id: "", jmeno: "Galaxy S8"}; }' % OTISK)
        klik(pg, '#segaOnlineClose')
        klik(pg, '#segaOnlineTlacitko', 400)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam(MISTNOSTI))
        pg.wait_for_timeout(200)
        rr = radky_seznamu(pg)
        over(len(rr) == 4 and 'Sonic · bracha' in rr[0] and '✔ máš stejnou hru' in rr[0]
             and 'Streets of Rage 2 · tester' in rr[1] and '⬇ hra se stáhne ze serveru N&P' in rr[1] and 'čeká 3 min' in rr[1]
             and 'Golden Axe II  World · René' in rr[2] and '✔ máš ji v HRY – spustí se sama' in rr[2]
             and 'Aladdin · kamarad' in rr[3] and '✖ tuhle hru v telefonu nemáš – potřebuješ stejný soubor' in rr[3]
             and all('PŘIPOJIT SE' in x for x in rr) and not any('Tekken' in x for x in rr),
             '7) seznam (jen Sega, %d): %s' % (len(rr), ' | '.join(x[:60] for x in rr)))
        snimek(pg, '4_seznam_her')

        # ---------------------------------------------------------------- 8) PŘIPOJIT SE
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(1) .onlinePripoj')
        v1 = volani(pg)[-1]
        zustal = otevreno(pg, '#segaOnlinePanel')
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(3) .onlinePripoj')
        v3 = volani(pg)[-1]
        pryc = not otevreno(pg, '#segaOnlinePanel')
        pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=SPOUSTIM\nsparovano=1\nfaze=KONEC\nstart=1')
        pg.wait_for_timeout(900)
        o = odznak(pg)
        over(v1 == 'pripojHru|0427|%s|16||Sonic|René' % OTISK and zustal
             and v3 == 'pripojHru|3141|%s|16||Golden Axe II  World|René' % OTISK3 and pryc and o[1] == 'ONLINE · SPOUŠTÍM HRU',
             '8) PŘIPOJIT SE: stejna hra -> %s (panel zustal); z HRY -> %s (panel pryc, odznak "%s")' % (v1, v3, o[1]))
        stav(pg, 'faze=NIC\nstart=1', 300)
        klik(pg, '#segaOnlineTlacitko', 400)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam(MISTNOSTI))
        pg.wait_for_timeout(200)
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(2) .onlinePripoj')
        v2 = volani(pg)[-1]
        pryc2 = not otevreno(pg, '#segaOnlinePanel')
        pg.evaluate('(s) => { window.__onl.stav = s; }', 'krok=STAHUJI\nsparovano=1\nfaze=NIC\nstart=1')
        pg.wait_for_timeout(900)
        o2 = odznak(pg)
        stav(pg, 'faze=NIC\nstart=1', 300)
        klik(pg, '#segaOnlineTlacitko', 400)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam(MISTNOSTI))
        pg.wait_for_timeout(200)
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(4) .onlinePripoj')
        v4 = volani(pg)[-1]
        pg.evaluate('() => window.napOnline.vysledek({akce: "pripoj", ok: false, zprava: "Hru „Aladdin“ v telefonu nemáš. Hráč 1 ji má ze svého telefonu – nahraj si přesně stejný soubor do HRY a dej PŘIPOJIT SE znovu."})')
        pg.wait_for_timeout(700)
        k4 = pg.evaluate('() => [document.getElementById("segaOnlineStav").className, document.getElementById("segaOnlineStav").textContent]')
        over(v2 == 'pripojHru|8812|%s|16|sega-0123456789abcdef|Streets of Rage 2|René' % OTISK2 and pryc2 and o2[1] == 'ONLINE · STAHUJI HRU'
             and v4 == 'pripojHru|2718|%s|16||Aladdin|René' % OTISK4 and otevreno(pg, '#segaOnlinePanel') and k4[0] == 'chyba'
             and k4[1].startswith('Hru „Aladdin“ v telefonu nemáš.'),
             '   ze serveru -> panel pryc, odznak "%s"; hru nema -> chyba v karte (%s): "%s"' % (o2[1], k4[0], k4[1][:50]))
        snimek(pg, '5_hru_nemas')

        # ---------------------------------------------------------------- 9) konec s chybou
        pg.evaluate('() => window.napOnline.vysledek({akce: "zaloz", ok: true, zprava: "OK"})')
        klik(pg, '#segaOnlineClose')
        klik(pg, '#segaOnlineTlacitko', 300)
        stav(pg, 'faze=KONEC\nchyba=server neodpovedel (timeout)\nkonec=\naktivni=0\nhraje=0\nstart=2')
        k9 = text(pg, '#segaOnlineStav')
        stav(pg, 'faze=KONEC\nchyba=engine odmitl online (HTTP 400): {"ok":false,"duvod":"neni_websocket","zprava":"Online hra potřebuje spojení WebSocket."}\nkonec=\naktivni=0\nhraje=0\nstart=3')
        k9b = text(pg, '#segaOnlineStav')
        over(k9.startswith('Online skončilo: engine N&P neodpověděl') and 'Zkontroluj, že engine N&P běží a telefon má internet' in k9
             and k9b == 'Online skončilo: Online hra potřebuje spojení WebSocket.',
             '9) konec: site "%s" | engine "%s"' % (k9[:90], k9b))
        snimek(pg, '6_konec_sit')

        # ---------------------------------------------------------------- 10) hra zacala
        stav(pg, 'faze=HRAJE\nkod=0427\ndruhy=bracha\nodezva=23\nzpozdeni=3\naktivni=1\nhraje=1\nstart=4')
        o = odznak(pg)
        over(not otevreno(pg, '#segaOnlinePanel') and o[0] == 'block' and o[1] == 'ONLINE · bracha · 23 MS',
             '10) hra zacala: panel se zavrel sam, odznak "%s"' % o[1])

        # ---------------------------------------------------------------- 15) odznak se neprepisuje
        pg.evaluate('() => { window.__zmen = 0; new MutationObserver(function (m) { window.__zmen += m.length; })'
                    '.observe(document.getElementById("segaOnlineOdznak"), {childList: true, subtree: true, characterData: true, attributes: true}); }')
        pg.wait_for_timeout(3000)
        zm = pg.evaluate('() => window.__zmen')
        over(zm == 0, '15) stejny stav 3 s: odznak nad obrazem hry se neprepisuje (%d zmen)' % zm)
        stav(pg, 'faze=NIC\nstart=4', 300)
        ctx.close()

        # ================================================================ 11) na sirku
        ctx, pg = novy(b, url, NA_SIRKU, chyby)
        pg.evaluate('() => { var o = document.getElementById("btnSegaOtoc"); if (o) { o.style.display = "flex"; } }')   # na telefonu ho ukaze nap_obrazovka.js
        r = obdelnik(pg, '#segaOnlineTlacitko')
        jine = [s for s in ('#dpad', '#btnA', '#btnB', '#btnC', '#btnStart', '#segaBottomSave', '#segaBottomLoad', '#btnSegaSettings', '#btnSegaOtoc',
                            '#btnCollection', '#btnSaves', '#btnGames', '#btnManuals', '#btnWeb', '#btnMenu', '#cartSlot', '#cartCenter')
                if prekryv(r, obdelnik(pg, s))]
        load = obdelnik(pg, '#segaBottomLoad')
        vedle = r and load and r[0] > load[2] and abs((r[1] + r[3]) / 2 - (load[1] + load[3]) / 2) < 3
        snimek(pg, '7_na_sirku_tlacitko_online')
        klik(pg, '#segaOnlineTlacitko', 400)
        over(r is not None and vedle and not jine and otevreno(pg, '#segaOnlinePanel'),
             '11) na sirku: tlacitko ONLINE dole vpravo vedle LOAD GAME %s, nic neprekryva%s, otevre panel' % ([round(x) for x in r] if r else None, '' if not jine else ' - PREKRYVA ' + ', '.join(jine)))
        ctx.close()

        # ================================================================ 12) anglicky
        ctx, pg = novy(b, url, NA_VYSKU, chyby, jazyk='en')
        btn = text(pg, '#segaOnlineTlacitko').strip()
        klik(pg, '#segaOnlineTlacitko', 400)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam(MISTNOSTI))
        pg.wait_for_timeout(300)

        def panel_texty():
            return pg.evaluate('() => { var b = document.querySelector("#segaOnlinePanel .box"); var t = [];'
                               ' var w = document.createTreeWalker(b, NodeFilter.SHOW_TEXT); var n;'
                               ' while ((n = w.nextNode())) { var x = n.nodeValue.trim(); var e = n.parentNode;'
                               '   var skryto = false; for (; e && e !== b; e = e.parentNode) if (getComputedStyle(e).display === "none") skryto = true;'
                               '   if (x && !skryto) t.push(x); } return t; }')
        t1 = panel_texty()
        stav(pg, 'faze=CEKA_NA_HRACE\nkod=0427\naktivni=1\nhraje=0\nstart=1')
        t2 = panel_texty()
        cesky = [x for x in t1 + t2 if any(c in x for c in 'áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ') and 'René' not in x]
        over(btn == '🌐 ONLINE' and 'PLAY ONLINE' in t1 and 'JOIN A GAME (PLAYER 2)' in t1 and 'OR CREATE YOUR OWN GAME (PLAYER 1)' in t1
             and 'HOW IT WORKS' in t1 and 'Player 1 starts a game and taps CREATE A GAME.' in t1 and 'PICK A GAME (GAMES)' in t1
             and any('you have it in GAMES – it starts by itself' in x for x in t1) and 'WAITING FOR PLAYER 2' in t2 and 'CANCEL WAITING' in t2
             and any(x.startswith('On the other phone: SEGA → 🌐 ONLINE → tap JOIN at game 0427.') for x in t2) and not cesky,
             '12) anglicky: tlacitko "%s", panel i cekani bez cestiny%s' % (btn, '' if not cesky else ' - CESKY: ' + ' | '.join(cesky)))
        snimek(pg, '8_anglicky_ceka')
        ctx.close()

        # ================================================================ 13) starsi appka
        ctx, pg = novy(b, url, NA_VYSKU, chyby, online=None)
        klik(pg, '#segaOnlineTlacitko', 400)
        t = text(pg, '#segaOnlineSeznam')
        over(t.startswith('Online tady nejde'), '13) bez mostu AHONLINE: "%s"' % t)
        ctx.close()
        stary = AHONLINE.replace("    pripojHru: function (k, o, n, id, h, j) { window.__onl.volani.push(['pripojHru', k, o, n, id, h, j].join('|')); },\n", '')
        ctx, pg = novy(b, url, NA_VYSKU, chyby, online=stary)
        pg.evaluate('() => { window.__onl.hra = {otisk: "%s", nastaveni: "16", nazev: "Sonic", hra_id: "", jmeno: "Galaxy S8"}; }' % OTISK)
        klik(pg, '#segaOnlineTlacitko', 400)
        pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam(MISTNOSTI))
        pg.wait_for_timeout(200)
        klik(pg, '#segaOnlineSeznam .onlineHra:nth-child(1) .onlinePripoj')
        over(volani(pg)[-1] == 'pripoj|0427|%s|16||Galaxy S8' % OTISK, '   most B357 bez pripojHru -> %s' % volani(pg)[-1])

        # ================================================================ 14) klic nazvu = Java
        ctx.close()
        ctx, pg = novy(b, url, NA_VYSKU, chyby)
        # klic se pocita uvnitr stranky - overi se pres seznam: hra hrace 1 s timto nazvem, v HRY soubor s ocekavanym klicem
        shody = []
        for vz, ocek in KLICE:
            pg.evaluate('(k) => { window.__onl.knihovna = [{soubor: "x.gen", klic: k}]; window.__onl.hra = {otisk: "", nastaveni: "16", nazev: "", hra_id: "", jmeno: "S8"}; }', ocek)
            if otevreno(pg, '#segaOnlinePanel'):
                klik(pg, '#segaOnlineClose')
            klik(pg, '#segaOnlineTlacitko', 300)
            pg.evaluate('(r) => window.napOnline.vysledek(r)', seznam([{'kod': '1111', 'system': 'sega', 'hra': vz, 'hra_id': '', 'otisk': OTISK4,
                                                                         'nastaveni': '16', 'hrac': 'x', 'ceka_s': 1}]))
            pg.wait_for_timeout(150)
            shody.append('máš ji v HRY' in (radky_seznamu(pg) or [''])[0])
        over(all(shody), '14) klic nazvu hry v JS = Java NapOnlineHra.klicNazvu: %d/%d vzorku %s' % (sum(shody), len(shody), shody))
        ctx.close()
        b.close()
    over(not chyby, 'zadna chyba JavaScriptu' + ('' if not chyby else ': ' + ' | '.join(chyby[:5])))
    srv.shutdown()
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b358_stranka.txt'), 'w', encoding='utf-8') as f:
        f.write('B358 - stranka Segy: ONLINE JEDNODUSE - Chromium, stranka z appky, falesny most AHONLINE\n')
        f.write('\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
