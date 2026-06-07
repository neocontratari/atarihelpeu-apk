AtariHelp.eu EMU-09 FIX262_CASSETTE_AUDIO_VISIBLE_WAV_TO_POKEY_CORE

Cil:
- zachovat stabilni UI z FIX261,
- odstranit posledni viditelne probliknuti starych nadpisu opakovanym relockem,
- udelat kazetu srozumitelnou pro uzivatele: jeden panel, motor, audio, progress, stav decode, stav POKEY serial,
- opravit nacitani built-in TurboBasicXXL.wav: zkousi TurboBasicXXL.wav bez ?query, ./ cesta, aktualni asset cesta a file:///android_asset cesta; fetch i XHR fallback,
- pridat realne WebView audio prehravani WAV + zvukovy beep test,
- pridat prvni cisty WAV->POKEY SERIN pokus: WAV se dekoduje na serialni bajty a OS/BASIC je dostava pres POKEY SERIN $D20D / IRQST $D20E, NE pres RAM inject.

Co to neni:
- neni fake READY,
- neni fake SELF TEST,
- neni fake LOAD,
- neni zapis TurboBasicu do RAM,
- neni herni hack.

Poznamka:
FIX262 muze jeste TurboBasic nenahrat, pokud ROM C: rutina potrebuje presnejsi cassette timing/IRQ/STIMER chovani. V tom pripade snapshot ukaze, jestli se nacetl WAV, bezi audio, kolik serial bytes bylo dekodovano a jestli OS cetl IRQST/SERIN.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody zustavaji beze zmen.
