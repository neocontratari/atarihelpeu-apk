#!/usr/bin/env python3
# B314: NAVOD CESKY / ANGLICKY (assets/napoveda/index.html a en.html) v Chromiu - PRESNE soubory z appky.
#  1) presmerovani podle jazyka appky (falesny most AHJAZYK jako v Jave):
#     anglicky: index.html#tv -> en.html#tv (stranka dojede na kapitolu), cesky: en.html#sega -> index.html#sega,
#     bez zapisu do historie (ZPET na telefonu = predchozi stranka), bez mostu se nepresmerovava,
#     na PC ?lang=en / ?lang=cs v adrese
#  2) anglicky navod: pismo, 13 kapitol v obsahu, odkazy, obrazky, ZPET DO MENU (../index.html),
#     klepnuti v obsahu bez historie, plovouci nahoru
#  3) hledani anglicky: "tape" i "tapes" (mnozne cislo), "BIOS" otevre otazku, nesmysl = NOTHING FOUND,
#     smazani vrati vse, hlaska FOUND n× · m CHAPTER(S)
#  4) anglicky navod bez zbytku cestiny (texty, atributy, titulek) - krome nazvu slozek v telefonu
#     (jsou cesky i v anglicke appce) a prepinace JAZYK · LANGUAGE / ČEŠTINA
#  5) cesky navod: texty jako v B313 (git HEAD) krome zamernych zmen B314 (jazyk, paticka)
#  6) 320 px, 360 px a na sirku: nic nepreteka do strany
#  7) zadna chyba JavaScriptu, zadny chybejici soubor
#   python3 test_b314_jazyk_navod.py [PRAC]          -> VYSTUP_test_b314_jazyk_navod.txt (+ snimky do PRAC, jen pro me)
import os
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

import jazyk_harness as H

ZDE = H.ZDE
A = H.ASSETS
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b314'
os.makedirs(PRAC, exist_ok=True)
vystup = []
kontrol = chyb = 0


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    r = ('OK     ' if ok else 'CHYBA  ') + co
    print(r, flush=True)
    vystup.append(r)


CZ = 'file://' + os.path.join(A, 'napoveda/index.html')
EN = 'file://' + os.path.join(A, 'napoveda/en.html')
KARTY = 'file://' + os.path.join(A, 'menu/karty.txt')
# nazvy slozek v telefonu (appka je tak opravdu zaklada - cesky i v anglicke verzi) a jmena
SLOVA_EN = ('Atari_emu', 'Sega_hry', 'Sega_ulozene_pozice', 'PS1_pametova_karta', 'DJ_mixy', 'PS1_LOG_predchozi',
            'csave', 'emu', 'René', "René's", 'Frank', 'NP_VISION_mixtape', 'Mixtape', 'pametova', 'karta', 'hry',
            'ulozene', 'pozice', 'mixy', 'predchozi', 'JAZYK', 'ČEŠTINA', 'Pitt', 'Kitt', 'Wolfenstein', 'Popeye',
            'Montezuma', 'Galactic', 'Chase', 'Moon', 'Patrol', 'Pac', 'Donkey', 'Kong', 'River', 'Raid', 'Bruce', 'Lee',
            'scph', 'AtariHelp', 'Download')

