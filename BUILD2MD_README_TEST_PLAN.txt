AtariHelp.eu EMU-10 BUILD2MD_DOC_HICANVAS_MAP_ZOOM_SCROLL_SAFE

ZAKLAD:
- BUILD2LR + dokumentacni opravy MA/MB/MC.
- Nejde o herni hack, zadny screen-paint, zadny filtr podle nazvu hry.
- UI / loader / klavesnice / joystick / kazeta / CLOAD / CSAVE beze zmen.
- KODY JSOU STEJNE.

PROC BUILD2MD:
- Test MC ukazal: Popeye je videt, barvy se zacinaji trefovat, ale objekty/vrstvy jsou na spatnych mistech.
- Proto MD neopravuje znovu barvy, ale SOURADNICE:
  1) Attribute Map pro HI canvas se prepocitava primo z 640px VBXE souradnic do GR.8 bunek.
  2) Blitter zoom bit3/bit7 jsou podle fx1.26 rezervovane; uz nedelaji INTLVE/mezerovani.
  3) SR hscroll v HI canvasu je 1 VBXE hires pixel, ne 2 pixely.

LOG MUSI OBSAHOVAT:
- AtariHelp.eu EMU-10 BUILD2MD_DOC_HICANVAS_MAP_ZOOM_SCROLL_SAFE pripraven
- VBXE BUILD VERIFY BUILD2MD
- VBXE HICANVAS MAP COORD DOC BUILD2MD
- VBXE BLITTER ZOOM DOC BUILD2MD
- VBXE SR HSCROLL HI DOC BUILD2MD

TEST:
1) Popeye VBXE
- Je obraz mene rozhazeny nez MC?
- Jsou plosiny/schody/sprites bliz spravnym mistum?
- Barvy zustaly stejne/lepsi, nebo se zhorsily?

2) Night Driver
- Pruh stejny/pryc/horsi?
- Auto cele/urizle?

3) W3D
- Stejne/lepsi/horsi?

POZOR:
Pokud ulozeny log neobsahuje BUILD2MD radku, neni testovan MD overlay/APK.
