# B207 — profi ovladač: citlivost, vzhled, haptika (versionCode 253)

Reného zadání, ostře: "ja chci po tobe profi ovladac s vlastnim
uzivatelskym rozhranim vlastnim nastavenim a vlastnim citlivosti."
Přesouvání tlačítek (B204-B206) samo o sobě neřešilo, JAK se ovladač
chová - jen KDE je. Tohle řeší to první.

===============================================================================
 CO PŘIBYLO
===============================================================================

  Nový samostatný panel "OVLÁDÁNÍ — profi nastavení", dostupný ze
  SETTINGS (tlačítko "OVLÁDÁNÍ: CITLIVOST A VZHLED", vedle "UPRAVIT
  ROZLOŽENÍ TLAČÍTEK" - stejná viditelnost: jen landscape, kde se
  skutečně hraje).

  1) CITLIVOST D-PADU (posuvník 20-70 %)
     Přímo mění mrtvou zónu ve funkci bindDpad() - kolik procent od
     středu musí prst ujet, než se směr sepne. Nízká hodnota = ovladač
     reaguje už u malého vychýlení; vysoká = musíš jet víc do kraje,
     méně náhodných sepnutí. Diagonály škálují úměrně se stejným
     poměrem jako předtím (deadY = deadX×0,762, diagX = deadX×1,333).

  2) PRŮHLEDNOST TLAČÍTEK (25-100 %) a VELIKOST TLAČÍTEK (70-150 %)
     Přes CSS proměnné --napBtnOpacity/--napBtnScale, aplikované na
     třídu .ctrlAdjustable (D-pad, čtyři akční tlačítka, L1/L2/R1/R2,
     Select, Start - tedy skutečný "ovladač", ne menu tlačítka jako
     Reset/Memory/Cd). Mění se živě při tažení posuvníku.

  3) HAPTICKÁ ODEZVA (zapnuto/vypnuto, výchozí zapnuto)
     Krátký (14ms) vibrační tik při KAŽDÉM novém stisku (ne při držení,
     ne při puštění - jen na přechodu nahoru→dolů, stejné místo, kde už
     appka dřív odesílala AHPS1.ps1Input). Nová metoda AHPS1.ps1Vibrate()
     v MainActivity.java, nová VIBRATE permission v AndroidManifest.xml.

  Nastavení se ukládá zvlášť (klíč napPs1CtrlSettings) od pozic tlačítek
  - jsou to dvě různé věci ("jak to vypadá/cítí" vs "kde to je") a dají
  se měnit nezávisle.

===============================================================================
 CO JE NOVÉ V JAVĚ (mimo emu_ps1/index.html)
===============================================================================

  AndroidManifest.xml: přidána `<uses-permission
  android:name="android.permission.VIBRATE" />`.

  MainActivity.java, třída AHPS1:
      ps1Vibrate(int ms) - vibruje `ms` (omezeno na 1-60, ať JS strana
      nemůže poslat nesmysl), pomocí VibratorManager na Androidu 31+ a
      starého Vibrator API níž. Když zařízení vibrátor nemá nebo něco
      selže, tiše vrátí "NO_VIBRATOR"/"ERR ..." - nikdy nespadne appka.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (55 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.
  Rovnováha { } v MainActivity.java - 2298/2298 (kontrolováno po zásahu).
  Plný `javac` proti Android SDK jsem znovu nemohl spustit (chybí mi
  Android SDK v tomhle prostředí) - syntaxi nové metody jsem ověřil
  ručně (závorky v přidaném bloku 10/10, 22/22).

  Nová jsdom simulace (test_ps1_ctrl_settings.js) ověřila:
    - výchozí hodnoty odpovídají PŮVODNÍMU pevně zadrátovanému chování
      (citlivost 42 %, opacity/size 100 %) - dokud uživatel nesáhne na
      posuvníky, appka se chová přesně jako v B206
    - stisk D-padu vyvolá jak ps1Input, tak ps1Vibrate(14)
    - vypnutí haptiky zastaví vibrace, ale vstup do hry projde dál
    - ZVÝŠENÍ citlivostního posuvníku na maximum genuinně mění chování:
      stejný dotykový bod, co při výchozí citlivosti (42 %) vyvolal
      RIGHT, při 70 % nevyvolal nic - není to jen kosmetický posuvník
    - průhlednost/velikost se promítnou do CSS proměnných živě
    - uložení do localStorage obsahuje přesně nastavené hodnoty
    - VÝCHOZÍ NASTAVENÍ OVLÁDÁNÍ vrátí vše na původní hodnoty
    - nové načtení stránky natáhne uložené nastavení (ověřeno na
      citlivosti 60 % přes reload)
    - žádná JS chyba v celém průběhu
