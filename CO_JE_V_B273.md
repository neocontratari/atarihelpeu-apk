# B273 — CSAVE automaticky ukládá WAV, žádné tlačítko navíc

Radost mám taky! Díky za tak přesný popis — 0,1-0,3s zpoždění
opravdu odpovídá tomu, jak rychle reaguje skutečné železo, tam už
asi víc nejde vytěžit.

## Hlavní věc tohohle kola

Samotné napsání **"CSAVE"** teď automaticky zahájí nahrávání zvuku —
appka už beztak generuje zvuk každý snímek v hlavní smyčce, takže
stačí, aby se, dokud nahrávání běží, každý takový kousek navíc uložil
do pole místo jen přehrání. Po 50 vteřinách bezpečné rezervy (změřeno
~28s pro 100řádkový testovací program) se všechno složí a uloží jako
.wav do `Downloads/AtariHelp/Atari_emu/` — stejná cesta jako předtím,
jen bez nutnosti mačkat zvláštní tlačítko.

Samostatné tlačítko "CSAVE → ULOŽIT WAV" jsem **odstranil** — přesně
jak jsi žádal, není potřeba.

## Dvě zbývající věci — poctivě, ne s odhadem

### RESET vs POWER

Přesně ověřeno testem, ne odhadem: program napsaný do BASICu **po
resetu z paměti mizí**, a appka po resetu dojde do stejného
finálního stavu za stejný počet snímků jako po plném bootu — RESET a
BOOT jsou teď v podstatě nerozeznatelné, přesně jak popisuješ.

Skutečná oprava vyžaduje najít, jak přesně operační systém v ROM
rozlišuje "teplý" a "studený" start (typicky kontrola konkrétní
paměti). To je hlubší otázka na úrovni ROM, ne rychlá záplata —
nechci ti poslat nedomyšlenou domněnku. Zůstává pro příští kolo.

### Chybějící závěrečný zvuk CSAVE

Tohle jsem v tomhle kole vůbec nezkoumal — upřednostnil jsem hlavní
požadavek (automatické ukládání WAV). Zůstává pro příště.

## Ověření

Všech 14 předchozích testů prochází beze změny — tahle úprava je
výhradně na straně JS/Java, samotné jádro (C++) zůstalo nedotčené.
