#!/usr/bin/env python3
"""B309: stranka PS1 - ULOZIT LOG A ODESLAT jako v HELP (Chromium).

Rene: "ten log udelej jako v atari emu help - proste s moznosti okamziteho odeslani".
Most do appky (window.AHPS1) napodoben podle MainActivity B309: ps1UlozAOdesli(jmeno, text)
ulozi log (+ log appky) do Downloads/AtariHelp a hned otevre nabidku "Poslat log".
Kontroluje se, co stranka posle (log stranky + denik PS1), na vysku (kolecko -> panel)
i na sirku (kolecko -> D-PAD panel -> rovnou ODESLAT), hlasky a starsi appka bez metody.

Pouziti: python3 test_b309_ps1_log.py
"""
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
STRANKA = os.path.join(KOREN, 'app/src/main/assets/emu_ps1/index.html')

MOST = r"""
(function(){
  var M = window.__most = {plocha:[], status:'idle', n:0, v:'', ulozeno:null, odeslano:[], denik:'', odpoved:null, bezMetody:false};
  window.AHPS1 = {
    ps1PlochaVisible:function(s){ M.plocha.push(!!s); },
    ps1MemCardInfo:function(){ return 'PS1_MEMCARD core_da_sram=ANO'; },
    tvCastActive:function(){ return false; },
    ps1Vibrate:function(){ return 'OK'; },
    ps1StartStav:function(){ return M.n+'|'+M.v; },
    ps1SaveBios:function(){ return ''; },
    ps1Reset:function(){ return 'PS1_RESET_OK'; },
    ps1RemoteCachePath:function(){ return '/x'; },
    ps1QuickSave:function(){ return ''; }, ps1QuickLoad:function(){ return ''; },
    ps1PickGame:function(){},
    ps1Log:function(){ return 'OK'; },
    ps1LibraryList:function(){ return '[]'; },
    ps1LibraryLaunch:function(){},
    ps1LibraryDelete:function(){ return ''; },
    ps1Input:function(){},
    ps1DownloadAndBoot:function(){},
    ps1Denik:function(){ return M.denik; },
    ps1ClearRemoteCache:function(){ return ''; },
    ps1Status:function(){ return 'PS1_RUN running=YES'; },
    ps1SetScreenRect:function(){},
    ps1RemoteStatus:function(){ return M.status; },
    ps1FramePreviewB64:function(){ return ''; },
    ps1UlozAOdesli:function(jm, text){ M.odeslano.push([jm, text]);
      return M.odpoved !== null ? M.odpoved : 'DOWNLOADS_OK:/storage/emulated/0/Download/AtariHelp/'+jm; }
  };
  if (M.bezMetody) delete window.AHPS1.ps1UlozAOdesli;
  window.AHSAVE = { save:function(jm, obsah){ M.ulozeno=[jm, obsah]; return 'DOWNLOADS_OK '+jm; } };
  window.AHNET = { openPs1Games:function(){} };
})();
"""

DENIK = ("2026-10-14 20:28:01.979  PS1 ==================== PS1 START: HRA ====================\\n"
         "2026-10-14 20:28:07.212  PS1 ZDRAVI 5 s: emulace 50.2 kroku/s | oprava Doomu: 64563 tenkych trojuhelniku ve 2x\\n")


