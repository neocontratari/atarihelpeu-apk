# B267 — Skutečný tón reproduktoru, ne jen jeden klik (versionCode 313)

Rene: "Je to jednoduché, proč první Return nic nespustí... je to
upozornění pro uživatele. Nehledej za tím nic složitého... nebuď
línej a nehádej. U Atari nestrpím žádný fake."

Vzal jsem si to k srdci a udělal to důkladně.

===============================================================================
 CO SE OPRAVDU DĚJE (POTVRZENO PŘÍMÝM TRASOVÁNÍM, NE ODHADEM)
===============================================================================

Přesně jak jsi popsal - první RETURN je varovný signál, druhý
potvrzuje. Prošel jsem to skutečným krokováním procesoru a potvrdil:
**appka po prvním RETURN SPRÁVNĚ prochází tou pípací smyčkou v ROM**
- žádná chyba v logice příkazu, žádné čekání na druhý stisk navíc,
nic rozbitého v tomhle směru.

===============================================================================
 SKUTEČNÁ PŘÍČINA CHYBĚJÍCÍHO ZVUKU
===============================================================================

Ta "pípací" rutina v ROM nepoužívá POKEY tón - přímo softwarově
**rychle přepíná** reproduktorový bit (stejná cesta jako klik při
bootu), a tím vytváří slyšitelný tón čistě rychlým bit-bangingem.

Appka ale generuje zvuk **jednou za obrazový snímek** a kontroluje jen
"je bit teď jiný než naposledy?" Jenže ta ROM smyčka přepíná bit
**mnohokrát během jednoho snímku** (skutečný tón, ne jednotlivé
kliknutí) - a když se kontroluje jen na konci snímku, všechny
mezilehlé přepnutí se ztratí. Zůstane jen poslední stav, místo
skutečného tónu.

===============================================================================
 OPRAVA
===============================================================================

Každý zápis do reproduktorového bitu se teď zaznamená s přesným
časem (CPU cyklem), ne jen jako poslední hodnota. Generování zvuku
pak zpracuje všechny zaznamenané přechody na jejich správných
pozicích uvnitř bufferu - přesně tak, jak už existující dozvuková
logika (nezměněná) očekává.

===============================================================================
 OVĚŘENÍ
===============================================================================

**Izolovaný test:** 20 rychlých přepnutí v jednom bufferu → 20
správných změn ve výstupu (dřív by to bylo 0 nebo nanejvýš 1).

**Beze změny pro původní případy:** ticho zůstává tiché, jednotlivé
kliknutí (při bootu) pořád správně dozní, návaznost mezi snímky
funguje.

**End-to-end se skutečnou ROM**, přesně stejným způsobem volání jako
produkční kód appky (generování zvuku jednou za snímek) - po
CSAVE+RETURN je teď skutečně detekovaná zvuková energie v 52 z 90
sledovaných snímků. Předtím to bylo nula.

Celkem 10 kontrol (6 předchozích + 4 nové) - všechny prochází. Celý
`nap_atari_native.cpp` znovu zkompilován čistě.
