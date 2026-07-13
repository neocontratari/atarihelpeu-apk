class WebView {
    String url = null;
    String getUrl() { return url; }
}

public class TvTapPatchCheck {
    volatile int napTvWebJpegQuality = 62;
    volatile int napTvWebFrameDelayMs = 55;
    volatile String napTvWebVideoProfile = "AUTO";
    volatile int napTvWebQualityTier = 0;
    WebView web = new WebView();

    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        if (!landscape)         table = new int[][]{{1120,72,55},{1280,82,50},{1440,90,45}};
        else if (djScreen)      table = new int[][]{{1120,72,65},{1280,82,55},{1440,90,45}};
        else if (hqLiteScreen)  table = new int[][]{{860,62,75}, {1000,72,65},{1180,80,55}};
        else                    table = new int[][]{{760,54,75}, {900,66,70}, {1080,74,60}};
        return table[t];
    }

    // ===== BLOK 1: draw-capture cesta (napTvWebFrameTick) =====
    void blok1(boolean landscape) {
                    boolean djScreen = false;
                    try { String cu = web == null ? null : web.getUrl(); djScreen = cu != null && cu.contains("/dj/"); } catch (Throwable ignored) {}
                    boolean hqLiteScreen = false;
                    try { String cu2 = web == null ? null : web.getUrl(); hqLiteScreen = cu2 != null && (cu2.contains("/emu_ps1/") || cu2.contains("/emu_sega/") || cu2.contains("/emu/")); } catch (Throwable ignored) {}
                    int[] qv = napTvWebQualityFor(djScreen, hqLiteScreen, landscape);
                    int maxSide = qv[0];
                    napTvWebJpegQuality = qv[1];
                    napTvWebFrameDelayMs = qv[2];
                    napTvWebVideoProfile = (landscape ? (djScreen ? "LANDSCAPE_DJ" : (hqLiteScreen ? "LANDSCAPE_EMU" : "LANDSCAPE_FAST")) : "PORTRAIT") + "_T" + napTvWebQualityTier;
        System.out.println(maxSide);
    }

    // ===== BLOK 2: MediaProjection cesta (napTvWebHandleSystemImage) =====
    void blok2(int w, int h) {
            boolean djScreenSys = false, hqLiteScreenSys = false;
            try { String cu = web == null ? null : web.getUrl(); djScreenSys = cu != null && cu.contains("/dj/"); hqLiteScreenSys = cu != null && (cu.contains("/emu_ps1/") || cu.contains("/emu_sega/") || cu.contains("/emu/")); } catch (Throwable ignored) {}
            int[] qvSys = napTvWebQualityFor(djScreenSys, hqLiteScreenSys, w > h);
            napTvWebJpegQuality = qvSys[1];
    }

    public static void main(String[] a) {
        TvTapPatchCheck t = new TvTapPatchCheck();
        t.web.url = "file:///android_asset/emu_ps1/index.html";
        t.blok1(true);
        System.out.println("PS1 landscape tier=0    -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 62 75 LANDSCAPE_EMU_T0)");
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok1(true);
        System.out.println("DJ landscape tier=0     -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 72 65 LANDSCAPE_DJ_T0)");
        t.napTvWebQualityTier = 2;
        t.web.url = "file:///android_asset/dj/index.html";
        t.blok1(true);
        System.out.println("DJ landscape tier=2 HIGH-> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + " profile=" + t.napTvWebVideoProfile + "  (cekano: 90 45 LANDSCAPE_DJ_T2)");
        t.web.url = "file:///android_asset/emu_sega/index.html";
        t.blok1(true);
        System.out.println("SEGA landscape tier=2   -> quality=" + t.napTvWebJpegQuality + " delay=" + t.napTvWebFrameDelayMs + "  (cekano: 80 55)");
        t.web.url = "file:///android_asset/player/index.html";
        t.blok1(true);
        System.out.println("PLAYER landscape tier=2 -> quality=" + t.napTvWebJpegQuality + "  (cekano: 74 - nema se chytit DJ ani EMU)");
        t.blok2(1920, 1080);
        System.out.println("PLAYER sys tier=2       -> quality=" + t.napTvWebJpegQuality + "  (cekano: 74)");
    }
}
