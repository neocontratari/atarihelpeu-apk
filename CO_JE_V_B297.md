# Co je v B297

Tvá slova po B296: „Běží to skvěle – budu dál testovat – ty mezitím prosím udělej
přetočení na široko a přidej tam d-pad podobný jako u Segy i s tím, aby si ho
uživatel opět mohl nastavit podle sebe.“

## Na šířku

Když v HELP otočíš telefon na šířku, místo malého přístroje se ukáže:

- **obraz Atari přes celou výšku displeje** (stejný poměr stran jako na výšku,
  ostrý – na šířku se teď kreslí v plném rozlišení displeje, kratší strana až
  1080 bodů),
- **vlevo dole D-pad** – kulatý „thumbstick“ jako u Segy, puntík jede pod
  palcem, 8 směrů (i šikmo), malá mrtvá zóna uprostřed,
- **vpravo sloupec FIRE, MEZERA, RETURN** (FIRE oranžové, největší),
- **vlevo nahoře START, SELECT, OPTION** (ne dole uprostřed – tam by zakrývaly
  spodek obrazu, třeba stavový řádek ve Wolfensteinu),
- **vpravo nahoře kolečko** = menu **D-PAD A OVLÁDÁNÍ**.

Vzhled je jako u Segy: průhledné modré sklo, stisknuté tlačítko svítí zlatě,
při stisku krátká vibrace. Funguje víc prstů najednou (D-pad + FIRE + MEZERA).

## Nastavení podle sebe (jako u Segy)

Kolečko vpravo nahoře:

- **UPRAVIT ROZLOŽENÍ TLAČÍTEK** – tlačítka dostanou čárkovaný rámeček a jdou
  přetáhnout prstem. Nahoře je lišta **PO SKUPINÁCH** (FIRE/MEZERA/RETURN, nebo
  START/SELECT/OPTION se táhnou najednou; D-pad vždy zvlášť), **VÝCHOZÍ** a
  **HOTOVO** (uloží).
- **OVLÁDÁNÍ: CITLIVOST A VZHLED** – citlivost D-padu, průhlednost, velikost
  tlačítek, velikost D-padu (samostatně), vibrace, VÝCHOZÍ NASTAVENÍ.
- **PROHODIT D-PAD A AKCE (LEVÁK)**, **VÝCHOZÍ ROZLOŽENÍ TLAČÍTEK**.
- Navíc **RESET ATARI**, **XEX/MOBIL**, **ATR/DISK**, **NET/HRY** a nápověda –
  na šířku nemusíš kvůli hře otáčet zpátky.

Nastavení si appka pamatuje (i po vypnutí).

## Na výšku

Beze změny: přístroj Atari, joystick na obrazovce (levá půlka směr, pravá FIRE),
herní ovladač funguje na výšku i na šířku. Přístroj na výšku je bod po bodu
stejný jako v B296.

## Ověřeno na počítači

- Obrázky na šířku (2400×1080, 1920×1080, 1440×1080) s Wolfensteinem a Popeyem –
  kontroloval jsem je já.
- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: **48 kontrol, 0 chyb** –
  z toho 21 na šířku: D-pad nahoru i šikmo, FIRE současně se směrem, MEZERA,
  START, vibrace, kolečko, přetažení FIRE v úpravě rozložení, HOTOVO, uložení,
  levák, velikost a citlivost, VÝCHOZÍ, otočení zpět na výšku.
- Snímek na šířku se na počítači kreslí za ~3 ms (telefon bude pomalejší, ale
  kreslí se v jiném vlákně než emulace).
