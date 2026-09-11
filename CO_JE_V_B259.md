# B259 — PS1 Doom: diagnostika, ne hádaná oprava (versionCode 305)

Rene: "Podívej se ještě na to PS1 a oprav to - ta hra Doom a
rozsypaná grafika mě opravdu vyděsila."

===============================================================================
 PROČ TOHLE NENÍ JAKO ATARI OPRAVA
===============================================================================

U Atari jsem každou opravu mohl skutečně zkompilovat a spustit -
přeložený C++ kód, žádný telefon potřeba. U PS1 grafiky to nejde -
je to OpenGL/Android vykreslovací kód (vendor knihovna PCSX-ReARMed,
`gpu-gles` plugin), který potřebuje skutečné GPU a Android sestavení.
V tomhle prostředí nemám žádný způsob, jak si ověřit, že oprava
skutečně kreslí správné pixely.

Přesně tohle mě zarazilo u vlastní dřívější formulace "obcházíme
to" - a nechtěl jsem udělat to samé: hodit tam neověřenou domněnku a
tvářit se, že je hotová.

===============================================================================
 CO JSEM NAŠEL DŮKLADNÝM PROZKOUMÁNÍM
===============================================================================

V `gpulib_if.c` je proměnná `nap_disp_interlace` (sleduje bit 5
příkazu GP1(08h) - prokládaný režim). Tahle proměnná se **zapisuje a
loguje**, ale **nikde v celém souboru neovlivňuje samotné
vykreslování** - přímo v kódu je komentář, který to přiznává: "jen
diagnostika; kreslíme vždy progresivně v plné výšce."

To je silný kandidát na příčinu - pokud Doom prokládaný režim
skutečně zapíná a appka ho ignoruje, přesně tenhle nesoulad by mohl
způsobit tu "rozsypanou" grafiku.

**Ale** - nemůžu si tím být jistý bez skutečných dat z běžícího
Doomu. Mohl by to být tenhle mechanismus, nebo něco úplně jiného v
tom obřím, mnohokrát upravovaném kódu.

===============================================================================
 CO JSEM MÍSTO HÁDÁNÍ UDĚLAL
===============================================================================

Přidal jsem cílenou diagnostiku - stejný přístup, jaký spolehlivě
fungoval u Atari (nejdřív data, pak oprava s jistotou):

**1) Nový, vždy se spouštějící log při každé změně prokládaného
bitu.** Předtím se to logovalo jen tehdy, když se současně změnil i
jiný příznak (RGB24) - řídké, snadno přehlédnutelné. Teď se zaloguje
při každém přepnutí, včetně skutečné výšky/šířky obrazu, jak je
hlásí jádro emulátoru.

**2) Opravené počítadlo logů pro zápisy do video paměti.** Předchozí
verze se omezovala na "prvních 40 záznamů CELKEM" - a to se
vyčerpalo už během úvodních BIOS/menu obrazovek, dávno předtím, než
Doom vůbec zapnul prokládaný režim během hraní. Teď má samostatných
40 záznamů SPECIFICKY pro dobu, kdy je prokládaný režim aktivní.

**Žádná změna vykreslovací logiky** - čistě přidání diagnostiky,
nízké riziko i bez možnosti to tady zkompilovat.

===============================================================================
 DALŠÍ KROK
===============================================================================

Spusť Doom (přes GitHub Actions sestavení, jak se PS1 změny v tomhle
projektu vždycky ověřovaly) a pošli mi log. Z nových řádků uvidíme s
jistotou, jestli Doom prokládaný režim vůbec zapíná a jakou výšku
obrazu jádro v tu chvíli hlásí - a pak to půjde opravit s jistotou,
ne hádáním.
