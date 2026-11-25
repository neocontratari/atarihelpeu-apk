#!/usr/bin/env python3
"""B364: NAPOVEDA po zmenach B364 v Chromiu (skutecne stranky, falesne mosty Javy jako ../b314/jazyk_harness).
Rene 25. 11.: "je docela slozite pochopit v atari jak nastavit pro dva", "kdyz nekdo zaklada hru jako novacek - at
nevybira ze sveho mobilu ale at najede na nas engine nase hry", "apka pri kontrole aktualizace v seting chce stahovat
i kdyz ma nejnovejsi aktualizaci", "mame chybu v cload".

  N1) CS i EN: blok #onlineatari ma novy postup hrace 1 presne s texty oken appky (HRA Z NASEHO SERVERU N&P, HRY NA
      SERVERU N&P, HRA, KTERA TED BEZI V ATARI, JAK BUDETE HRAT?, ZALOZIT, ONLINE: STAHUJI HRU..., HRA ZALOZENA - CISLO)
      - kazdy text je opravdu v MainActivity.java (tx("cesky", "anglicky")); kroky v poradi: zalozit -> jak hrat ->
      pripojit se; stary krok "spusti hru (klidne jen titulku)" pryc
  N2) OPTIONS -> AKTUALIZOVAT APLIKACI: MAS NEJNOVEJSI VERZI / PRESTO STAHNOUT ZNOVU (jako NapAktualizace.java),
      otazka "Jak aktualizuju appku?" rika, ze se nic nestahuje
  N3) kazetak: TIP o opravenych starych nahravkach (POKE 53775,0, EJECT); tlacitko ONLINE pristroje zminuje hry
      ze serveru; TIP setrne kresleni "jen kdyz telefon opravdu nestiha"
  N4) hledani "STAHUJI HRU" / "DOWNLOADING THE GAME" najde blok online Atari, "53775" najde TIP u kazetaku
  N5) na telefonu (412 px) se nic nevejde vedle - stranka nejde posouvat do strany
  J)  zadna chyba JavaScriptu
Pouziti (z test_overeni/b364): python3 test_b364_napoveda.py [PRAC]  -> VYSTUP_test_b364_napoveda.txt (+ obrazky v PRAC)
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b364')
os.makedirs(PRAC, exist_ok=True)
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
MAIN = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
AKT = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java'), encoding='utf-8').read()

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


# (cesky, anglicky, jak je v kodu appky) - text okna musi v kodu byt presne (i s mezerou / dvojteckou za nim)
OKNA = [
    ('HRA Z NAŠEHO SERVERU N&P', 'A GAME FROM OUR N&P SERVER', 'tx("HRA Z NAŠEHO SERVERU N&P", "A GAME FROM OUR N&P SERVER")'),
    ('HRY NA SERVERU N&P', 'GAMES ON THE N&P SERVER', 'tx("HRY NA SERVERU N&P (", "GAMES ON THE N&P SERVER (")'),
    ('HRA, KTERÁ TEĎ BĚŽÍ V ATARI', 'THE GAME RUNNING IN THE ATARI NOW', 'tx("HRA, KTERÁ TEĎ BĚŽÍ V ATARI: ", "THE GAME RUNNING IN THE ATARI NOW: ")'),
    ('JAK BUDETE HRÁT?', 'HOW WILL YOU PLAY?', 'tx("JAK BUDETE HRÁT? ", "HOW WILL YOU PLAY? ")'),
    ('ZALOŽIT', 'CREATE', 'tx("ZALOŽIT", "CREATE")'),
    ('ONLINE: STAHUJI HRU, PAK JI ZALOŽÍM...', 'ONLINE: DOWNLOADING THE GAME, THEN CREATING IT...',
     'tx("ONLINE: STAHUJI HRU, PAK JI ZALOŽÍM...", "ONLINE: DOWNLOADING THE GAME, THEN CREATING IT...")'),
    ('HRA ZALOŽENA – ČÍSLO', 'GAME CREATED – NUMBER', 'tx("HRA ZALOŽENA – ČÍSLO ", "GAME CREATED – NUMBER ")'),
    ('KAŽDÝ SVŮJ JOYSTICK', 'EACH THEIR OWN JOYSTICK', 'tx("KAŽDÝ SVŮJ JOYSTICK – hrajete oba naráz'),
    ('OBA NA JOYSTICKU 1', 'BOTH ON JOYSTICK 1', 'tx("OBA NA JOYSTICKU 1 – střídáte se (hra má jen joystick 1)"'),
]
AKTUALIZACE = [('MÁŠ NEJNOVĚJŠÍ VERZI', 'YOU HAVE THE LATEST VERSION', 'NapJazyk.t(a, "MÁŠ NEJNOVĚJŠÍ VERZI", "YOU HAVE THE LATEST VERSION")'),
               ('PŘESTO STÁHNOUT ZNOVU', 'DOWNLOAD AGAIN ANYWAY', 'NapJazyk.t(a, "PŘESTO STÁHNOUT ZNOVU", "DOWNLOAD AGAIN ANYWAY")')]
HLEDAT = {'cs': [('STAHUJI HRU', 'online'), ('53775', 'kazeta')], 'en': [('DOWNLOADING THE GAME', 'online'), ('53775', 'kazeta')]}
KROKY = {'cs': ('HRA Z NAŠEHO SERVERU N&P', 'JAK BUDETE HRÁT?', 'PŘIPOJIT SE'), 'en': ('A GAME FROM OUR N&P SERVER', 'HOW WILL YOU PLAY?', 'JOIN')}
STARY = {'cs': 'spustí hru (klidně jen titulku)', 'en': 'starts a game (the title screen is enough)'}
VETY = {
    'cs': {'faq': 'Když už máš nejnovější verzi, appka to řekne a nic nestahuje.', 'tip': 'appka při vložení sama opraví',
           'online': 'ZALOŽIT HRU s hrou z našeho serveru', 'setrne': 'Zapne se jen tehdy, když telefon plný počet snímků opravdu nestíhá.'},
    'en': {'faq': 'When you already have the latest version, the app says so and downloads nothing.', 'tip': 'are repaired by the app when you insert them',
           'online': 'CREATE A GAME with a game from our server', 'setrne': 'It turns on only when the phone really cannot keep up with the full frame rate.'},
}

for cs, en, kod in OKNA + AKTUALIZACE:
    zdroj = AKT if kod.startswith('NapJazyk') else MAIN
    over(kod in zdroj, 'K) "%s" / "%s" je v appce: %s' % (cs, en, kod[:90]))

chybyJs = []
srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()
    for jazyk, st in (('cs', 'napoveda/index.html'), ('en', 'napoveda/en.html')):
        J = jazyk.upper()
        i = 0 if jazyk == 'cs' else 1
        ctx = b.new_context(viewport={'width': 412, 'height': 915}, has_touch=True, is_mobile=True, device_scale_factor=2)
        ctx.add_init_script(H.init_skript(jazyk, {}))
        pg = ctx.new_page()
        pg.on('pageerror', lambda e, st=st: chybyJs.append(st + ': ' + str(e).split('\n')[0][:200]))
        pg.goto(zaklad + st)
        pg.wait_for_timeout(1200)
        # N1 blok online Atari
        text = pg.evaluate("(document.getElementById('onlineatari') || {innerText: ''}).innerText.replace(/\\s+/g, ' ')")
        chybi = [o[i] for o in OKNA if o[i] not in text]
        over(len(text) > 500 and not chybi, 'N1) %s blok #onlineatari (%d znaku) ma novy postup s texty oken appky%s'
             % (J, len(text), '' if not chybi else ' - CHYBI: ' + ', '.join(chybi)))
        kroky = pg.evaluate("""() => Array.from(document.querySelectorAll('#onlineatari ol.kroky > li')).map(l => l.innerText.replace(/\\s+/g, ' '))""")
        poz = [next((k for k, t in enumerate(kroky) if s in t), -1) for s in KROKY[jazyk]]
        over(len(kroky) == 5 and poz == [0, 1, 2] and not any(STARY[jazyk] in t for t in kroky),
             'N1) %s %d kroku: zalozit (krok %d) -> jak budete hrat (krok %d) -> pripojit se (krok %d), stary krok "%s" pryc'
             % (J, len(kroky), poz[0] + 1, poz[1] + 1, poz[2] + 1, STARY[jazyk]))
        el = pg.query_selector('#onlineatari')
        if el:
            el.screenshot(path=os.path.join(PRAC, 'onlineatari_%s.png' % jazyk))
        # N2 aktualizace
        dd = pg.evaluate("""() => { const dt = Array.from(document.querySelectorAll('dt')).find(d => /^(AKTUALIZOVAT APLIKACI|UPDATE THE APP)$/.test(d.innerText.trim()));
            return dt && dt.nextElementSibling ? dt.nextElementSibling.innerText.replace(/\\s+/g, ' ') : ''; }""")
        # zavrena otazka (details) nema obsah v innerText - textContent
        faq = pg.evaluate("""() => Array.from(document.querySelectorAll('details')).map(d => d.textContent.replace(/\\s+/g, ' '))
            .find(t => /Jak aktualizuju appku\\?|How do I update the app\\?/.test(t)) || ''""")
        over(all(a[i] in dd for a in AKTUALIZACE) and VETY[jazyk]['faq'] in faq,
             'N2) %s AKTUALIZOVAT APLIKACI: %s / %s; otazka o aktualizaci: "%s"'
             % (J, AKTUALIZACE[0][i], AKTUALIZACE[1][i], VETY[jazyk]['faq']))
        # N3 kazeta, tlacitko ONLINE, setrne kresleni
        tip = pg.evaluate("""() => { const t = Array.from(document.querySelectorAll('p.tip')).find(p => p.innerText.includes('53775'));
            return t ? (t.closest('.blok').querySelector('h3') || {textContent: '?'}).textContent.trim() + ' | ' + t.innerText.replace(/\\s+/g, ' ') : ''; }""")
        over(tip.startswith(('Kazeťák |', 'Tape deck |')) and VETY[jazyk]['tip'] in tip and 'EJECT' in tip and 'B36' not in tip,
             'N3) %s TIP u kazetaku: %s' % (J, tip[:150]))
        online = pg.evaluate("""() => { const dt = Array.from(document.querySelectorAll('dt')).find(d => d.innerText.trim() === 'ONLINE');
            return dt && dt.nextElementSibling ? dt.nextElementSibling.innerText.replace(/\\s+/g, ' ') : ''; }""")
        over(VETY[jazyk]['online'] in online and VETY[jazyk]['setrne'] in text,
             'N3) %s tlacitko ONLINE: "%s..."; setrne kresleni: "%s"' % (J, VETY[jazyk]['online'], VETY[jazyk]['setrne']))
        # N4 hledani
        for slovo, co in HLEDAT[jazyk]:
            pg.fill('#hledat', slovo)
            pg.wait_for_timeout(600)
            if co == 'online':
                vid = pg.evaluate("document.getElementById('onlineatari').offsetParent !== null")
            else:
                vid = pg.evaluate("""() => { const t = Array.from(document.querySelectorAll('p.tip')).find(p => p.innerText.includes('53775'));
                    return !!t && t.offsetParent !== null; }""")
            over(vid, 'N4) %s hledani "%s" najde %s' % (J, slovo, 'blok online Atari' if co == 'online' else 'TIP u kazetaku'))
        pg.fill('#hledat', '')
        pg.wait_for_timeout(400)
        # N5 nic nepreteka do strany
        sirka = pg.evaluate("[document.documentElement.scrollWidth, window.innerWidth]")
        over(sirka[0] <= sirka[1], 'N5) %s stranka na telefonu nejde posouvat do strany (sirka obsahu %d, okno %d)' % (J, sirka[0], sirka[1]))
        ctx.close()
    b.close()
srv.shutdown()
over(not chybyJs, 'J) zadna chyba JavaScriptu' + ('' if not chybyJs else ': ' + ' | '.join(chybyJs[:3])))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b364_napoveda.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
