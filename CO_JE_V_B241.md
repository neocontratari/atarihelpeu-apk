# B241 — SBÍRKA znovu funguje, konečně skutečná příčina prolínání obrazu (versionCode 287)

Rene poslal `sega.txt` (skutečný log) + screenshot: "Teď ti nefunguje
sbírka - nejde se dostat na www stránky pro hru... a to ani z PS1 a
Atari. A stránky mě fungují. Trvá to strašně dlouho a já nemám na www
stránkách chybu... A furt tam máš ten obraz na výšku."

===============================================================================
 1) SBÍRKA - PŘESNÁ PŘÍČINA NALEZENA V LOGU
===============================================================================

  Appka nikdy nenavigovala na atarihelp.eu PŘÍMO - vždycky přes jednu
  ze tří veřejných CORS proxy služeb (kvůli dřívějšímu blokování ze
  strany hostingu WEDOS). Log ukázal, že VŠECHNY TŘI teď selhávají:

      proxy.cors.sh      → DNS nejde vůbec rozřešit (služba zmizela?)
      api.allorigins.win → HTTP 522 (Cloudflare timeout)
      corsproxy.io       → HTTP 401 (teď zřejmě vyžaduje API klíč)

  Appka to zkoušela ve smyčce desítky vteřin, a když všechny tři
  selhaly, VZDALA TO ÚPLNĚ - nikdy nezkusila připojit se přímo. A
  přesně přímé připojení funguje bez jediného problému (ověřil jsem
  to sám - `atarihelp.eu/?page_id=1003` se načte v pořádku).

  OPRAVA: když všechny tři proxy selžou, appka teď zkusí PŘÍMÉ
  připojení jako poslední záchranu, než to vzdá úplně. Ověřeno
  izolovaným testem kontrolní logiky (Java kompilátor v tomhle
  prostředí bohužel není k dispozici, takže jsem stejný algoritmus
  ověřil v JS - logika je jazykově nezávislá):
    - všechny 3 proxy selžou → přímé připojení uspěje → vrátí obsah
    - první proxy uspěje → další se vůbec nezkouší (žádná zbytečná
      volání)
    - úplně všechno selže (i přímé) → vyhodí chybu, ne nekonečná
      smyčka

===============================================================================
 2) PROLÍNÁNÍ OBRAZU - KONEČNĚ SKUTEČNÁ PŘÍČINA
===============================================================================

  B235, B239 i B240 řešily reálné, ale NEDOSTATEČNÉ kousky skládačky.
  Skutečná příčina byla nakonec jednoduchá - a byla to MOJE VLASTNÍ
  grafika: panely mají "skleněné" pozadí, schválně POLOPRŮHLEDNÉ
  (35-62% krytí), aby skrz ně bylo vidět tu hvězdnou/vesmírnou scénu
  za nimi.

  Problém: `segaPlocha` (živá hra) leží POD stránkou. I když appka
  správně požádá o schování, DOKUD se ta žádost nestihne projevit
  (a to může trvat déle, než jsem odhadoval v B240 - 90ms zjevně
  nestačilo), poloprůhledné sklo ukazuje AKTIVNÍ HRU skrz sebe -
  úplně bez ohledu na to, jak dlouho nativní schování trvá.

  SPRÁVNÉ ŘEŠENÍ: ne další čekání, ale udělat panely SAMY O SOBĚ
  prakticky neprůhledné (96-98% krytí místo 35-62%). WebView
  vykresluje svůj vlastní obsah spolehlivě a bez zpoždění (na rozdíl
  od mezivrstevného JS→Java mostu) - neprůhledný panel schová
  cokoli pod sebou VŽDY, bez ohledu na časování nativní strany.

  Aplikováno na OBOU emulátorech (Sega i PS1) - stejný "sklo" vzor,
  stejné riziko, i když PS1 tohle konkrétně nehlásil.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check (obě emu stránky), `stranka_kontrola.py` (TV cesta) -
  0 chyb. Rovnováha `{ }` v MainActivity.java - 2445/2445.

  Všech dvanáct existujících jsdom simulací (osm PS1, čtyři Sega)
  spuštěno znovu - beze změny prošly (čistě CSS/síťová změna, žádná
  JS logika se nedotkla).

  Nová izolovaná simulace kontrolní logiky (`test_provider_relay_logic.js`)
  - tři scénáře popsané výše, všechny potvrzeny.

  CO NEJDE OVĚŘIT ODSUD: jestli SBÍRKA na tvém telefonu teď doopravdy
  projde přes přímé připojení (to je čistě síťová věc, na tvém
  zařízení), a jestli neprůhledné panely doopravdy odstraní prolínání
  obrazu vizuálně - to je na tvém testu. Očekávám, že ano, protože
  tahle oprava už nezávisí na časování nativního mostu vůbec.
