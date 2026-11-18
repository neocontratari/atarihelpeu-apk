#!/usr/bin/env python3
"""B346 v Chromiu - skutecne stranky appky + falesne mosty Javy (../b314/jazyk_harness):

  U1-U2) panel PS1 (kolecko na vysku) CZ / EN: bez vyvojarskych tlacitek PS1 PRIMA URL, PS1 CACHE CESTA, SMAZAT PS1
         CACHE; zbytek panelu zustal (HRY ZE SERVERU N&P, TV WEB CAST, D-PAD A OVLADANI, LOG); klikani po panelu
         nevola mosty ps1RemoteCachePath / ps1ClearRemoteCache / ps1DownloadAndBoot; anglicky bez cestiny
  T1-T2) OPTIONS CZ / EN: bez TV POKUS / TV TEST, AHINTRO.tvPokus nikdo nevola, ostatni tlacitka OPTIONS zustala
  O1-O2) stranka PS1, hra se nespustila kvuli poskozenemu obrazu (stav z Javy "PS1_REMOTE_FAIL boot <veta>",
         ps1StartStav "1|CHYBA <veta>" - veta z NapPravidla.ps1ProcNespustila): okno pryc, hlaska
         "HRA SE NESPUSTILA: obraz disku je poškozený ..." (12 s - dlouha veta), pruh pod monitorem ji neopakuje;
         anglicky "THE GAME DID NOT START: the disc image is damaged ..."
  O3)    chyba prisla mezi dvema dotazy (okno start nehlidalo - napr. HRY V TELEVIZI): rekne ji pruh pod monitorem
  N1)    napoveda CZ / EN: veta o poskozenem obrazu, TV POKUS / TV TEST pryc
  J)     zadna chyba JavaScriptu
Pouziti (z test_overeni/b346): python3 test_b346_stranky.py [PRAC] -> VYSTUP_test_b346_stranky.txt
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b346')
os.makedirs(PRAC, exist_ok=True)

vystup = []
chyb = 0
chybyJs = []

# vety presne z NapPravidla.ps1ProcNespustila (TestB346 P1)
VETA_CZ = ('obraz disku je poškozený (soubor hry nemá celé sektory CD) – smaž hru v CD ISO a stáhni ji znovu; '
           'když to nepomůže, je vadný soubor na serveru – podrobnosti v LOGu (deník PS1)')
VETA_EN = ('the disc image is damaged (the game file does not have whole CD sectors) – delete the game in CD ISO and '
           'download it again; if that does not help, the file on the server is bad – details in the LOG (PS1 diary)')


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def otevri(b, zaklad, stranka, jazyk='cs', mock=None, w=412, h=846):
    ctx = b.new_context(viewport={'width': w, 'height': h}, has_touch=True, is_mobile=True, device_scale_factor=2)
    ctx.add_init_script(H.init_skript(jazyk, mock or {}))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(stranka + ': ' + str(e).split('\n')[0][:200]))
    pg.goto(zaklad + stranka)
    pg.wait_for_timeout(1000)
    return ctx, pg


def volani(pg):
    return pg.evaluate('() => window.__napVolani.slice()')


srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()

    # ---------------- U) panel PS1 ----------------
    for jazyk, ocek in (('cs', ['HRY ZE SERVERU N&P', 'TV WEB CAST', 'D-PAD A OVLÁDÁNÍ', 'ULOŽIT LOG A ODESLAT', 'KOPIROVAT LOG']),
                        ('en', ['GAMES FROM THE N&P SERVER', 'TV WEB CAST', 'D-PAD AND CONTROLS', 'SAVE AND SEND THE LOG', 'COPY LOG'])):
        ctx, pg = otevri(b, zaklad, 'emu_ps1/index.html', jazyk)
        pg.click('#btnSettings')
        pg.wait_for_timeout(600)
        tl = pg.evaluate("() => Array.prototype.map.call(document.querySelectorAll('#helpPanel button'), function (x) { return x.textContent.trim(); })")
        nema = pg.evaluate("() => ['psRemoteUrl', 'psCachePath', 'psCacheClear'].filter(function (i) { return document.getElementById(i); })")
        text = pg.evaluate("() => document.getElementById('helpPanel').innerText")
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'ps1_panel_cz.png'))
        pg.click('#tvWebCast')
        pg.wait_for_timeout(300)
        pg.click('#logCopy')
        pg.wait_for_timeout(300)
        v = volani(pg)
        spatne = [x for x in v if x in ('AHPS1.ps1RemoteCachePath', 'AHPS1.ps1ClearRemoteCache', 'AHPS1.ps1DownloadAndBoot')]
        dobre = all(o in tl for o in ocek) and not nema and not spatne
        for x in ('PRIMA URL', 'CACHE CESTA', 'SMAZAT PS1 CACHE', 'DIRECT URL', 'CACHE PATH', 'DELETE PS1 CACHE'):
            dobre = dobre and x not in text
        if jazyk == 'cs':
            over(dobre, 'U1) CZ panel PS1: %s (vyvojarska tlacitka pryc, mosty cache / prima URL nikdo nevola)' % tl)
        else:
            zb = H.ceska_slova(text, povolena=('jadro',))
            over(dobre and not zb, 'U2) EN panel PS1: %s%s' % (tl, (' - CESKY: %s' % zb) if zb else ''))
        ctx.close()

    # ---------------- T) OPTIONS ----------------
    for jazyk, nesmi, musi in (('cs', ('TV POKUS', 'OBOJÍ', 'JEN ZVUK', 'JEN OBRAZ'), ('AKTUALIZOVAT APLIKACI', 'STÁHNOUT HRY A BIOS', 'PUSTIT INTRO')),
                               ('en', ('TV TEST', 'SOUND ONLY', 'PICTURE ONLY'), ('UPDATE THE APP', 'DOWNLOAD GAMES AND BIOS', 'PLAY INTRO'))):
        ctx, pg = otevri(b, zaklad, 'index.html', jazyk)
        pg.evaluate('() => window.napOtevriNastaveni()')
        pg.wait_for_timeout(700)
        t = pg.evaluate("() => document.body.innerText")
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'options_cz.png'), full_page=True)
        ok = (not any(x in t for x in nesmi)) and all(x in t for x in musi) \
            and pg.evaluate("() => !document.getElementById('stTvPokus')") and 'AHINTRO.tvPokus' not in volani(pg)
        over(ok, 'T%d) %s OPTIONS: bez %s, zustalo %s' % (1 if jazyk == 'cs' else 2, jazyk.upper(), nesmi[0], ', '.join(musi)))
        ctx.close()

    # ---------------- O) poskozeny obraz na strance PS1 ----------------
    for jazyk, veta, titul, toast in (('cs', VETA_CZ, 'HRA SE NESPUSTILA', 'HRA SE NESPUSTILA: '),
                                      ('en', VETA_EN, 'THE GAME DID NOT START', 'THE GAME DID NOT START: ')):
        mock = {'AHPS1.ps1StartStav': '0|', 'AHPS1.ps1RemoteAktivni': False, 'AHPS1.ps1RemoteJmeno': 'Command & Conquer - Red Alert Retaliation',
                'AHPS1.ps1RemoteStatus': 'PS1_REMOTE_BOOTING #1 Command & Conquer - Red Alert Retaliation'}
        ctx, pg = otevri(b, zaklad, 'emu_ps1/index.html', jazyk, mock)
        okno = pg.evaluate("() => { var o = document.getElementById('psMonitorLoading'); return o && o.style.display !== 'none' ? o.innerText : ''; }")
        pg.evaluate("(v) => { NAP_MOCK['AHPS1.ps1StartStav'] = '1|CHYBA ' + v; NAP_MOCK['AHPS1.ps1RemoteStatus'] = 'PS1_REMOTE_FAIL boot ' + v; }", veta)
        pg.wait_for_timeout(900)
        ts = pg.evaluate("() => document.getElementById('toast').textContent")
        pr = pg.evaluate("() => { var b = document.getElementById('remoteBanner'); return b.classList.contains('show') ? b.innerText : ''; }")
        konec = pg.evaluate("() => { var o = document.getElementById('psMonitorLoading'); return !o || o.style.display === 'none'; }")
        if jazyk == 'cs':
            pg.screenshot(path=os.path.join(PRAC, 'obraz_poskozeny_cz.png'))
        over('Command & Conquer - Red Alert Retaliation' in okno and konec and ts == toast + veta and pr == '',
             'O%d) %s: okno "%s" -> hlaska "%s…", pruh "%s…"' % (1 if jazyk == 'cs' else 2, jazyk.upper(),
                                                              okno.split('\n')[0], ts[:60], pr.replace('\n', ' | ')[:60]))
        ctx.close()
    ctx, pg = otevri(b, zaklad, 'emu_ps1/index.html', 'cs', {'AHPS1.ps1StartStav': '0|'})
    pg.evaluate("(v) => { NAP_MOCK['AHPS1.ps1StartStav'] = '1|CHYBA ' + v; NAP_MOCK['AHPS1.ps1RemoteStatus'] = 'PS1_REMOTE_FAIL boot ' + v; }", VETA_CZ)
    pg.wait_for_timeout(900)
    pr = pg.evaluate("() => { var b = document.getElementById('remoteBanner'); return b.classList.contains('show') ? b.innerText : ''; }")
    over(pr == 'HRA SE NESPUSTILA\n' + VETA_CZ, 'O3) chyba mezi dotazy (okno start nehlidalo): pruh "%s…"' % pr.replace('\n', ' | ')[:70])
    ctx.close()

    # ---------------- N) napoveda ----------------
    for jazyk, st, veta, pryc in (('cs', 'napoveda/index.html', 'a appka napíše obraz disku je poškozený, soubor hry nemá celé sektory CD', 'TV POKUS'),
                                  ('en', 'napoveda/en.html', 'and the app says the disc image is damaged, the game file does not have whole CD sectors', 'TV TEST')):
        ctx, pg = otevri(b, zaklad, st, jazyk)
        t = pg.evaluate('() => document.body.innerText')
        over(veta in t and pryc not in t, 'N1) %s napoveda: veta o poskozenem obrazu, %s pryc' % (jazyk.upper(), pryc))
        ctx.close()
    b.close()
srv.shutdown()

over(not chybyJs, 'J) zadna chyba JavaScriptu' + (': %s' % chybyJs[:3] if chybyJs else ''))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b346_stranky.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
