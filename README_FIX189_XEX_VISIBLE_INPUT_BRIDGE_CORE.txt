FIX189_XEX_VISIBLE_INPUT_BRIDGE_CORE

Navazuje na potvrzeny FIX188:
- REAL OS/BASIC, G7 tunnel a GTIA 9/10/11 BARS zustavaji
- XEX BRIDGE DONKEY / COBRA potvrzeny
- ZVUK TEST potvrzeny

Zmena ve FIX189:
- OVLADANI MAPA uz neni jen status text nahore; je to viditelny panel.
- Panel ukazuje zive STICK0 $0278, PORTA $D300, TRIG0 $D010, STRIG0 $0284, CONSOL $D01F, CH/KBCODE.
- Panel se da zapnout jak nahore, tak dole u Atari ovladani.
- Pri drzeni joysticku/FIRE/START se hodnoty meni v panelu, takze Rene presne vidi, co emulator predava hre.
- XEX bridge a POKEY audio zustavaji beze zmeny.

Test plan:
1. G7 TUNNEL 96 TEST jen smoke.
2. ZVUK TEST jen smoke.
3. Kliknout OVLADANI MAPA - ma se objevit panel.
4. Drzet joystick a FIRE, zkontrolovat zive hodnoty.
5. XEX BRIDGE DONKEY a COBRA jen smoke.
6. Snapshot: hledat INPUT LIVE FIX189.
