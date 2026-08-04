# B57 — HRA DO MONITORU (versionCode 105)

## Kde byla ta druhá plocha

Z tvého logu, tři řádky za sebou po načtení hry:

    GLES_INIT_OK pbuffer=1024x768                 <- kresli se do NEVIDITELNE plochy
    PS1_DRUHE_PLATNO_ZRUSENO duvod=hra bezi v nativnim okne
    PS1_GRABFRAME_HEARTBEAT n=121 gfw=0 gfh=0     <- odebrany obraz je 0 x 0

Hra se otevírala v **samostatném okně** (`NativeActivity`) na šířku. Jádro
v něm kreslilo do neviditelné plochy a odebraný snímek měl nulovou velikost —
proto černá obrazovka jen s ovladačem a zvukem. A v portrétu mezitím pořád
visel BIOS. To byly ty dvě plochy.

## Co jsem udělal

**Hra se teď spouští do TÉHOŽ monitoru jako BIOS.** Jedna zobrazovací cesta
pro obojí: jádro kreslí přes OpenGL ES a snímek jde do monitoru.

- V nativní části je z BIOSové cesty jedna společná funkce
  `ps1BootDoMonitoru(systemDir, saveDir, gamePath)`. Prázdná cesta = start bez
  disku (BIOS), vyplněná = hra. Všechno ostatní je stejné: příprava grafiky,
  zvuk, vlákno emulace.
- V Javě se místo otevření samostatného okna zavolá `bootGameSafe(...)`.
  Cesta ke hře se vezme ze složky, kam se hra nakopírovala (`.cue` má přednost,
  pak `.chd`, `.pbp`, `.iso`, `.img`).
- Otevírání `NativeActivity` je pryč.

## Smazaná druhá mrtvá plocha

`ps1GlEnable()` měla v sobě taky `if (true) return;` a pod tím vytváření
`Ps1GlTextureView` — tedy druhou plochu, která se nikdy nezapnula. Smazáno.
(Soubor `Ps1GlTextureView.java` musí zůstat, protože z něj TV používá
`borrowFrame`, ale žádný pohled se z něj už nevyrábí.)

Dohromady s B56 je tedy pryč **obojí** mrtvé zobrazování.

## Ověřeno

Složené závorky sedí ve všech třech změněných souborech
(`MainActivity.java`, `NativePs1CoreBridge.java`, `nap_ps1_native.cpp`)
a název nativní funkce sedí s tím, co volá Java.

Co ověřit nedokážu: NDK překlad a chování na telefonu.

## Co testovat

1. **PS1 bez disku** — musí naskočit BIOS v monitoru jako dosud.
2. **ISO CD z mobilu** i **LOAD GAME z netu** — hra se musí objevit
   **v tom samém monitoru**, ne v černé obrazovce na šířku.
3. V logu hledej řádek `PS1_HRA_DO_MONITORU cesta=... vysledek=PS1_HRA_OK`.

Kdyby hra nenaskočila vůbec, je to na jeden klik zpátky: GitHub Desktop ->
History -> pravým na tenhle commit -> Revert.

## Co zatím není

Grafika menu BIOSu (zelená změť). Na to jdu dál u sebe.
