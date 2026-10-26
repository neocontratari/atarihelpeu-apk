"""B314: stranky appky pro test jazyka - cesta, falesne mosty Javy (co vrati) a stavy (kroky).
Krok = (popis, javascript, cekani ms). Po kazdem kroku se sebere cely text stranky."""

STRANKY = {}

# ---------------- ATARI 130XE - zadni strana (log a test jadra) ----------------
STRANKY['atari'] = {
    'cesta': 'emu_atari_cpp/index.html',
    'mock': {
        'AHATARICPP.jeNactene': True,
        'AHATARICPP.stavZarizeni': 'B291 PRISTROJ 412x846 snimku 1234',
        'AHATARICPP.cestyKLogu': '{"aktualni":"/storage/emulated/0/Download/AtariHelp/EMU10_LOG.txt","bajtu":34567,"predchozi":"(zadny)"}',
        'AHATARICPP.logAtari': 'B291 ... (log)',
        'AHATARICPP.ulozAOdesli': '{"ok":true,"znaku":34567,"soubor":"/storage/emulated/0/Download/AtariHelp/EMU10_LOG_1.txt"}',
        'AHATARICPP.samotest': '{"cpuHash":"29C55806","cpuInstr":1000000,"memHash":"D3949DC5","memReads":500000,"mips":12.5,"realtimeX":7.1,"abi":"arm64-v8a","cpuSec":0.08,"memSec":0.04,"charSigned":false}',
        'AHATARICPP.ocekavane': '{"cpuHash":"29C55806","cpuInstr":1000000,"memHash":"D3949DC5","memReads":500000}',
    },
    'kroky': [
        ('test jadra', "document.getElementById('btn').click()", 400),
        ('ulozit log', "document.getElementById('btnCopy').click()", 500),
        ('obnovit vypis', "document.getElementById('btnRefresh').click()", 200),
        ('test jadra - chyba jadra', "NAP_MOCK['AHATARICPP.samotest']='{\"chyba\":\"x\"}'; document.getElementById('btn').click()", 300),
        ('log se nepovedl', "NAP_MOCK['AHATARICPP.ulozAOdesli']='{\"ok\":false,\"chyba\":\"disk\"}'; document.getElementById('btnCopy').click()", 500),
        ('test jadra - nesedi', "NAP_MOCK['AHATARICPP.samotest']='{\"cpuHash\":\"1\",\"cpuInstr\":1,\"memHash\":\"2\",\"memReads\":2,\"mips\":1,\"realtimeX\":1,\"abi\":\"x86\",\"cpuSec\":1,\"memSec\":1,\"charSigned\":true}'; document.getElementById('btn').click()", 300),
    ],
    # B314: seznam kroku "CO TESTOVAT" je prazdny (Rene dostava kroky ve zprave)
    'zmenene_cesky': ('SPRAVNE:', 'SPATNE:', 'V PORADKU', 'MP3', 'MP3 a DZUBOX', 'OPTIONS:', 'ZAMKY Z WEBU', 'DZUBOX:',
                      'MENU, Atari', 'V poradku:', 'Kroky k vyzkouseni', 'SPATNE', '1. MP3', '2. MP3', '3. MP3', '4. OPTIONS',
                      '5. ZAMKY', '6. DZUBOX', '7. MP3', '8. MENU',
                      'B291'),          # log a stav pristroje: data-nepreklad (test je nesbira)
    # nazvy souboru / slozek a klic v logu - zustavaji, jak jsou
    'slova_en': ('karty', 'mixy', 'funguje', 'zapis', 'napoveda'),
}

