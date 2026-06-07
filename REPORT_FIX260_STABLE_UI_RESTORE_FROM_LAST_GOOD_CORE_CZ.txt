FIX260_STABLE_UI_RESTORE_FROM_LAST_GOOD_CORE

Commit summary:
FIX260 stable UI restore from last good core

CIL:
- krizovy navrat pred UI chaosem FIX253-FIX258,
- nepouzivat FIX258 jako stabilni zaklad,
- vyjit z posledni stabilni vetve FIX251 + zachovat poznatek FIX248 pro SELF TEST ROM-bus DLIST,
- obnovit normalni AtariHelp.eu strukturu: EMU-09 jako pododdil, jedna klavesnice hned pod obrazem,
- snapshot vratit tak, aby byl videt v TXT textarea + download fallback,
- vynutit cisty idle profil realos misto defaultniho pitt pri prvnim snapshotu bez XEX,
- kazetak v tomhle recovery kroku vypnout jako UI funkci; TurboBasicXXL.wav asset zustava v APK pro dalsi cisty krok.

NEDELA:
- zadny fake READY,
- zadny fake SELF TEST,
- zadny fake LOAD,
- zadny RAM inject,
- zadny SIO skok,
- zadny herni hack.

TEST PLAN:
1) Rozbal ZIP overlay do lokalniho repo.
2) Commit: FIX260 stable UI restore from last good core
3) GitHub Actions postavi APK.
4) Spust AtariHelp.eu a otevri EMU-09.
5) Zkontroluj UI screenshotem:
   - AtariHelp.eu index ma normalni oddily,
   - EMU-09 je pododdil,
   - obraz Atari je nahore,
   - jedna klavesnice je hned pod obrazem,
   - nejsou zadne duplicitni panely FIX253-FIX258,
   - kazetak ukazuje VYPNUTO PRO RECOVERY.
6) Hned dej SNAPSHOT pred bootem.
   Hledej:
   - BUILD FIX260_STABLE_UI_RESTORE_FROM_LAST_GOOD_CORE,
   - profile=realos,
   - UI_CHAOS_FIX253_258_ELEMENTS=0,
   - KEYBOARD_COUNT=1.
7) POWER XL/XE BASIC.
8) Otestuj READY a klavesnici: jedno pismeno = jeden znak, RETURN = jeden radek.
9) Zadej ? FRE(0) a porovnej s milnikem FIX239: ma zustat 37902.
10) Dej SNAPSHOT po BASICu a posli TXT + screenshot.
11) POWER OPTION SELF TEST jen informacne: necekame fake hotovou obrazovku, ale zachovava se ROM-bus DLIST cesta z FIX248.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci programy nejsou menene.
