#!/usr/bin/env python3
"""B360: STRANKA SEGY - ODZNAK ZA ONLINE HRY (Chromium, stranka presne z appky - assets/emu_sega/index.html).
Rene 23. 11. vecer: "telefony se sekaji". Kdyz hra stoji (ceka na stisky druheho telefonu), hrac nevedel proc.
Most AHONLINE je falesny (harness ../b358/test_b358_stranka.py): stav = radky klic=hodnota jako z C++ (onlineStav).
Kontroly:
  1) hraje normalne: odznak "ONLINE · jmeno · odezva MS" (jako B358), zeleny
  2) hra stoji < 1,5 s (bezne kratke cekani): odznak se nemeni (zadne blikani)
  3) hra stoji >= 1,5 s: "ONLINE · ČEKÁM NA DRUHÝ TELEFON 2 S" cervene, odpocet roste; po navratu zase zeleny normalni
  4) synchronizace po rozjeti: "ONLINE · SYNCHRONIZUJI HRU…"
  5) duvody konce z C++ B360: ticho -> "druhý telefon přestal odpovídat (15 s nic nepřišlo) – spadla mu Wi-Fi, nebo se
     zasekl"; rozjeti po neuspesne synchronizaci -> "hra se rozjela a ani synchronizace nepomohla"; starsi text rozjeti
     dal "hra se rozjela – telefony už nehrají totéž"
  6) anglicky: odznak cekani / synchronizace a duvody bez cestiny
  vsude: zadna chyba JavaScriptu
Pouziti: python3 test_b360_stranka.py [slozka pro snimky]   -> VYSTUP_test_b360_stranka.txt
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b358'))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402
import test_b358_stranka as T                 # noqa: E402  (falesny most AHONLINE, pomocne funkce)

SNIMKY = sys.argv[1] if len(sys.argv) > 1 else ''
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


def snimek(pg, jmeno):
    if SNIMKY:
        os.makedirs(SNIMKY, exist_ok=True)
        pg.screenshot(path=os.path.join(SNIMKY, jmeno + '.png'))


HRAJE = 'faze=HRAJE\nkod=5649\ndruhy=SM-A125F\nodezva=9\nzpozdeni=3\naktivni=1\nhraje=1\nhrac1=1\nstart=1\nzpozdeni_ted=3\nsync=0\nstoji_ms=%d'
CEKA = 'ČEKÁM NA DRUHÝ TELEFON'


def zmeny_odznaku(pg, ms):
    pg.evaluate('() => { window.__zmen = 0; window.__mo && window.__mo.disconnect(); window.__mo = new MutationObserver(function (m) { window.__zmen += m.length; });'
                ' window.__mo.observe(document.getElementById("segaOnlineOdznak"), {childList: true, subtree: true, characterData: true, attributes: true}); }')
    pg.wait_for_timeout(ms)
    return pg.evaluate('() => window.__zmen')


def konec_text(pg, duvod):
    T.stav(pg, 'faze=KONEC\nchyba=\nkonec=%s\naktivni=0\nhraje=0\nstart=9' % duvod, 800)
    return T.odznak(pg), T.text(pg, '#segaOnlineStav')


def main():
    srv, url = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        ctx, pg = T.novy(b, url, T.NA_SIRKU, chyby)
        # hrac otevre 🌐 ONLINE a zalozi / pripoji se - od te chvile se stranka pta na stav (jako v appce)
        T.klik(pg, '#segaOnlineTlacitko', 300)
        pg.evaluate('() => window.napOnline.vysledek({akce: "zaloz", ok: true, zprava: "OK"})')
        T.stav(pg, 'faze=CEKA_NA_HRACE\nkod=5649\naktivni=1\nhraje=0\nstart=1', 700)
        # ---------------------------------------------------------------- 1) hraje normalne (panel se zavre sam)
        T.stav(pg, HRAJE % 0)
        o1 = T.odznak(pg)
        over(o1[0] == 'block' and o1[1] == 'ONLINE · SM-A125F · 9 MS' and 'chyba' not in o1[2],
             '1) hraje: odznak "%s" (zeleny, jako B358)' % o1[1])
        # ---------------------------------------------------------------- 2) kratke cekani - nic
        T.stav(pg, HRAJE % 300, 300)
        zm = zmeny_odznaku(pg, 1300)
        o2 = T.odznak(pg)
        over(o2[1] == o1[1] and 'chyba' not in o2[2] and zm == 0, '2) hra stoji 300 ms (kratke cekani): odznak beze zmeny (%d zmen)' % zm)
        # ---------------------------------------------------------------- 3) dlouhe cekani
        T.stav(pg, HRAJE % 2400)
        o3 = T.odznak(pg)
        snimek(pg, '1_ceka_na_druhy_telefon')
        T.stav(pg, HRAJE % 5100)
        o3b = T.odznak(pg)
        T.stav(pg, HRAJE % 0)
        o3c = T.odznak(pg)
        over(o3[1] == 'ONLINE · ' + CEKA + ' 2 S' and 'chyba' in o3[2] and o3b[1] == 'ONLINE · ' + CEKA + ' 5 S' and o3c[1] == o1[1] and 'chyba' not in o3c[2],
             '3) hra stoji 2,4 s: "%s" (cervene), 5,1 s: "%s", pak zase "%s" (zeleny)' % (o3[1], o3b[1], o3c[1]))
        # ---------------------------------------------------------------- 4) synchronizace
        T.stav(pg, (HRAJE % 800).replace('sync=0', 'sync=1'))
        o4 = T.odznak(pg)
        snimek(pg, '2_synchronizace')
        over(o4[1] == 'ONLINE · SYNCHRONIZUJI HRU…', '4) synchronizace po rozjeti: odznak "%s"' % o4[1])
        # ---------------------------------------------------------------- 5) duvody konce
        T.klik(pg, '#segaOnlineTlacitko', 300)
        _, k1 = konec_text(pg, 'od druheho hrace dlouho nic neprislo - spadla mu sit, nebo se telefon zasekl')
        _, k2 = konec_text(pg, 'hra se rozjela a synchronizace nepomohla (otisk stavu se lisi)')
        _, k3 = konec_text(pg, 'hra se rozjela - telefony uz nehraji totez (otisk stavu se lisi)')
        snimek(pg, '3_konec_ticho')
        over(k1.startswith('Online skončilo: druhý telefon přestal odpovídat (15 s nic nepřišlo) – spadla mu Wi-Fi, nebo se zasekl'),
             '5) ticho: "%s"' % k1[:110])
        over(k2.startswith('Online skončilo: hra se rozjela a ani synchronizace nepomohla') and
             k3.startswith('Online skončilo: hra se rozjela – telefony už nehrají totéž'),
             '   rozjeti: "%s" | starsi text: "%s"' % (k2[:80], k3[:80]))
        ctx.close()
        # ---------------------------------------------------------------- 6) anglicky
        ctx, pg = T.novy(b, url, T.NA_SIRKU, chyby, jazyk='en')
        T.klik(pg, '#segaOnlineTlacitko', 300)
        pg.evaluate('() => window.napOnline.vysledek({akce: "zaloz", ok: true, zprava: "OK"})')
        T.stav(pg, 'faze=CEKA_NA_HRACE\nkod=5649\naktivni=1\nhraje=0\nstart=1', 700)
        T.stav(pg, HRAJE % 3300)
        e1 = T.odznak(pg)
        T.stav(pg, (HRAJE % 0).replace('sync=0', 'sync=1'))
        e2 = T.odznak(pg)
        T.klik(pg, '#segaOnlineTlacitko', 300)
        _, e3 = konec_text(pg, 'od druheho hrace dlouho nic neprislo - spadla mu sit, nebo se telefon zasekl')
        _, e4 = konec_text(pg, 'hra se rozjela a synchronizace nepomohla (otisk stavu se lisi)')
        cz = [x for x in (e1[1], e2[1], e3, e4) if any(ch in x for ch in 'ěščřžýáíéůú')]
        over(e1[1] == 'ONLINE · WAITING FOR THE OTHER PHONE 3 S' and e2[1] == 'ONLINE · SYNCING THE GAME…'
             and 'the other phone stopped responding (nothing for 15 s)' in e3 and 'even re-syncing did not help' in e4 and not cz,
             '6) anglicky: "%s" | "%s" | "%s" | "%s"' % (e1[1], e2[1], e3[:80], e4[:70]))
        ctx.close()
        b.close()
    over(not chyby, 'zadna chyba JavaScriptu' + ('' if not chyby else ': ' + ' | '.join(chyby[:5])))
    srv.shutdown()
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b360_stranka.txt'), 'w', encoding='utf-8') as f:
        f.write('B360 - stranka Segy: odznak za online hry (cekani na druhy telefon, synchronizace), duvody konce\n')
        f.write('\n'.join(vystup) + '\n')
    return 0 if chyb == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
