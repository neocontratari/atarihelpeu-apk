# KROK O5 — OBRAZ PŘÍMO Z GRAFIKY

Verze: **EMU10-O5-PRIMO-Z-GRAFIKY** (versionCode 35)

## Co se stalo s tím kolečkem

Do teď obraz cestoval takhle:

```
jádro nakreslí na grafice
  → stáhne se do procesoru (glReadPixels)
  → průchod po pixelech (převrácení řádků)
  → druhý průchod po pixelech (přehození barev)
  → přes JNI do Javy
  → kopie do bufferu
  → nahraje se ZPÁTKY na grafiku
  → nakreslí
```

Sedm sáhnutí po každém pixelu za něčím, co v grafice dávno hotové bylo.

Teď:

```
jádro nakreslí na grafice
  → nakreslí se na obrazovku
```

Zobrazovací vlákno má vlastní EGL kontext **sdílený** s tím, ve kterém
kreslí gpu-gles — takže smí sáhnout přímo na jeho textury. Ta samá věc,
kterou už dělá čtecí vlákno, a na tvém telefonu prokazatelně funguje
(v logu `GLES_READER_THREAD_READY`).

Zmizel tím i celý problém „ze kterého bufferu se zrovna čte" — protože
se už nečte vůbec. Obraz se kreslí z přesně těch souřadnic, které
gpu-gles k tomu snímku vydalo.

## Pojistka

Kdyby to nešlo — nepodařilo se sdílet kontext, chyběla plocha, cokoli —
aplikace **spadne zpátky na dnešní cestu**. Nejhorší případ je dnešní
stav, ne černá obrazovka. V logu bude přesně napsáno, na čem to selhalo:

```
O5 O5_OK prime zobrazeni bezi           ← povedlo se
O5 O5_FAIL eglCreateContext(sdileny)    ← nepovedlo, a víme kde
```

Hledej v logu řádek začínající `O5`.

## Co ještě zůstává

**TV cast pořád jede starou cestou.** Obraz se na televizi posílá jako
JPEG po síti, což bez pixelů v procesoru nejde. Takže na TV se zatím
nezlepší nic — až se ustálí obraz na telefonu, přijde na řadu ona.

**Poctivě:** může se objevit občasné trhnutí. Zobrazovací vlákno kreslí
z textury, kterou jádro může v tu chvíli přepisovat. Je to jiná vada než
dnešní blikání a řeší se snadno (třetí textura), ale nechtěl jsem do
jednoho kroku cpát dvě věci naráz.

## Postup

1. Rozbal ZIP → složka `emu10_B`
2. Zkopíruj CELÝ obsah přes repozitář → **Nahradit vše**
3. GitHub Desktop → „krok O5 primo z grafiky" → **Commit** → **Push**
4. Zelená → Actions → Artifacts → `app-debug` → do telefonu

## Co mě zajímá

Očima:

1. Je obraz **celý**, nebo pořád chybí kusy?
2. **Bliká** to ještě?
3. Je to **ostřejší**?

A z logu jediný řádek: ten, co začíná `O5`.

---

## Kdyby se to nepostavilo

Je to velký kus nového nativního kódu a psal jsem ho bez možnosti si ho
přeložit. Když build spadne, pošli mi červený výpis z Actions — chyba
překladu se opravuje rychle, na rozdíl od hádání.
