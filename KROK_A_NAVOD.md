# KROK A — obnova tvé velké aplikace (základ pro přestavbu)

Parťáku, tohle je PRVNÍ ze série balíčků, kterými do tvé velké aplikace
AtariHelp.eu dostaneme náš plynulý obraz. Musíme jít po jednom kroku —
jinak by se v tak velké aplikaci nedalo poznat, co co rozbilo.

## Co tenhle balíček dělá

NIC v tvé aplikaci zatím nemění. Jen ji obnovuje a ověřuje, že se po tom
stěhování zase **postaví do zeleného**. To je základní kámen — až budu
mít jistotu, že aplikace zase žije, začnu v příštích balíčcích vyměňovat
staré zobrazování PS1 za náš renderer.

Aby ses nebál: tahle verze se nainstaluje jako **samostatná aplikace
vedle** té tvojí pravé (jmenuje se „AtariHelp TEST" a má jiné ID).
Tvoje pravá AtariHelp.eu zůstane úplně nedotčená. Můžeš je mít obě
najednou.

## Co uděláš (tvůj rituál)

Tenhle balíček je VELKÝ (obsahuje celý emulátor Segy i PS1 jádro) —
je normální, že se bude nahrávat a stavět déle než renderer.

1. Rozbal ZIP. Uvnitř je složka `emu10_A`.
2. Tohle je NOVÝ, čistý projekt. Nejjednodušší cesta: v GitHub Desktopu
   dej **File → New repository**, pojmenuj třeba `atarihelp-emu10-nove`,
   a do jeho složky zkopíruj CELÝ obsah `emu10_A`. Pak Commit → Publish.
   (Nový repozitář použij schválně — ať se to neplete se starým, kde je
   binec z 1084 buildů. Tvůj starý repozitář zůstane nedotčený.)
3. GitHub sám spustí build. Poprvé u tak velké aplikace to může trvat
   **15–25 minut** (staví se celý emulátor). Počkej na zelenou fajfku.
4. Actions → poslední běh „Build APK" → dole Artifacts → `app-debug`.
5. Nainstaluj do telefonu. Objeví se jako „AtariHelp TEST" vedle tvé
   pravé apky.

## Co čekat

Aplikace se otevře stejně jako tvoje pravá (menu, hry, PS1, cast) —
protože je to zatím její věrná kopie, jen přejmenovaná. Spusť PS1 hru
jako obvykle a ověř, že jede jako ve tvé pravé apce.

## Co mi pošli

1) Jestli byl build zelený (nebo pošli posledních ~40 řádků z kroku
   „Sestaveni APK", kdyby zčervenal — u tak velké aplikace se první
   build klidně může o něco zaškobrtnout a já to opravím).
2) Jestli se „AtariHelp TEST" nainstalovala a spustila.

Až tohle bude stát, pošlu balíček B: přidání našeho rendereru dovnitř
jako druhé nativní knihovny (pořád beze změny obrazu — jen ověření, že
se náš kód s tvou aplikací snese). Pak teprve výměna zobrazování.

Jdeme na to postupně a bezpečně. Tvoje pravá apka je celou dobu v suchu.
