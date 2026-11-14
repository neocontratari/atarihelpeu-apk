#!/usr/bin/env python3
"""B322: STRANKA ODEMKNOUT HNED (hry/odemknout.html) + oprava automaticke aktivace (hry/aktivace.html)
v Chromiu, stranky presne z appky (assets). Most AHHRY je falesny (odpovida asynchronne jako Java).
QR kody z platna stranky i obrazek pro Galerii se ctou ZXing (zxing-cpp; jinak OpenCV) - musi
dat presne text QR Platby (SPD) / evropskeho QR (EPC), ktery posila server N&P.

  O1  Sega zamcena: postup 123 / 1500, cena 75 Kc, QR Platba = SPD ze serveru, ucet / IBAN / VS / zprava
  O2  zalozka SEPA: 2,99 EUR, QR = EPC ze serveru (BCD / 002 / ... / EUR2.99 / /VS/)
  O3  KOPIROVAT -> AHHRY.kopiruj(VS, ucet, castka)
  O4  ULOZIT QR DO GALERIE -> AHHRY.ulozQr(PNG): obrazek nese stejny QR, hlaska kam se ulozil
  O5  ZAPLATIL JSEM -> AHHRY.zaplatil('sega') -> CEKA NA POTVRZENI, tlacitko zmizi, kazdych 20 s platba()
  O6  spravce odemkl (napHry.obnov) -> "SEGA je odemcena", SPUSTIT SEGU -> AHHRY.spustKonzoli('sega')
  O7  PS1 bez Segy: PS1 az po Seze, ODEMKNOUT SEGU -> AHHRY.odemknout('sega'), zadna platba
  O8  PS1 s otevrenou Segou: odehrano 3 h 12 min ze 120 h, 125 Kc, QR PS1
  O9  server nedostupny -> hlaska + ZKUSIT ZNOVU;  O10 platby nenastavene;  O11 PS1 nejdriv_sega
  O12 most odpovi az po chvili (start appky) -> stranka pocka a funguje
  O13 anglicky: zadna cestina (krome uctu, jmena, Kc, QR Platba);  O14 DO MENU -> AHHRY.menu()
  R1  AKTIVACE: most pri startu appky odpovi "{}" (B321 test Reneho: klic v APK se nezkusil) -> stranka
      pocka a aktivuje se SAMA klicem z APK (aktivuj(''))
  R2  N&P HRY: most odpovi "{}" -> stranka NEskoci na aktivaci a nacte seznam
  M1  HTML zaloha menu (index.html): texty zamku stejne jako Java (NapPravidla, TestB322 B2-B4)
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b322_stranky.py      -> VYSTUP_test_b322_stranky.txt
"""
import base64
import json
import os
import sys

import cv2
import numpy as np
import zxingcpp
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
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co + ('' if ok or not detail else '   -> ' + str(detail)[:400]))
    print(vystup[-1], flush=True)


NA_VYSKU = {'width': 412, 'height': 846}

SPD_SEGA = 'SPD*1.0*ACC:CZ6508000000192000145399*AM:75.00*CC:CZK*X-VS:10011*MSG:N&P SEGA 10011*RN:Rene Novacek'
EPC_SEGA = 'BCD\n002\n1\nSCT\n\nRene Novacek\nCZ6508000000192000145399\nEUR2.99\n\n\n/VS/10011 N&P SEGA'
SPD_PS1 = 'SPD*1.0*ACC:CZ6508000000192000145399*AM:125.00*CC:CZK*X-VS:10012*MSG:N&P PS1 10012*RN:Rene Novacek'
EPC_PS1 = 'BCD\n002\n1\nSCT\n\nRene Novacek\nCZ6508000000192000145399\nEUR4.99\n\n\n/VS/10012 N&P PS1'


