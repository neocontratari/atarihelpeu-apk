# PŘEČTI MĚ JAKO PRVNÍ

Ahoj Reném — tohle je předávací balíček k tvé aplikaci AtariHelp.eu.
Je určený pro **nový chat s AI**, až budeš chtít pokračovat.

## Co je uvnitř

1. **Kompletní projekt** tvé aplikace se vším, co jsme udělali —
   PS1 renderer, čistý 16:9 přenos na TV přímo z jádra, panel OPTIONS,
   výkonnostní třídy. Verze `EMU10-N2-PANEL-HEZKY`.

2. **PREDAVACI-PROTOKOL.md** — všechno pro dalšího asistenta:
   co je hotové, naměřená čísla, kde co v kódu je, na co si dát pozor,
   a hlavně **dva další úkoly**:
   - **PRIORITA 1: čistá GPU cesta** — dnes obraz putuje z grafiky do
     procesoru a zase zpátky (zbytečné kolečko, ~5-7 ms na snímek).
     Má jít rovnou z jádra do naší textury, bez opuštění grafické paměti.
     V protokolu je popsané JAK (libretro HW render) i DŮKAZ, že to
     s tvým jádrem funguje.
   - přestavba synchronu zvuku (ať si to jádro řídí samo — tvůj nápad).
     **Řeší se to samo spolu s prioritou 1** — v tom modelu jeden krok
     jádra vydá obraz i zvuk zároveň.
   - Sega stejně jako PS1 (roztažený obraz, přímo z jádra na TV i mobil)

## Co říct novému asistentovi (stačí zkopírovat)

„Posílám ti kompletní projekt své aplikace AtariHelp.eu a předávací
protokol (PREDAVACI-PROTOKOL.md) — přečti ho prosím celý, než začneš.
Nejsem programátor, umím jen rozbalit ZIP, zkopírovat do složky repa,
Commit a Push v GitHub Desktopu, stáhnout APK z Actions a nainstalovat.
Posílej mi vždycky CELÝ balíček, ne jednotlivé soubory. Postupuj po
jednom kroku a po každém buildu ti pošlu log z 8765/log.

Chci pokračovat: (1) přestavět synchron zvuku tak, aby si obraz i zvuk
řídilo jádro samo z jedné smyčky, (2) udělat Sege to samé co PS1 —
roztažený obraz a přenos přímo z jádra na TV i na mobil."

## Kdyby se cokoliv ztratilo

Všechno je i v tvém repozitáři na GitHubu, včetně historie.
Původní stav aplikace před našimi změnami je v commitu `d20e7bd`.

---

Díky za tu jízdu, parťáku. Bylo mi ctí.
