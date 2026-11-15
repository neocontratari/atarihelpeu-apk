# Co je v B323

Tvoje zpráva: „Zjistil jsem, že nám v Atari nefungují 2 hry – žádná katastrofa – ale opatrně, ať
nerozbijeme skvěle fungující emu.“ A: „Nechápu, proč nejde uzamknout Segu a PS1 i bez platby
(kvůli testům).“

## 1. Air Strike jde

- **Proč nešel:** je to stará hra z bootovací diskety převedená do XEX. Její zavaděč si nastaví start
  hry a udělá „teplý start“ Atari. Atari ten start zavolá, jen když si myslí, že je zavedený DOS.
  Naše Atari pouštělo XEX bez DOSu, takže start přeskočilo a hra skončila na černé obrazovce.
- **Oprava:** při spuštění XEX teď Atari vypadá jako po zavedení DOSu (jako na skutečném Atari
  s disketou DOS). Air Strike ukáže titulek, START → hrací plocha, START → hraje se.

## 2. Sonic jde

- **Proč nešel:** hra si změří paměť a napíše „NOT ENOUGH EXTENDED MEMORY – REQUIRES 320K“.
  Chce 320 kB paměti, Atari 130XE má 128 kB.
- **Oprava:** Atari má teď rozšíření paměti **320K** (stejné jako známé rozšíření Compy Shop).
  Čtyři banky 130XE jsou přesně tam, kde byly – hry pro 130XE to nepoznají. Hry, které chtějí víc
  (Sonic), mají dalších 12 bank. Sonic ukáže úvod, titulek a hraje se.

## 3. Opatrně – co jsem ověřil

- **16 programů** (hry z testů a tvoje: Ghostbusters, KITT, Wonderboy VBXE, Popeye VBXE, Night Driver
  VBXE, Decathlon, Turbo-BASIC XL, Wolfenstein z diskety…), 40 vteřin hraní se stejným mačkáním
  (START, FIRE, joystick): starý a nový emulátor dají **úplně stejný obraz** (kontrolní součet obrazu
  každé 2 vteřiny). Změnily se jen Air Strike a Sonic.
- Všechny starší testy jádra (start, BASIC, kazeta, disketa, cartridge, VBXE, klávesnice, přenos na TV)
  dají ve starém i novém jádru **stejné výsledky**.
- Samokontrola jádra v telefonu má nové číslo paměti (16 bank místo 4) – v appce je už nastavené.
- Nápověda: „Atari 130XE se 320 kB paměti“.

## 4. 🔒 ZAMKNOUT (TEST) ze serveru (server v11.2)

- V okně **KÓDY A TELEFONY** je nové tlačítko **🔒 ZAMKNOUT (TEST)**. Vyber řádek, klikni a zvol:
  - **🔒 SEGA i PS1** – telefon zamkne obě a vynuluje řádky i hodiny (jako servis ZAMKNOUT VŠE),
  - **🔒 jen PS1** – Sega zůstane, PS1 se zamkne a hodiny Segy jsou od nuly.
- Funguje i u Segy / PS1 otevřené **zadarmo** (1500 řádků, 120 hodin nebo ještě z B321).
  Zaplacené odemčení a čekající platba se tím taky smažou.
- Telefon zámek provede **při příštím spojení se serverem** (spuštění appky, obrazovka ODEMKNOUT)
  a napíše „Správce N&P zamkl … (test)“. V tabulce do té doby svítí **„🔒 zamyká se – čeká na
  telefon“**, v logu pak **„🔒 ZAMČENO … provedl“**.
- Potřebuje **appku B323** – stará appka zámek neumí (dokud si ji telefon neaktualizuje, server její
  postup nebere a zámek čeká).

## Na rovinu

- **Ověřeno na PC**, na skutečném jádru appky: Air Strike a Sonic se hrají (obraz se mění, hra reaguje
  na START / FIRE / joystick). Server v11.2: 214 kontrol, třídy appky proti skutečnému serveru 18 + staré 42.
- **Na telefonu zatím ne.** Hlavně Sonic je náročný (velká hra, hodně paměti) – zkus, jestli běží plynule.
- Další hry, které chtějí 320K, by teď měly jít taky. Hry, které chtějí ještě víc (576K, 1088K), ne.
