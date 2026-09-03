# B221 — plocha se schová v panelech, delší hláška při spuštění (versionCode 267)

Rene poslal dva screenshoty ("VYBRAT HRU (CD/ISO)" panel s prolínajícím
se BIOS pozadím vzadu) a napsal: "mota se nam obraz do nabidek s vysky
obrazu nevim proc - musim aplikaci minimalizovat a pak to naskoci jak
ma... pozor at to nerozbijeme funguje to super ted." Plus: hláška při
spuštění hry z knihovny zmizí moc rychle, appka vypadá zaseknutá i
když jen normálně bootuje (~10s).

===============================================================================
 1) PROČ SE OBRAZ "ZAMOTÁVAL" DO PANELŮ
===============================================================================

  `ps1Plocha` (nativní `SurfaceView`, kreslí do ní přímo C++ jádro) je
  položená NAD WebView (`zOrderOnTop`, kvůli plynulému hraní). To
  znamená - je to úplně JINÁ vrstva než HTML, ne něco, co CSS nebo
  z-index v HTML panelu může ovlivnit. Když appka běžela s BIOSem/hrou
  a otevřel se nový HTML panel (knihovna, D-PAD nastavení...), nativní
  plocha zůstávala vykreslená NAD panelem, i když v HTML vypadal panel
  jako "navrchu".

  V logu (`ps1.txt`, cos poslal) to sedí přesně: "PLOCHA: odpojena"
  se objevuje jen při `activityPause` (minimalizace) - proto to
  minimalizací "spravilo" - full teardown nativní plochy je jediná
  věc, co ji dosud schovala.

===============================================================================
 OPRAVA - MALÁ, BEZPEČNÁ, NE PLNÝ RESTART
===============================================================================

  Nová metoda `AHPS1.ps1PlochaVisible(show)` v Javě - jen
  `ps1Plocha.setVisibility(VISIBLE/INVISIBLE)`. Přesně stejný, už
  existující a prověřený mechanismus, jaký appka používá pro přechody
  v intru (řádky 6537/6546 v MainActivity.java) - ŽÁDNÝ zásah do EGL
  kontextu, vlákna renderování, ani bootování/zastavování jádra.

  Každý panel teď při otevření schová plochu a při zavření ji vrátí:
  knihovna (CD/ISO), D-PAD A OVLÁDÁNÍ, OVLÁDÁNÍ (citlivost/vzhled),
  PAMĚŤOVÁ KARTA (stav), i editace rozložení tlačítek (ta se dřív
  chovala jako "zavřít D-PAD panel, pak začít editovat" - což by
  plochu na okamžik vrátilo zpět a hned zase schovalo; teď se schová
  a zůstane schovaná po celou dobu editace, žádné blikání).

===============================================================================
 2) HLÁŠKA PŘI SPUŠTĚNÍ HRY
===============================================================================

  "Spouštím hru…" se dřív schovala po 2,4 sekundy, ale skutečné
  nabootování trvá kolem 10 sekund - zbylých ~7-8 sekund appka
  vypadala, jako by nic nedělala. Teď: "Spouštím hru… může to trvat
  kolem 10 vteřin, nic se nekazí." - viditelné 11 sekund.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76 zkontrolováno).
  `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha { } v
  MainActivity.java - 2355/2355.

  Nová jsdom simulace (test_ps1_plocha_visibility.js):
    - otevření knihovny (CD/ISO) zavolá ps1PlochaVisible(false)
    - zavření knihovny zavolá ps1PlochaVisible(true)
    - totéž pro PAMĚŤOVOU KARTU (stav)
    - D-PAD panel → vstup do editace: plocha končí schovaná (poslední
      volání je false), i když cestou projde přechodným
      show→hide kvůli zavírání jiných panelů - výsledný stav správný
    - HOTOVO v editaci plochu vrátí
    - hláška při SPUSTIT v knihovně obsahuje zmínku o 10 vteřinách