# ---------------- SEGA MEGA DRIVE ----------------
STRANKY['sega'] = {
    'cesta': 'emu_sega/index.html',
    'mock': {
        'AHSega.segaLibraryList': '[{"key":"sonic.bin","nazev":"Sonic the Hedgehog","bytes":524288,"ma_ulozenou_hru":true},{"key":"columns.md","nazev":"Columns","bytes":131072,"ma_ulozenou_hru":false}]',
        'AHSega.segaSavedPositionsList': '[{"key":"sonic.bin","nazev":"Sonic the Hedgehog","bytes":81920}]',
        'AHSega.segaSaveCurrentToLibrary': 'SEGA_SAVE_TO_LIBRARY_OK',
        'AHSega.segaLibraryDelete': 'OK',
        'AHNATIVE.setPerformanceMode': 'OK',
    },
    'kroky': [
        ('NAVODY na monitoru', "document.getElementById('btnManuals').click()", 300),
        ('AUTO COMPAT', "document.getElementById('btnEuBranch').click()", 300),
        ('vykon (dlouhy stisk SBIRKA)', "document.getElementById('btnCollection').dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,cancelable:true}))", 900),
        ('D-PAD A OVLADANI', "document.getElementById('perfHigh').click(); document.getElementById('btnSegaSettings').click()", 500),
        ('OVLADANI profi', "document.getElementById('segaBtnCtrlSettings').click()", 500),
        ('zpet z profi', "document.getElementById('segaCtrlClose').click()", 300),
        ('uprava rozlozeni', "document.getElementById('btnSegaSettings').click(); document.getElementById('segaBtnEditLayout').click()", 400),
        ('po skupinach', "document.getElementById('segaModeToggle').click()", 200),
        ('konec upravy', "document.getElementById('segaLayoutDone').click()", 300),
        ('MOJE HRY', "document.getElementById('btnGames').click()", 600),
        ('smazat hru', "document.querySelector('#segaLibraryList .libDel').click()", 300),
        ('zrusit smazani', "document.getElementById('segaLibraryDeleteCancel').click()", 200),
        ('MOJE HRY prazdne', "NAP_MOCK['AHSega.segaLibraryList']='[]'; document.getElementById('segaLibraryRefresh').click()", 300),
        ('ULOZENE POZICE', "document.getElementById('segaLibraryClose').click(); document.getElementById('btnSaves').click()", 600),
        ('ULOZENE prazdne', "NAP_MOCK['AHSega.segaSavedPositionsList']='[]'; document.getElementById('segaSavedRefresh').click()", 300),
    ],
}

# ---------------- PS1 ----------------
STRANKY['ps1'] = {
    'cesta': 'emu_ps1/index.html',
    'mock': {
        'AHPS1.ps1LibraryList': '[{"key":"crash","nazev":"Crash Bandicoot","bytes":524288000,"ma_ulozenou_hru":true,"znacka":"cd"},{"key":"doom","nazev":"Doom","bytes":73400320,"ma_ulozenou_hru":false}]',
        'AHPS1.ps1MemCardInfo': 'PS1_MEMCARD path=/x core_da_sram=ANO core_sram_bytes=131072 soubor_existuje=ANO soubor_bytes=131072 posledni_ulozeni=OK',
        'AHPS1.ps1QuickSave': 'PS1_STATE_SAVE_OK',
        'AHPS1.ps1QuickLoad': 'PS1_STATE_LOAD_OK',
        'AHPS1.ps1UlozAOdesli': 'OK /storage/emulated/0/Download/AtariHelp/AtariHelp_PS1_LOG.txt',
        'AHPS1.ps1RemoteCachePath': '/data/user/0/eu.atarihelp.emu10/cache/ps1',
        'AHPS1.ps1ClearRemoteCache': 'PS1_CACHE_CLEARED',
        'AHPS1.ps1StartStav': '0|',
        'AHPS1.ps1Reset': 'OK',
        'AHPS1.ps1Log': 'PS1 log',
        'AHTVWEB.start': 'TV_WEB_CAST_OK http://192.168.1.50:8765/ APP',
        'AHSAVE.saveText': 'DOWNLOADS_OK',
    },
    'kroky': [
        ('HELP (kolecko na vysku)', "document.getElementById('btnSettings').click()", 500),
        ('D-PAD A OVLADANI', "document.getElementById('btnDpadMenuFromHelp').click()", 500),
        ('OVLADANI profi', "document.getElementById('btnCtrlSettings').click()", 500),
        ('vychozi ovladani', "document.getElementById('ctrlReset').click()", 200),
        ('CD ISO knihovna', "document.getElementById('ctrlClose').click(); document.getElementById('btnCd').click()", 600),
        ('smazat hru', "document.querySelector('#libraryList .libDel').click()", 300),
        ('knihovna prazdna', "document.getElementById('libraryDeleteCancel').click(); NAP_MOCK['AHPS1.ps1LibraryList']='[]'; document.getElementById('libraryRefresh').click()", 300),
        ('MEMORY CARD', "document.getElementById('libraryClose').click(); document.getElementById('btnMemory').click()", 500),
        ('karta: zapis selhal', "NAP_MOCK['AHPS1.ps1MemCardInfo']='PS1_MEMCARD core_da_sram=NE soubor_existuje=NE posledni_ulozeni=SELHALO'; document.getElementById('memCardRefresh').click()", 300),
        ('karta: jadro nebezi', "NAP_MOCK['AHPS1.ps1MemCardInfo']=''; document.getElementById('memCardRefresh').click()", 300),
        ('karta: nezkouseno', "NAP_MOCK['AHPS1.ps1MemCardInfo']='PS1_MEMCARD core_da_sram=ANO soubor_existuje=NE'; document.getElementById('memCardRefresh').click()", 300),
        ('uprava rozlozeni', "document.getElementById('memCardClose').click(); document.getElementById('btnSettings').click(); document.getElementById('btnDpadMenuFromHelp').click(); document.getElementById('btnEditLayout').click()", 400),
        ('po skupinach', "document.getElementById('layoutModeToggle').click()", 200),
        ('vychozi rozlozeni', "document.getElementById('layoutEditReset').click()", 200),
        ('hotovo', "document.getElementById('layoutEditDone').click()", 300),
        ('SAVE GAME', "document.getElementById('btnSave').click()", 300),
        ('rychla pozice', "document.getElementById('btnQuickLoad').click()", 300),
        ('cache cesta', "document.getElementById('psCachePath').click()", 300),
        ('cache smazat', "document.getElementById('psCacheClear').click()", 300),
        ('TV WEB CAST', "document.getElementById('tvWebCast').click()", 300),
        ('kopirovat log', "document.getElementById('logCopy').click()", 300),
        ('ulozit log', "document.getElementById('logSave').click()", 500),
        ('RESET (nahravani)', "document.getElementById('helpClose').click(); document.getElementById('btnReset').click()", 1200),
    ],
}

