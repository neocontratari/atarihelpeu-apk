AtariHelp.eu EMU-09
FIX242_SAFE_FAST_BASIC_THROTTLE_SELFTEST_CONSOL_CORE

CIL:
- Opravit rozstrel BASIC vypisu po FIX241 FAST BASIC RAW CH pumpe.
- Zastavit automaticky SELF TEST START edge, ktery mohl poslat ROM do cerne dalsi faze.
- Zachovat cisty PURE HW: zadny fake READY, zadny fake SELF TEST, zadny PC jump, zadne prime zapisovani programu do obrazovky nebo BASIC pameti.

ROOT CAUSE PODLE LOGU:
- FIX241/FIX240 fronta sice vlozila vsechny znaky, ale ROM editor cetl CH $02FC vicekrat v jedne emulacni frame fazi, takze se pumpovalo moc rychle a vznikaly prehozy typu POSTTION, SOUDD, zdvojene znaky a ERROR.
- SELF TEST automaticky START edge z FIX240 byl prilis agresivni. Mohl preskocit viditelnou menu cestu a poslat self-test do cerne ROM faze.

ZMENY:
1) SAFE FAST BASIC QUEUE:
- FIX242 vlastni frontu; stare FIX240/FIX241 KGETCH/RAW pumpy jsou pri startu fronty vypnute.
- Znaky se podavaji jen pri skutecnem ROM cteni CH $02FC.
- Maximalne 1 znak za emulacni frame.
- ATASCII EOL $9B ma delsi pauzu pro tokenizaci radku.
- Zadny frame seed; nic se nesype mimo ROM keyboard polling.
- READY gate zustava: DLIST $9C20, SAVMSC $9C40, NMIEN VBI, BASIC window ON, SELFTEST OFF.

2) SELF TEST:
- Automaticky START-low edge je blokovan.
- CONSOL $D01F se vraci ciste podle realneho vstupu: OPTION drzi self-test okno, START jen kdyz uzivatel skutecne pouzije HOLD START.
- Zadny fake SELF TEST a zadny PC jump.

3) UI:
- FAST BASIC tlacitka jsou znovu bindnuta na FIX242 SAFE frontu.
- Klavesnice zustava primo pod obrazovkou.

LOG HLEDEJ:
BUILD FIX242_SAFE_FAST_BASIC_THROTTLE_SELFTEST_CONSOL_CORE
FIX242 SAFE FAST BASIC/SELFTEST CHECK
FAST FAST BASIC QUEUE START ... SAFE throttle 1 char/frame EOL delay 10
FAST SAFE RAW CH FEED ...
LAST QUEUE COMPLETE ... SAFE_THROTTLED_RAW_CH ... LATCH CLEAR
SELFTEST SELFTEST CONSOL passthrough ... AUTO_START_BLOCKED ...
RULE SAFE_FAST_THROTTLED_RAW_CH / MAX_ONE_CHAR_PER_FRAME / EOL_9B_DELAY / NO_FRAME_SEED / SELFTEST_AUTO_START_BLOCKED / CONSOL_PASSTHROUGH / NO_FAKE_READY_SELFTEST

TEST PLAN:
1) POWER XL/XE BASIC.
2) VLOZIT FAST s kratkym programem:
   10 PRINT "AHOJ PARTAKU"
   20 GOTO 20
   Ocekavani: listing bez prehazovani pismen, bez ERROR u cisteho radku.
3) VLOZIT FAST + RUN.
4) SUBMARINE FAST - bude pomalejsi nez FIX241, ale nesmi rozsypat kod.
5) SNAPSHOT - poslat TXT.
6) POWER OPTION SELF TEST - nema automaticky skocit do cerne; pokud zustane cekat, zkus HOLD START rucne a posli snapshot.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