def platba(co='sega', **zmeny):
    d = {'ok': True, 'server_cas': 1794600000, 'co': co, 'nastaveno': True, 'koupeno': False, 'zadost': False,
         'sega_otevrena': co == 'ps1', 'kc': 75 if co == 'sega' else 125, 'eur': '2.99' if co == 'sega' else '4.99',
         'vs': '10011' if co == 'sega' else '10012', 'iban': 'CZ6508000000192000145399', 'iban_hezky': 'CZ65 0800 0000 1920 0014 5399',
         'ucet_cz': '19-2000145399/0800', 'bic': '', 'jmeno': 'Rene Novacek', 'zprava': 'N&P SEGA 10011' if co == 'sega' else 'N&P PS1 10012',
         'spd': SPD_SEGA if co == 'sega' else SPD_PS1, 'epc': EPC_SEGA if co == 'sega' else EPC_PS1}
    d.update(zmeny)
    return d


MISTNI = {'sega': False, 'ps1': False, 'segaKoupena': False, 'ps1Koupena': False, 'radku': 123, 'potrebaRadku': 1500,
          'segaMs': 0, 'potrebaMs': 432000000, 'ceka': False, 'sparovano': True, 'jazyk': 'cs'}


def ahhry(mistni, platby=None, zaplatil=None, prazdnych=0, stav=None, aktivace=None, seznam=None, stav_prazdnych=0):
    """falesny most AHHRY: odemknuti() vraci mistni (prvnich 'prazdnych' volani "{}" - start appky),
    platba() postupne z pole platby (posledni se opakuje), zaplatil() z 'zaplatil'."""
    return '''
(function(){
  window.__hry = {m: %s, platby: %s, zaplatil: %s, prazdnych: %d, stav: %s, aktivace: %s, seznam: %s, stavPrazdnych: %d, volani: []};
  var X = window.__hry;
  function zapis(t) { X.volani.push(t); }
  window.AHHRY = {
    odemknuti: function () { zapis('odemknuti'); if (X.prazdnych > 0) { X.prazdnych--; return '{}'; } return JSON.stringify(X.m); },
    platba: function (co) {
      zapis('platba:' + co);
      var r = X.platby.length > 1 ? X.platby.shift() : X.platby[0];
      var o = JSON.parse(JSON.stringify(r || {ok: false, kod: 0, duvod: 'sit', zprava: '', data: null}));
      // jako Java: mistni stav (odemknuti) az po odpovedi serveru
      setTimeout(function () { o.mistni = JSON.parse(JSON.stringify(X.m)); window.napHry.platba(o); }, 30);
    },
    zaplatil: function (co) {
      zapis('zaplatil:' + co);
      var r = X.zaplatil || {ok: true, kod: 200, duvod: '', zprava: ''};
      setTimeout(function () { window.napHry.zaplatil(JSON.parse(JSON.stringify(r))); }, 30);
    },
    ulozQr: function (png, nazev) { zapis('ulozQr:' + nazev); X.png = png; return 'Pictures/AtariHelp/NaP_platba_' + nazev + '.png'; },
    kopiruj: function (t) { zapis('kopiruj:' + t); },
    odemknout: function (co) { zapis('odemknout:' + co); },
    spustKonzoli: function (co) { zapis('spustKonzoli:' + co); },
    menu: function () { zapis('menu'); },
    zkontroluj: function () { zapis('zkontroluj'); },
    // aktivace / N&P HRY
    stav: function () { zapis('stav'); if (X.stavPrazdnych > 0) { X.stavPrazdnych--; return '{}'; } return JSON.stringify(X.stav || {}); },
    aktivuj: function (k) {
      zapis('aktivuj:' + k);
      var r = (X.aktivace && X.aktivace.length) ? X.aktivace.shift() : {ok: false, kod: 0, duvod: 'sit', zprava: ''};
      if (r.ok && X.stav) X.stav.sparovano = true;
      setTimeout(function () { window.napHry.aktivace(JSON.parse(JSON.stringify(r))); }, 30);
    },
    pokracuj: function () { zapis('pokracuj'); },
    over: function () { zapis('over'); },
    nastavServer: function (u) { return u; },
    seznam: function () { zapis('seznam'); setTimeout(function () { window.napHry.seznam(JSON.parse(JSON.stringify(X.seznam))); }, 30); },
    spust: function () {}, zpet: function () {}, web: function () {}
  };
})();
''' % (json.dumps(mistni), json.dumps(platby or []), json.dumps(zaplatil), prazdnych, json.dumps(stav), json.dumps(aktivace or []),
       json.dumps(seznam or {}), stav_prazdnych)


