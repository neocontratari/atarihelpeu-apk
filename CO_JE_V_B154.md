# B154 — POSTUP HRÁČE (versionCode 202)

## 1) Opakované stahování

V logu:

```
BUILD2SA29 SOUBORY sonic=CHYBI bios=CHYBI -> PTAM SE
BUILD2SA27 STAZENI sonic=OK(1) bios=OK(4)
```

Stáhlo se, a přesto při dalším startu zase `CHYBI`. Cesty přitom seděly —
ukládám do `emu/sega` a `PS1_BIOS` a tam i hledám.

Příčina je v tom, že **na novějším Androidu může výpis adresáře
v `Download/AtariHelp` vrátit prázdno**, i když soubory na disku jsou.

Kontrola už na výpisu nezávisí: po stažení se **zapamatují přesné cesty**
a příště se ověřuje přímo na nich. Výpis adresáře zůstává jako záloha.

## 2) Postup hráče

```
ATARI    od začátku otevřené
SEGA     otevře se, až hráč napíše ten kód v Atari
PS1      otevře se po 30 minutách hraní Segy
```

Zamčené tlačítko se nespustí a řekne, co je potřeba:

```
ZAMCENO

NAPIS KOD V ATARI
```

nebo `JESTE 24 MIN SEGY`.

**Kontrola kódu není znak po znaku** — to by bylo k vzteku. Hledají se
klíčové kusy: `USR(1536)`, `POKE 1536+I,A`, `GRAPHICS 0` a aspoň polovina
bajtů té rutiny. Ověřeno spuštěním:

```
správný kód          -> OTEVŘE
jen půlka DATA       -> nic
bez USR              -> nic
prázdno              -> nic
jiné odsazení        -> OTEVŘE
malými písmeny       -> OTEVŘE
```

**Čas Segy se počítá jen když hra opravdu běží** — ne na pozadí, ne
v jiném emulátoru a ne během intra.

## 3) Zadní vrátka pro tebe

V nabídce OPTIONS:

```
ODEMKNOUT DALSI    odemkne postupně jednu úroveň
ZAMKNOUT VSE       vrátí to na začátek, pohled nového hráče
```

Ověřeno spuštěním:

```
na začátku              SEGA: ZAMČENA (NAPIS KOD V ATARI)
                        PS1:  ZAMČENA (NEJDRIV SEGA)
po 1. ODEMKNOUT DALSI   SEGA: otevřená
                        PS1:  ZAMČENA (JESTE 30 MIN SEGY)
po 2. ODEMKNOUT DALSI   obojí otevřené
po ZAMKNOUT VSE         zase od začátku
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 46 tříd |
| JS rozcestníku i všech etap | 0 chyb |
| kontrola kódu | 7/7 případů správně |
| zámky a odemykání | 4/4 stavy správně |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci **podruhé** — už se nemá ptát na stahování.
**3)** SEGA a PS1 mají být **zamčené** a říct proč.
**4)** OPTIONS → **ODEMKNOUT DALSI** → Sega se otevře.
**5)** Znovu → PS1 se otevře.
**6)** **ZAMKNOUT VSE** → zase od začátku.
**7)** Zahraj Segu pár minut a v logu zkontroluj přičtený čas.

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA29 SOUBORY sonic=zapamatovan bios=zapamatovan -> vse je, neptam se
BUILD2SA40 SEGA_CAS +Ns -> sega=otevrena ps1=ZAMCENA odehrano=Nmin
BUILD2SA40 ODEMKNUTO: SEGA
```

---

## Co zbývá

Na TV pořád **není zvuk Atari v etapě 1 a 5** a u Segy s PS1 jde zvuk,
ale ne obraz. Na mobilu je všechno v pořádku. To je samostatná věc a mám
ji rozpracovanou — řekni, jestli ji vzít jako další krok.
