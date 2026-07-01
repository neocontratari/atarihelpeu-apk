BUILD2QX_SEGA_NATIVE_CPP_ONLY_MOBILE_LANDSCAPE_RECOVER_STAGE114

CIL:
- Zachovat dobrou opravu z QW: v Noxu uz po navratu z Atari 130XE nema byt cerna obrazovka.
- Odstranit viditelne tlacitko C++ CORE ze Sega obrazovky.
- Na S8/A12 opravit landscape: ne horni napisy a cerna plocha, ale native video pres obraz + pruhledny joystick nad nim.
- Nehybat uz se zvukem: audio profil zustava QT/QP keep.

TEST NOX:
1. Aplikuj overlay pres BUILD2QW.
2. Spust appku -> SEGA MEGA DRIVE.
3. Over, ze tlacitko C++ CORE nahore neni videt.
4. SBIRKA/CARTRIDGE -> Sonic.
5. Odejdi do Atari 130XE.
6. Vrat se do Sega -> SBIRKA/CARTRIDGE -> Sonic nebo Aladdin.
Ocekavani: zadna cerna obrazovka se zvukem.

TEST S8/A12:
1. Spust Sega -> SBIRKA/CARTRIDGE -> Sonic.
2. Pockej na obraz.
3. Otoc mobil na landscape.
Ocekavani:
- zadne viditelne horni texty SBIRKA/ULOZENE/HRY/NAVODY/WEB/MENU/CART/C++ CORE,
- obraz hry ma byt pod pruhlednym joystickem a A/B/C,
- joystick nesmi byt oddeleny dole mimo video,
- po navratu na vysku nesmi zustat cerna obrazovka.

LOG MARKERY:
- BUILD2QX_SEGA_NATIVE_CPP_ONLY_MOBILE_LANDSCAPE_RECOVER_STAGE114
- NATIVE_CPP_BUTTON_REMOVED
- VISIBILITY_HIDDEN_IGNORED_MOBILE_NATIVE
- MOBILE_NATIVE_VIEW_RECOVER orientationchange
- NATIVE_RECT_DPR ... landscapeFullVideo=YES
- NATIVE_Z_ORDER_QX landscapeWebControlsOverNative=YES topLabelsHidden=YES
- NATIVE_AUDIO_STREAM_START_QX_QT_AUDIO_KEEP

KDYZ TO SELZE:
Posli prosim log a screenshot jen z S8/A12:
- po otoceni do landscape,
- po navratu na vysku,
- a napis, jestli zvuk bezi a obraz je cerny, nebo jestli nejde ani zvuk.
