#!/usr/bin/env python3
"""B308: stranka PS1 (assets/emu_ps1/index.html) v Chromiu - nahravani hry podle
SKUTECNEHO stavu appky, banner stahovani mimo monitor, LOG s denikem PS1.

Most do appky (window.AHPS1, AHSAVE) je tu napodoben PRESNE podle toho, co
v B308 dela MainActivity:
  - ps1LibraryLaunch(key): status "PS1_LIBRARY_LAUNCH ...", pak na vlakne
    "PS1_REMOTE_BOOTING #cislo jmeno", jadro startuje (test urci, jak dlouho),
    pak "PS1_REMOTE_BOOT_OK jmeno" a ps1StartDokoncen("OK jmeno")
  - CD ISO (vyber souboru) / stazeni z Drive: jen status "PS1_REMOTE_BOOTING #cislo"
  - ps1Reset: BIOS znovu, ps1StartDokoncen("BIOS_OK")
  - ps1PlochaVisible(show): zaznamena se (nativni plocha s obrazem jadra)

Pouziti: python3 test_b308_ps1_stranka.py
"""
import os
import re
import sys
import time

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
STRANKA = os.path.join(KOREN, 'app/src/main/assets/emu_ps1/index.html')

MOST = r"""
(function(){
  var M = window.__most = {plocha:[], status:'idle', n:0, v:'', ulozeno:null, spusteno:[], denik:'',
                          resetu:0, bootMs:1500, vysledek:'OK'};
  function dokonci(v){ M.v=v; M.n++; }
  window.AHPS1 = {
    ps1PlochaVisible:function(s){ M.plocha.push(!!s); },
    ps1MemCardInfo:function(){ return 'PS1_MEMCARD core_da_sram=ANO'; },
    tvCastActive:function(){ return false; },
    ps1Vibrate:function(){ return 'OK'; },
    ps1StartStav:function(){ return M.n+'|'+M.v; },
    ps1SaveBios:function(){ return ''; },
    ps1Reset:function(){ M.resetu++; setTimeout(function(){ dokonci('BIOS_OK'); }, 700); return 'PS1_RESET_OK stop=PS1_STOPPED'; },
    ps1RemoteCachePath:function(){ return '/x'; },
    ps1QuickSave:function(){ return ''; }, ps1QuickLoad:function(){ return ''; },
    ps1PickGame:function(){},
    ps1Log:function(){ return 'OK'; },
    ps1LibraryList:function(){ return JSON.stringify([{key:'doom',nazev:'Doom (USA)',bytes:512*1024*1024,ma_ulozenou_hru:false}]); },
    ps1LibraryLaunch:function(key){
      M.spusteno.push(key);
      M.status='PS1_LIBRARY_LAUNCH Doom.cue';
      setTimeout(function(){
        var c=M.n+1; M.status='PS1_REMOTE_BOOTING #'+c+' doom__Doom.cue';
        setTimeout(function(){
          if(M.vysledek==='OK'){ M.status='PS1_REMOTE_BOOT_OK doom__Doom.cue'; dokonci('OK doom__Doom.cue'); }
          else { M.status='PS1_REMOTE_FAIL boot PS1_HRA_FAIL jadro'; dokonci('CHYBA jadro hru nespustilo (PS1_HRA_FAIL jadro) - podrobnosti v LOGu (denik PS1)'); }
        }, M.bootMs);
      }, 60);
    },
    ps1LibraryDelete:function(){ return ''; },
    ps1Input:function(){},
    ps1DownloadAndBoot:function(){},
    ps1Denik:function(){ return M.denik; },
    ps1ClearRemoteCache:function(){ return ''; },
    ps1Status:function(){ return 'PS1_RUN running=YES'; },
    ps1SetScreenRect:function(){},
    ps1RemoteStatus:function(){ return M.status; },
    ps1FramePreviewB64:function(){ return ''; }
  };
  window.AHSAVE = { save:function(jm, obsah){ M.ulozeno=[jm, obsah]; return 'DOWNLOADS_OK '+jm; } };
  window.AHNET = { openPs1Games:function(){} };
  window.__dokonci = dokonci;
})();
"""