# ---------------- MP3 PREHRAVAC ----------------
STRANKY['mp3'] = {
    'cesta': 'player/index.html',
    'mock': {
        'AHPICK.listLocalAudio': '{"ok":true,"items":[{"name":"Night Drive.mp3","uri":"content://x/1","size":4200000},{"name":"Retro Wave.wav","uri":"content://x/2","size":9800000}]}',
        'AHTVWEB.status': '',
    },
    'kroky': [
        ('MY PLAYLISTS', "document.getElementById('btnMyPlaylists').click()", 500),
        ('obnovit seznam', "document.getElementById('panelScan').click()", 300),
        ('vymazat playlist', "document.getElementById('panelClear').click()", 200),
        ('zavrit', "document.getElementById('panelClose').click()", 200),
        ('SETTINGS', "document.getElementById('btnSettingsTop').click()", 300),
        ('reset zvuku', "document.getElementById('panelResetSound').click(); document.getElementById('panelCloseSettings').click()", 200),
        ('FAVORITES', "document.getElementById('navFavorites').click()", 300),
        ('PLAY bez skladby', "document.getElementById('favClose').click(); document.getElementById('btnPlay').click()", 300),
        ('PAUSE', "document.getElementById('btnPause').click()", 200),
        ('STOP', "document.getElementById('btnStop').click()", 200),
        ('REWIND', "document.getElementById('btnRew').click()", 200),
        ('RECORD bez skladeb', "document.getElementById('btnRec').click()", 400),
        ('bez pristupu k hudbe', "NAP_MOCK['AHPICK.listLocalAudio']='{\"needsPermission\":true}'; document.getElementById('panelClose').click(); document.getElementById('btnMyPlaylists').click(); document.getElementById('panelScan').click()", 400),
        ('LIBRARY prazdna', "NAP_MOCK['AHPICK.listLocalAudio']='{\"ok\":true,\"items\":[]}'; document.getElementById('panelScan').click()", 300),
        ('PLAYER', "document.getElementById('panelClose').click(); document.getElementById('navPlayer').click()", 200),
    ],
}

# ---------------- DZUBOX (DJ) ----------------
STRANKY['dj'] = {
    'cesta': 'dj/index.html',
    'okno': {'width': 915, 'height': 412},
    'mock': {},
    'kroky': [],
}
