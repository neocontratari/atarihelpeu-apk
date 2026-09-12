# B269 — CSAVE ukládá skutečný WAV soubor pro testování na reálném Atari

Rozumím teď přesně, proč chceš čisté, přesné jádro nejdřív: chceš
generovat testovací kód, nechat appku ho uložit jako zvuk, a ten zvuk
přehrát do SKUTEČNÉHO Atari 130XE — ne věřit Altirře ani žádnému
jinému emulátoru.

===============================================================================
 CO PŘIBYLO
===============================================================================

Nové tlačítko/funkce **"Uložit CSAVE jako WAV"** (JS most
`atariCsaveDoWav(nazev)`):

1. Napíše CSAVE + oba RETURN — přesně stejným způsobem, jakým appka
   posílá klávesy normálně (pevné zpoždění 8 snímků na klávesu — to je
   důležité, viz poučení z B268: jiné časování by appku poslalo jinou
   větví kódu).
2. Poté zachytí **veškerý zvuk**, co appka po celou dobu operace
   vygeneruje (45 vteřin rezervy, i pro delší programy).
3. Uloží to jako standardní .wav (44100 Hz, 16bit, mono) do
   `Downloads/AtariHelp/Atari_emu/` — veřejné úložiště, které přežije
   i odinstalaci appky (stejně jako uložené hry).

**Důležité: žádný zvláštní "enkodér zvuku" jsem nepsal.** Appka už
teď správně nastavuje zvukové registry přesně jako skutečný hardware
(to jsme spolu ověřili v předchozím kole — počty pípnutí, časování
motoru, ERROR 138 — všechno sedělo). Stačilo tedy zachytit zvuk, co
appka STEJNĚ generuje, jen po celou dobu operace místo krátkých
kousíčků.

===============================================================================
 OVĚŘENÍ
===============================================================================

**Test se 100řádkovým programem** (každý řádek REM komentář, ~4700
bajtů celkem):
- Všech 100 řádků se napsalo bez zaseknutí
- Ověřeno příkazem LIST na řádcích 1, 50 a 100 — obsah přesně
  odpovídá napsanému
- CSAVE tohoto dlouhého programu doběhl bez chyby, motor běžel 28,2
  vteřiny (proti ~18 vteřinám pro krátký program — doba se správně
  prodlužuje s délkou)

**Analýza zachyceného zvuku po vteřinách:**
- Souvislý tón přesně od 0 do 18 vteřin (zápis dat)
- Pak čisté ticho po zbytek 45vteřinového okna — žádný šum ani
  artefakt po dokončení

===============================================================================
 POCTIVĚ: CO TOHLE NEŘEŠÍ
===============================================================================

Tohle je **jen směr CSAVE → WAV** (zápis). Načtení té samé nahrávky
zpátky (CLOAD ← WAV, ne jen scénář "žádná kazeta") **není součástí
téhle změny** — to vyžaduje samostatnou "virtuální kazetu": zachytit
bajty při zápisu a při čtení je pustit zpátky jako signál se správným
časováním, včetně vodicích tónů mezi 132bajtovými bloky.

Potvrdil jsi, že přesně tohle byla ta jediná známá chyba i v JS
referenci u delších/vícenásobných nahrávek — takže tohle zůstává pro
příští kolo.
