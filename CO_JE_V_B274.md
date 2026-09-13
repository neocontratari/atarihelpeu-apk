# B274 — RESET teď zachovává program, přidáno tlačítko POWER

## Skutečná příčina, přesně ověřená

V referenci je `M.reset()` jen `cpu.reset()` — nic víc. Moje C++
funkce navíc dělala `mem.pia = Pia();` — resetovala celou PIA, včetně
PORTB, který na 130XE řídí **bankování paměti**.

Na skutečném hardwaru signál RESET vůbec neresetuje PIA — PORTB je
jen softwarová západka, přežije reset stejně jako zbytek paměti. Když
se PORTB při resetu změnilo, appka se najednou dívala na **jinou
banku paměti** než tu, kde byl napsaný program. Program pak vypadal
"ztracený", i když fyzicky nikde nezmizel.

Mimochodem — po cestě jsem taky ověřil, že ROM má vlastní mechanismus
rozlišení teplý/studený start (kontrola konkrétních bajtů v paměti) a
appka ho po bootu správně nastavuje. To NEBYL problém — jen ta PIA.

## Oprava

Řádek `mem.pia = Pia();` odstraněn z `reset()`. RESET teď dělá
výhradně to, co dělá reference — reset procesoru, nic víc.

**Přímo ověřeno na skutečné opravené funkci:** napsal jsem
"10 PRINT HI", zavolal reset, počkal, napsal LIST — program se
správně zobrazil. Předtím LIST po resetu ukázal prázdno.

## Nové tlačítko POWER

Vedle RESET přibylo tlačítko POWER — dělá přesně to, co dřív dělalo
hlavní bootovací tlačítko (úplná obnova stroje od nuly). RESET teď
zachovává paměť/program přesně jako na skutečném 130XE.

## Ověření

Všech 9 předchozích testů prochází beze změny (změna je izolovaná
jen na `reset()`, žádný jiný mechanismus není dotčený) + nový
specifický test přímo proti opravené produkční funkci potvrzuje
zachování programu.

## Zůstává otevřené

Chybějící závěrečný "chrčící" zvuk CSAVE — v tomhle kole nezkoumáno,
zůstává pro příště.
