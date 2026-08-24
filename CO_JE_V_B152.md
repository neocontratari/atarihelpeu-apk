# B152 — POLOVIČNÍ STROJAK A SKUTEČNÁ OBRAZOVKA (versionCode 200)

## 1) Turbo BASIC je teď poloviční strojak

**74 bajtů v devíti řádcích `DATA`.** BASIC už jen nahraje rutinu a zavolá
ji přes `USR` — všechnu práci dělá strojový kód:

```
$0600 INIT    projde display list a zapne přerušení na řádku (ORA #$80),
              zavěsí se na VDSLST, přihlásí odložený VBI přes SETVBV,
              povolí NMIEN
$0631 DLI     na každém řádku: VCOUNT + čítač, AND #$F0, WSYNC, COLPF2
$0643 VBI     jednou za snímek zvedne čítač - tím se duha posouvá
$0649 POCET   čítač snímků
```

Díky VBI už není potřeba ta smyčka `GOTO` v BASICu.

Sestaveno překladačem se dvěma průchody, ne ručně — proto adresy
`$0631`, `$0643` a `$0649` uvnitř `DATA` sedí.

## 2) Obrazovka ve finále kreslí to samé, co ten program

Duha se počítá **přesně tou rutinou**:

```
COLPF2 = (VCOUNT + citac) AND $F0
```

Barvy jdou z **palety GTIA**, kterou jsme odvodili z datasheetu C014805 —
odstín není index do tabulky, ale zpoždění na zpožďovací lince: krok
27 stupňů, počátek 190. Ne `hsl()`, ne odhad.

Kontrola: `$0C` vyjde `(193,193,193)`, tvoje Atari dává `(203,203,203)`.

**Písmo má jas z `COLPF1`, ale odstín z `COLPF2`** — to je ta atarácká
zvláštnost, kvůli které v Altirře text mizel. Ověřen kontrast ve všech
šestnácti odstínech:

```
nejmenší rozdíl jasu: 99 z 255  ->  čitelné všude
```

Kolem monitoru zůstává moje grafika s krtečkem, plazmou a hvězdami.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS všech tří etap | 0 chyb |
| listing se stihne napsat | 20,8 s z 24,0 s |
| výstup se stihne napsat | 19,6 s z 27,0 s |
| kontrast textu ve všech odstínech | 16/16 čitelných |
| řetěz etap | 1 → 2 → jádra → 5 → konec |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Etapa 2 — napíše se celý listing včetně devíti řádků `DATA`?
**3)** Po PS1 etapa 5 — **je v monitoru duha jako na Atari** a kolem krteček?
**4)** Dopíše se text až po „A JEHO AI PARTAK"?
**5)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | celý listing | uřízne se |
| 3 | tmavá duha jako z DLI | duhové pruhy jako dřív |
| 4 | text až do konce | uřízne se |
| 5 | jako dosud | jakákoli změna |