def main():
    vystup = []

    def tisk(s):
        print(s, flush=True)
        vystup.append(s)

    chyb = [0]
    kontrol = [0]

    def over(ok, co):
        kontrol[0] += 1
        if not ok:
            chyb[0] += 1
        tisk(('OK     ' if ok else 'CHYBA  ') + co)

    tisk('B308: stranka PS1 v Chromiu - nahravani hry podle skutecneho stavu, banner, LOG s denikem')
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        try:
            br = p.chromium.launch()
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome')
        # ---- na vysku (jako telefon) ----
        ctx = br.new_context(viewport={'width': 412, 'height': 860}, device_scale_factor=2, has_touch=True)
        page = ctx.new_page()
        page.route(re.compile(r'^https?://.*'), lambda r: r.abort())
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        page.add_init_script(MOST)
        page.goto('file://' + STRANKA)
        page.wait_for_timeout(800)

        def most(k):
            return page.evaluate('window.__most.' + k)

        def overlay():
            return page.evaluate("(function(){var o=document.getElementById('psMonitorLoading');"
                                 "return o&&o.style.display!=='none'?o.textContent:'';})()")

        def rect(sel):
            return page.evaluate("(function(){var r=document.querySelector('%s').getBoundingClientRect();"
                                 "return [r.left,r.top,r.right,r.bottom];})()" % sel)

        # 1) banner stahovani na vysku mimo monitor
        page.evaluate("window.__most.status='PS1_REMOTE_DOWNLOAD 45%% doom.zip path=/x/doom'")
        page.wait_for_timeout(700)
        vid = page.evaluate("document.getElementById('remoteBanner').classList.contains('show')")
        mon, ban = rect('#psMonitor'), rect('#remoteBanner')
        tisk('   monitor %s, banner %s' % ([round(x) for x in mon], [round(x) for x in ban]))
        over(vid and ban[1] >= mon[3] - 0.5, 'na vysku je banner stahovani POD monitorem (neprekryva obraz hry ani nadpis)')
        page.evaluate("window.__most.status='idle'")
        page.wait_for_timeout(600)

        # 2) knihovna -> hra se nahrava 1,5 s -> okno v monitoru, plocha schovana, pak hra
        page.click('#btnCd')
        page.wait_for_timeout(300)
        t0 = time.time()
        page.click('.libGame .libPlay')
        page.wait_for_timeout(250)
        o = overlay()
        tisk('   nahravaci okno: ' + o)
        over('NAHRÁVÁM HRU' in o and 'Doom (USA)' in o and re.search(r'\d+ s', o) is not None,
             'po klepnuti na hru je v monitoru nahravaci okno (jmeno hry, cas)')
        over(most('spusteno') == ['doom'], 'appka dostala pokyn spustit hru')
        pl = most('plocha')
        over(pl and pl[-1] is False, 'obraz jadra (nativni plocha) je behem nahravani schovany - okno neni pod nim a stary obraz nezustava')
        page.wait_for_timeout(900)
        over('NAHRÁVÁM HRU' in overlay(), 'po 1,1 s (hra jeste nebezi) okno porad je - zadny pevny odpocet')
        page.wait_for_function("(function(){var o=document.getElementById('psMonitorLoading');return !o||o.style.display==='none';})()", timeout=5000)
        dt = time.time() - t0
        over(1.4 < dt < 2.6, 'okno zmizelo, az appka hlasila, ze hra OPRAVDU bezi (za %.1f s, start trval 1,5 s)' % dt)
        pl = most('plocha')
        over(pl[-1] is True, 'obraz jadra je zase videt')
        toast = page.evaluate("document.getElementById('toast').classList.contains('show')")
        over(not toast, 'pri uspechu zadna zbytecna hlaska')

        # 3) dlouhy start (velka hra): okno vydrzi 12 s
        page.evaluate('window.__most.bootMs=12500')
        page.click('#btnCd')
        page.wait_for_timeout(300)
        page.click('.libGame .libPlay')
        page.wait_for_timeout(11000)
        o = overlay()
        over('NAHRÁVÁM HRU' in o and re.search(r'1[01] s', o), 'start trvajici 12 s: okno je v 11. sekunde porad videt (%s)' % o[-40:])
        page.wait_for_function("(function(){var o=document.getElementById('psMonitorLoading');return !o||o.style.display==='none';})()", timeout=6000)
        over(most('plocha')[-1] is True, 'po dlouhem startu obraz zpet')

        # 4) chyba startu -> okno zmizi, hlaska proc
        page.evaluate("window.__most.bootMs=800;window.__most.vysledek='CHYBA'")
        page.click('#btnCd')
        page.wait_for_timeout(300)
        page.click('.libGame .libPlay')
        page.wait_for_function("(function(){var o=document.getElementById('psMonitorLoading');return o&&o.style.display==='none';})()", timeout=5000)
        page.wait_for_timeout(300)
        t = page.evaluate("document.getElementById('toast').textContent")
        over(t.startswith('HRA SE NESPUSTILA: jadro hru nespustilo (PS1_HRA_FAIL jadro)'), 'chyba startu: okno pryc a hlaska proc (%s)' % t[:70])
        over(most('plocha')[-1] is True, 'po chybe obraz zpet (BIOS)')
        page.evaluate("window.__most.vysledek='OK'")

        # 5) CD ISO / stazeni z Drive: appka hlasi BOOTING #cislo -> okno se zapne samo
        page.evaluate("(function(){var M=window.__most;var c=M.n+1;M.status='PS1_REMOTE_BOOTING #'+c+' Crash.bin';"
                      "setTimeout(function(){M.status='PS1_REMOTE_BOOT_OK Crash.bin';window.__dokonci('OK Crash.bin');},1800);})()")
        page.wait_for_timeout(900)
        o = overlay()
        over('NAHRÁVÁM HRU' in o and 'Crash.bin' in o, 'vyber souboru / Drive: nahravaci okno se zaplo samo (stav PS1_REMOTE_BOOTING)')
        page.wait_for_function("(function(){var o=document.getElementById('psMonitorLoading');return o&&o.style.display==='none';})()", timeout=4000)
        over(most('plocha')[-1] is True, '...a zmizelo po startu')
        # start uz dobehl driv, nez si ho stranka vsimla
        page.evaluate("(function(){var M=window.__most;var c=M.n+1;window.__dokonci('OK Rychla.bin');M.status='PS1_REMOTE_BOOTING #'+c+' Rychla.bin';})()")
        page.wait_for_timeout(1200)
        over(overlay() == '', 'start rychlejsi nez kontrola stavu: okno se hned zavre, nezustane viset')
        page.evaluate("window.__most.status='PS1_REMOTE_BOOT_OK Rychla.bin'")

        # 6) RESET -> okno do skutecneho naskoceni BIOSu
        t0 = time.time()
        page.click('#btnReset')
        page.wait_for_timeout(200)
        over('RESETUJI' in overlay(), 'RESET: okno "RESETUJI" v monitoru')
        page.wait_for_function("(function(){var o=document.getElementById('psMonitorLoading');return o&&o.style.display==='none';})()", timeout=4000)
        dt = time.time() - t0
        over(dt < 1.6 and most('resetu') == 1, 'RESET: okno zmizi, jakmile BIOS nabehne (%.1f s, drive vzdy 8 s)' % dt)

        # 7) LOG: ULOZIT LOG i KOPIROVAT LOG obsahuji denik PS1 z C++
        page.evaluate("window.__most.denik='2026-10-14 18:00:00.000  PS1 ==================== PS1 START: HRA ====================\\n"
                      "2026-10-14 18:00:05.000  PS1 ZDRAVI 5 s: emulace 59.9 kroku/s\\n'")
        page.click('#btnSettings')
        page.wait_for_timeout(300)
        jeHelp = page.evaluate("document.getElementById('helpPanel').classList.contains('open')")
        if not jeHelp:
            page.evaluate("document.getElementById('helpPanel').classList.add('open')")
        page.evaluate("document.getElementById('logSave').click()")
        u = most('ulozeno')
        over(u and u[0].startswith('AtariHelp_PS1_LOG_') and 'DENIK PS1 (C++, jadro)' in u[1] and 'ZDRAVI 5 s' in u[1]
             and 'NAHRAVANI_KONEC' in u[1], 'ULOZIT LOG: log stranky + denik PS1 z C++ (start, zdravi) v jednom souboru')
        page.evaluate("document.getElementById('logCopy').click()")
        v = page.evaluate("document.getElementById('ps1LogView').value")
        over('DENIK PS1 (C++, jadro)' in v and 'PS1 START: HRA' in v, 'KOPIROVAT LOG: totez v okne logu')
        over(not chyby_js, 'na vysku zadna chyba JavaScriptu %s' % chyby_js[:3])
        ctx.close()

        # ---- na sirku (hrani) ----
        ctx = br.new_context(viewport={'width': 860, 'height': 412}, device_scale_factor=2, has_touch=True)
        page = ctx.new_page()
        page.route(re.compile(r'^https?://.*'), lambda r: r.abort())
        chyby2 = []
        page.on('pageerror', lambda e: chyby2.append(str(e)))
        page.add_init_script(MOST)
        page.goto('file://' + STRANKA)
        page.wait_for_timeout(800)
        page.evaluate("(function(){var M=window.__most;var c=M.n+1;M.status='PS1_REMOTE_BOOTING #'+c+' Tekken.cue';"
                      "setTimeout(function(){M.status='PS1_REMOTE_BOOT_OK Tekken.cue';window.__dokonci('OK Tekken.cue');},1200);})()")
        page.wait_for_timeout(700)
        o = page.evaluate("(function(){var o=document.getElementById('psMonitorLoading');return o&&o.style.display!=='none'?o.textContent:'';})()")
        over('NAHRÁVÁM HRU' in o and page.evaluate('window.__most.plocha')[-1] is False,
             'na sirku: nahravaci okno + obraz jadra schovany')
        page.wait_for_timeout(1500)
        o = page.evaluate("(function(){var o=document.getElementById('psMonitorLoading');return o&&o.style.display!=='none'?o.textContent:'';})()")
        over(o == '' and page.evaluate('window.__most.plocha')[-1] is True, 'na sirku: po startu okno pryc, hra videt (nezakryva ji 10 s jako drive)')
        over(not chyby2, 'na sirku zadna chyba JavaScriptu %s' % chyby2[:3])
        br.close()

    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b308_ps1_stranka.txt'), 'w') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
