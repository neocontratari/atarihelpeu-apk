README FIX249 CASSETTE SCAFFOLD SELFTEST COLORROLE CORE

Cil:
- zachovat FIX248 ROM-bus DLIST opravu pro SELF TEST,
- zkusit lepsi SELF TEST color-role render: playfield pozadi COLPF2, border COLBK,
- pridat virtualni kazetak scaffold: INSERT WAV/CAS, PLAY, STOP, REWIND, CLOAD, CLOAD + RUN.

Bez fake:
- CLOAD tlacitko jen posle prikaz CLOAD do real BASIC po READY pres stavajici servisni editor helper.
- WAV/CAS se zatim nedekoduje a neposila data do RAM.
- Kazetak zatim loguje stav zarizeni, motor, play/stop, vlozeny soubor a epoch.
- Zadny fake LOAD, zadny fake READY, zadny fake SELF TEST.

Log hledej:
BUILD FIX249_CASSETTE_SCAFFOLD_SELFTEST_COLORROLE_CORE
FIX249 CASSETTE/SelfTest CHECK
SELFTEST ... color-role renderer ...
CASSETTE STATE inserted ... play ... motor ...
RULE FIX249_SELFTEST_COLORROLE_COLPF2_BG / ROM_BUS_DLIST / RAM_DMA_DATA / CASSETTE_SCAFFOLD_ONLY / WAV_CAS_INSERT_PLAY_STOP_REWIND / CLOAD_COMMAND_ONLY_AFTER_READY / NO_FAKE_LOAD_READY_SELFTEST

KODY JSOU STEJNE