chybyJs, neslo = [], []
with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files'])

    def kontext(jazyk=None, w=412, h=915):
        c = b.new_context(viewport={'width': w, 'height': h}, device_scale_factor=2, is_mobile=True, has_touch=True)
        if jazyk:
            c.add_init_script(H.init_skript(jazyk))
        return c

    def stranka(c):
        pg = c.new_page()
        pg.on('pageerror', lambda e: chybyJs.append(str(e).split('\n')[0][:200]))
        # net::ERR_ABORTED = nacitani zrusene odchodem ze stranky (ZPET, zavreni) - to neni chybejici soubor
        pg.on('requestfailed', lambda r: None if 'ERR_ABORTED' in str(r.failure) else
              neslo.append('%s [%s]' % (r.url.split('/assets/')[-1], r.failure)))
        return pg

    # ---- 1) presmerovani ----
    for jazyk, start, cil, kap in (('en', CZ + '#tv', EN, 'tv'), ('cs', EN + '#sega', CZ, 'sega'),
                                   ('en', EN, EN, None), ('cs', CZ, CZ, None)):
        c = kontext(jazyk)
        pg = stranka(c)
        pg.goto(KARTY)
        hist0 = pg.evaluate('history.length')
        pg.goto(start)
        pg.wait_for_timeout(1300)
        st = pg.evaluate("""(kap) => { const e = kap && document.getElementById(kap);
            return {url: location.href, lang: document.documentElement.lang, hist: history.length,
                    top: e ? e.getBoundingClientRect().top : null, lista: document.querySelector('.lista').offsetHeight,
                    titul: document.title}; }""", kap)
        konec = cil + ('#' + kap if kap else '')
        ok = st['url'] == konec and st['lang'] == ('en' if cil == EN else 'cs') and st['hist'] == hist0 + 1
        if kap:
            ok = ok and 0 <= st['top'] - st['lista'] <= 30
        over(ok, '1) appka %s: %s -> %s (lang=%s, titulek "%s", historie %d -> %d = navod jen jednou%s)' % (
            'anglicky' if jazyk == 'en' else 'cesky', os.path.basename(start), os.path.basename(st['url']), st['lang'], st['titul'],
            hist0, st['hist'], ', stranka dojede na kapitolu %s' % kap if kap else ''))
        pg.go_back()
        pg.wait_for_timeout(400)
        over(pg.url == KARTY, '1)   ZPET z navodu vede na predchozi stranku (%s), ne na presmerovany navod' % os.path.basename(pg.url))
        c.close()
    # bez mostu (PC, prohlizec): nic se nepresmeruje; ?lang= v adrese
    c = kontext(None)
    pg = stranka(c)
    for start, cil in ((CZ, CZ), (EN, EN), (CZ + '?lang=en#mp3', EN + '?lang=en#mp3'), (EN + '?lang=cs', CZ + '?lang=cs')):
        pg.goto(start)
        pg.wait_for_timeout(500)
        over(pg.url == cil, '1) bez mostu Javy: %s -> %s' % (start.split('/napoveda/')[1], pg.url.split('/napoveda/')[1]))
    c.close()

    # ---- 2) anglicky navod: stavba ----
    c = kontext('en')
    pg = stranka(c)
    pg.goto(EN)
    pg.wait_for_timeout(900)
    pismo = pg.evaluate("""async () => { await document.fonts.ready;
        return [document.fonts.check('700 20px NapChakra'), document.fonts.check('600 20px NapChakra')]; }""")
    over(all(pismo), '2) pismo Chakra Petch z appky nacteno')
    dl = pg.evaluate("""() => [...document.querySelectorAll('#obsah a')].map(a => [a.getAttribute('href'), a.innerText.replace(/\\s+/g,' ').trim()])""")
    existuji = pg.evaluate("ids => ids.map(h => !!document.getElementById(h.slice(1)))", [h for h, _ in dl])
    over(len(dl) == 13 and all(existuji), '2) obsah: 13 dlazdic, kazda vede na existujici kapitolu: ' + ' | '.join(t for _, t in dl))
    kap = pg.evaluate("""() => [...document.querySelectorAll('section.kap')].map(s => ({id: s.id,
        nadpis: s.querySelector('h2').innerText, menu: [...s.querySelectorAll('.kap-paticka a')].map(a => [a.getAttribute('href'), a.innerText])}))""")
    over(len(kap) == 13 and all(['../index.html', '◀ BACK TO THE MENU'] in k['menu'] and ['#obsah', '▲ CONTENTS'] in k['menu'] for k in kap),
         '2) 13 kapitol, kazda konci tlacitky ▲ CONTENTS a ◀ BACK TO THE MENU (../index.html)')
    vnitrni = pg.evaluate("""() => [...document.querySelectorAll('main a[href^="#"]')].map(a => a.getAttribute('href').slice(1))
        .filter(id => !document.getElementById(id))""")
    over(not vnitrni, '2) vsechny odkazy uvnitr navodu vedou na existujici misto' + (' (chybi: %s)' % vnitrni if vnitrni else ''))
    obr = pg.evaluate("""async () => { const im = [...document.images]; for (const i of im) { i.loading = 'eager'; }
        await new Promise(r => setTimeout(r, 400)); await Promise.all(im.map(i => i.decode().catch(() => null)));
        return im.map(i => [i.getAttribute('src'), i.naturalWidth]); }""")
    spatne = [s for s, w in obr if w <= 0]
    over(len(obr) == 12 and not spatne, '2) vsech %d obrazku se nacte' % len(obr) + (' - NEJDOU: %s' % spatne if spatne else ''))
    pg.screenshot(path=os.path.join(PRAC, 'navod_en_1_nahore.png'))
    hist0 = pg.evaluate('history.length')
    url0 = pg.url
    pg.click('#obsah a[href="#options"]')
    pg.wait_for_timeout(1200)
    st = pg.evaluate("""() => { const r = document.getElementById('options').getBoundingClientRect();
        return {y: scrollY, top: r.top, lista: document.querySelector('.lista').offsetHeight, hist: history.length, url: location.href}; }""")
    over(st['y'] > 1000 and 0 <= st['top'] - st['lista'] <= 30 and st['hist'] == hist0 and st['url'] == url0,
         '2) klepnuti v obsahu na OPTIONS: stranka dojede na kapitolu, historie %d -> %d, adresa beze zmeny' % (hist0, st['hist']))
    pg.screenshot(path=os.path.join(PRAC, 'navod_en_2_options.png'))
    over(pg.evaluate("document.getElementById('nahoru').classList.contains('ukaz')"), '2) plovouci tlacitko nahoru se ukaze')
    jaz = pg.evaluate("""() => { const b = [...document.querySelectorAll('#options .blok')].find(x => /language/i.test(x.querySelector('h3') ? x.querySelector('h3').textContent : ''));
        return b ? b.innerText.replace(/\\s+/g, ' ') : ''; }""")
    over('JAZYK · LANGUAGE' in jaz and 'ČEŠTINA' in jaz and 'ENGLISH' in jaz, '2) OPTIONS popisuje prepinac jazyka: "%s"' % jaz[:150])

    # ---- 3) hledani anglicky ----
    pg.evaluate('scrollTo(0,0)')

    def hledej(q):
        pg.fill('#hledat', q)
        pg.wait_for_timeout(500)
        return pg.evaluate("""() => ({info: document.getElementById('hledatInfo').innerText,
            marky: [...document.querySelectorAll('main mark')].map(m => m.textContent),
            videt: [...document.querySelectorAll('section.kap')].filter(s => !s.classList.contains('skryto')).map(s => s.id),
            otevrene: [...document.querySelectorAll('details[open]')].map(d => d.querySelector('summary').innerText),
            obsah: getComputedStyle(document.getElementById('obsah')).display})""")
    h1 = hledej('tape')
    over(h1['marky'] and all(m.lower() == 'tape' for m in h1['marky']) and h1['info'].startswith('FOUND ') and 'CHAPTER' in h1['info']
         and h1['obsah'] == 'none' and 'atari' in h1['videt'] and 'dzubox' not in h1['videt'],
         '3) hledani "tape": %s (videt: %s)' % (h1['info'], ', '.join(h1['videt'])))
    h2 = hledej('tapes')
    over(h2['info'] == h1['info'] and h2['videt'] == h1['videt'], '3) "tapes" (mnozne cislo) najde totez: %s' % h2['info'])
    h3 = hledej('BIOS')
    over(any('PS1 game' in o for o in h3['otevrene']) and 'ps1' in h3['videt'] and 'options' in h3['videt'],
         '3) "BIOS" otevre shodnou otazku (%s) a ukaze PS1 i OPTIONS' % '; '.join(h3['otevrene']))
    h4 = hledej('cestina')
    over('options' in h4['videt'] and any(m.lower() == 'čeština' for m in h4['marky']), '3) "cestina" (bez hacku) najde prepinac ČEŠTINA: %s' % h4['info'])
    h5 = hledej('xyzqw')
    over(h5['info'] == 'NOTHING FOUND – TRY ANOTHER WORD' and not h5['videt'], '3) nesmysl: "%s", zadna kapitola' % h5['info'])
    jedna = hledej('Wolfenstein')
    over(jedna['info'].endswith(' 1 CHAPTER'), '3) jedna kapitola = jednotne cislo: %s' % jedna['info'])
    pg.click('#hledatZrus')
    pg.wait_for_timeout(400)
    h6 = pg.evaluate("""() => ({skryto: document.querySelectorAll('.skryto').length, marky: document.querySelectorAll('mark').length,
        otevrene: document.querySelectorAll('details[open]').length, info: document.getElementById('hledatInfo').hidden,
        hodnota: document.getElementById('hledat').value})""")
    over(h6['skryto'] == 0 and h6['marky'] == 0 and h6['otevrene'] == 0 and h6['info'] and h6['hodnota'] == '',
         '3) smazani hledani vrati vse')

    # ---- 4) bez cestiny ----
    pg.evaluate("document.querySelectorAll('details').forEach(d => d.open = true)")
    sber = pg.evaluate(H.SBER_JS)
    zb = H.zbytky_cestiny(sber, povolene_vety=('JAZYK · LANGUAGE',), povolena_slova=SLOVA_EN)
    over(not zb and sber['lang'] == 'en', '4) anglicky navod bez zbytku cestiny (%d textu, %d atributu, titulek "%s")%s' % (
        len(sber['texty']), len(sber['atributy']), sber['titulek'],
        '' if not zb else ' - ZBYTKY: ' + ' | '.join('%s "%s" (%s)' % (d, t[:70], ','.join(s[:3])) for d, t, s in zb[:10])))
    with open(os.path.join(ZDE, 'VYSTUP_texty_navod_en.txt'), 'w', encoding='utf-8') as f:
        f.write('B314 - vsechny texty anglickeho navodu (napoveda/en.html), pro kontrolu ocima\n')
        for t in dict.fromkeys(sber['texty'] + sber['atributy'] + ['[titulek] ' + sber['titulek']]):
            f.write(t + '\n')
    pg.evaluate("document.getElementById('otazky').scrollIntoView()")
    pg.screenshot(path=os.path.join(PRAC, 'navod_en_3_otazky.png'))
    c.close()

    # ---- 5) cesky navod jako v B313 ----
    tmp = tempfile.mkdtemp(prefix='b314_navod_')
    subprocess.run('git -C "%s" archive HEAD app/src/main/assets/napoveda | tar -x -C "%s"' % (H.KOREN, tmp), shell=True, check=True)
    sbery = []
    for url in (CZ, 'file://' + os.path.join(tmp, 'app/src/main/assets/napoveda/index.html')):
        c = kontext('cs')
        pg = stranka(c) if url == CZ else c.new_page()
        pg.goto(url)
        pg.wait_for_timeout(700)
        pg.evaluate("document.querySelectorAll('details').forEach(d => d.open = true)")
        sbery.append(pg.evaluate(H.SBER_JS))
        c.close()
    nove, stare = sbery
    plus = [t for t in nove['texty'] if t not in stare['texty']]
    minus = [t for t in stare['texty'] if t not in nove['texty']]
    zamerne = ('JAZYK · LANGUAGE', 'ČEŠTINA', 'ENGLISH', 'Appka mluví jazykem telefonu', 'Celá appka se přepne hned',
               'Jak přepnu appku do angličtiny', 'Jazyk', 'B314', 'B311', 'zvolený jazyk', 'nastavení appky.',
               'Postup odemykání (zámky Segy a PS1) a nastavení appky', '. Přepneš ho kdykoli:', 'Přepneš ho kdykoli',
               '.', '–', '/', '→', ',')
    necekane = [t for t in plus + minus if not any(z in t for z in zamerne)]
    over(not necekane and nove['lang'] == 'cs' and nove['titulek'] == stare['titulek'],
         '5) cesky navod: texty jako v B313, zmeneno jen zamerne (+%d -%d: %s)%s' % (
             len(plus), len(minus), ' | '.join((plus + minus)[:8]), '' if not necekane else ' - NECEKANE: %s' % necekane[:5]))

    # ---- 6) uzky telefon a na sirku ----
    for (w, hh, jm) in ((320, 640, 'navod_en_6_320.png'), (360, 740, 'navod_en_6_360.png'), (915, 412, 'navod_en_7_sirka.png')):
        c = kontext('en', w, hh)
        pg = stranka(c)
        pg.goto(EN)
        pg.wait_for_timeout(700)
        sirka = pg.evaluate("""() => { const sw = document.documentElement.scrollWidth, cw = document.documentElement.clientWidth;
            document.querySelectorAll('details').forEach(d => d.open = true);
            const siroke = [...document.querySelectorAll('main *')].filter(e => e.getBoundingClientRect().right > cw + 1).map(e => e.tagName + '.' + e.className).slice(0, 5);
            return [sw, cw, siroke]; }""")
        over(sirka[0] <= sirka[1] and not sirka[2], '6) %dx%d: nic nepreteka do strany (scrollWidth %d, sirka %d)%s' % (
            w, hh, sirka[0], sirka[1], ' ' + str(sirka[2]) if sirka[2] else ''))
        pg.evaluate("document.getElementById('atari').scrollIntoView()")
        pg.wait_for_timeout(300)
        pg.screenshot(path=os.path.join(PRAC, jm))
        c.close()
    b.close()

over(not chybyJs, '7) zadna chyba JavaScriptu' + (': ' + '; '.join(sorted(set(chybyJs))[:5]) if chybyJs else ''))
over(not neslo, '7) zadny soubor, ktery stranka chce, nechybi' + (': ' + ', '.join(sorted(set(neslo))[:5]) if neslo else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.insert(0, 'B314 - navod cesky / anglicky v Chromiu (soubory z appky, most AHJAZYK falesny jako v Jave)')
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b314_jazyk_navod.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
