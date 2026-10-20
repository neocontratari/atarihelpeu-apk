#!/usr/bin/env python3
# B312: MP3 PREHRAVAC (assets/player/index.html) v Chromiu - RECORD (mixtape) a FAVORITES.
# Java je napodobena: AHPICK.listLocalAudio / playLocalAudio (cte "soubor v mobilu" a posle ho
# jako MainActivity.injectAudio: AHLOCAL_AUDIO_BEGIN / PART / END, chyba = AHJAVA_ERROR),
# AHSAVE.souborZacni / Pridej / Dokonci / Zrus (sklada bajty jako Java do souboru).
# Pisnicky = MP3 z ffmpeg (sinusovky), nahravky se kontroluji ffprobe / ffmpeg.
#   python3 test_b312_prehravac.py [PRAC]      -> VYSTUP_test_b312_prehravac.txt
import base64
import json
import math
import os
import re
import struct
import subprocess
import sys
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b312/prehravac'
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


# ---- pisnicky "v mobilu" ----
PISNE = [('Ranni_vlna.mp3', 330, 8), ('Neon_Drive.mp3', 440, 8), ('Kazeta_C90.mp3', 550, 8),
         ('Kratka_A.mp3', 392, 3), ('Kratka_B.mp3', 494, 3)]
soubory = {}
knihovna = []
for i, (jm, f, s) in enumerate(PISNE):
    cesta = os.path.join(PRAC, jm)
    if not os.path.exists(cesta):
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'lavfi', '-i', 'sine=frequency=%d:duration=%d:sample_rate=44100' % (f, s),
                        '-af', 'volume=0.5', '-ac', '2', '-c:a', 'libmp3lame', '-b:a', '128k', cesta], check=True)
    uri = 'content://media/external/audio/media/%d' % (101 + i)
    soubory[uri] = base64.b64encode(open(cesta, 'rb').read()).decode()
    knihovna.append({'name': jm, 'uri': uri, 'size': os.path.getsize(cesta), 'modified': 1760000000 - i})

MOST = """
window.__soubory = %s;
window.__knihovna = %s;
window.__mock = {pozadavky: [], zacni: [], kusu: 0, bajty: [], dokonci: 0, zrus: 0, padniPri: %d};
window.AHPICK = {
  listLocalAudio: function(){ return JSON.stringify({ok: true, count: __knihovna.length, items: __knihovna}); },
  requestAudioPermission: function(){ return 'AUDIO_PERMISSION_OK'; },
  pickMp3: function(){},
  openExternalUrl: function(){},
  playLocalAudio: function(name, uri){
    __mock.pozadavky.push([name, uri]);
    setTimeout(function(){
      var b64 = __soubory[uri];
      if (!b64) { AHJAVA_ERROR('LOCAL AUDIO CHYBA - FileNotFoundException: ' + uri); return; }
      AHLOCAL_AUDIO_BEGIN(name);
      for (var i = 0; i < b64.length; i += 262144) AHLOCAL_AUDIO_PART(b64.substring(i, i + 262144));
      AHLOCAL_AUDIO_END();
    }, 80);
    return 'LOCAL_AUDIO_LOADING';
  }
};
%s
// test: zjistit <audio>, ktery stranka pouziva (new Audio() neni v DOM)
window.__audia = [];
(function(){ var op = HTMLMediaElement.prototype.play;
  HTMLMediaElement.prototype.play = function(){ if (__audia.indexOf(this) < 0) __audia.push(this); return op.apply(this, arguments); }; })();
"""
SAVE = """
window.AHSAVE = {
  save: function(){ return 'DOWNLOADS_OK:x'; },
  souborZacni: function(slozka, jm, typ){ __mock.zacni.push([slozka, jm, typ]); __mock.jm = jm; __mock.bajty = []; __mock.kusu = 0; return 9; },
  souborPridej: function(id, b64){
    if (id !== 9) return false;
    __mock.kusu++;
    if (__mock.padniPri && __mock.kusu >= __mock.padniPri) return false;
    var s = atob(b64); for (var i = 0; i < s.length; i++) __mock.bajty.push(s.charCodeAt(i)); return true; },
  souborDokonci: function(id){ __mock.dokonci++; return 'OK:Download/AtariHelp/' + __mock.zacni[__mock.zacni.length-1][0] + '/' + __mock.jm; },
  souborZrus: function(id){ __mock.zrus++; }
};
window.napMixKus = 16384;
"""
BEZ_MP4 = """
(function(){ var o = MediaRecorder.isTypeSupported.bind(MediaRecorder);
  MediaRecorder.isTypeSupported = function(t){ return /mp4/i.test(t) ? false : o(t); }; })();
"""
URL = 'file://' + os.path.join(A, 'player/index.html')
STAV = """() => ({st: document.getElementById('status').textContent,
  rec: document.getElementById('btnRec').classList.contains('recOn'),
  hraje: !!(window.__audia[0] && !window.__audia[0].paused), src: window.__audia[0] ? window.__audia[0].src : '',
  cas: window.__audia[0] ? window.__audia[0].currentTime : 0, vol: window.__audia[0] ? window.__audia[0].volume : -1,
  poz: __mock.pozadavky.length, zacni: __mock.zacni, kusu: __mock.kusu, n: __mock.bajty.length, dokonci: __mock.dokonci, zrus: __mock.zrus,
  pl: document.querySelectorAll('#playlistItems .radek').length,
  obl: (function(){ try { return JSON.parse(localStorage.getItem('nap_player_oblibene_v1') || '[]'); } catch(e) { return 'CHYBA'; } })()})"""

