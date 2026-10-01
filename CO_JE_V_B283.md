# Co je v B283

Tohle je první build, který ti místo zipu pošlu rovnou pushnutý do
tvého GitHub repa – test nové cesty, na kterou jsme se spolu domluvili
po B282 (balíček pod 30 MB dál chodí jako zip jako vždy, nad 30 MB
nebo na vyžádání pushnu rovnou).

## Co se změnilo

Malá, bezpečná věc – žádná změna chování appky. Při hledání vhodné
malé úpravy jsem si zpětně zkontroloval vlastní komentář z B282 u
50vteřinové záložní pojistky pro ukládání WAV (`zahajNahravani()` v
`index.html`). Tvrdil jsem tam, že je "změřeno ~28s pro 100řádkový
program" – po kontrole to ale **nebylo skutečně změřeno**: test (i
appka samotná u svých rychlých tlačítek typu CSAVE/CLOAD) píše jen
pomocí zjednodušené funkce, co umí písmena, mezeru a RETURN – číslice
řádkových čísel by v ní zmizely, takže žádný skutečný 100řádkový
program by takhle nikdy nevznikl.

Opravil jsem tedy jen ten komentář na pravdivé tvrzení – skutečně
změřené je jen CSAVE minimálního programu (~24s), 50s tomu dává asi
2× rezervu, a i kdyby ta pojistka někdy musela zasáhnout, dělá přesně
to, co appka dělala vždycky před B282 (nic se nezhorší). Žádný kód se
funkčně nezměnil, jen jsem si nenechal ve vlastní poznámce nepravdivé
tvrzení – přesně v duchu "neodhaduj, ověřuj", co jsi na mně celou dobu
žádal.

## Jak to ověřit

Appka se chová úplně stejně jako B282 – nic tu není co rozbít. Hlavní
věc, kterou sleduj, je samotné doručení: jestli build v Actions proběhl
zeleně a jestli se ti commit objevil v GitHub Desktopu správně.