def otevri(prohl, base, cesta, jazyk, skript, chyby_js, hodiny=False):
    ctx = prohl.new_context(viewport=NA_VYSKU, is_mobile=True, has_touch=True)
    ctx.add_init_script(H.init_skript(jazyk))
    ctx.add_init_script(skript)
    s = ctx.new_page()
    if hodiny:
        s.clock.install()
    s.on('pageerror', lambda e: chyby_js.append(cesta + ': ' + str(e)))
    s.goto(base + cesta)
    return ctx, s


def volani(s):
    return s.evaluate('window.__hry ? window.__hry.volani.slice() : []')


def text(s, sel):
    return s.evaluate('(q) => { var e = document.querySelector(q); return e ? e.textContent : null; }', sel)


def viditelne(s, sel):
    return s.evaluate('(q) => { var e = document.querySelector(q); if (!e) return false; var c = getComputedStyle(e);'
                      ' if (c.display === "none" || c.visibility === "hidden") return false;'
                      ' for (var p = e.parentElement; p; p = p.parentElement) if (getComputedStyle(p).display === "none") return false; return true; }', sel)


def radky(s):
    return s.evaluate('Array.prototype.map.call(document.querySelectorAll("#udaje .radek"), function (r) {'
                      ' return r.querySelector(".co").textContent + "=" + r.querySelector(".hodnota").textContent; })')


def qr_z_png(data_url):
    """QR z obrazku: ZXing (zxing-cpp, stejna knihovna jako cte mnoho bankovnich appek), jinak OpenCV."""
    b = base64.b64decode(data_url.split(',', 1)[1])
    img = cv2.imdecode(np.frombuffer(b, np.uint8), cv2.IMREAD_COLOR)
    if img is None:
        return None
    r = [x for x in zxingcpp.read_barcodes(img) if x.format == zxingcpp.BarcodeFormat.QRCode]
    if len(r) == 1:
        return r[0].text
    txt, body, _ = cv2.QRCodeDetector().detectAndDecode(img)
    return txt


def qr_strany(s):
    return qr_z_png(s.evaluate('document.getElementById("qr").toDataURL("image/png")'))


def cestina(s, povolene=()):
    sber = s.evaluate(H.SBER_JS)
    return H.zbytky_cestiny(sber, povolena_slova=povolene)


