// MINIMALNI OVERENI: stub WebView (jen getUrl(), presne to jedine co zmena pouziva)
// + PRESNE ty radky, ktere diff ukazal jako pridane (zkopirovano programove ze
// skutecneho souboru, ne prepsano rucne) - overuje typovou spravnost a syntaxi
// realnym javac, ne jen ocima.
class WebView {
    String url = null;
    String getUrl() { return url; }
}

public class TvTapPatchCheck {
    // presne stejne typy jako ve skutecnem MainActivity.java (int, int, String)
    volatile int napTvWebJpegQuality = 62;
    volatile int napTvWebFrameDelayMs = 55;
    volatile String napTvWebVideoProfile = "AUTO";
    WebView web = new WebView();

    // ===== BLOK 1: draw-capture cesta (napTvWebFrameTick) =====
    void blok1(boolean landscape) {
                    // BUILD2SK11: DJ obrazovka je staticka UI (jemny text, EQ tahla) - vyhody
                    // je z ostrosti vic nez z vysokeho FPS jako u emulatoru s pohybujici se hrou.
                    // V landscape proto dostane stejnou kvalitu/rozliseni jako portret misto
                    // "rychleho ale mekkeho" profilu, ktery zustava beze zmeny pro vse ostatni.
                    boolean djScreen = false;
                    try { String cu = web == null ? null : web.getUrl(); djScreen = cu != null && cu.contains("/dj/"); } catch (Throwable ignored) {}
                    // BUILD2SK12: PS1 dostava KONZERVATIVNI boost (ne az na uroven DJ) -
                    // PS1 ma rychle se menici 3D obsah, takze prilis velky skok v kvalite/
                    // rozliseni by mohl zhorsit uz existujici "trhanost" (frame rate), coz
                    // je JINA osa nez ostrost a schvalne se jí tady nedotykam (frameDelay
                    // zustava stejny jako driv i pro PS1 - testujeme jen jednu promennou).
                    boolean ps1Screen = false;
                    try { String cu2 = web == null ? null : web.getUrl(); ps1Screen = cu2 != null && cu2.contains("/emu_ps1/"); } catch (Throwable ignored) {}
                    int maxSide = landscape ? (djScreen ? 1120 : (ps1Screen ? 860 : 760)) : 1120;
                    napTvWebJpegQuality = landscape ? (djScreen ? 72 : (ps1Screen ? 62 : 54)) : 72;
                    napTvWebFrameDelayMs = landscape ? (djScreen ? 65 : 75) : 55;
                    napTvWebVideoProfile = landscape ? (djScreen ? "LANDSCAPE_DJ_HQ" : (ps1Screen ? "LANDSCAPE_PS1_HQ" : "LANDSCAPE_FAST")) : "PORTRAIT_HD";
        System.out.println(maxSide);
    }

    // ===== BLOK 2: MediaProjection cesta (napTvWebHandleSystemImage) =====
    void blok2(int w, int h) {
            boolean djScreenSys = false, ps1ScreenSys = false;
            try { String cu = web == null ? null : web.getUrl(); djScreenSys = cu != null && cu.contains("/dj/"); ps1ScreenSys = cu != null && cu.contains("/emu_ps1/"); } catch (Throwable ignored) {}
            napTvWebJpegQuality = w > h ? (djScreenSys ? 68 : (ps1ScreenSys ? 62 : 56)) : 68;
    }

    public static void main(String[] a) {
        TvTapPatchCheck t = new TvTapPatchCheck();
        // scenar 1: emulator (NE ps1, napr. Sega) - MUSI zustat na puvodnich hodnotach
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok1(true);
        System.out.println("SEGA landscape -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 54 75 LANDSCAPE_FAST - beze zmeny)");
        // scenar 2: PS1 - MUSI dostat KONZERVATIVNI boost, ne DJ-uroven
        t.web.url = "file:///android_asset/emu_ps1/index.html";
        t.blok1(true);
        System.out.println("PS1 landscape  -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 62 75 LANDSCAPE_PS1_HQ - delay NEZMENEN)");
        // scenar 3: DJ - porad ma dostavat plny boost jako v SK11
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok1(true);
        System.out.println("DJ landscape   -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 72 65 LANDSCAPE_DJ_HQ - beze zmeny od SK11)");
        // scenar 4: Atari - take musi zustat netknuty
        t.web.url = "file:///android_asset/atari/index.html";
        t.blok1(true);
        System.out.println("ATARI landscape-> quality=" + t.napTvWebJpegQuality + "  (cekano: 54 - beze zmeny)");
        // scenar 5+6: druha (system) cesta - PS1 i ne-PS1
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok2(1920, 1080);
        System.out.println("SEGA sys cesta -> quality=" + t.napTvWebJpegQuality + "  (cekano: 56)");
        t.web.url = "file:///android_asset/emu_ps1/index.html";
        t.blok2(1920, 1080);
        System.out.println("PS1 sys cesta  -> quality=" + t.napTvWebJpegQuality + "  (cekano: 62)");
    }
}
