# Co je v B315

Tvoje zpráva: „Problém se zvukem při LAN přenosu. Po spuštění LAN streamu běžel zvuk pouze jednou
v Atari – pak už ne – ani v Atari, ani v Sega, ani v PS1 – na mobilu zvuk funguje. Ale na TV nebo PC
už ne.“ (chyba už od B313)

## 1. Co bylo špatně

- Stránka v prohlížeči na TV / PC má dvě cesty zvuku: **proud přímo z C++** (Atari, Sega, PS1, menu)
  a **starou cestu** (MP3 přehrávač, YouTube a krátké mezery, kdy proud z C++ zrovna neběží).
  Obě šly přes jeden společný „regulátor hlasitosti“.
- Stará cesta při každém navázání regulátor na chvilku stáhne na nulu a za „zpoždění zvuku“ ho vrátí
  nahoru. To zpoždění se nastavuje šipkami nahoru / dolů (na obrazovce blikne zelené „ZVUK 250 ms“)
  a prohlížeč si ho pamatuje i po F5.
- Šipky ho měnily i **v menu na TV** – a tam se šipkami listuje kartami (v B313 jsem tě o to sám
  prosil: „Pak listuj šipkami“). Šest stisků šipky dolů = zpoždění 0. Při nule se regulátor „vrátil
  nahoru“ dřív, než se stáhl dolů → **zůstal na nule napořád**. Pryč byl celý zvuk stránky – Atari,
  Sega, PS1 i MP3 – a po F5 zase (nula zůstala uložená).
- **Proč hrálo první Atari:** stará cesta poprvé naváže až v mezeře po první hře (v tvém logu
  NET → stažení hry → Atari znovu, 17:32:03 až 17:32:05). Do té doby byl regulátor ještě nahoře.
- Telefon to neovlivnilo – zvuk v telefonu jde jinudy (OpenSL v C++).
- Stránka přitom do logu psala „zvuk hraje“ – hlásila jen, že prohlížeč zvuk pustil, ne co opravdu
  jde do reproduktoru. Proto to v logu nebylo vidět.
- Poctivě: jaké zpoždění máš v prohlížeči uložené, z logu B314 přímo vidět nejde (stránka ho dosud
  nehlásila). Test na PC ale tvou chybu se stránkou B314 zopakoval **přesně** (viz níž) a jinou cestu
  k trvalému tichu jsem v kódu nenašel. B315 to při otevření stránky zapíše do logu – když tam bude
  „ulozene bylo 0 ms“, byla to přesně tahle příčina.

## 2. Oprava (jen stránka WEB TV, nic jiného se neměnilo)

- **Proud z C++ (Atari, Sega, PS1, menu) má vlastní regulátor**, se kterým stará cesta nemůže hnout.
- Stará cesta se po navázání vždy vrátí na plnou hlasitost, i kdyby zpoždění bylo 0.
- Zpoždění nejméně 100 ms; uložená nula (nebo nesmysl) se při otevření stránky vrátí na 300 ms.
- **Šipky nahoru / dolů v menu zpoždění nemění** (tam listují kartami), ani v posuvnících panelu OBRAZ.
- Navíc: při přechodu ze staré cesty na proud (třeba po stažení hry) hrála stará cesta ještě
  0,25–0,3 s přes nový zvuk (ozvěna; při delším zpoždění déle). Teď skončí přesně tam, kde začne
  zvuk proudu (změřeno do 0,03 s, bez mezery).
- **Do logu** (každých 30 s): skutečná úroveň zvuku, který jde do reproduktoru (např.
  „vystup max -13 dB, zvuk v 95 % mereni“, nebo „vystup TICHO“ i s tím, který regulátor je dole),
  a když proud neběží, i stará cesta („B315 STARA CESTA … zvuk 29.8 s, navazani 1, zpozdeni 300 ms“).

## Co prosím vyzkoušet

1. Nainstaluj B315 přes B314 (postup zůstane).
2. Na TV / PC otevři stránku (nebo F5) a klikni do obrazu.
3. ATARI 130XE → NET → stáhni hru → zvuk musí hrát (dřív tady zmizel). Pak ještě jedna hra.
4. Menu → SEGA → menu → PS1 → zvuk všude.
5. V menu zkus šipky (i nahoru / dolů) – zelené „ZVUK … ms“ se už neobjeví a zvuk zůstane.
6. MP3 přehrávač na TV – zvuk.
7. Pošli log z `8765/log` – uvidím v něm řádek „B315 ZVUK: …“ (jestli bylo uložené zpoždění 0)
   a každých 30 s, kolik zvuku opravdu šlo do reproduktoru.

## Ověřeno na PC

- **Zvuk na TV / PC jako v tvém logu** (stránka přesně z appky v Chromiu, server dělá, co Java: menu,
  kliknutí, Atari, NET → stažení hry → Atari znovu, menu, Sega, menu, PS1, MP3; měří se, co opravdu
  jde do reproduktoru): **20/0**.
  - Stránka **B314**: s výchozím zpožděním hraje všude; po 6× šipce dolů v menu (nebo s uloženou
    nulou) hraje **jen první Atari, pak ticho v Atari, Seze, PS1 i MP3 = přesně tvoje chyba**.
  - Stránka **B315**: zvuk hraje v Atari poprvé i podruhé, v Seze, PS1 i MP3 ve všech čtyřech
    případech (výchozí zpoždění, 6× šipka dolů v menu, uložená nula, uložené maximum 2 s).
    Šipky v menu zpoždění nemění. Ozvěna při přechodu na proud 0,30 s (B314) → do 0,03 s, bez mezery.
    Do logu jde „ulozene bylo 0 ms …“ a každých 30 s skutečná úroveň zvuku (−13 dB u zkušebního tónu).
- **Co přesně se ve stránce změnilo:** stránka B315 = B314 + 22 změn, všechny ve zvuku (1849 z 1862
  příkazů stránky beze změny) – 0 nechtěných změn.
- **Skripty stránky** bez chyby syntaxe, **Java** proti skutečnému Androidu (API 34): 0 chyb.
- **Regrese** (testy stránky TV z B302–B314 se stránkou B315): **6/0** – přepnutí proudu po odchodu
  z menu, menu na TV (skutečné C++ menu, klávesnice), stránka česky stejná jako dřív / anglicky bez
  češtiny, klávesnice PC do Atari, Atari na TV + Ctrl+V, Sega / PS1 / demo se zvukem (výzva ke
  kliknutí, F5, zvuk za obrazem Sega 87 ms / PS1 62 ms, zadrhávající síť, žádné lupance).
- Telefon, C++ a všechno ostatní beze změny.
