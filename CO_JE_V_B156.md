# B156 — AKTUALIZACE PŘÍMO ZE SÍTĚ (versionCode 204)

## Jak to funguje

```
síť  ->  ZipInputStream  ->  PackageInstaller  ->  systémové okno
```

APK **protéká rovnou do instalace**. Na telefonu nezůstane žádný soubor —
ani ten ZIP, ani APK. Rozbaluje se za letu v paměti, takže ten ZIP,
co už na webu máš, stačí a nemusíš nahrávat čisté APK.

M�š to na dvou místech:

**Automaticky při startu** — aplikace si šest vteřin po spuštění stáhne
těch 69 bajtů z `emu10_verze.txt`, porovná číslo se svojí verzí, a **jen
když je tam vyšší**, zeptá se. Když soubor chybí nebo není síť, mlčí.

**Ručně z nabídky OPTIONS** — tlačítko `AKTUALIZOVAT APLIKACI`. To stáhne
i když je verze stejná, na testování.

## Co se obejít nedá

- **Systémové okno „Nainstalovat"** uživatel uvidí vždycky. Android to
  schovat nedovolí a je to správně.
- **Poprvé se telefon zeptá na povolení** instalovat z tohoto zdroje.
  Když není, aplikace ho pošle rovnou do toho nastavení.

## Ověřeno spuštěním

Čtení souboru s verzí, šest případů:

```
přesně jak jsem ho vytvořil  -> verze 203 není novější - mlčím
s koncem řádku               -> mlčím
s mezerami navíc             -> mlčím
novější verze                -> NABÍDNU aktualizaci na 205
rozbitý                      -> NEPŘEČTU - mlčím
prázdný                      -> NEPŘEČTU - mlčím
```

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS rozcestníku i etap | 0 chyb |
| čtení souboru s verzí | 6/6 správně |
| oprávnění v manifestu | doplněno |
| jádra Segy i PS1 (C++) | nesaháno |

---

## DŮLEŽITÉ: aktualizuj ten soubor na webu

Přiložil jsem **nový `emu10_verze.txt`** s číslem **204**:

```
204|https://atarihelp.eu/wp-content/uploads/2026/08/app-debug-43.zip
```

**Až nahraješ tenhle build na web, přepiš tam i ten soubor** — jinak si
aplikace bude myslet, že je pořád venku 203, a nic nenabídne.

Až vydáš další verzi, změníš v něm zase to číslo a odkaz. Číslo ti vždycky
napíšu k balíčku.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nahraj zip na web a **přepiš `emu10_verze.txt` na 204**.
**3)** Na telefonu ještě se starou verzí spusť aplikaci — **za pár vteřin
       se má zeptat na aktualizaci**.
**4)** Dej AKTUALIZOVAT — projde to bez uložení souboru?
**5)** Zkontroluj Soubory / Stažené — **nesmí tam nic přibýt**.
**6)** OPTIONS → `AKTUALIZOVAT APLIKACI` — ruční spuštění.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | zeptá se na aktualizaci | mlčí |
| 4 | systémové okno a instalace | chyba |
| 5 | ve Stažených nic nepřibylo | leží tam zip |
| 6 | zeptá se a stáhne | nic |

## CO POSLAT ZPĚT

Log:

```
BUILD2SA42 AKTUALIZACE na webu=204 moje=203
BUILD2SA42 AKTUALIZACE OK (48123 kB, na disku nic)
```
