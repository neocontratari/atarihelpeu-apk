BUILD2SA13_STAGE201_TV_OUTPUT_WHOLE_APP (obsahuje SA12B; +SA11 memcard zvlast)

VLASTNI TV VYSTUP PRO CELOU APKU (plan bod 5, povyseny):
- Appka SAMA hlida pripojene displeje (DisplayManager) a kdyz se objevi
  TV/monitor, VYKRESLUJE NA NEJ SVUJ OBRAZ - vsechny emulatory (Atari pilot,
  Sega, PS1), prehravac i stranky. Zadna externi aplikace.
- ~25 fps, dvojity buffer, obraz se meritkem prizpusobi TV (FIT_CENTER,
  cerne okraje). Telefon zustava plne funkcni jako ovladac.
- JAK PRIPOJIT: a) HDMI/USB-C adapter do telefonu = nejplynulejsi;
  b) bezdratove: pripoj TV pres systemove pripojeni obrazovky - jakmile
  system displej prida, appka ho PREVEZME vlastnim vystupem.
- Markery: TV_MIRROR_ON display=... / TV_MIRROR_OFF.
POCTIVE: prvni verze zrcadli obraz apky (vc. UI). "Cisty rezim" (na TV jen
hra bez ovladacich prvku) je pripraveny dalsi krok - rekni po testu.

JOYSTICK (bluetooth, pro vsechny emu): plumbing mam rozmysleny (Android
KeyEvents -> aktivni jadro), ale bez tveho gamepadu ho nemam jak poctive
otestovat - JAKMILE DOKOUPIS, posli nazev/typ a udelame ho na miru,
vcetne Atari (pilot!).

TEST: pripoj TV (adapter nebo bezdratove) -> na TV se objevi obraz apky ->
spust hru (Sega i PS1) -> hra bezi na TV, telefon = ovladac.
Log: TV_MIRROR_ON. Kdyby obraz na TV chybel, posli log + jak je TV pripojena.

BUILD2SA13C - TV WEB CAST FALLBACK (kdyz Smart View/Miracast pada):
- V PS1 panelu je tlacitko TV WEB CAST. Telefon spusti lokalni HTTP stream
  obrazu appky pres Wi-Fi a vypise URL typu http://192.168.x.x:8765/.
- Tuhle adresu otevri na TCL/Android TV v prohlizeci nebo receiver appce.
  Je to workaround pro TV, ktera umi Chromecast/Google Home, ale neudrzi
  Samsung Smart View/Miracast. Telefon zustava ovladac.
- BUILD2SA13C2 pouziva primarne MJPEG stream a na Androidu 8+ PixelCopy
  ze skutecneho okna. V obrazu je maly napis LIVE cas #frame PIXELCOPY/DRAW:
  kdyz se cas a cislo hybou, stream je zivy.
- Neni to pravy YouTube Cast receiver: bez Google Cast receiver aplikace
  Android appka nemuze sama spustit vlastni HTML/emulator na Chromecastu.
- Markery: BUILD2SA13C TV_WEB_CAST_ON url=... / TV_WEB_CAST_OFF.
