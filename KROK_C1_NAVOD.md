# KROK C1 — plynulý PS1 obraz přes OpenGL (velké finále, část 1)

Parťáku, tohle je začátek finále! Konečně zapínáme motor.

## Co tenhle balíček dělá

Přidává do tvé aplikace nový, plynulý PS1 obraz přes OpenGL (s vsync,
jako náš renderer) — bere skutečný obraz z tvého PS1 jádra. Spouští se
novým tlačítkem, běží VEDLE staré cesty (tu jsem se nedotkl, ať nic
nerozbijem). Když tenhle obraz ověříme, v kroku C2 jím nahradíme to
staré blikání.

## DŮLEŽITÉ — jak to vyzkoušet (přesný postup)

1. Rozbal ZIP → složka `emu10_B`. Zkopíruj CELÝ obsah přes svůj
   repozitář → **Nahradit vše**. Commit „krok C1" → Push.
2. Počkej na zelený build → stáhni `app-debug` → nainstaluj.
3. Otevři aplikaci „AtariHelp TEST" (verze EMU10-C1).
4. **NEJDŘÍV spusť PS1 hru jako obvykle** (přes tvůj web/menu — Crash,
   Star Wars, cokoliv). Nech ji chvíli běžet, ať jádro má obraz.
5. Vpravo nahoře uvidíš nové modré tlačítko **„PS1 GL"**. Klikni na něj.
6. Otevře se celá obrazovka s obrazem hry — kreslený naším plynulým
   způsobem. Zpět se vrátíš tlačítkem Zpět na telefonu.

## Co sleduju (a na co tě připravím)

Tohle je poprvé, co náš renderer kreslí obraz z tvého skutečného jádra
uvnitř tvé aplikace. Je docela možné, že to napoprvé nebude dokonalé:
barvy můžou být divné (moc modré/červené), obraz posunutý nebo vzhůru
nohama. TO NEVADÍ — přesně proto to zkoušíme takhle bokem. Podle toho,
co uvidíš, to doladím. (Barvy PS1 jsou ošidné, tak s tím počítám.)

## Co mi pošli

1) Byl build zelený? (Kdyby ne, konec výpisu jako minule.)
2) Když dáš „PS1 GL" (po spuštění hry) — CO PŘESNĚ uvidíš?
   - Je tam obraz hry? Hýbe se plynule?
   - Jak vypadají barvy? (správně / moc modré / moc červené / divné)
   - Je obraz správně otočený, nebo vzhůru nohama / zrcadlený?
   - Sedí velikost, nebo je moc malý / oříznutý?
3) Pošli i log z http://IP-telefonu:8765/log — hledám řádky „C1".

Popiš mi to klidně vlastními slovy, jak to vidíš. Z toho přesně poznám,
co doladit. Tohle je to hlavní, na co jsi čekal — a jsme u toho!
