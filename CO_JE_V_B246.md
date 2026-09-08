# B246 — SBÍRKA a HRY měly prohozené ikony (versionCode 292)

Při psaní návodu pro začátečníky (PDF pro PS1 a Sega) vyšlo najevo, že
tlačítko **SBÍRKA** mělo ikonu **knihovny/poličky s hrami** (vypadá jako
"tohle už mám"), zatímco **HRY** mělo ikonu **jedné vložené kazety**
(vypadá jako "vlož jednu konkrétní hru") — přesně obráceně, než
odpovídá tomu, co tlačítka doopravdy dělají:

- SBÍRKA → stáhni si **novou** hru z webu (jedna konkrétní volba)
- HRY → **moje** stažená knihovna (celá sbírka toho, co už mám)

Rene: "to je moje slepota, kterou sem nevidel."

===============================================================================
 OPRAVA
===============================================================================

Ikony jsou napečené přímo v obrázku pozadí (`sega_megadrive_screen.png`),
ne jako samostatná vrstva v kódu — oprava tedy musela jít přes úpravu
obrázku, ne CSS/JS.

Postup: v datech pixelů jsem našel přesnou tmavou mezeru mezi ikonou a
popiskem ("SBÍRKA"/"HRY") pod ní, vyřízl jen HORNÍ část každého
tlačítka (samotnou ikonu, bez textu), a tyhle dvě ikony mezi sebou
prohodil. Popisky, pozice, velikosti i funkce tlačítek zůstávají úplně
beze změny — mění se jen to, která ikona je nad kterým nápisem.

===============================================================================
 OVĚŘENÍ
===============================================================================

Čistě obrázková změna — žádný CSS/JS/Java soubor se nedotkl, takže
existující testy nemá smysl spouštět znovu (nic, co testují, se
nezměnilo). Vizuálně ověřeno zvětšeným výřezem před a po - švy mezi
vyříznutou a vloženou částí nejsou vidět.

Použito i v novém PDF návodu pro Segu (screenshot na titulní straně
teď ukazuje opravenou appku).
