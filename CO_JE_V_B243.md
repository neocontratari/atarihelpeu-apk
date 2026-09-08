# B243 — nalezena SKUTEČNÁ příčina prolínání obrazu (versionCode 289)

Rene: "Ne nic neopraveno - ani začátek - neustále stahuje dva soubory
při spuštění appky i když jsou v mobilu. Sega žádná změna. Jediné
načítání z netu je rychlé."

Potvrzeno: SBÍRKA je teď rychlá (B242 fungovalo). Zbylé dva problémy
byly hlubší, než jsem myslel.

===============================================================================
 KONEČNĚ SKUTEČNÁ PŘÍČINA PROLÍNÁNÍ OBRAZU
===============================================================================

  Nový log odhalil to, co jsem hledal od B235. Appka má pro Segu DVĚ
  SOUBĚŽNÉ vykreslovací cesty:

      segaPlocha (SurfaceView) - "SEGA_OBRAZ_PRIMO_ZAPNUT" - TUHLE
        jsem řešil celou dobu (B235-B242)

      nativeInPlaceView (TextureView) - "ENABLE_IN_PLACE" /
        "NATIVE_TEXTURE_FRAME_RV" - volaná PŘÍMO Z JS při KAŽDÉM
        výběru hry (`startNativeCppInPlace` → `AHNATIVE.enableInPlace()`)

  Log ukázal OBĚ cesty aktivní SOUČASNĚ, pro TUTÉŽ hru. Přesně tvoje
  původní hypotéza - "máš asi veškerá videa v jednom okně, akorát že
  pouštíš jen něco, ale vzadu jedou všechny videa" - byla od začátku
  správná.

  `nativeInPlaceView` je ta SKUTEČNÁ, primární plocha (komentáře v
  kódu: "normalSegaUI=YES noSeparateWindow=YES" - to je oficiální,
  zamýšlená cesta). `segaPlocha` je vedlejší/starší cesta, která se
  taky pořád spouští, ale není to ta, na které vidíš hru hrát.

  Celou dobu jsem správně schovával JEDNU plochu, zatímco hra
  prosvítala přes DRUHOU, kterou appka vůbec neřešila.

===============================================================================
 OPRAVA
===============================================================================

  Schovávání `nativeInPlaceView` přidáno na OBOU místech, kde už byla
  `segaPlocha`: přímé volání z JS (`segaPlochaVisible`) i 300ms
  hlídač. `TextureView` je normální součást hierarchie (ne zvláštní
  hardwarová vrstva jako `SurfaceView`) - `setVisibility()` by tu měl
  fungovat spolehlivě bez rizika, že by ničil a znovu stavěl povrch.

===============================================================================
 "FURT SE TO STAHUJE PO REINSTALACI, I KDYŽ JSOU V MOBILU"
===============================================================================

  B242 opravila ROZHODOVACÍ logiku (spoléhat na soubory, ne na
  vzpomínku appky) - ale kontrola SAMOTNÁ ("jsou soubory tam?") pořád
  závisela na dodatečném prohledání adresáře (`listFiles()`), které
  appka SAMA v komentářích uznává jako nespolehlivé na novějším
  Androidu (Scoped Storage).

  Teď appka zapamatuje PŘESNOU CESTU K SOUBORU hned v okamžiku, kdy
  ho úspěšně zapíše (kdy JISTĚ VÍ, že existuje - protože ho právě
  vytvořila) - ne až později, dodatečným hádáním přes `listFiles()`.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check (obě emu stránky - beze změny, JS logika se nedotkla),
  `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha `{ }` v
  MainActivity.java - 2455/2455, v NapStahovaniSeSouhlasem.java -
  86/86.

  Všechny čtyři existující Sega jsdom simulace spuštěny znovu - beze
  změny prošly (čistě Java-strana změna).

  CO NEJDE OVĚŘIT ODSUD: jestli schování `nativeInPlaceView` doopravdy
  vyřeší prolínání na reálném zařízení - to je na tvém testu. Dávám
  tomu vysokou důvěru, protože log teď PŘÍMO POJMENOVAL tu druhou,
  dosud neřešenou plochu - není to další teorie, je to přímý nález.
