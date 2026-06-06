AtariHelp.eu EMU-09 FIX214 SELFTEST DLIST CONSOL CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nejsou zmenene.

Cil:
- ne dalsi hra ani duha, ale skutecne rozbehnout Atari XL/XE SELF TEST z ROM.

Zmeny:
- self-test ROM okno $5000-$57FF se pri SELF TEST/BYE zrcadli z ATARIXL.ROM off $1000 i do RAM pohledu rendereru, aby ANTIC/DLIST metrika a renderer videly realny DLIST z ROM, ne prazdnou RAM.
- REALOS getDlistPtr pri self-testu preferuje SDLST $0230/$0231, kdyz ukazuje do $5000-$57FF, tedy napr. ROM DLIST $513A, misto staleho editor HW DLIST $9C20.
- CONSOL $D01F ma jednorazovy START-low pulse pro self-test smycku na $5059-$5060. Log z FIX213 ukazal PC $505E, coz je self-test ROM cekani na START bit.
- ROM SELF TEST stale nedokresluje fake obraz. Pokud uvidis SELF TEST, jde pres ROM kod + ROM DLIST.

Log radky:
- BUILD TAG FIX214_SELFTEST_DLIST_CONSOL_CORE
- FIX213 SELFTEST ROM MAP
- FIX214 SELFTEST DLIST
- FIX214 SELFTEST CONSOL

Test:
1. Klikni ROM SELF TEST.
2. Kdyz se neukaze SELF TEST menu, posli snapshot TXT.
3. Pak pripadne zkus BYE SELFTEST.
