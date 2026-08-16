# B120 — ZPĚT NA B118 (versionCode 168)

## Obě změny z B119 jsou vrácené

Byly moje a obě uškodily.

**1. Menší zvukový zásobník shodil zvuk Segy na TV.**

Snížil jsem ho z 21 na 11 ms, abych zkrátil zpoždění. Zvuk v telefonu hrál
dál (v logu `got=384`, žádná podtečení), ale **na televizi přestal chodit**.
Menší zásobník znamená kratší dávky a jiná časování — a na těch závisí
odbočka zvuku pro TV.

Vráceno na 1024 / 2048 / 4096 a `min * 2`, jak to bylo.

**2. Přednostní zařazení snímku nepomohlo.**

Zkusil jsem tím obejít to, že smyčka běží na hlavním vlákně. V logu je
`avgTickGapMs=42` — **stejně jako před tím**, takže to nezabralo, a obraz
na TV se podle tebe ještě zhoršil.

Vráceno.

## Co jsem se z toho naučil

Zapsal jsem do předávacího balíčku dvě věci jako **nesahat**:

- velikost zvukového zásobníku (rozbije zvuk na TV)
- přednostní zařazení smyčky (nepomůže a zhorší obraz)

A k tomu poznámku, že smyčku **nelze** přehodit na vlastní vlákno, protože
sahá na pohledy aplikace.

## Co zůstává — všechno dobré z B117 a B118

- Sega kreslí přes **OpenGL ES** přímo z jádra (B117)
- Sega je v **rychlé větvi** snímání pro TV (B118)
- rychlejší dotyk, klávesnice a joystick s Xbox mapováním (B116)

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Sega C++ | `clang` pro aarch64 | 0 chyb |
| PS1 C++ | `clang` pro aarch64 | 0 chyb |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |

---

## CO TESTOVAT

**1)** **Sega — zvuk na TV.** Hraje zase?
**2)** **Sega — obraz na TV.** Je zpoždění jako v B118?
**3)** **PS1** — kontrola.

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | zvuk na TV hraje | ticho |
| 2 | jako v B118 | horší |
| 3 | jako v B118 | zhoršilo se |

Tohle má být přesně B118 plus nic. Když to sedí, jsme zpátky na jisté zemi
a to zpoždění zvuku zkusím jinou cestou — ne přes zásobník.
