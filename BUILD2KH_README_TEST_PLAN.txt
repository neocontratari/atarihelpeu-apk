AtariHelp.eu EMU-10 BUILD2KH_DEEP_GTIA_PMG_VDELAY_OVERLAP_CORE

Ucel:
- Navazuje na KG, ale KG bereme jako neuspesny pokus.
- Tohle neni dalsi mikro-guard pro Night Driver.
- Je to hlubsi GTIA/PMG jadro podle dodane dokumentace a Postcard testu.

Zaklad:
- KB/JZ/JX/KE/KF/KG kodovy zaklad.
- Commando Arnold + hra musi zustat chranene.
- Mission musi zustat spustitelna jako v KB/KE.
- KD PMBASE zasah zustava pryc.
- KODY JSOU STEJNE.

Zmeny:
1) GTIA VDELAY
   - Predchozi kod pouzival logiku "pri VDELAY ber DMA jen na lichych radcich".
   - BUILD2KH prepina podle dodane dokumentace na predchozi DMA hodnotu:
     VDELAY bit = pouzij predchozi PM DMA bajt, aktualni uloz pro dalsi scanline.
   - CIL: G2F/Postcard PMG vrstvy a obecne PMG multiplexy.

2) GTIA player overlap color
   - PRIOR bit5 (0x20) kresli prekryv paru P0/P1 a P2/P3 jako OR COLPM barev.
   - Bez bit5 je prekryv paru cerny.
   - CIL: G2F obrazky pouzivajici PMG jako barvici vrstvy.

Neopravuji / nelakuji:
- Night Driver stale neprohlasuji za opraveny.
- Decathlon stale neprohlasuji za opraveny.
- Zvuk Night Driveru stale neprohlasuji za opraveny.

Test bez kolecka:
1) Nahraj BUILD2KH overlay.
2) Spust Postcard_Atari_Rocky.xex z test_assets.
3) Porovnej proti Altirre.
4) Pokud neni viditelny posun, stop: neposilej dalsi hry, posli jen LOG + screenshot.
5) Pokud je viditelny posun, potom az kratce:
   - Commando: Arnold ANO + hra ANO
   - Mission: nabehne ano/ne
   - Night Driver: auto/pruh/zvuk jen orientacne

Log markery:
- AtariHelp.eu EMU-10 BUILD2KH_DEEP_GTIA_PMG_VDELAY_OVERLAP_CORE pripraven
- GTIA VDELAY BUILD2KH
- GTIA PLAYER OVERLAP BUILD2KH
