#!/usr/bin/env python3
"""B347: stranka ODEMKNOUT HNED (hry/odemknout.html), kdyz server N&P v12.3 pozna platbu z e-mailu banky sam
("banka_auto" v /api/v1/platba). Chromium, stranka presne z appky (assets), falesny most AHHRY z testu B322
(odpovida asynchronne jako Java), hodiny Playwright (20 s se nemusi cekat doopravdy).

  A1 banka_auto: okenko 🏦 ODEMKNE SE SAMO s vetou pro rodice, rada pod ZAPLATIL JSEM "Neodemklo se ani po
     čtvrthodině?", ZAPLATIL JSEM zustava (zaloha)
  A2 banka_auto: stranka se sama kazdych 20 s pta serveru (platba) - i bez ZAPLATIL JSEM
  A3 server mezitim odemkl (koupeno) -> stranka ukaze odemceno a prestane se ptat
  A4 navrat do appky (visibilitychange, treba z bankovni appky) -> hned platba()
  A5 ZAPLATIL JSEM pri banka_auto: ČEKÁ NA POTVRZENÍ s textem "Server N&P platbu pozná z banky…", okenko
     ODEMKNE SE SAMO zmizi (dvakrat totez neukazuje)
  B1 bez banka_auto (stary server / banka vypnuta): vse jako B322 - zadne okenko, puvodni rada, zadne hlidani
  B2 banka se vypne za behu (banka_auto false) -> hlidani skonci, texty zpet
  C1 anglicky: UNLOCKS BY ITSELF + anglicke vety, zadna cestina (krome uctu, jmena, Kc, QR Platba)
  vsude: zadna chyba JavaScriptu
Pouziti (z test_overeni/b347): python3 test_b347_stranka.py -> VYSTUP_test_b347_stranka.txt
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b322'))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402
import test_b322_stranky as B                  # noqa: E402  (platba, MISTNI, ahhry, otevri, volani, text, viditelne, cestina)

vystup = []
chyb = 0


def over(ok, co, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co + ('' if ok or not detail else '   -> ' + str(detail)[:400]))
    print(vystup[-1], flush=True)


RADA_SAMA = 'Neodemklo se ani po čtvrthodině? Klepni na ZAPLATIL JSEM – správce N&P platbu zkontroluje ručně.'
RADA_RUCNE = 'Po zaplacení klepni na ZAPLATIL JSEM. Odemčení potvrzuje správce N&P ručně, když platbu uvidí v bance – může to chvíli trvat.'
CEKA_SAMA = 'Server N&P platbu pozná z banky a appka se odemkne sama – stačí ji mít zapnutou.'
CEKA_RUCNE = 'Správce N&P platbu zkontroluje v bance a appka se odemkne sama – stačí ji mít zapnutou.'


def ok_platba(**z):
    return {'ok': True, 'kod': 200, 'data': B.platba(**z)}


def hlavni():
    srv, base = H.server()
    chyby_js = []
    with sync_playwright() as p:
        prohl = p.chromium.launch()

        # ------------------------------------------------ A1-A4 server pozna platbu sam
        ctx, s = B.otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', B.ahhry(B.MISTNI, platby=[ok_platba(banka_auto=True)]), chyby_js, hodiny=True)
        s.clock.run_for(1500)
        over(B.viditelne(s, '#sama') and 'ODEMKNE SE SAMO' in (B.text(s, '#sama b') or '')
             and 'třeba rodič? Ať naskenuje tenhle QR kód svou bankovní appkou' in (B.text(s, '#samaText') or ''),
             'A1 okénko 🏦 ODEMKNE SE SAMO s větou pro rodiče', (B.text(s, '#sama'),))
        over(B.viditelne(s, '#zaplatil') and B.text(s, '#radaZaplatil') == RADA_SAMA and not B.viditelne(s, '#ceka'),
             'A1 ZAPLATIL JSEM zůstává jako záloha, rada „Neodemklo se ani po čtvrthodině?“', B.text(s, '#radaZaplatil'))
        n0 = B.volani(s).count('platba:sega')
        s.clock.run_for(41000)
        n1 = B.volani(s).count('platba:sega')
        over(n0 == 1 and n1 == 3, 'A2 bez ZAPLATIL JSEM se stránka sama ptá serveru každých 20 s', (n0, n1))
        s.evaluate('document.dispatchEvent(new Event("visibilitychange"))')
        s.clock.run_for(100)
        over(B.volani(s).count('platba:sega') == 4, 'A4 návrat do appky (visibilitychange) -> hned platba()', B.volani(s)[-3:])
        # server odemkl: dalsi odpoved koupeno, Java si to zapamatovala
        s.evaluate('window.__hry.platby = [%s]; window.__hry.m.sega = true; window.__hry.m.segaKoupena = true;'
                   % __import__('json').dumps(ok_platba(banka_auto=True, koupeno=True)))
        s.clock.run_for(20500)
        n2 = B.volani(s).count('platba:sega')
        s.clock.run_for(61000)
        n3 = B.volani(s).count('platba:sega')
        over('odemčená' in (B.text(s, '#stavText') or '') and B.viditelne(s, '#spustit') and n3 == n2,
             'A3 server odemkl -> „SEGA je odemčená“, SPUSTIT, stránka se přestane ptát', (B.text(s, '#stavText'), n2, n3))
        ctx.close()

        # ------------------------------------------------ A5 ZAPLATIL JSEM pri banka_auto
        ctx, s = B.otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', B.ahhry(B.MISTNI, platby=[ok_platba(banka_auto=True)]), chyby_js)
        s.wait_for_selector('#zaplatil:not(.skryte)')
        s.click('#zaplatil')
        s.wait_for_selector('#ceka:not(.skryte)')
        over(B.text(s, '#cekaText') == CEKA_SAMA and not B.viditelne(s, '#sama') and not B.viditelne(s, '#zaplatil'),
             'A5 ZAPLATIL JSEM: ČEKÁ s „Server N&P platbu pozná z banky…“, okénko SAMO zmizí', (B.text(s, '#cekaText'), B.viditelne(s, '#sama')))
        ctx.close()

        # ------------------------------------------------ B1 bez banka_auto = jako B322
        ctx, s = B.otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', B.ahhry(B.MISTNI, platby=[ok_platba()]), chyby_js, hodiny=True)
        s.clock.run_for(1500)
        n0 = B.volani(s).count('platba:sega')
        s.clock.run_for(61000)
        n1 = B.volani(s).count('platba:sega')
        over(not B.viditelne(s, '#sama') and B.text(s, '#radaZaplatil') == RADA_RUCNE and B.text(s, '#cekaText') == CEKA_RUCNE and n0 == n1 == 1,
             'B1 bez banka_auto: žádné okénko, původní rada a ČEKÁ text, stránka se sama neptá', (B.text(s, '#radaZaplatil'), n0, n1))
        ctx.close()

        # ------------------------------------------------ B2 banka se vypne za behu
        ctx, s = B.otevri(prohl, base, 'hry/odemknout.html?co=sega', 'cs', B.ahhry(B.MISTNI, platby=[ok_platba(banka_auto=True), ok_platba()]),
                          chyby_js, hodiny=True)
        s.clock.run_for(1500)
        s.clock.run_for(20500)    # druha odpoved: banka_auto false
        n1 = B.volani(s).count('platba:sega')
        s.clock.run_for(61000)
        n2 = B.volani(s).count('platba:sega')
        over(n1 == 2 and n2 == 2 and not B.viditelne(s, '#sama') and B.text(s, '#radaZaplatil') == RADA_RUCNE,
             'B2 banka vypnutá za běhu: hlídání skončí, texty zpět', (n1, n2, B.text(s, '#radaZaplatil')))
        ctx.close()

        # ------------------------------------------------ C1 anglicky
        m_en = dict(B.MISTNI, jazyk='en')
        ctx, s = B.otevri(prohl, base, 'hry/odemknout.html?co=sega', 'en', B.ahhry(m_en, platby=[ok_platba(banka_auto=True)]), chyby_js)
        s.wait_for_selector('#sama:not(.skryte)')
        s.wait_for_timeout(300)
        over('UNLOCKS BY ITSELF' in (B.text(s, '#sama b') or '') and 'Is someone else paying for you, e.g. a parent?' in (B.text(s, '#samaText') or '')
             and B.text(s, '#radaZaplatil') == 'Still locked after a quarter of an hour? Tap I HAVE PAID – the N&P admin will check the payment by hand.',
             'C1 anglicky: UNLOCKS BY ITSELF + anglické věty', (B.text(s, '#sama'), B.text(s, '#radaZaplatil')))
        zbytky = B.cestina(s, povolene=('Rene', 'Novacek', 'Kč', 'QR', 'Platba'))
        over(not zbytky, 'C1 anglicky: žádná čeština (kromě účtu, jména, Kč, QR Platba)', zbytky)
        s.click('#zaplatil')
        s.wait_for_selector('#ceka:not(.skryte)')
        over(B.text(s, '#cekaText') == 'The N&P server sees the payment from the bank and the app unlocks by itself – just keep it running.',
             'C1 anglicky: ČEKÁ text „The N&P server sees the payment…“', B.text(s, '#cekaText'))
        ctx.close()

        prohl.close()
    srv.shutdown()
    over(not chyby_js, 'žádná chyba JavaScriptu', chyby_js)
    vystup.append('')
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (sum(1 for r in vystup if r.startswith('OK')), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b347_stranka.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(hlavni())
