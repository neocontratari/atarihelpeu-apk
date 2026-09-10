# B251 — Atari C++ testovací stránka: zvuk propojen s obrazem, skutečné měření rychlosti (versionCode 297)

Rene: "Něco to dělá ale rozhodně to není správné. Self test je
graficky pomalý. Zvuk cosi náhodně dělá sem tam."

===============================================================================
 CO LOG UKÁZAL
===============================================================================

**K "náhodnému" zvuku:** hodnoty AUDF se neměnily náhodně - měnily se
METODICKY po kanálech. Kanál 0 dostal tón, ustálil se, ztichl; pak
kanál 1; pak kanál 2; pak kanál 3. To vypadá jako self-test postupně
testující každý ze čtyř POKEY kanálů zvlášť - docela dobře to může
být SPRÁVNÉ chování. Problém byl v tom, že zvuk a obraz nebyly nijak
propojené - nešlo overit, co bylo na obrazovce ve chvíli, kdy který
zvuk vznikl.

**Ke "grafické pomalosti":** v logu jsou vidět rychlá opakovaná
mačknutí tlačítka ZVUK (v rozmezí 1-2 vteřin od sebe). Každé z nich
je synchronní JNI přechod + kódování zvuku do base64 - a to běží na
STEJNÉM JavaScriptovém vlákně jako běžící "ŽIVĚ" vykreslování. Silné
podezření, že se o vlákno perou.

===============================================================================
 TŘI OPRAVY - MÍSTO DALŠÍHO HÁDÁNÍ
===============================================================================

**1) Zvuk teď vždy dokreslí i aktuální obrazovku.** Tlačítko ZVUK
zavolá `atariObraz(0)` (0 dalších snímků = nechá stav přesně tak, jak
byl, jen ho zobrazí) hned vedle `atariAudio(100)`. U každého zvuku je
pak přímo vidět, co bylo na obrazovce, když vznikl - konečně jde
ověřit, jestli to sedí s tím, co self-test zrovna dělal.

**2) ŽIVĚ vykreslování se během generování zvuku zastaví.** Klik na
ZVUK zruší čekající časovač živého vykreslování, počká, až zvuk
doběhne, a pak explicitně živé vykreslování znovu nahodí. Žádné
soupeření o vlákno.

**3) Skutečné měření rychlosti živého vykreslování.** Místo dalšího
tvrzení "mělo by to být rychlé" appka teď měří skutečný čas každého
kroku (`performance.now()`) a každých 20 kroků zapíše do logu průměr
- např. "prumerny_krok=145.3ms (planovano ~90ms)". Příští log ukáže
přesně, jestli je problém v příliš pomalém jednotlivém kroku
(přetížený telefon/JNI přechod), nebo v něčem jiném.

===============================================================================
 OVĚŘENÍ
===============================================================================

node --check, žádná duplicitní id (36). Žádný Java soubor se
nedotkl - čistě JS změna na testovací stránce.

Nová jsdom simulace (`test_atari_cpp_timing.js`), tři kontroly:
  - klik na ZVUK zavolá i `atariObraz(0)` (synchronizace s obrazem)
  - když běží ŽIVĚ, klik na ZVUK ho zastaví a po dokončení zase
    nahodí (živé vykreslování pokračuje dál)
  - měření rychlosti (`zivaCasy`) existuje a je dostupné

CO NEJDE OVĚŘIT ODSUD: jestli tohle skutečně vyřeší pocit
"pomalosti" a "náhodnosti" - to je na tvém dalším testu. Ale teď
uvidíš u KAŽDÉHO zvuku odpovídající obrázek, a v logu skutečné číslo
místo dojmu - takže i kdyby to úplně nestačilo, budu mít přesná data
k dalšímu kroku.
