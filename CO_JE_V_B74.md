# B74 — TV UŽ NEZDRŽUJE (versionCode 122)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno je jen to, co jde ověřit bez telefonu: překlad Javy proti
> `android.jar`, překlad C++ přes clang pro aarch64, překlad a spuštění
> rendereru proti skutečnému OpenGL ES. Chování na Mali ověřené není.

## Co se kousalo — čísla z tvého logu

    PS1_PREVIEW_AVG        avgMs=16                    (ze 77, uz dobre)
    TV_WEB_H264_FRAME_SLOW drawMs=112  w=1280 h=720    <- tady
    pomalych snimku: 1149,  median 90 ms,  maximum 586 ms
    TV_WEB_TICK_AVG        avgTickGapMs=70  (cil byl 5)

`drawMs` **není kreslení**. Je to `lockHardwareCanvas()` na vstupu
enkodéru — a ten **čeká**, dokud enkodér neuvolní buffer. Při 1280×720 to
S8 nestíhá, takže se čekalo 90 ms na snímek. Celé vlákno tím stálo a
kousal se obraz i zvuk — na mobilu i na TV.

## Oprava

Když minulý snímek trval přes 40 ms, tenhle se **vynechá**. Radši méně
snímků na TV než zadrhávající emulace. Rozlišení 1280×720 zůstává, takže
o kvalitu na TV nepřijdeš — jen se při zahlcení sníží plynulost streamu,
ne emulace.

V logu se to hlásí jako `TV_H264_VYNECHAVAM snimky, enkoder nestiha`.

## Co v tomhle buildu NENÍ

Ovladač na TV u obrazovky BIOSu a její roztažení. Podíval jsem se do kódu:
TV bere snímek přímo z jádra (`borrowFrame`) a ořezává černé okraje, takže
by se ovladač dostat neměl. Zatím nevím, kterou větví to u BIOSu jde jinudy
— na to potřebuju vidět, co v logu bude po týhle opravě, až se přestane
zahlcovat. Až to bude, dohledám to a opravím; nebudu to hádat.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |

## Co testovat

Hru na mobilu i na TV. Sleduj plynulost obrazu a zvuku.
V logu: `TV_WEB_H264_FRAME_SLOW` má výrazně ubýt a `TV_WEB_TICK_AVG`
`avgTickGapMs` má klesnout ze 70 blíž k pěti.
