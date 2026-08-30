# B200 — ÚKLID (versionCode 246)

Napsal Claude, na žádost Reného po B197: "Vyhazet mrtvoly a kostlivce ven z
apky." Žádná změna chování appky. Jen odstranění souborů, na které appka
nikde neodkazuje — každý bod dole byl před smazáním ověřen (grep přes celý
Java + assets strom) a po smazání appka prošla oběma kontrolami z bodu 4
protokolu (`stranka_kontrola.py` a `node --check` na zbylé HTML/JS).

Pokud jsi AI, co čte tohle: než cokoli z týhle appky měníš, přečti si
PŘEDEVŠÍM `PREDAVACI_PROTOKOL_PRO_NASTUPCE.txt` a `PREDAVACI_BALICEK_PS1.txt`.
Tenhle soubor je jen seznam toho, co zmizelo a proč — ne návod k appce.

===============================================================================
 1. SMAZANÉ ASSET SLOŽKY (17) — nikde v Java ani v jiných assets neodkazované
===============================================================================

Ověřeno hromadným gerpem přes celý strom (Java/HTML/JS/XML/gradle), pro
každou složku zvlášť, ne jen odhadem podle jména:

  atari, atari8, atari8bit, atari_8bit, atari_8_bit, atari_legacy,
  legacy_atari
      Osm variant Atari stránky z různých pokusů o pojmenování. Živá Atari
      stránka je emu_vbxe/ (odkazovaná z hlavního menu, tlačítko
      "ATARI 130XE EMULATOR"). Žádná z těchto osmi nikdy nebyla.

  emu1, emu2, genesis, sega, megadrive, mega_drive, legacy, emulator
      Byly to jen 5řádkové přesměrovací můstky (`location.replace(...)`) na
      emu_vbxe/ nebo emu_sega/. Podobný, pořád živý můstek je emu/ — ten
      zůstal, protože na něj `MainActivity.java` odkazuje (`EMU_URL`).
      Tyhle na rozdíl od něj nemá odkud spustit nikdo.

  emu_sega_usa_stable, emu_sega_eu
      Osiřelý pár — emu_sega_eu byl odkazovaný jen z emu_sega_usa_stable,
      a tu neodkazoval nikdo. Živá Sega stránka je emu_sega/.

===============================================================================
 2. SMAZANÉ ZÁLOHY (9 souborů, ve slozce emu/)
===============================================================================

  index.html.BACKUP_BEFORE_BUILD2EI_19-06-2026__1-04-16,06
  index.html.BACKUP_BEFORE_BUILD2EJ_19-06-2026__1-24-44,08
  index.html.BACKUP_BEFORE_BUILD2EK_19-06-2026__1-53-19,75
  index.html.BACKUP_BEFORE_BUILD2EM_19-06-2026__2-08-16,83
  index.html.BACKUP_BEFORE_BUILD2EN2_22385
  index.html.BACKUP_BEFORE_BUILD2EQ_4351
  index.html.backup_BUILD2EP_062026_112803
  index_BACKUP_BEFORE_BUILD2ES.html
  index_BACKUP_BEFORE_BUILD2ET.html

  Appka vždycky natahuje jen `index.html` (viz `emu/index.html` — je to
  přesměrovací můstek na emu_vbxe/, popsaný výš). Tyhle záložní kopie se
  jen vezly v balíčku.

===============================================================================
 3. STARÁ WASM VĚTEV SEGY (5 souborů + prázdná složka cores/)
===============================================================================

  emu_sega/clown_local_boot.html
  emu_sega/genesis_runner.html
  emu_sega/nap_sega_core_bridge.js
  emu_sega/nap_sega_core_slot.js
  emu_sega/nap_sega_lr_genesis_adapter.js
  emu_sega/cores/   (obsahovala jen README_DROP_REAL_SEGA_CORE_HERE.txt)

  Za appky bezel drive JEDEN track pres <iframe id="clownFrame">, ktery
  zkousel natahnout skutecny WASM Genesis engine (Clownacy ClownMDEmu) z
  emu_sega/cores/ nebo ze site. Do cores/ nikdy nikdo zadny engine nedal.

  Rene potvrdil: appka se u Segy (i PS1) prepisovala na C++/C a tenhle JS
  track byl ZALOZNI reseni pro pripad, ze by se prepis nepovedl. Prepis
  se povedl - primy dukaz je primo v `emu_sega/index.html`:

      <!-- BUILD2RV: old Java/WebView Sega iframe removed from DOM;
           Sega runtime is C++ native only. -->

  a funkce `startNativeCppInPlace()` s hlaskou "SEGA C++ ONLY". Iframe uz
  v HTML vubec neni, takze zbyle soubory nemely odkud byt spustene -
  overeno gerpem, nic na ne needkazovalo.

  DULEZITE - tohle NENI totez jako "Sega nema jadro": skutecne jadro
  (clownmdemu, napsany v C, Clownacyho engine) je zabudovane primo v
  appce, staticky slinkovane do knihovny `napsega_native_proof` (viz
  CMakeLists.txt - `clown68000-*`, `clownz80-*`, `clownmdemu-core`).
  `NativeSegaCoreBridge` z ni vola `realCoreLoadRom`/`realCoreStep`/
  `grabFrame` pro skutecnou hru. Nic z tohohle jsem nemazal ani nemenil.

  NEDOTCENO, JEN ZAZNAMENANO (viz bod 5): v `emu_sega/index.html` samotne
  zustava ~19 radku mrtveho kodu (CSS pro #clownFrame uz natvrdo
  `display:none`, JS funkce jako `postRomToWrapper`/`postPad`, co se ptaji
  na `$('clownFrame')`, ktery uz neexistuje - bezpecne se nic nedeje, ale
  jsou tam). Neopravoval jsem to - je to chirurgie uvnitr 654radkoveho
  ZIVEHO souboru, ne smazani celeho mrtveho souboru, a presne tenhle druh
  zasahu mi minule (viz bod 4) nevysel napoprve.

