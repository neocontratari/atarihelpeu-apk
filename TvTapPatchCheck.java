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
                    boolean djScreen = false;
                    try { String cu = web == null ? null : web.getUrl(); djScreen = cu != null && cu.contains("/dj/"); } catch (Throwable ignored) {}
                    boolean hqLiteScreen = false;
                    try { String cu2 = web == null ? null : web.getUrl(); hqLiteScreen = cu2 != null && (cu2.contains("/emu_ps1/") || cu2.contains("/emu_sega/") || cu2.contains("/emu/")); } catch (Throwable ignored) {}
                    int maxSide = landscape ? (djScreen ? 1120 : (hqLiteScreen ? 860 : 760)) : 1120;
                    napTvWebJpegQuality = landscape ? (djScreen ? 72 : (hqLiteScreen ? 62 : 54)) : 72;
                    napTvWebFrameDelayMs = landscape ? (djScreen ? 65 : 75) : 55;
                    napTvWebVideoProfile = landscape ? (djScreen ? "LANDSCAPE_DJ_HQ" : (hqLiteScreen ? "LANDSCAPE_EMU_HQ" : "LANDSCAPE_FAST")) : "PORTRAIT_HD";
        System.out.println(maxSide);
    }

    // ===== BLOK 2: MediaProjection cesta (napTvWebHandleSystemImage) =====
    void blok2(int w, int h) {
            boolean djScreenSys = false, hqLiteScreenSys = false;
            try { String cu = web == null ? null : web.getUrl(); djScreenSys = cu != null && cu.contains("/dj/"); hqLiteScreenSys = cu != null && (cu.contains("/emu_ps1/") || cu.contains("/emu_sega/") || cu.contains("/emu/")); } catch (Throwable ignored) {}
            napTvWebJpegQuality = w > h ? (djScreenSys ? 68 : (hqLiteScreenSys ? 62 : 56)) : 68;
    }

    public static void main(String[] a) {
        TvTapPatchCheck t = new TvTapPatchCheck();
        t.web.url = "file:///android_asset/emu_ps1/index.html";
        t.blok1(true);
        System.out.println("PS1 landscape   -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 62 75 LANDSCAPE_EMU_HQ)");
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok1(true);
        System.out.println("SEGA landscape  -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 62 75 LANDSCAPE_EMU_HQ - NOVE boostnuto)");
        t.web.url = "file:///android_asset/emu/index.html";
        t.blok1(true);
        System.out.println("ATARI landscape -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 62 75 LANDSCAPE_EMU_HQ - NOVE boostnuto)");
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok1(true);
        System.out.println("DJ landscape    -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 72 65 LANDSCAPE_DJ_HQ - beze zmeny)");
        // kontrola falesne shody: emu_ps1/emu_sega NESMI spadnout pod DJ check, uz overeno vyse ze
        // spravne spadaji pod hqLiteScreen. Tady navic over ze "/emu/" substring test nezachyti
        // omylem neco jineho jako "design" nebo "player" slozku:
        t.web.url = "file:///android_asset/player/index.html";
        t.blok1(true);
        System.out.println("PLAYER landscape-> quality=" + t.napTvWebJpegQuality + "  (cekano: 54 - NEMA se chytit)");
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok2(1920, 1080);
        System.out.println("SEGA sys cesta  -> quality=" + t.napTvWebJpegQuality + "  (cekano: 62)");
        t.web.url = "file:///android_asset/emu/index.html";
        t.blok2(1920, 1080);
        System.out.println("ATARI sys cesta -> quality=" + t.napTvWebJpegQuality + "  (cekano: 62)");
    }
}
