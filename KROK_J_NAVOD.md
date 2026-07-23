# KROK J — zvuk počká na obraz (sesouhlasení)

## Přiznání: minulý krok zvuku nepomohl

Log mě usvědčil. Poměr hladových smyček je pořád **47 %** — přesně jako
předtím. To sdílení snímku sice ušetřilo jádru práci, ale na zvuk to
nemělo vliv. Moje domněnka byla špatná.

## Zato mě to donutilo přečíst si tvůj popis znovu — a tam je odpověď

Ty říkáš, že zvuk je **čistý, netrhaný, jen ujíždí**.
**To není hladovění!** Kdyby zvuku chyběla data, praskal by a sekal.
On ale hraje krásně, jen **v jiný čas než obraz**.

Příčina je logická: **zvuk letí k tobě skoro okamžitě, ale obraz se musí
cestou zabalit do videa a prohlížeč si ho ještě chvíli schovává do
zásoby, než ho začne přehrávat.** Obraz proto dorazí o něco později.

Těch 47 % "prázdných pokusů" je normální chování — zvukové vlákno se
prostě ptá častěji, než jádro stíhá vyrábět. Není to porucha.

## Oprava

Zvuk teď **schválně počká** a drží si zásobu 0,75 vteřiny, aby na obraz
počkal.

**A můžeš si to sám doladit bez nového buildu!** Stačí v prohlížeči
připsat za adresu `?av=` a číslo v sekundách:

- `192.168.0.34:8765/?av=0.5` — kratší čekání
- `192.168.0.34:8765/?av=0.9` — delší čekání
- `192.168.0.34:8765/?av=0` — bez čekání (jako dřív)

Zkoušej, dokud nebude pusa sedět s hlasem, a řekni mi vítězné číslo —
nastavím ho pak natrvalo.

## Verze: EMU10-J-ZVUK-CEKA-NA-OBRAZ

## Co mi pošli

1. Sedí zvuk s obrazem líp?
2. Jaké číslo v `?av=` ti sedlo nejlíp?
3. Je zvuk pořád čistý (netrhá se)?