def hlavni():
    srv, base = H.server()
    chyby_js = []
    with sync_playwright() as p:
        prohl = p.chromium.launch()

        # ---------------------------------------------------------------- O1-O6 Sega
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', ahhry(MISTNI, platby=[{'ok': True, 'kod': 200, 'data': platba()}]), chyby_js)
        s.wait_for_timeout(500)
        over(text(s, '#stavText') == 'SEGA je zamčená.' and text(s, '#konzole') == 'SEGA MEGA DRIVE', 'O1 stav: SEGA je zamčená', text(s, '#stavText'))
        over(text(s, '#ukol') == 'Napiš v Atari 1500 řádků programu.' and text(s, '#pruhText') == 'Napsáno 123 z 1500 řádků.'
             and abs(float(s.evaluate('parseFloat(document.getElementById("pruh").style.width)')) - 8.2) < 0.1,
             'O1 zdarma: 1500 řádků, napsáno 123, pruh 8,2 %', (text(s, '#ukol'), text(s, '#pruhText')))
        over(text(s, '#cenaHlavni') == '75 Kč' and text(s, '#cenaVedlejsi') == 'z ciziny 2,99 € (SEPA)', 'O1 cena 75 Kč (z ciziny 2,99 €)',
             (text(s, '#cenaHlavni'), text(s, '#cenaVedlejsi')))
        r = radky(s)
        over(r == ['Částka=75 Kč', 'Účet=19-2000145399/0800', 'IBAN=CZ65 0800 0000 1920 0014 5399', 'VS=10011', 'Zpráva=N&P SEGA 10011'],
             'O1 údaje: částka, účet, IBAN, VS, zpráva', r)
        q = qr_strany(s)
        over(q == SPD_SEGA, 'O1 QR na stránce (ZXing) = QR Platba ze serveru', repr(q))
        over(viditelne(s, '#zaplatil') and not viditelne(s, '#ceka') and not viditelne(s, '#spustit'), 'O1 ZAPLATIL JSEM vidět, ČEKÁ ne')

        s.click('.zalozka[data-druh=eu]')
        s.wait_for_timeout(100)
        q = qr_strany(s)
        r = radky(s)
        over(q == EPC_SEGA, 'O2 SEPA: QR (ZXing) = evropský QR ze serveru (EUR2.99, /VS/10011)', repr(q))
        over(text(s, '#cenaHlavni') == '2,99 €' and r == ['Částka=2,99 €', 'IBAN=CZ65 0800 0000 1920 0014 5399', 'Příjemce=Rene Novacek',
                                                         'Zpráva=/VS/10011 N&P SEGA'], 'O2 SEPA: 2,99 €, IBAN, příjemce, zpráva', (text(s, '#cenaHlavni'), r))
        s.click('.zalozka[data-druh=cz]')
        s.click('#udaje .radek:nth-child(4) .kopie')
        s.click('#udaje .radek:nth-child(2) .kopie')
        s.click('#udaje .radek:nth-child(1) .kopie')
        v = volani(s)
        over('kopiruj:10011' in v and 'kopiruj:19-2000145399/0800' in v and 'kopiruj:75' in v, 'O3 KOPÍROVAT: VS, účet, částka', v)

        s.click('#ulozit')
        png = s.evaluate('window.__hry.png')
        over('ulozQr:SEGA_CZ' in volani(s) and png and png.startswith('data:image/png;base64,') and qr_z_png(png) == SPD_SEGA,
             'O4 ULOŽIT QR: PNG pro Galerii nese stejnou QR Platbu', (volani(s)[-1:], qr_z_png(png) if png else None))
        over(text(s, '#radaQr').startswith('Uloženo do galerie: Pictures/AtariHelp/NaP_platba_SEGA_CZ.png.'), 'O4 hláška, kam se QR uložil', text(s, '#radaQr'))

        s.click('#zaplatil')
        s.wait_for_timeout(200)
        over('zaplatil:sega' in volani(s) and viditelne(s, '#ceka') and not viditelne(s, '#zaplatil'),
             'O5 ZAPLATIL JSEM -> AHHRY.zaplatil(sega), ČEKÁ NA POTVRZENÍ, tlačítko zmizí', (volani(s)[-3:], viditelne(s, '#ceka')))
        s.click('#zkontroluj')
        over(volani(s)[-2:] == ['zkontroluj', 'platba:sega'], 'O5 ZKONTROLOVAT -> AHHRY.zkontroluj() + platba()', volani(s)[-2:])
        # spravce odemkl: Java zavola napHry.obnov()
        s.evaluate('window.__hry.m.sega = true; window.__hry.m.segaKoupena = true; window.napHry.obnov();')
        s.wait_for_timeout(300)
        over(text(s, '#stavText') == 'SEGA je odemčená.' and viditelne(s, '#spustit') and text(s, '#spustit') == 'SPUSTIT SEGU'
             and not viditelne(s, '#hned') and not viditelne(s, '#zdarma'), 'O6 odemčeno: SEGA je odemčená, SPUSTIT SEGU, platba schovaná',
             (text(s, '#stavText'), text(s, '#spustit')))
        s.click('#spustit')
        over(volani(s)[-1] == 'spustKonzoli:sega', 'O6 SPUSTIT SEGU -> AHHRY.spustKonzoli(sega)', volani(s)[-1:])
        s.click('#doMenu')
        over(volani(s)[-1] == 'menu', 'O14 DO MENU -> AHHRY.menu()', volani(s)[-1:])
        ctx.close()

        # O5b hlidani: ceka -> kazdych 20 s platba() (hodiny Playwright)
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', ahhry(MISTNI, platby=[{'ok': True, 'kod': 200, 'data': platba(zadost=True)}]),
                        chyby_js, hodiny=True)
        s.clock.run_for(1000)
        n0 = volani(s).count('platba:sega')
        s.clock.run_for(41000)
        n1 = volani(s).count('platba:sega')
        over(n0 == 1 and n1 == 3 and viditelne(s, '#ceka') and not viditelne(s, '#zaplatil'),
             'O5 už čeká (po restartu appky): ČEKÁ vidět a každých 20 s se ptá serveru', (n0, n1))
        ctx.close()

        # ---------------------------------------------------------------- O7 PS1 bez Segy
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=ps1', 'cs', ahhry(MISTNI, platby=[{'ok': True, 'kod': 200, 'data': platba('ps1')}]), chyby_js)
        s.wait_for_timeout(400)
        over(text(s, '#stavText') == 'PS1 jde až po Seze. Nejdřív odemkni Segu.' and viditelne(s, '#naSegu') and not viditelne(s, '#hned')
             and 'platba:ps1' not in volani(s) and text(s, '#ukol') == 'Nejdřív Sega, pak 120 hodin hraní na Seze.',
             'O7 PS1 bez Segy: PS1 až po Seze, žádná platba', (text(s, '#stavText'), volani(s)))
        s.click('#naSegu')
        over(volani(s)[-1] == 'odemknout:sega', 'O7 ODEMKNOUT SEGU -> AHHRY.odemknout(sega)', volani(s)[-1:])
        ctx.close()

        # ---------------------------------------------------------------- O8 PS1 se Segou
        m8 = dict(MISTNI, sega=True, segaMs=(3 * 60 + 12) * 60000)
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=ps1', 'cs', ahhry(m8, platby=[{'ok': True, 'kod': 200, 'data': platba('ps1')}]), chyby_js)
        s.wait_for_timeout(500)
        q = qr_strany(s)
        over(text(s, '#stavText') == 'PS1 je zamčená.' and text(s, '#ukol') == 'Hraj 120 hodin na Seze.'
             and text(s, '#pruhText') == 'Odehráno 3 h 12 min ze 120 h.' and text(s, '#cenaHlavni') == '125 Kč' and q == SPD_PS1
             and text(s, '#konzole') == 'PLAYSTATION · PS1', 'O8 PS1 se Segou: 3 h 12 min ze 120 h, 125 Kč, QR PS1', (text(s, '#pruhText'), text(s, '#cenaHlavni'), q))
        s.click('.zalozka[data-druh=eu]')
        over(qr_strany(s) == EPC_PS1, 'O8 PS1 SEPA: 4,99 € (EPC)')
        ctx.close()

        # ---------------------------------------------------------------- O9-O11 chyby
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', ahhry(MISTNI, platby=[
            {'ok': False, 'kod': 0, 'duvod': 'sit', 'zprava': 'server nedostupný', 'data': None},
            {'ok': True, 'kod': 200, 'data': platba()}]), chyby_js)
        s.wait_for_timeout(300)
        over(text(s, '#platbaStav') == 'Server N&P je nedostupný. Zkontroluj připojení a zkus to znovu.' and viditelne(s, '#znovu')
             and not viditelne(s, '#platba'), 'O9 server nedostupný: hláška + ZKUSIT ZNOVU', text(s, '#platbaStav'))
        s.click('#znovu')
        s.wait_for_timeout(300)
        over(viditelne(s, '#platba') and qr_strany(s) == SPD_SEGA and not viditelne(s, '#znovu'), 'O9 ZKUSIT ZNOVU -> platba se načte')
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', ahhry(MISTNI, platby=[{'ok': True, 'kod': 200, 'data': platba(nastaveno=False)}]), chyby_js)
        s.wait_for_timeout(300)
        over(text(s, '#platbaStav') == 'Platby zatím nejsou nastavené. Napiš správci N&P.' and not viditelne(s, '#platba'),
             'O10 platby nenastavené na serveru', text(s, '#platbaStav'))
        ctx.close()

        m11 = dict(MISTNI, sega=True)
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=ps1', 'cs', ahhry(m11, platby=[{'ok': True, 'kod': 200, 'data': platba('ps1')}],
                                                                              zaplatil={'ok': False, 'kod': 409, 'duvod': 'nejdriv_sega', 'zprava': 'Nejdřív odemkni Segu.'}), chyby_js)
        s.wait_for_timeout(300)
        s.click('#zaplatil')
        s.wait_for_timeout(200)
        over(text(s, '#stavText') == 'PS1 jde až po Seze. Nejdřív odemkni Segu.' and not s.is_disabled('#zaplatil'),
             'O11 server: PS1 nejdřív_sega -> hláška, tlačítko znovu aktivní', text(s, '#stavText'))
        ctx.close()

        # ---------------------------------------------------------------- O12 most odpovi pozde
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', ahhry(MISTNI, platby=[{'ok': True, 'kod': 200, 'data': platba()}], prazdnych=6), chyby_js)
        s.wait_for_timeout(1200)
        v = volani(s)
        over(v.count('odemknuti') >= 7 and 'platba:sega' in v and text(s, '#pruhText') == 'Napsáno 123 z 1500 řádků.' and qr_strany(s) == SPD_SEGA,
             'O12 most odpoví až po 0,6 s: stránka počká a funguje', (v[:9], text(s, '#pruhText')))
        ctx.close()

        # ---------------------------------------------------------------- O13 anglicky
        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=sega', 'en', ahhry(dict(MISTNI, jazyk='en'), platby=[{'ok': True, 'kod': 200, 'data': platba()}]), chyby_js)
        s.wait_for_timeout(600)
        st = [text(s, x) for x in ('#stavText', '#ukol', '#pruhText', '#cenaVedlejsi', '#zaplatil', '#ulozit', '.stitek')]
        over(st == ['The SEGA is locked.', 'Type 1500 program lines on the Atari.', 'Typed 123 of 1500 lines.', 'from abroad 2,99 € (SEPA)',
                    '✅ I HAVE PAID', '📷 SAVE QR TO GALLERY', 'FOR FREE'], 'O13 anglicky: stav, úkol, postup, cena, tlačítka', st)
        zb = cestina(s, povolene=('kč', 'platba', 'novacek', 'rene'))
        over(not zb, 'O13 anglicky: žádná čeština (kromě Kč, QR Platba, jména)', zb)
        s.click('#zaplatil')
        s.wait_for_timeout(200)
        over(text(s, '#ceka b') == '⏳ WAITING FOR CONFIRMATION' and not cestina(s, povolene=('kč', 'platba', 'novacek', 'rene')),
             'O13 anglicky: čeká na potvrzení', text(s, '#ceka b'))
        ctx.close()

        ctx, s = otevri(prohl, base, 'hry/odemknout.html?co=ps1', 'en', ahhry(dict(m8, jazyk='en'), platby=[{'ok': True, 'kod': 200, 'data': platba('ps1')}]), chyby_js)
        s.wait_for_timeout(500)
        over(text(s, '#ukol') == 'Play the Sega for 120 hours.' and text(s, '#pruhText') == 'Played 3 h 12 min of 120 h.'
             and not cestina(s, povolene=('kč', 'platba', 'novacek', 'rene')), 'O13 anglicky PS1: hodiny', (text(s, '#ukol'), text(s, '#pruhText')))
        ctx.close()

        # ---------------------------------------------------------------- R1 aktivace: most pozde
        STAV = {'sparovano': False, 'zablokovano': False, 'server': 'http://46.33.114.139:8080', 'vychozi': 'http://46.33.114.139:8080',
                'kod': '', 'klic': True, 'verze': 'EMU10-B322-PLATBY', 'jazyk': 'cs', 'telefon': '0718'}
        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(MISTNI, stav=STAV, stav_prazdnych=4,
                                                                      aktivace=[{'ok': True, 'kod': 200, 'duvod': '', 'zprava': ''}]), chyby_js)
        s.wait_for_timeout(1600)
        v = volani(s)
        over('aktivuj:' in v and v[-1] == 'pokracuj' and text(s, '#zprava') == 'Aplikace je aktivovaná' and v[:4] == ['stav'] * 4,
             'R1 AKTIVACE: most odpoví "{}" (start appky) -> stránka počká a spáruje se SAMA klíčem z APK', (v, text(s, '#zprava')))
        ctx.close()
        ctx, s = otevri(prohl, base, 'hry/aktivace.html', 'cs', ahhry(MISTNI, stav=dict(STAV, klic=False), stav_prazdnych=3), chyby_js)
        s.wait_for_timeout(700)
        over(viditelne(s, '#formular') and not [x for x in volani(s) if x.startswith('aktivuj')] and text(s, '#zprava') == 'Zadej svůj osobní kód',
             'R1 bez klíče v APK: po čekání formulář kódu (jako dřív)', (volani(s), text(s, '#zprava')))
        ctx.close()

        # ---------------------------------------------------------------- R2 N&P HRY: most pozde
        HRY = {'ok': True, 'offline': False, 'kod': 200, 'duvod': '', 'zprava': '', 'zamceno': False, 'data': {'ok': True, 'hry': [
            {'id': 'atari-0123456789abcdef', 'system': 'atari', 'nazev': 'Air Strike', 'soubor': 'Air Strike.xex', 'velikost': 13719, 'sha256': '', 'zmeneno': 1}]}}
        ctx, s = otevri(prohl, base, 'hry/index.html?system=atari', 'cs', ahhry(MISTNI, stav=dict(STAV, sparovano=True, kod='NAP-VB**-****'),
                                                                                stav_prazdnych=3, seznam=HRY), chyby_js)
        s.wait_for_timeout(900)
        over('aktivace.html' not in s.url and 'seznam' in volani(s), 'R2 N&P HRY: most odpoví "{}" -> NEskočí na aktivaci, načte seznam', (s.url, volani(s)))
        ctx.close()

        # ---------------------------------------------------------------- M1 HTML zaloha menu
        # funkce jsou v uzaverce stranky - vezmou se presne z index.html a spusti samostatne
        import re as _re
        zdroj = open(os.path.join(H.ASSETS, 'index.html'), encoding='utf-8').read()
        fce = []
        for jm in ('radku', 'casText', 'zamekKarty'):
            mt = _re.search(r'^function ' + jm + r'\(.*?^}$' if jm == 'zamekKarty' else r'^function ' + jm + r'\(.*?$', zdroj, _re.S | _re.M)
            fce.append(mt.group(0) if mt else '')
        ctx = prohl.new_context(viewport=NA_VYSKU)
        s = ctx.new_page()
        s.set_content('<p>menu</p>')
        t = s.evaluate('(kod) => { var EN = false; function J(cs, en) { return EN ? en : cs; } eval(kod); return ['
                       'zamekKarty({zamek:"sega"}, {sega:false, zbyvaRadku:1437}),'
                       'zamekKarty({zamek:"ps1"}, {sega:true, ps1:false, zbyvaMinut:7127}),'
                       'zamekKarty({zamek:"ps1"}, {sega:false, ps1:false, zbyvaMinut:7200}),'
                       'zamekKarty({zamek:"sega"}, {sega:true, zbyvaRadku:0})]; }', '\n'.join(fce))
        over(all(fce) and t == ['Napiš v Atari ještě 1437 řádků programu – nebo odemkni hned.', 'Hraj na Seze ještě 118 h 47 min – nebo odemkni hned.',
                                'Nejdřív odemkni Segu, pak 120 hodin hraní na Seze.', ''], 'M1 HTML záloha menu (index.html): texty zámků jako Java (B2-B4)', t)
        tl = 'location.href=\'hry/odemknout.html?co=\'+k.zamek' in zdroj and "J('ODEMKNOUT','UNLOCK')" in zdroj
        over(tl, 'M1 HTML záloha menu: tlačítko ODEMKNOUT -> hry/odemknout.html?co=sega|ps1')
        ctx.close()

        prohl.close()
    srv.shutdown()
    over(not chyby_js, 'žádná chyba JavaScriptu', chyby_js)
    vystup.append('')
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (sum(1 for r in vystup if r.startswith('OK')), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b322_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(hlavni())