chybyJs = []


def stranka(b, padni=0, save=True, bezMp4=False, sirka=412, vyska=915, stahovani=False):
    ctx = b.new_context(viewport={'width': sirka, 'height': vyska}, accept_downloads=stahovani)
    ctx.add_init_script(MOST % (json.dumps(soubory), json.dumps(knihovna), padni, SAVE if save else '') + (BEZ_MP4 if bezMp4 else ''))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto(URL)
    pg.wait_for_timeout(500)
    return ctx, pg


def cekej(pg, podminka, ms=8000, krok=100):
    s = None
    for _ in range(int(ms / krok)):
        s = pg.evaluate(STAV)
        if podminka(s):
            return s
        pg.wait_for_timeout(krok)
    return s


def klik(pg, sel):
    pg.click(sel, force=True)


def knihovna_klik(pg, i):
    klik(pg, '#btnMyPlaylists')
    pg.wait_for_timeout(150)
    pg.locator('#libraryItems .radek button.hlavni').nth(i).click()


def bajty_do_souboru(pg, jm):
    b64 = pg.evaluate("""() => { const b = __mock.bajty; let s = ''; for (let i = 0; i < b.length; i += 8192)
        s += String.fromCharCode.apply(null, b.slice(i, i + 8192)); return btoa(s); }""")
    cesta = os.path.join(PRAC, jm)
    open(cesta, 'wb').write(base64.b64decode(b64))
    return cesta


def rozbor(cesta):
    """(format, kodek, delka z hlavicky, delka dekodovanim, RMS, chyba dekodovani)"""
    pr = subprocess.run(['ffprobe', '-v', 'error', '-show_entries', 'format=format_name,duration:stream=codec_name', '-of', 'json', cesta],
                        capture_output=True, text=True)
    j = json.loads(pr.stdout or '{}')
    fmt = j.get('format', {}).get('format_name', '')
    dh = j.get('format', {}).get('duration')
    kod = ','.join(s.get('codec_name', '') for s in j.get('streams', []))
    dek = subprocess.run(['ffmpeg', '-v', 'error', '-i', cesta, '-ac', '1', '-ar', '48000', '-f', 'f32le', '-'], capture_output=True)
    n = len(dek.stdout) // 4
    vz = struct.unpack('<%df' % n, dek.stdout[:n * 4]) if n else ()
    rms = math.sqrt(sum(v * v for v in vz) / n) if n else 0.0
    return fmt, kod, (float(dh) if dh else None), n / 48000.0, rms, dek.stderr.decode().strip()


