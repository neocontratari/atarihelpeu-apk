# Co je v B294

Stejné jádro jako B293 (Atari 130XE PAL po cyklech, kazeta, kontrola 130XE).
Nové je jen **„CO TESTOVAT“ na stránce LOG/CHYBA** – 12 kroků pro B293/B294.
U každého kroku klepneš V PORADKU / SPATNE, zapíše se to do logu, takže pak
stačí poslat log (ULOZIT LOG A ODESLAT).

Krok 11 porovnává skutečné 130XE s emulátorem – stejné příkazy v BASICu po
zapnutí. V emulátoru vyjde:

| příkaz | emulátor |
|---|---|
| `?FRE(0)` | 37902 |
| `POKE 20,0:FOR I=1 TO 2000:NEXT I:?PEEK(20)` | 184 |
| `POKE 559,0:POKE 20,0:FOR I=1 TO 2000:NEXT I:POKE 559,34:?PEEK(20)` | 136 |

Druhé a třetí číslo je čas smyčky v padesátinách vteřiny – se zapnutým
obrazem si ANTIC bere procesoru cykly (184), s vypnutým (`POKE 559,0`) ne
(136). Když skutečné 130XE ukáže stejná čísla, sedí časování procesoru
i ANTIC na skutečný hardware. (Emulátor atari800 v režimu 130XE PAL dává
185 / 136 a 37902.)