def main():
    vystup = []
    chyb = [0]
    kontrol = [0]

    def tisk(s):
        print(s, flush=True)
        vystup.append(s)

    def over(ok, co):
        kontrol[0] += 1
        if not ok:
            chyb[0] += 1
        tisk(('OK     ' if ok else 'CHYBA  ') + co)

    tisk('B309: stranka PS1 - ULOZIT LOG A ODESLAT (Chromium, most appky podle MainActivity B309)')
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        try:
            br = p.chromium.launch()
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome')

        def otevri(sirka, vyska, bez_metody=False):
            ctx = br.new_context(viewport={'width': sirka, 'height': vyska}, device_scale_factor=2, has_touch=True)
            page = ctx.new_page()
            page.route(re.compile(r'^https?://.*'), lambda r: r.abort())
            chyby = []
            page.on('pageerror', lambda e: chyby.append(str(e)))
            page.add_init_script(MOST.replace('bezMetody:false', 'bezMetody:%s' % ('true' if bez_metody else 'false')))
            page.goto('file://' + STRANKA)
            page.wait_for_timeout(800)
            page.evaluate("window.__most.denik='%s'" % DENIK)
            return ctx, page, chyby

        def toast(page):
            return page.evaluate("(function(){var t=document.getElementById('toast');return t&&t.classList.contains('show')?t.textContent:'';})()")

        # ---- na vysku: kolecko -> panel s logem ----
        ctx, page, chyby = otevri(412, 860)
        page.click('#btnSettings')
        page.wait_for_timeout(300)
        over(page.evaluate("document.getElementById('helpPanel').classList.contains('open')"), 'na vysku: kolecko otevre panel s logem')
        txt = page.inner_text('#logSave')
        over(txt.strip() == 'ULOŽIT LOG A ODESLAT', 'tlacitko se jmenuje jako v HELP: "%s"' % txt.strip())
        page.click('#logSave')
        page.wait_for_timeout(300)
        od = page.evaluate('window.__most.odeslano')
        over(len(od) == 1, 'jedno klepnuti = jedno odeslani (appka ulozi a hned otevre nabidku "Poslat log")')
        if od:
            jm, text = od[0]
            tisk('   soubor %s, %d znaku' % (jm, len(text)))
            over(re.match(r'AtariHelp_PS1_LOG_\d{8}_\d{6}\.txt$', jm) is not None, 'nazev souboru s datem a casem')
            over('DENIK PS1 (C++, jadro)' in text and 'oprava Doomu: 64563' in text and 'ready' in text,
                 'v logu je log stranky i denik PS1 z C++ (log appky pridava Java)')
        t = toast(page)
        over('vyber, cim ho poslat' in t, 'hlaska: "%s"' % t)
        over(page.evaluate('window.__most.ulozeno') is None, 'stara cesta (jen ulozit) se nepouzila')
        over(not chyby, 'na vysku zadna chyba JavaScriptu %s' % chyby[:2])
        ctx.close()

        # ---- na sirku (hrani): kolecko -> D-PAD panel -> rovnou ODESLAT ----
        ctx, page, chyby = otevri(860, 412)
        page.click('#btnSettings')
        page.wait_for_timeout(300)
        over(page.evaluate("document.getElementById('dpadMenuPanel').classList.contains('open')"), 'na sirku: kolecko otevre D-PAD panel (jako dosud)')
        b = page.query_selector('#btnLogOdeslat')
        over(b is not None and b.is_visible() and b.inner_text().strip() == 'ULOŽIT LOG A ODESLAT',
             'v D-PAD panelu je hned ULOZIT LOG A ODESLAT (ne az pres DALSI)')
        if b:
            b.click()
            page.wait_for_timeout(300)
        od = page.evaluate('window.__most.odeslano')
        over(len(od) == 1 and 'DENIK PS1 (C++, jadro)' in od[0][1], 'na sirku: log odeslan jednim klepnutim')
        over(not page.evaluate("document.getElementById('dpadMenuPanel').classList.contains('open')"), 'panel se zavrel (hra je zase videt)')
        over(not chyby, 'na sirku zadna chyba JavaScriptu %s' % chyby[:2])
        ctx.close()

        # ---- chyba ulozeni ----
        ctx, page, chyby = otevri(412, 860)
        page.evaluate("window.__most.odpoved='CHYBA:plne uloziste'")
        page.click('#btnSettings')
        page.wait_for_timeout(200)
        page.click('#logSave')
        page.wait_for_timeout(300)
        t = toast(page)
        over('LOG SE NEULOZIL' in t and 'KOPIROVAT LOG' in t, 'kdyz se ulozit nepodari, hlaska rekne proc a poradi KOPIROVAT LOG (%s)' % t)
        ctx.close()

        # ---- starsi appka (bez ps1UlozAOdesli) ----
        ctx, page, chyby = otevri(412, 860, bez_metody=True)
        page.click('#btnSettings')
        page.wait_for_timeout(200)
        page.click('#logSave')
        page.wait_for_timeout(300)
        u = page.evaluate('window.__most.ulozeno')
        over(u is not None and 'DENIK PS1 (C++, jadro)' in u[1], 'appka bez nove metody: log se aspon ulozi postaru (AHSAVE)')
        over(not chyby, 'zadna chyba JavaScriptu %s' % chyby[:2])
        ctx.close()
        br.close()

    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b309_ps1_log.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