===============================================================================
 4. SMAZANÉ JAVA TŘÍDY A CI
===============================================================================

  Ps1GlActivity.java (199 radku)
      Neni v AndroidManifest.xml, appka ji nemuze spustit, nikdo jiny na
      ni needkazuje. Bezpecne smazano.

  .github/workflows/main.yml
      Duplicitni CI recept vedle build.yml. build.yml ma vlastni pojistku,
      ktera pri kazdem pushi VYPINA vsechny ostatni aktivni recepty - takze
      main.yml uz beztak nikdy nic nepostavi. Smazano at uz neni co vypinat.

  CHYBA, KTEROU JSEM UDELAL A CHYTIL AZ NA KONTROLE:
      Nejdriv jsem smazal i Ps1GlTextureView.java, protoze jsem videl
      komentar "uz se pro obraz nepouziva" a zkontroloval jen, jestli se
      nekde vola `new Ps1GlTextureView(`. Nevola. Ale metoda `borrowFrame()`
      je STATICKA a vola se ze ctyr mist v MainActivity.java (radky 1408,
      1413, 1503, 1508) - to prvni test nezachytil. Objevilo se to az pri
      povinne kontrole po smazani (bod 4 protokolu), soubor jsem hned
      vratil. PONAUCENI PRO PRISTE: hledat i staticke volani metody
      (`TridaX.metoda(`), ne jen `new TridaX(`.

===============================================================================
 5. NALEZENO, ALE NEDOTČENO — čeká na Reného rozhodnutí
===============================================================================

  HELP tlacitko v hlavnim menu vede na emu_atari_cpp/index.html
      Puvodne jsem to nahlasil jako chybu (spletene tlacitko). Rene
      potvrdil, ze je to zamerne - docasne odlozene misto pro testovani
      noveho C++ Atari jadra, dokud neni hotove. Az bude, presune se na
      normalni tlacitko ATARI a z HELP zmizi. Nesahat.

  NativeSegaProofActivity.java (524 radku)
      Java trida sama je mrtva (neni v manifestu). ALE jeji C++ soubor
      (nap_sega_native_proof.cpp) je STEJNY soubor, ze ktereho zije
      skutecny NativeSegaCoreBridge - funkce se vzajemne volaji
      (napr. NativeSegaCoreBridge_romInfo uvnitr vola
      NativeSegaProofActivity_nativeRomInfo). Java tridu by teoreticky
      slo smazat samostatne, ale k C++ souboru se NESMI nikdo priblizit
      bez peclive rozdeleni zivych a mrtvych funkci uvnitr nej. Vetsi
      riziko nez uzitek pro tenhle balicek - necham na Reneho.

  ~19 radku mrtveho kodu primo v emu_sega/index.html
      Popsano v bode 3. Bezpecne (neskodne), ale chirurgicky zasah do
      ziveho souboru - zaslouzi si vlastni, pomalejsi pruchod.

  PS1 - dve BIOS stahovani
      Rene potvrdil: jedno se stahuje spolu se Sonicem hned na startu
      appky (kvuli intru), druhe appka stahuje sama znovu pri skutecnem
      hrani PS1, protoze se nikdy nepodiva do slozky, kam si BIOS ulozila
      poprve (viz PREDAVACI_BALICEK_PS1.txt - "Do emu/ps1 se PS1 nikdy
      nepodiva"). Rene sam rekl, ze tohle je na jindy - nezkoumano dal.

===============================================================================
 6. STAV TESTU B197 (pro kontext, nic se kvuli tomu v B200 nemenilo)
===============================================================================

  Rene potvrdil: trhani obrazu na projektoru (3,8 x 2,4 m) je v B197
  OPRAVENE. Zvuk na projektoru se ALE porad kouse - a to i po fixu.
  Dulezity novy fakt: na chytre TV a na PC zvuk funguje bez problemu,
  jen na projektoru ne. Diagnostika CPU/GPU cesty (gpu-gles readback u
  PS1, sdileny hlavni vlakno u TV smycky) je zapsana v historii tehle
  konverzace, ne v tomhle souboru - tohle je jen seznam uklidu.

===============================================================================
