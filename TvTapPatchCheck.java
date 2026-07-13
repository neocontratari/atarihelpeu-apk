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
                    int maxSide = landscape ? (djScreen ? 1120 : 760) : 1120;
                    napTvWebJpegQuality = landscape ? (djScreen ? 72 : 54) : 72;
                    napTvWebFrameDelayMs = landscape ? (djScreen ? 65 : 75) : 55;
                    napTvWebVideoProfile = landscape ? (djScreen ? "LANDSCAPE_DJ_HQ" : "LANDSCAPE_FAST") : "PORTRAIT_HD";
        // pouzij maxSide, aby "unused variable" nebylo hlaseno jako chyba (v realnem
        // souboru se maxSide pouziva dal v teto metode - tady jen simulujeme pouziti)
        System.out.println(maxSide);
    }

    // ===== BLOK 2: MediaProjection cesta (napTvWebHandleSystemImage) =====
    void blok2(int w, int h) {
            boolean djScreenSys = false;
            try { String cu = web == null ? null : web.getUrl(); djScreenSys = cu != null && cu.contains("/dj/"); } catch (Throwable ignored) {}
            napTvWebJpegQuality = w > h ? (djScreenSys ? 68 : 56) : 68;
    }

    public static void main(String[] a) {
        TvTapPatchCheck t = new TvTapPatchCheck();
        // scenar 1: neni DJ (napr. emulator) - MUSI zustat na puvodnich hodnotach
        t.web.url = "file:///android_asset/emu/index.html";
        t.blok1(true);
        System.out.println("EMULATOR landscape -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 54 75 LANDSCAPE_FAST)");
        // scenar 2: JE DJ pult - MUSI dostat boost
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok1(true);
        System.out.println("DJ landscape      -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 72 65 LANDSCAPE_DJ_HQ)");
        // scenar 3: druha (MediaProjection) cesta, DJ i ne-DJ
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok2(1920, 1080);
        System.out.println("EMULATOR sys cesta -> quality=" + t.napTvWebJpegQuality + "  (cekano: 56)");
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok2(1920, 1080);
        System.out.println("DJ sys cesta        -> quality=" + t.napTvWebJpegQuality + "  (cekano: 68)");
    }
}
