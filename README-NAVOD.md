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

Celoobrazovková, **pomalu se přelévající tmavá barva** – naprosto plynule,
bez blikání a bez trhání. To je důkaz, že double buffering + vsync +
`eglSwapBuffers` fungují správně. Zrcadlení na TV spusť jako obvykle –
obraz musí být klidný i tam.

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

## 7) Kam se později píše grafika

Soubor `app/src/main/cpp/main.c`, funkce `draw_frame()` – je tam
vyznačené místo `>>> SEM POZDEJI PRIJDOU POLYGONY <<<`.
Všechno okolo (EGL, smyčka, swap, vsync, přežití otočení displeje
a zamknutí telefonu) je už hotové a ošetřené.

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
