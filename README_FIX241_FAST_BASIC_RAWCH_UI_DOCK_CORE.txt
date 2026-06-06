AtariHelp.eu EMU-09 FIX241_FAST_BASIC_RAWCH_UI_DOCK_CORE

CIL:
- Opravit FIX240 stav, kdy FAST BASIC tlacitka vypadala mrtve: log ukazoval READY OK a fastQ 2205, ale fronta se v PURE ROM BASIC nedrenovala.
- Zachovat cisty PURE HARDWARE: zadne fake READY, zadny fake SELF TEST, zadny PC jump.
- Presunout mobilni Atari klavesnici primo pod obrazovku.

ROOT CAUSE:
- FIX240 armoval ATASCII/KGETCH frontu, ale realny ROM BASIC/editor v PURE HW ceste cetl CH $02FC primo.
- Stary E420/E424 service shim se proto nemusel zavolat, fastQ zustala plna a uzivatel nevidel zadnou reakci.

ZMENY:
- RAW CH $02FC pumpa: kdyz ROM cte idle CH a FAST fronta je aktivni, FIX241 poda dalsi ATASCII jako jednorazovy CH edge.
- READY gate zustava: DLIST $9C20, SAVMSC $9C40, BASIC window, VBI, SELFTEST OFF.
- EOL zustava ATASCII $9B.
- Touch/click bind pro BASIC TXT FAST, VLOZIT FAST, VLOZIT FAST + RUN, SUBMARINE FAST.
- Klavesnice grid je presunuta hned pod obrazovku; servisni tlacitka a FAST box jsou pod ni.
- SELF TEST pouziva stale FIX240 CONSOL START edge, zadny fake screen.

LOG:
- BUILD FIX241_FAST_BASIC_RAWCH_UI_DOCK_CORE
- FIX241 FAST BASIC RAW-CH/UI CHECK
- FASTPUMP RAW CH FEED / QUEUE COMPLETE
- UI keyboard grid moved directly under screen
- RULE FAST_BUTTON_TOUCH_CLICK_BOUND / KEYBOARD_DIRECTLY_UNDER_SCREEN / RAW_CH_02FC_ATASCII_PUMP / READY_GATED / EOL_9B / NO_FAKE_READY_SELFTEST

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
