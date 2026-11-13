#!/usr/bin/env python3
"""B321: STRANKY AKTIVACE TELEFONU A N&P HRY (Chromium, stranky presne z appky - assets).

Most AHHRY (MainActivity.AHHry) je falesny: zapisuje, co stranka zavolala, a odpovida asynchronne
jako Java (napHry.aktivace / napHry.seznam / napHry.over). Obaly https://nap.hry/obal/<id> obslouzi
Playwright (v appce MainActivity.napObal).

Aktivace (hry/aktivace.html):
  A1 klic v APK: sama aktivuje (AHHRY.aktivuj('')), "Aplikace je aktivovaná", pak AHHRY.pokracuj()
  A2 preposlana kopie (kod_pouzit): hlaska + formular; kod "nap 7k3q m9xd" -> NAP-7K3Q-M9XD -> aktivace
  A3 bez klice: hned formular; spatny kod -> "Tenhle kód neznám"; kratky kod se ani neposle
  A4 server nedostupny: hlaska + ZKUSIT ZNOVU -> znovu aktivuj('')
  A5 zablokovany telefon: hlaska + ZKUSIT ZNOVU -> AHHRY.over() -> odblokovano -> pokracuj
  A6 sparovany telefon: rovnou pokracuj;  A7 ZMENIT server -> AHHRY.nastavServer(...)
  A8 8 znaku bez NAP -> doplni NAP;  A9 mimo appku (bez AHHRY): hlaska;  A10 anglicky bez cestiny
N&P HRY (hry/index.html):
  L1 seznam: jen hry zalozky, pocty v zalozkach, stav "Server N&P · 3 hry"; jmeno s <img onerror> jen jako text
  L2 obaly z https://nap.hry/obal/<id>;  L3 hledani bez diakritiky;  L4 zalozka SEGA (adresa, barva)
  L5 klepnuti -> AHHRY.spust(system, id, soubor) + okno SPOUSTIM;  L6 bez site s ulozenym seznamem
  L7 server nedostupny bez seznamu;  L8 zamceno / nesparovano -> aktivace.html
  L9 ZPET -> AHHRY.zpet(system), web -> AHHRY.web();  L10 anglicky bez cestiny (krome jmen her)
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b321_stranky.py      -> VYSTUP_test_b321_stranky.txt
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


def over(ok, co, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co + ('' if ok or not detail else '   -> ' + str(detail)[:300]))
    print(vystup[-1], flush=True)


NA_VYSKU = {'width': 412, 'height': 846}
PNG = bytes.fromhex('89504e470d0a1a0a0000000d4948445200000001000000010806000000'
                    '1f15c4890000000d49444154789c6360f8cfc0f01f0005000201e2a3f0a30000000049454e44ae426082')


def ahhry(stav, aktivace=None, seznam=None, over_odp=None, prompt_odp=None):
    """falesny most AHHRY: odpovedi na aktivuj() postupne z pole aktivace, seznam() z 'seznam'"""
    return '''
(function(){
  window.__hry = {stav: %s, aktivace: %s, seznam: %s, over: %s, volani: []};
  function zapis(t) { window.__hry.volani.push(t); }
  window.AHHRY = {
    stav: function () { zapis('stav'); return JSON.stringify(window.__hry.stav); },
    aktivuj: function (k) {
      zapis('aktivuj:' + k);
      var r = window.__hry.aktivace.length ? window.__hry.aktivace.shift() : {ok: false, kod: 0, duvod: 'sit', zprava: ''};
      if (r.ok) { window.__hry.stav.sparovano = true; }
      setTimeout(function () { window.napHry.aktivace(JSON.parse(JSON.stringify(r))); }, 30);
    },
    over: function () {
      zapis('over');
      var r = window.__hry.over || {ok: false, kod: 0, duvod: 'sit'};
      if (r.ok) window.__hry.stav.zablokovano = false;
      setTimeout(function () { window.napHry.over(JSON.parse(JSON.stringify(r))); }, 30);
    },
    pokracuj: function () { zapis('pokracuj'); },
    nastavServer: function (u) { zapis('nastavServer:' + u); window.__hry.stav.server = u; return u; },
    seznam: function () {
      zapis('seznam');
      var r = window.__hry.seznam;
      setTimeout(function () { window.napHry.seznam(JSON.parse(JSON.stringify(r))); }, 30);
    },
    spust: function (s, id, f) { zapis('spust:' + s + ':' + id + ':' + f); },
    zpet: function (s) { zapis('zpet:' + s); },
    web: function () { zapis('web'); }
  };
  %s
})();
''' % (json.dumps(stav), json.dumps(aktivace or []), json.dumps(seznam or {}), json.dumps(over_odp),
       ('window.prompt = function (m, d) { (window.__napDialogy = window.__napDialogy || []).push("prompt:" + m); return %s; };'
        % json.dumps(prompt_odp)) if prompt_odp is not None else '')


STAV = {'sparovano': False, 'zablokovano': False, 'server': 'http://46.33.114.139:8080', 'vychozi': 'http://46.33.114.139:8080',
        'kod': '', 'klic': True, 'verze': 'EMU10-B321-SERVER', 'jazyk': 'cs', 'telefon': '0718'}
STAV_OK = dict(STAV, sparovano=True, kod='NAP-7K**-****')
HRY = {'ok': True, 'server_cas': 1794600000, 'hry': [
    {'id': 'atari-0123456789abcdef', 'system': 'atari', 'nazev': 'Moon Cresta v1.7 (PAL)', 'soubor': 'Moon Cresta v1.7 (PAL).car',
     'velikost': 131088, 'sha256': '', 'zmeneno': 1},
    {'id': 'atari-1111111111111111', 'system': 'atari', 'nazev': "Tom & Jerry's Žluťoučký kůň", 'soubor': "Tom & Jerry's Žluťoučký kůň.xex",
     'velikost': 5000, 'sha256': '', 'zmeneno': 1},
    {'id': 'atari-2222222222222222', 'system': 'atari', 'nazev': '<img src=x onerror="window.__xss=1">Zla', 'soubor': 'zla.xex',
     'velikost': 900, 'sha256': '', 'zmeneno': 1},
    {'id': 'sega-3333333333333333', 'system': 'sega', 'nazev': 'Sonic The Hedgehog (USA, Europe)', 'soubor': 'Sonic The Hedgehog (USA, Europe).gen',
     'velikost': 524288, 'sha256': '', 'zmeneno': 1},
    {'id': 'ps1-4444444444444444', 'system': 'ps1', 'nazev': 'Crash Bandicoot (Europe)', 'soubor': 'Crash Bandicoot (Europe).zip',
     'velikost': 371000000, 'sha256': '', 'zmeneno': 1},
    {'id': 'ps1-5555555555555555', 'system': 'ps1', 'nazev': 'Dune 2000', 'soubor': 'Dune 2000.zip',
     'velikost': 290000000, 'sha256': '', 'zmeneno': 1},
]}


def otevri(prohl, base, cesta, jazyk, skript, chyby_js, obaly=None):
    ctx = prohl.new_context(viewport=NA_VYSKU, is_mobile=True, has_touch=True)
    ctx.add_init_script(H.init_skript(jazyk))
    ctx.add_init_script(skript)

    def obal(route):
        if obaly is not None:
            obaly.append(route.request.url)
        route.fulfill(status=200, content_type='image/png', body=PNG)
    ctx.route('https://nap.hry/**', obal)
    s = ctx.new_page()
    s.on('pageerror', lambda e: chyby_js.append(cesta + ': ' + str(e)))
    s.goto(base + cesta)
    return ctx, s


def volani(s):
    return s.evaluate('window.__hry ? window.__hry.volani.slice() : []')


def text(s, sel):
    return s.evaluate('(q) => { var e = document.querySelector(q); return e ? e.textContent : null; }', sel)


def viditelne(s, sel):
    return s.evaluate('(q) => { var e = document.querySelector(q); if (!e) return false; var c = getComputedStyle(e); return c.display !== "none" && c.visibility !== "hidden"; }', sel)


def cestina(s, povolene=()):
    sber = s.evaluate(H.SBER_JS)
    return H.zbytky_cestiny(sber, povolena_slova=povolene)


def hlavni():
    srv, base = H.server()
    chyby_js = []
    with sync_playwright() as p:
        prohl = p.chromium.launch()

        # ---------------------------------------------------------------- aktivace
        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(STAV, aktivace=[{'ok': True, 'kod': 200, 'duvod': '', 'zprava': ''}]), chyby_js)
        s.wait_for_timeout(1300)
        v = volani(s)
        over(v[:1] == ['stav'] and 'aktivuj:' in v and v[-1] == 'pokracuj' and text(s, '#zprava') == 'Aplikace je aktivovaná',
             'A1 klic v APK: sama se aktivuje a pokracuje do appky', (v, text(s, '#zprava')))
        over(not viditelne(s, '#formular'), 'A1 formular kodu se neukazal')
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(STAV, aktivace=[
            {'ok': False, 'kod': 403, 'duvod': 'kod_pouzit', 'zprava': 'Tahle kopie aplikace patří jinému telefonu. Zadej svůj osobní kód.'},
            {'ok': True, 'kod': 200, 'duvod': '', 'zprava': ''}]), chyby_js)
        s.wait_for_timeout(300)
        over(text(s, '#zprava') == 'Tahle kopie aplikace patří jinému telefonu' and viditelne(s, '#formular') and 'chyba' in s.get_attribute('#zprava', 'class'),
             'A2 preposlana kopie: hlaska a formular kodu', text(s, '#zprava'))
        s.fill('#kod', '')
        s.type('#kod', 'nap 7k3q m9xd')
        over(s.input_value('#kod') == 'NAP-7K3Q-M9XD', 'A2 kod se pri psani upravi na NAP-7K3Q-M9XD', s.input_value('#kod'))
        s.click('#potvrdit')
        s.wait_for_timeout(1300)
        v = volani(s)
        over('aktivuj:NAP-7K3Q-M9XD' in v and v[-1] == 'pokracuj', 'A2 zadany kod -> aktivace -> pokracuj', v)
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(dict(STAV, klic=False), aktivace=[
            {'ok': False, 'kod': 403, 'duvod': 'spatny_kod', 'zprava': 'Tenhle kód neznám.'}]), chyby_js)
        s.wait_for_timeout(200)
        over(viditelne(s, '#formular') and 'aktivuj:' not in volani(s) and text(s, '#zprava') == 'Zadej svůj osobní kód',
             'A3 APK bez klice: hned formular, nic se neposila', volani(s))
        s.fill('#kod', 'NAP-12')
        s.click('#potvrdit')
        s.wait_for_timeout(150)
        over(text(s, '#zprava') == 'Kód má tvar NAP-XXXX-XXXX' and not any(x.startswith('aktivuj') for x in volani(s)),
             'A3 neuplny kod se ani neposle', volani(s))
        s.fill('#kod', 'NAP-ZZZZ-ZZZZ')
        s.click('#potvrdit')
        s.wait_for_timeout(300)
        over(text(s, '#zprava') == 'Tenhle kód neznám' and viditelne(s, '#formular'), 'A3 spatny kod -> "Tenhle kód neznám"', text(s, '#zprava'))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(STAV, aktivace=[
            {'ok': False, 'kod': 0, 'duvod': 'sit', 'zprava': 'server nedostupný (ConnectException)'},
            {'ok': True, 'kod': 200, 'duvod': '', 'zprava': ''}]), chyby_js)
        s.wait_for_timeout(300)
        over(text(s, '#zprava') == 'Server N&P je nedostupný' and viditelne(s, '#znovu'), 'A4 server nedostupny: hlaska + ZKUSIT ZNOVU', text(s, '#zprava'))
        s.click('#znovu')
        s.wait_for_timeout(1300)
        v = volani(s)
        over(v.count('aktivuj:') == 2 and v[-1] == 'pokracuj', 'A4 ZKUSIT ZNOVU -> znovu klicem z APK -> pokracuj', v)
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(dict(STAV_OK, zablokovano=True), over_odp={'ok': True, 'kod': 200, 'duvod': ''}), chyby_js)
        s.wait_for_timeout(200)
        over(text(s, '#zprava') == 'Tento telefon je zablokovaný' and viditelne(s, '#znovu') and not viditelne(s, '#formular'),
             'A5 zablokovany telefon: hlaska, bez formulare', text(s, '#zprava'))
        s.click('#znovu')
        s.wait_for_timeout(1300)
        v = volani(s)
        over('over' in v and v[-1] == 'pokracuj', 'A5 po odblokovani ZKUSIT ZNOVU -> over -> pokracuj', v)
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(STAV_OK, prompt_odp='46.33.114.140:8080'), chyby_js)
        s.wait_for_timeout(1200)
        over(volani(s)[-1] == 'pokracuj' and not any(x.startswith('aktivuj') for x in volani(s)), 'A6 sparovany telefon: rovnou dal', volani(s))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(dict(STAV, klic=False), prompt_odp='46.33.114.140:8080'), chyby_js)
        s.click('#zmenit')
        s.wait_for_timeout(100)
        over('nastavServer:46.33.114.140:8080' in volani(s) and text(s, '#server') == '46.33.114.140:8080', 'A7 ZMĚNIT server', volani(s))
        s.fill('#kod', '')
        s.type('#kod', '7k3qm9xd')
        over(s.input_value('#kod') == 'NAP-7K3Q-M9XD', 'A8 8 znaku bez NAP -> NAP-7K3Q-M9XD', s.input_value('#kod'))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', '/* bez AHHRY */', chyby_js)
        s.wait_for_timeout(200)
        over(text(s, '#zprava') == 'Aktivace běží jen v aplikaci', 'A9 mimo appku: hlaska, zadna chyba', text(s, '#zprava'))
        ctx.close()

        # anglicky: forwarded copy (formular), zablokovany, nedostupny
        for nazev, st, akt in (('preposlana kopie', STAV, [{'ok': False, 'kod': 403, 'duvod': 'kod_pouzit',
                                                            'zprava': 'Tahle kopie aplikace patří jinému telefonu. Zadej svůj osobní kód.'}]),
                               ('zablokovany', dict(STAV_OK, zablokovano=True), []),
                               ('server nedostupny', STAV, [{'ok': False, 'kod': 0, 'duvod': 'sit', 'zprava': ''}]),
                               ('bez klice', dict(STAV, klic=False), [])):
            ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'en', ahhry(dict(st, jazyk='en'), aktivace=akt), chyby_js)
            s.wait_for_timeout(400)
            z = cestina(s)
            over(not z and s.evaluate('document.documentElement.lang') == 'en', 'A10 anglicky (%s): bez cestiny' % nazev, z)
            if nazev == 'preposlana kopie':
                over(text(s, '#zprava') == 'This copy of the app belongs to another phone', 'A10 anglicky: hlaska preposlane kopie', text(s, '#zprava'))
            ctx.close()

        # ---------------------------------------------------------------- N&P HRY
        obaly = []
        ctx, s = otevri(prohl, base, 'hry/index.html?system=atari', 'cs', ahhry(STAV_OK, seznam={'ok': True, 'offline': False, 'kod': 200,
                        'duvod': '', 'zprava': '', 'zamceno': False, 'data': HRY}), chyby_js, obaly)
        s.wait_for_timeout(500)
        nazvy = s.evaluate('Array.prototype.map.call(document.querySelectorAll(".hra .nazev"), function (e) { return e.textContent; })')
        over(nazvy == ['Moon Cresta v1.7 (PAL)', "Tom & Jerry's Žluťoučký kůň", '<img src=x onerror="window.__xss=1">Zla'],
             'L1 seznam: jen hry Atari, v poradi ze serveru', nazvy)
        over(s.evaluate('!window.__xss && !document.querySelector(".nazev img")'), 'L1 jmeno hry s <img onerror> je jen text (zadny skript)')
        over([text(s, '#pocet-atari'), text(s, '#pocet-sega'), text(s, '#pocet-ps1')] == ['3', '1', '2'] and text(s, '#stav') == 'Server N&P · 3 hry',
             'L1 pocty v zalozkach a stav', [text(s, '#pocet-atari'), text(s, '#pocet-sega'), text(s, '#pocet-ps1'), text(s, '#stav')])
        over(s.evaluate('document.querySelector(".hra .znacka").textContent') == 'CAR' and
             s.evaluate('document.querySelector(".hra .meta span:last-child").textContent') == '128 kB', 'L1 znacka CAR a velikost 128 kB')
        over(len(obaly) >= 1 and all(u.startswith('https://nap.hry/obal/atari-') for u in obaly), 'L2 obaly z https://nap.hry/obal/<id>', obaly)
        s.fill('#hledat', 'zlutou')
        s.wait_for_timeout(100)
        nazvy = s.evaluate('Array.prototype.map.call(document.querySelectorAll(".hra .nazev"), function (e) { return e.textContent; })')
        over(nazvy == ["Tom & Jerry's Žluťoučký kůň"], 'L3 hledani bez diakritiky ("zlutou")', nazvy)
        s.fill('#hledat', 'xyzxyz')
        s.wait_for_timeout(100)
        over(text(s, '.prazdno') == 'Nic nenalezeno.', 'L3 nic nenalezeno', text(s, '.prazdno'))
        s.fill('#hledat', '')
        s.click('.zalozka[data-system=sega]')
        s.wait_for_timeout(100)
        over(s.evaluate('location.search') == '?system=sega' and s.evaluate('document.body.className') == 'sega' and
             s.evaluate('document.querySelectorAll(".hra").length') == 1, 'L4 zalozka SEGA: adresa, barva, 1 hra')
        s.click('.hra')
        s.wait_for_timeout(100)
        over('spust:sega:sega-3333333333333333:Sonic The Hedgehog (USA, Europe).gen' in volani(s) and
             s.evaluate('document.getElementById("prekryv").className') == 'zobraz' and text(s, '#prekryvNazev') == 'Sonic The Hedgehog (USA, Europe)',
             'L5 klepnuti -> AHHRY.spust + okno spousteni', volani(s))
        s.evaluate('document.getElementById("prekryv").className = ""')
        s.click('#zpet')
        s.click('#web')
        s.wait_for_timeout(100)
        over('zpet:sega' in volani(s) and 'web' in volani(s), 'L9 ZPET -> zpet(sega), web -> web()', volani(s))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/index.html?system=ps1', 'cs', ahhry(STAV_OK, seznam={'ok': True, 'offline': True, 'kod': 0,
                        'duvod': 'sit', 'zprava': '', 'zamceno': False, 'data': HRY}), chyby_js)
        s.wait_for_timeout(400)
        over(text(s, '#stav') == 'Bez připojení k serveru – ukazuji uložený seznam.' and s.get_attribute('#stav', 'class') == 'chyba'
             and s.evaluate('document.querySelectorAll(".hra").length') == 2, 'L6 bez site: ulozeny seznam + upozorneni', text(s, '#stav'))
        s.click('.hra')
        over(text(s, '#prekryvText') == 'Hra se stahuje v PS1 – průběh uvidíš tam.', 'L6 PS1: text o prubehu v PS1', text(s, '#prekryvText'))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/index.html', 'cs', ahhry(STAV_OK, seznam={'ok': False, 'offline': True, 'kod': 0,
                        'duvod': 'sit', 'zprava': 'server nedostupný (ConnectException)', 'zamceno': False, 'data': None}), chyby_js)
        s.wait_for_timeout(400)
        over(text(s, '#stav') == 'Server N&P je nedostupný. Zkontroluj připojení a klepni na ↻.' and s.evaluate('document.querySelectorAll(".hra").length') == 0
             and s.evaluate('location.search') == '?system=atari', 'L7 server nedostupny bez seznamu (vychozi zalozka ATARI)', text(s, '#stav'))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/index.html?system=sega', 'cs', ahhry(STAV_OK, seznam={'ok': False, 'offline': True, 'kod': 401,
                        'duvod': 'nesparovano', 'zprava': '', 'zamceno': True, 'data': None}), chyby_js)
        s.wait_for_timeout(500)
        over(s.url.endswith('hry/aktivace.html'), 'L8 server zrusil parovani -> aktivace', s.url)
        ctx.close()
        ctx, s = otevri(prohl, base, 'hry/index.html', 'cs', ahhry(STAV), chyby_js)
        s.wait_for_timeout(400)
        over(s.url.endswith('hry/aktivace.html'), 'L8 nesparovany telefon -> aktivace', s.url)
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/index.html?system=atari', 'en', ahhry(dict(STAV_OK, jazyk='en'), seznam={'ok': True, 'offline': False,
                        'kod': 200, 'duvod': '', 'zprava': '', 'zamceno': False, 'data': HRY}), chyby_js)
        s.wait_for_timeout(500)
        z = cestina(s)
        over(not z and text(s, '#stav') == 'N&P server · 3 games' and s.get_attribute('#hledat', 'placeholder') == 'Search games'
             and text(s, '#zpet') == '◀ BACK', 'L10 anglicky: bez cestiny (jmena her se neprekladaji)', (z, text(s, '#stav'), text(s, '#zpet')))
        s.fill('#hledat', 'xyz')
        s.wait_for_timeout(100)
        over(text(s, '.prazdno') == 'Nothing found.', 'L10 anglicky: Nothing found.', text(s, '.prazdno'))
        ctx.close()

        prohl.close()
    srv.shutdown()
    over(not chyby_js, 'zadna chyba JavaScriptu', chyby_js)
    vystup.append('')
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (sum(1 for r in vystup if r.startswith('OK')), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b321_stranky.txt'), 'w', encoding='utf-8').write(
        'B321 - stranky AKTIVACE a N&P HRY v Chromiu (assets appky, falesny most AHHRY)\n\n' + '\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(hlavni())
