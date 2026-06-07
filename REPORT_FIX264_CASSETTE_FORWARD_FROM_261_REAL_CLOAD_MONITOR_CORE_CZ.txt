AtariHelp.eu EMU-09 FIX264_CASSETTE_FORWARD_FROM_261_REAL_CLOAD_MONITOR_CORE

SMER DOPREDU, NE ROLLBACK.
Zaklad: funkcni FIX261 UI lock + single cassette panel.
FIX262 audio/WAV->POKEY vrstva NENI pouzita.

Cil:
- zachovat stabilni appku z FIX261,
- opravit built-in TurboBasicXXL.wav insert pres fetch/XHR fallbacky,
- ukazat uzivateli co kazeta dela: decode, bajty, IRQ/SERIN cteni,
- pustit WAV jako slyšitelny audio/progress pokud WebView dovoli,
- zkusit poctivy CLOAD: WAV se dekoduje na serialni bajty a OS/BASIC je cte pres POKEY SERIN $D20D a IRQST $D20E.

NO FAKE:
- zadny fake READY,
- zadny fake SELF TEST,
- zadny fake LOAD,
- zadny RAM inject TurboBasicu,
- zadny hotovy BASIC program do pameti,
- zadny herni hack.

Stav pravdy:
Tohle je prvni WAV->byte serial pilot. Neni jeste plna analogova POKEY bitova fyzika.
Pokud CLOAD zustane viset, snapshot ma rict, jestli se WAV nacetl, dekodoval, a jestli ROM cte IRQST/SERIN.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
