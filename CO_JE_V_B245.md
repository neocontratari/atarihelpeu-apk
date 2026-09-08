# B245 — Atari C++ Fáze 0: nástroje pro testování (versionCode 291)

Rene souhlasil s navrženým postupem (viz analýza v předchozí zprávě) a
požádal: "nachystej si log přímo v C++ Atari, ať vidím přesně to, co
ty. Žádné odhady, žádný fake."

Tohle je Fáze 0 - rychlé zprůchodnění testování, než se pustíme do
skutečných emulačních vrstev (zvuk, VBXE).

===============================================================================
 CO PŘIBYLO
===============================================================================

**OPTION tlačítko** - most (`atariKonzole('option')`) už existoval od
dřívějška, jen mu chybělo tlačítko na testovací stránce.

**HELP tlačítko** - úplně nové. Na 130XE je HELP klávesa klávesnicové
matice, ne konzolový přepínač jako SELECT/START/OPTION. Hodnota
`KBCODE=17` ověřena přímo z JS reference (`emu_vbxe/index.html`) - na
DVOU nezávislých místech (mapa fyzické klávesnice `'F1':17 /*HELP*/`
i skutečné tlačítko HELP, které volá `M.keyDown(17)`) - stejné číslo
na obou místech, takže spolehlivé. **Žádné změny v C++ jádru nebyly
potřeba** - `keyNative()` už přijímá libovolný kód, HELP je jen další
hodnota.

**Živé vykreslování** - nové tlačítko "▶ SPUSTIT ŽIVĚ", které místo
jen statických snímků po kliknutí běží průběžně (3 snímky každých
~90ms), přesně jako skutečné Atari před tebou. Jde kdykoli zastavit a
udělat přesný ruční krok. Záměrně neběží 60×/s - každé volání je JNI
přechod + base64 kódování obrazu, takže tahle rychlost je pro
diagnostiku dost plynulá a méně zatěžuje telefon.

**Podrobné logování každé akce** - dřív se logoval jen uříznutý JSON
(`atariDoSelfTestu` logovala jen prvních 200 znaků, což by beztak
uřízlo obrazová data, ale i registry se ztrácely). Teď každá akce
(BOOT, NAPSANO_BYE, KONZOLE:select/start/option, KLAVESA:HELP)
zapisuje čitelný řádek s PLNÝM stavem registrů (PC, DMACTL, DLIST,
PORTB, snímků, a zvýrazněné VAROVÁNÍ, pokud jádro zaseklo) - přímo
z odpovědi jádra, žádné hádání.

Živá smyčka NEloguje každý snímek (to by log zahltilo) - loguje jen
když jádro zaseklo, což je vždy důležité vědět.

**Volná poznámka** - nové textové pole a tlačítko ZAPSAT. René si
může přímo do stejného logu napsat, co zrovna testuje ("OPTION ->
SOUND TEST" apod.) - log je pak čitelný jako příběh, ne jen suchý
výpis čísel, přesně jak žádal ("ať vidím přesně to, co ty").

===============================================================================
 CO SE NEMĚNILO
===============================================================================

Žádný C++ soubor. Celá Fáze 0 šla na Java/JS straně - nativní most už
podporoval vše potřebné (libovolný KBCODE, libovolnou masku CONSOL).

===============================================================================
 OVĚŘENÍ
===============================================================================

node --check (nová Atari C++ stránka i PS1/Sega/starý VBXE - pro
jistotu, i když nedotčené) - 0 chyb. Žádná duplicitní id (35).
`stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha `{ }` v
MainActivity.java - 2473/2473, v NativeAtariCoreBridge.java - 37/37.

Existující Sega jsdom simulace spuštěna znovu (sdílený
MainActivity.java) - beze změny prošla.

CO NEJDE OVĚŘIT ODSUD: skutečné chování na telefonu - to je přesně to,
co má nový log umožnit otestovat TOBĚ, s podrobnostmi, které mi pak
pošleš. Až budeš mít self-test v C++ konečně kompletně průchozí
(OPTION i HELP fungují, obraz běží živě), měl bys být schopný projít
self-test stejně, jako na skutečném Atari vedle tebe, a přesně
zaznamenat, kde se to (pokud vůbec) liší.
