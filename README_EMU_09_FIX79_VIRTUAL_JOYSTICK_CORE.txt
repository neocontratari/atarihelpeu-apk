EMU-09 FIX79 VIRTUAL JOYSTICK + GALAXIAN CORE

Cíl FIX79:
- upravit mobilní křížové ovládání na kruhový virtuální joystick jako v novějších mobilních hrách;
- uživatel drží prst v kolečku a pohybem po kruhu drží směr trvale sepnutý;
- podporovat diagonály;
- nechat stará tlačítka jako nouzový fallback;
- nezasahovat do emulačního jádra víc, než je nutné.

Test:
- na mobilu zkus držet joystick vlevo/vpravo/nahoru/dolů a jezdit po kruhu bez pouštění prstu;
- sleduj input readout UP/DOWN/LEFT/RIGHT;
- hry mají vnímat směr jako držený joystick, ne jako jednotlivé tapy.