with sync_playwright() as p:
    b = p.chromium.launch(args=['--allow-file-access-from-files', '--autoplay-policy=no-user-gesture-required'])

    # ===== 1) knihovna v mobilu, nacitani po jedne, uvolneni pameti, bez duplikatu =====
    ctx, pg = stranka(b, sirka=360, vyska=780)
    klik(pg, '#btnMyPlaylists')
    pg.wait_for_timeout(200)
    radky = pg.locator('#libraryItems .radek')
    hv = pg.evaluate("""() => [...document.querySelectorAll('#libraryItems .radek')].map(r => {
        const h = r.querySelector('button.hvezda'), m = r.querySelector('button.hlavni'), box = document.querySelector('#playlistPanel .box').getBoundingClientRect();
        const bh = h.getBoundingClientRect(), bm = m.getBoundingClientRect();
        return {t: h.textContent, w: Math.round(bh.width), pravy: bh.right <= box.right + 0.5, mw: Math.round(bm.width), vedle: bm.right <= bh.left}; })""")
    over(radky.count() == 5 and all(x['t'] == '☆' and x['w'] == 52 and x['pravy'] and x['mw'] > 150 and x['vedle'] for x in hv),
         '1) seznam hudby v mobilu: 5 skladeb, u kazde hvezdicka ☆ 52 px vedle jmena, nic nepreteka (sirka 360 px): %s' % hv[0])
    pg.screenshot(path=os.path.join(PRAC, 'prehravac_knihovna_360.png'))
    pg.locator('#libraryItems .radek button.hlavni').nth(0).click()
    s = cekej(pg, lambda s: s['hraje'] and s['st'].startswith('PLAY'))
    src1 = s['src']
    over(s['hraje'] and s['poz'] == 1 and s['st'] == 'PLAY • Ranni_vlna.mp3' and s['pl'] == 1 and src1.startswith('blob:'),
         '1) klepnuti na skladbu: Java ji nacte (1 zadost), hraje, status "%s", v playlistu 1' % s['st'])
    knihovna_klik(pg, 1)
    s = cekej(pg, lambda s: s['hraje'] and 'Neon_Drive' in s['st'])
    zrusena = pg.evaluate("(u) => fetch(u).then(() => 'ZIJE', () => 'ZRUSENA')", src1)
    over(s['hraje'] and s['pl'] == 2 and s['poz'] == 2 and zrusena == 'ZRUSENA',
         '1) druha skladba hraje, predchozi blob se uvolnil z pameti (%s) - drive zustavaly v pameti vsechny' % zrusena)
    knihovna_klik(pg, 0)
    s = cekej(pg, lambda s: s['hraje'] and 'Ranni_vlna' in s['st'])
    over(s['pl'] == 2 and s['poz'] == 3, '1) znovu prvni: nacte se znovu (3. zadost), v playlistu porad 2 (zadny duplikat)')
    ctx.close()

    # ===== 2) FAVORITES: hvezdicka, trvalost, HRAT VSECHNY, presunuty / chybejici soubor, odebrani =====
    ctx, pg = stranka(b, sirka=360, vyska=780)
    klik(pg, '#btnMyPlaylists')
    pg.wait_for_timeout(200)
    pg.locator('#libraryItems .radek button.hvezda').nth(1).click()
    pg.locator('#libraryItems .radek button.hvezda').nth(2).click()
    s = pg.evaluate(STAV)
    hv = pg.evaluate("() => [...document.querySelectorAll('#libraryItems .radek button.hvezda')].map(h => h.textContent + (h.classList.contains('ano') ? '+' : ''))")
    over(hv[1] == '★+' and hv[2] == '★+' and hv[0] == '☆' and [o['n'] for o in s['obl']] == ['Neon_Drive.mp3', 'Kazeta_C90.mp3']
         and s['obl'][0]['u'] == knihovna[1]['uri'] and s['poz'] == 0,
         '2) hvezdicka v seznamu hudby: ☆ -> ★, ulozeno (jmeno + adresa), soubor se pritom nenacita: %s' % hv)
    pg.reload()
    pg.wait_for_timeout(500)
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(200)
    fav = pg.evaluate("() => ({open: document.getElementById('favoritesPanel').classList.contains('open'), polozky: [...document.querySelectorAll('#favItems .radek button.hlavni')].map(b => b.textContent), tg: document.getElementById('favToggle').textContent, tgOff: document.getElementById('favToggle').disabled})")
    over(fav['open'] and fav['polozky'] == ['1. ★ Neon_Drive.mp3', '2. ★ Kazeta_C90.mp3'] and fav['tgOff'],
         '2) po znovunacteni stranky (= novy start appky) jsou oblibene porad tam: %s, "%s" (nic nehraje)' % (fav['polozky'], fav['tg']))
    pg.screenshot(path=os.path.join(PRAC, 'prehravac_oblibene_360.png'))
    klik(pg, '#favPlayAll')
    s = cekej(pg, lambda s: s['hraje'] and 'Neon_Drive' in s['st'])
    over(s['hraje'] and s['pl'] == 2 and s['poz'] == 1,
         '2) HRAT VSECHNY OBLIBENE: playlist = 2 oblibene, hraje prvni (nacetla se jen ta jedna)')
    # na konec prvni skladby -> dalsi se nacte a hraje
    pg.evaluate("() => { const a = __audia[0]; a.currentTime = Math.max(0, a.duration - 0.4); }")
    s = cekej(pg, lambda s: s['hraje'] and 'Kazeta_C90' in s['st'], ms=6000)
    over(s['hraje'] and s['poz'] == 2, '2) po konci prvni oblibene se nacte a hraje druha')
    # aktualni skladbu pridat tlacitkem v panelu FAVORITES (uz je oblibena -> odebrat)
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(150)
    tg = pg.evaluate("() => document.getElementById('favToggle').textContent")
    klik(pg, '#favToggle')
    s = pg.evaluate(STAV)
    over(tg == '☆ ODEBRAT HRAJICI SKLADBU' and [o['n'] for o in s['obl']] == ['Neon_Drive.mp3'],
         '2) tlacitko v panelu: "%s" -> hrajici skladba odebrana z oblibenych' % tg)
    klik(pg, '#favToggle')
    s = pg.evaluate(STAV)
    over(pg.evaluate("() => document.getElementById('favToggle').textContent") == '☆ ODEBRAT HRAJICI SKLADBU' and len(s['obl']) == 2,
         '2) a znovu pridana (★ PRIDAT HRAJICI SKLADBU)')
    # odebrani krizkem s potvrzenim
    x = pg.locator('#favItems .radek button.pryc').nth(0)
    x.click()
    t1 = x.text_content()
    n1 = len(pg.evaluate(STAV)['obl'])
    x.click()
    s = pg.evaluate(STAV)
    over(t1 == 'ODEBRAT?' and n1 == 2 and [o['n'] for o in s['obl']] == ['Kazeta_C90.mp3'],
         '2) ✕ u oblibene: 1. klepnuti "%s" (nic se nesmaze), 2. klepnuti odebere' % t1)
    klik(pg, '#favClose')
    # presunuty soubor (jina adresa, stejne jmeno) a soubor, ktery uz v mobilu neni
    pg.evaluate("""() => { localStorage.setItem('nap_player_oblibene_v1', JSON.stringify([
        {n: 'Ranni_vlna.mp3', u: 'content://media/external/audio/media/9999', s: 0, t: 1},
        {n: 'zmizela.mp3', u: 'content://media/external/audio/media/8888', s: 0, t: 2}])); }""")
    pg.reload()
    pg.wait_for_timeout(500)
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(150)
    pg.locator('#favItems .radek button.hlavni').nth(0).click()
    s = cekej(pg, lambda s: s['hraje'] and 'Ranni_vlna' in s['st'])
    over(s['hraje'] and s['poz'] == 2 and s['obl'][0]['u'] == knihovna[0]['uri'],
         '2) oblibena se presunula (stara adresa nejde): najde se v mobilu podle jmena, hraje a oblibena ma novou adresu')
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(150)
    pg.locator('#favItems .radek button.hlavni').nth(1).click()
    s = cekej(pg, lambda s: 'NENI DOSTUPNA' in s['st'])
    over(s['st'] == 'SKLADBA NENI DOSTUPNA • zmizela.mp3', '2) oblibena, ktera uz v mobilu neni: "%s"' % s['st'])
    # ZPET (Java vola napZpet): panel zavrit, jinak 0
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(100)
    z1 = pg.evaluate("() => [napZpet(), document.getElementById('favoritesPanel').classList.contains('open')]")
    z2 = pg.evaluate("() => napZpet()")
    over(z1 == [1, False] and z2 == 0, '2) ZPET na telefonu: otevreny panel zavre (1), jinak necha appku jit zpet (0)')
    ctx.close()

    # ===== 3) RECORD = mixtape: pauza se nenahrava, STOP ulozi, M4A, nezavisle na VOLUME =====
    ctx, pg = stranka(b)
    klik(pg, '#btnRec')
    pg.wait_for_timeout(200)
    s = pg.evaluate(STAV)
    over(s['st'] == 'RECORD • NEJDRIV VYBER SKLADBY' and not s['rec'] and pg.evaluate("() => document.getElementById('playlistPanel').classList.contains('open')"),
         '3) RECORD bez skladeb: "%s" a otevre se seznam' % s['st'])
    pg.locator('#libraryItems .radek button.hlavni').nth(1).click()
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnStop')
    pg.wait_for_timeout(300)
    klik(pg, '#btnRec')
    s = cekej(pg, lambda s: s['hraje'] and s['st'].startswith('● REC'), ms=4000)
    over(s['rec'] and s['hraje'] and re.match(r'^● REC 00:0\d • Neon_Drive\.mp3$', s['st']) is not None and s['vol'] == 1,
         '3) RECORD: tlacitko zhava, prehravani se samo rozjede, status "%s"' % s['st'])
    pg.wait_for_timeout(2500)
    klik(pg, '#btnPause')
    pg.wait_for_timeout(200)
    s1 = pg.evaluate(STAV)
    pg.wait_for_timeout(1800)
    s2 = pg.evaluate(STAV)
    over(s1['st'] == 'PAUSE • RECORD CEKA' and s2['st'].startswith('❚❚ REC 00:0') and s2['rec'],
         '3) PAUSE: "%s", pak "%s" (cas nahravky stoji)' % (s1['st'], s2['st']))
    klik(pg, '#btnPlay')
    pg.wait_for_timeout(1500)
    klik(pg, '#btnStop')
    s = cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=10000)
    jm = s['zacni'][0][1] if s['zacni'] else ''
    print('   stav:', {k: s[k] for k in ('st', 'zacni', 'kusu', 'n', 'dokonci', 'zrus')})
    over(len(s['zacni']) == 1 and s['zacni'][0][0] == 'Mixtape' and re.match(r'^NP_VISION_mixtape_\d{8}_\d{6}\.m4a$', jm) and s['zacni'][0][2] == 'audio/mp4'
         and s['dokonci'] == 1 and s['kusu'] >= 2 and not s['rec'],
         '3) STOP ulozi: souborZacni(Mixtape, %s, %s), %d kousku, souborDokonci 1x' % (jm, s['zacni'][0][2] if s['zacni'] else '-', s['kusu']))
    over(s['st'].startswith('MIXTAPE ULOZENA 00:0') and ('Download/AtariHelp/Mixtape/' + jm) in s['st'], '3) hlaska: "%s"' % s['st'])
    f1 = bajty_do_souboru(pg, 'mixtape_1.m4a')
    fmt, kod, dh, dd, rms1, ch = rozbor(f1)
    over('mp4' in fmt and kod in ('opus', 'aac') and dh is not None and 3.4 <= dh <= 4.8 and 3.4 <= dd <= 4.8 and not ch and rms1 > 0.02,
         '3) nahravka: %s / %s, delka %.2f s (hlavicka) / %.2f s (dekodovano) = 2,5 + 1,5 s hrani, 2 s pauzy NENI v ni; RMS %.3f'
         % (fmt.split(',')[0], kod, dh or -1, dd, rms1))
    # VOLUME na 30 % -> nahravka stejne silna (knoflik zesiluje jen poslech)
    box = pg.locator('#knVolume').bounding_box()
    pg.mouse.move(box['x'] + box['width'] / 2, box['y'] + box['height'] * 0.7)
    pg.mouse.down()
    pg.mouse.up()
    pg.wait_for_timeout(200)
    st_vol = pg.evaluate(STAV)['st']
    pg.evaluate("() => { __mock.zacni = []; __mock.dokonci = 0; }")
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['hraje'] and s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(2500)
    klik(pg, '#btnRec')
    s = cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=10000)
    f2 = bajty_do_souboru(pg, 'mixtape_2_volume30.m4a')
    fmt2, kod2, dh2, dd2, rms2, ch2 = rozbor(f2)
    pomer = rms2 / rms1 if rms1 else 0
    over(st_vol.startswith('VOLUME • 0.3') and s['vol'] == 1 and 0.8 <= pomer <= 1.25 and s['hraje'],
         '3) VOLUME na 30 %% ("%s"): nahravka stejne silna (RMS %.3f / %.3f = %.2f); <audio>.volume = %s (hlasitost dela jen graf - drive 2x, 30 %% hralo jako 9 %%); RECORD znovu = konec, hraje se dal'
         % (st_vol, rms2, rms1, pomer, s['vol']))
    # BALANCE uplne vlevo -> v nahravce je pravy kanal ticho (nahrava se az za EQ / BASS / TREBLE / BALANCE)
    box = pg.locator('#knBalance').bounding_box()
    pg.mouse.move(box['x'] + 1, box['y'] + box['height'] / 2)
    pg.mouse.down()
    pg.mouse.up()
    pg.wait_for_timeout(200)
    st_bal = pg.evaluate(STAV)['st']
    pg.evaluate("() => { __mock.zacni = []; __mock.dokonci = 0; }")
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['hraje'] and s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(1500)
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=10000)
    f_bal = bajty_do_souboru(pg, 'mixtape_balance_vlevo.m4a')
    dek = subprocess.run(['ffmpeg', '-v', 'error', '-i', f_bal, '-ac', '2', '-ar', '48000', '-f', 'f32le', '-'], capture_output=True)
    n = len(dek.stdout) // 8
    vz = struct.unpack('<%df' % (2 * n), dek.stdout[:8 * n]) if n else ()
    rl = math.sqrt(sum(vz[i] * vz[i] for i in range(0, 2 * n, 2)) / n) if n else 0
    rp = math.sqrt(sum(vz[i] * vz[i] for i in range(1, 2 * n, 2)) / n) if n else 0
    over(st_bal == 'BALANCE • L' and rl > 0.02 and rp < rl * 0.1,
         '3) BALANCE uplne vlevo ("%s"): v nahravce levy kanal RMS %.3f, pravy %.4f (o vic nez 20 dB min) - nahrava se zvuk az za efekty (EQ, BASS, TREBLE, BALANCE)' % (st_bal, rl, rp))
    ctx.close()

    # ===== 4) RECORD pres dve skladby: konec playlistu = mixtape hotova, nacitani dalsi se nenahrava =====
    ctx, pg = stranka(b)
    pg.evaluate("""(k) => localStorage.setItem('nap_player_oblibene_v1', JSON.stringify([
        {n: k[3].name, u: k[3].uri, s: 0, t: 1}, {n: k[4].name, u: k[4].uri, s: 0, t: 2}]))""", knihovna)
    pg.reload()
    pg.wait_for_timeout(500)
    klik(pg, '#navFavorites')
    pg.wait_for_timeout(150)
    klik(pg, '#favPlayAll')
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnStop')
    pg.wait_for_timeout(300)
    klik(pg, '#btnRec')
    s = cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=15000)
    f3 = bajty_do_souboru(pg, 'mixtape_3_dve_skladby.m4a')
    fmt3, kod3, dh3, dd3, rms3, ch3 = rozbor(f3)
    over(s['st'].startswith('MIXTAPE ULOZENA') and s['dokonci'] == 1 and not s['rec'] and 5.5 <= dd3 <= 6.6 and not ch3,
         '3) RECORD + 2 skladby po 3 s: na konci playlistu se sama ulozi, delka %.2f s (dekodovano) = obe skladby bez mezery na nacitani' % dd3)
    # MENU behem nahravani: nejdriv ulozit, pak menu
    klik(pg, '#btnPlay')
    cekej(pg, lambda s: s['hraje'])
    pg.evaluate("() => { __mock.dokonci = 0; }")
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(1200)
    klik(pg, '#btnMenu')
    pg.wait_for_timeout(250)
    url_hned = pg.url
    ulozeno = 0
    for _ in range(60):
        try:
            if pg.url.endswith('/assets/index.html'):
                break
            ulozeno = max(ulozeno or 0, pg.evaluate('() => __mock.dokonci'))  # (po navigaci uz bezi v nove strance)
        except Exception:
            pass
        pg.wait_for_timeout(100)
    over(url_hned.endswith('/player/index.html') and ulozeno == 1 and pg.url.endswith('/assets/index.html'),
         '3) MENU behem nahravani: prehravac pocka, ulozi mixtape (souborDokonci %d) a teprve pak jde do menu' % ulozeno)
    ctx.close()

    # ===== 5) ZPET (Java napZpet) behem nahravani =====
    ctx, pg = stranka(b)
    knihovna_klik(pg, 0)
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(1200)
    z = pg.evaluate('() => napZpet()')
    for _ in range(60):
        try:
            if pg.url.endswith('/assets/index.html'):
                break
        except Exception:
            pass
        pg.wait_for_timeout(100)
    over(z == 1 and pg.url.endswith('/assets/index.html'), '5) ZPET na telefonu behem nahravani: napZpet = 1, mixtape se ulozi, pak menu')
    ctx.close()

    # ===== 6) WebView bez M4A: WebM s doplnenou delkou =====
    ctx, pg = stranka(b, bezMp4=True)
    knihovna_klik(pg, 2)
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(3000)
    klik(pg, '#btnStop')
    s = cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=10000)
    jm = s['zacni'][0][1] if s['zacni'] else ''
    f4 = bajty_do_souboru(pg, 'mixtape_4.webm')
    fmt4, kod4, dh4, dd4, rms4, ch4 = rozbor(f4)
    dpl = pg.evaluate("""() => new Promise(res => { const b = new Uint8Array(__mock.bajty); const a = new Audio();
        a.onloadedmetadata = () => res(String(a.duration)); a.onerror = () => res('CHYBA'); setTimeout(() => res('CAS'), 4000);
        a.src = URL.createObjectURL(new Blob([b], {type: 'audio/webm'})); })""")
    over(re.match(r'^NP_VISION_mixtape_\d{8}_\d{6}\.webm$', jm) and s['zacni'][0][2] == 'audio/webm' and 'webm' in fmt4 and kod4 == 'opus'
         and dh4 is not None and abs(dh4 - dd4) < 0.25 and 2.6 <= dd4 <= 3.6 and not ch4 and dpl not in ('Infinity', 'CHYBA', 'CAS'),
         '6) bez M4A: %s (%s), delka v hlavicce %.2f s = dekodovano %.2f s, Chromium ukaze delku %s (drive Infinity)'
         % (jm, s['zacni'][0][2] if s['zacni'] else '-', dh4 or -1, dd4, dpl))
    ctx.close()

    # ===== 7) chyba zapisu v Jave, prohlizec na PC (bez AHSAVE) =====
    ctx, pg = stranka(b, padni=1)
    knihovna_klik(pg, 0)
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(1500)
    klik(pg, '#btnStop')
    s = cekej(pg, lambda s: s['st'].startswith('MIXTAPE'), ms=8000)
    over(s['st'] == 'MIXTAPE SE NEULOZILA • CHYBA ZAPISU' and s['zrus'] == 1 and s['dokonci'] == 0,
         '7) chyba zapisu v Jave: "%s", souborZrus 1x' % s['st'])
    klik(pg, '#btnRec')
    s = cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    over(s['rec'], '7) po chybe jde nahravat znovu')
    ctx.close()
    ctx, pg = stranka(b, save=False, stahovani=True)
    knihovna_klik(pg, 0)
    cekej(pg, lambda s: s['hraje'])
    klik(pg, '#btnRec')
    cekej(pg, lambda s: s['st'].startswith('● REC'), ms=4000)
    pg.wait_for_timeout(1500)
    with pg.expect_download(timeout=8000) as d:
        klik(pg, '#btnStop')
    dl = d.value
    cesta = os.path.join(PRAC, dl.suggested_filename)
    dl.save_as(cesta)
    fmt5, kod5, dh5, dd5, rms5, ch5 = rozbor(cesta)
    over(re.match(r'^NP_VISION_mixtape_\d{8}_\d{6}\.m4a$', dl.suggested_filename) and 'mp4' in fmt5 and 1.0 <= dd5 <= 2.2 and not ch5,
         '7) prohlizec na PC: stazeni %s (%d B, %.2f s)' % (dl.suggested_filename, os.path.getsize(cesta), dd5))
    ctx.close()
    b.close()

over(not chybyJs, '8) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs) if chybyJs else ''))
vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b312_prehravac.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
