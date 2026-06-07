# AtariHelp.eu EMU-09 FIX272_CUSTOM_BASIC_EDITOR_TURBO_XEX_KEEP_CORE

Forward fix z FIX271/FIX270/FIX267/FIX261.

Cil:
- zachovat Turbo Basic XL XEX z FIX271,
- pridat opravdovy editor pro VLASTNI BASIC kod,
- tlacitka VLOZIT musi cist aktualni textarea, ne hardcoded demo,
- zachovat line-block ATASCII/KGETCH helper po real BASIC READY,
- neopakovat fake READY/LOAD/RAM inject/screen RAM write,
- opravit reset bind tak, aby klavesnicovy RESET sel na WARM RESET pres WARMSV $E474 a nemazal BASIC program.

Turbo Basic:
- asset app/src/main/assets/turbo-basic-xl-1.5-copy.xex zachovan,
- panel Turbo BASIC XL XEX START+OPTION zachovan,
- nove tlacitko v custom BASIC panelu pouze deleguje na FIX271 Turbo XEX loader.

CLOAD/WAV poznamka:
- zatim se nedela fake zvuk ani fake LOAD,
- Atari ma pri CLOAD vlastni kazetovy zvuk; dalsi krok musi jit pres skutecnou POKEY/cassette/SIO cestu, ne pres beep jako dukaz.
