# AH EGL Render – návod krok za krokem

Kompletní projekt, ze kterého ti GitHub **sám postaví APK**. Nemusíš nic
programovat ani instalovat Android Studio. Nová apka se jmenuje
**AH EGL Render** a nainstaluje se **vedle** tvého AtariHelp.eu (emu10) –
nic ti nepřepíše.

---

## 1) Nahrání na GitHub (přes GitHub Desktop)

1. Rozbal ZIP – vznikne složka `egl-render`.
2. V GitHub Desktopu: **File → Add local repository…** a vyber složku `egl-render`.
3. Desktop napíše, že to ještě není repozitář → klikni na modrý odkaz
   **create a repository** → nic neměň → **Create Repository**.
4. Klikni na **Publish repository** (klidně nech zaškrtnuté „Keep this code
   private“) → **Publish**.
5. Hotovo. Build se na GitHubu spustí úplně sám.

## 2) Stažení hotového APK

1. Otevři svůj repozitář na github.com → záložka **Actions**.
2. Klikni na poslední běh „Build APK“. Poprvé to trvá cca **5–10 minut**
   (stahuje se Android SDK). Počkej na zelenou fajfku ✅.
3. Úplně dole na stránce běhu je sekce **Artifacts** → klikni na
   **app-debug** → stáhne se ZIP a uvnitř je `app-debug.apk`.

## 3) Instalace do mobilu

1. Přetáhni APK do telefonu (kabel, Google Drive, cokoliv) a otevři ho.
2. Povolit „instalaci z neznámých zdrojů“, když se telefon zeptá.
3. Podpisový klíč je uložený přímo v projektu, takže **každá další verze
   půjde nainstalovat přes tu předchozí** bez odinstalace.
   (Jen úplně první instalace může chtít odinstalovat případný starý
   pokus se stejným jménem.)

## 4) Co máš vidět

Verze 1.3: stejný ostrý „retro" obraz (pruhy, šachovnice, dvě jedoucí
bílé linky), ale nově se **každých ~7 vteřin samo přepne rozlišení
a formát obrazu**, přesně jako to dělají skutečné hry:

256×224 (Sega NTSC) → 320×240 (Sega/PS1) → 512×240 (PS1 hi-res,
16bit barvy) → 640×480 (PS1 menu/BIOS, formát XRGB) → a dokola.

Rámeček (letterbox) se při každém skoku sám přizpůsobí a barvy musí
zůstat správné i u XRGB (prohození kanálů dělá grafický čip, ne
procesor). V logu ke každému skoku přibude řádek
„Jadro zmenilo rozliseni: …".

## 5) Logy (stejný postup, jaký znáš z emu10)

- V prohlížeči na PC (stejná wifi jako telefon): `http://IP-TELEFONU:8765/log`
- IP telefonu: Nastavení → Wi-Fi → detail připojení.
- Uvidíš celý průběh inicializace EGL krok za krokem, rozlišení, FPS
  a případné chyby s přesným EGL názvem.
- Pozn.: když by zároveň běžel emu10 a držel port 8765, nová apka to jen
  zapíše do logcatu a jede dál (logcat tag: `EGLRender`).

## 6) Když build selže (červený křížek)

Actions → klikni na červený běh → krok **Sestaveni APK** → zkopíruj
posledních ~50 řádků a pošli mi je. Z nich přesně poznám, co opravit.

## 7a) PS1 režim (od verze 1.4)

V balíčku je přibalené tvoje skutečné PS1 jádro (`libnapps1core.so`
z emu10) a renderer s ním mluví standardním libretro protokolem,
který jádro samo nabízí. Postup po instalaci:

1. **Povol úložiště:** Nastavení → Aplikace → AH EGL Render →
   Oprávnění → Úložiště/Soubory → Povolit. Pak apku úplně zavři
   (vymáchnout z posledních aplikací) a spusť znovu.
2. Apka si sama najde hru ve složce `Download/AtariHelp/PS1`
   (tam ji ukládá emu10 – Crash Bandicoot tam už je). Bere první
   `.cue`, případně `.chd`/`.bin`.
3. BIOS není nutný (jede vestavěný náhradní). Když chceš originální,
   nakopíruj soubory BIOSu do `Download/AtariHelp/PS1/bios`.
4. V této verzi jede **jen obraz** – zvuk a ovládání jsou další
   samostatné kroky.
5. Když PS1 nenaběhne (chybí povolení, není hra…), apka dál ukazuje
   demo pruhy a v logu 8765/log je přesný důvod – řádky začínají
   „PS1:". Hlášky samotného jádra začínají „PS1c:".

## 7b) Kam se napojí Sega

Rozhraní je v `app/src/main/cpp/core_api.h` („zásuvka"): jádro dodává
jen ukazatel na svůj framebuffer + šířku + výšku + formát. Renderer si
sám přestaví texturu při změně rozlišení, sám řeší letterbox i barvy –
vše na GPU, procesor se pixelů nedotýká.

Napojení = nahradit soubor `core_demo.c` napojením na skutečné jádro:
`core_step()` zavolá krok emulace (u PS1 `realCoreStep`), a
`core_get_frame()` vrátí buffer jádra – u PS1 je to obraz, který plní
gpulib (formát „fmt=1" z tvých logů = `CORE_FMT_XRGB8888`, rozměry
dispW×dispH), u Segy výstup mostu (320×240 / 256×224). Tohle je ta
jediná zbývající operace a proběhne při přesazení rendereru do velké
aplikace, kde jádra žijí.

## 8) Další změny

Stačí upravit soubor, v GitHub Desktopu dole vyplnit popisek →
**Commit to main** → **Push origin**. Nové APK se postaví samo
a najdeš ho zase v Actions → Artifacts.

## 9) Pojistka: v Actions vždy jen jeden balíček

Build má v sobě úklidový krok „Pojistka“. Při každém spuštění se podívá,
jestli v repozitáři neleží ještě nějaký jiný recept na build, a pokud ano,
sám ho přepne na vypnuto. Nic se nemaže a v protokolu běhu je vypsané,
co pojistka našla a co vypnula. Výsledek: **jeden push = jeden běh =
jeden balíček `app-debug`.** Jediná výjimka je úplně první push této
verze – starý recept se stihne rozběhnout ještě naposledy, než ho
pojistka zhasne.
