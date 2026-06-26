AtariHelp.eu EMU-10 BUILD2KI_CPU_IO_WRITE_CYCLE_GTIA_RASTER_CORE

STAV:
- Toto neni kosmeticky ZIP a neni to rollback.
- Zaklad je KH/KG/KF/KE/KB/JZ/JX vetev.
- Commando Arnold cesta zustava chranena.
- Mission/Tetris/VBXE zustava chranene.
- KD PMBASE zasah je porad pryc.
- KODY JSOU STEJNE.

CO JE OPRAVENO V KODU:
- 6502 store instrukce STA/STX/STY do pametove mapovaneho IO uz nedavaji GTIA/ANTIC/PMG zapis do casu zacatku instrukce.
- BUILD2KI prida realny write-cycle timestamp pro store opkody:
  STA (zp,X), STA zp, STX zp, STY zp, STA abs, STX abs, STY abs, STA (zp),Y, STA abs,Y, STA abs,X.
- GTIA/ANTIC segmenty COLPM/COLPF/COLBK/PRIOR, HPOS/SIZE/GRAF/GRACTL a CHBASE ted pouzivaji tento skutecny write-cycle cas.
- Cil: G2F/Postcard svisle pruhy a obecny DLI/PMG raster timing. Predtim se zapisy kreslily moc brzy, protoze cpu.cycles byl jeste na zacatku opcode.

TEST MINIMUM:
1) Nahraj overlay pres GitHub Desktop.
2) Spust appku.
3) V LOGu musi byt:
   AtariHelp.eu EMU-10 BUILD2KI_CPU_IO_WRITE_CYCLE_GTIA_RASTER_CORE pripraven
4) Spust test_assets/Postcard_Atari_Rocky.xex.
5) Porovnej proti Altirra:
   - jestli zmizely / zmensily se fialove a tyrkysove svisle pruhy,
   - jestli se obraz barevne vic podoba Altirre,
   - jestli neni horsi nez KH.
6) Pokud Postcard nema viditelny posun, stop a posli screenshot + LOG.

VOLITELNE AZ PO POSTCARD POSUNU:
- Night Driver: auto ano/ne, caj u auta pryc/mensi/stejny, zvuk ano/ne.
- Decathlon: lepsi/stejny/horsi.
- Commando: Arnold ANO + hra ANO.
- Mission: nabehne ano/ne, grafika lepsi/stejna/horsi.

NEKONTROLOVAT RUCNE V LOGU:
Tester jen posle LOG / CHYBA. Programator hleda markery sam.

OCEKAVANE MARKERY:
- BUILD2KI_CPU_IO_WRITE_CYCLE_GTIA_RASTER_CORE
- GTIA COLOR/PRIOR SEG BUILD2IT s posunutym realnym cyklem zapisu
- BUILD2JT PM SUMMARY Postcard_Atari_Rocky.xex

POZNAMKA:
Tenhle build je presnejsi raster timing rez. Pokud ani Postcard nepohne, dalsi krok neni filtr ani priorita, ale CPU cycle-by-cycle write schedule pro vsechny RMW/store varianty a pripadne NMI/WSYNC rozpad po jednotlivych cyklech.
