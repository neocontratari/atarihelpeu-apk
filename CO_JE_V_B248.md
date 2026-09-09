# B248 — SAVE/LOAD GAME konečně vidět i uprostřed hraní (versionCode 294)

Rene: "u D-padu jak máme LOAD, SAVE a START — posun trošičku výš a
tlačítkům LOAD a SAVE dej barevné okraje, nejsou vidět. Není to
chyba, ale ve hře tlačítka nejsou vidět."

===============================================================================
 CO SE ZMĚNILO
===============================================================================

Celý řádek (START uprostřed, SAVE vlevo, LOAD GAME vpravo) posunut výš
- ze 1.8vh/2.3vh na 3.2vh/3.7vh, stejný vzájemný odstup mezi nimi
zůstal zachovaný.

SAVE a LOAD GAME dřív měly jen tenký (1.5px), téměř průhledný
(32% krytí) modrý okraj — na pestrém pozadí hry snadno zaniknul.
Teď mají:

- silnější okraj (2.5px)
- výrazně vyšší krytí (85% místo 32%)
- KAŽDÉ tlačítko jinou barvu — SAVE zelenou, LOAD GAME zlatou (START
  zůstává modré) — na první pohled rozeznatelné od sebe i od START
- vyšší celkové krytí tlačítka (55% → 85%)
- text i jeho záře sladěné s barvou tlačítka, čitelnější

===============================================================================
 OVĚŘENÍ
===============================================================================

node --check, žádná duplicitní id (74). Obě existující Sega jsdom
simulace (layout, knihovna) spuštěny znovu - beze změny prošly (čistě
CSS změna, žádná JS logika se nedotkla).

CO NEJDE OVĚŘIT ODSUD: jak přesně to vypadá na reálné obrazovce - to
je na tvém testu, ale barvy a krytí jsou teď výrazně nad úrovní, kde
by měly zaniknout v pozadí hry.
