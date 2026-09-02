# B213 — oprava špatného pochopení z B212 (versionCode 259)

Rene: "Ten ovladač co tam byl tam samozřejmě musí zůstat - ten ovladač
byl na výšku funkční - ale tlačítka hitboxy byly skryté a neviditelné.
Já se bavil o tom, že jsi tam přidal další hitboxy, které překreslily
originál obrázek."

B212 byl založený na špatném pochopení: myslel jsem, že chce fotku
ovladače pryč. Ve skutečnosti fotka měla zůstat a být funkční jako
vždycky - problém byl, že moje `.padBtn` třída (přidaná v B204 pro D-pad/
akční tlačítka) dělala jejich hitboxy VIDITELNÝMI (poloprůhledný kruh +
zlatavý glyf trojúhelník/kolečko/čtverec/křížek) i v portrait, kde už
pod nimi ležela hotová fotka se svými vlastními, jinak umístěnými
ikonkami. Výsledek - dvojité, posunuté obrázky přes sebe - přesně to,
co ukazoval jeho screenshot.

===============================================================================
 CO SE VRÁTILO (zrušení B212)
===============================================================================

  `ps1_final_screen.png` - vrácen původní soubor s fotkou ovladače
  (941×1672 px). Zálohovaná oříznutá verze z B212 zůstává jen pro
  záznam, do appky nejde.

  `.stage{aspect-ratio:941/1672}` (bylo dočasně 941/1247).

  Pozice btnBack/btnSettings/psMonitor/btnReset/Memory/Cd/Load/Save -
  vráceny na původní hodnoty (top:3.1%/14.2%/82.7% atd.).

  D-pad/Triangle/Circle/Cross/Square/Select/Start - odebrány z portrait
  `display:none`. V portrait `display:none` zůstávají jen L1/L2/R1/R2
  (ty na fotce ovladače zepředu vidět nejsou - je to boční/horní hrana
  skutečného padu, takže tam žádná odpovídající kresba není).

===============================================================================
 SKUTEČNÁ OPRAVA (to, co mělo být opraveno od začátku)
===============================================================================

  Portrait `.padBtn` (BEZE media query, základní pravidlo):

      PŘED (B204-B212, chyba):
      color:rgba(255,236,176,.82); font:900 18px...; background:rgba(255,255,255,.055);
      border:1.5px solid rgba(255,255,255,.24); box-shadow:...; text-shadow:...

      PO (B213):
      color:transparent;font-size:0;background:transparent;border:0;
      box-shadow:none;text-shadow:none;

  Hitboxy (dotykové zóny) fungují úplně stejně jako předtím - dotyk na
  ně pošle vstup do hry - jen se přes fotku nekreslí nic navíc.
  `.padBtn:active` (zvýraznění při stisku) v portrait taky vynulováno -
  o krátký modrý záblesk při klepnutí se stará obecné `.hit:active`,
  stejně jako u všech ostatních neviditelných tlačítek appky.

  Landscape `.padBtn` (zlatý PS1-BIOS styl z B212) beze změny - tam
  žádná fotka ovladače není, viditelný styl tam dává smysl a zůstává.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (62 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.

  Nová jsdom simulace (test_ps1_portrait_invisible.js):
    - #dpad v portrait NENÍ display:none (na rozdíl od B212)
    - dotyk na D-pad v portrait skutečně pošle vstup (AHPS1.ps1Input
      zavoláno) - hitbox je funkční
    - computed style #btnTriangle v portrait: color transparent,
      font-size 0px, background transparent, border 0px - nic viditelného
    - textově ověřeno, že landscape `.padBtn` pravidlo se zlatým stylem
      je v souboru beze změny přítomné
