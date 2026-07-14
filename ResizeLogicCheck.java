public class ResizeLogicCheck {
    volatile int napTvWebQualityTier = 0;
    int napTvWebSystemWidth = 1120, napTvWebSystemHeight = 544; // simulace stavu po SK16 startu na LOW (spravna hodnota, ne stary pre-SK16 hardcoded 960)

    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        if (!landscape)         table = new int[][]{{1120,72,55},{1360,84,48},{1920,94,36}};
        else if (djScreen)      table = new int[][]{{1120,72,65},{1360,84,52},{1920,94,38}};
        else if (hqLiteScreen)  table = new int[][]{{860,62,75}, {1120,76,62},{1680,90,45}};
        else                    table = new int[][]{{760,54,75}, {1000,70,68},{1440,86,48}};
        return table[t];
    }

    // presna kopie rozhodovaci logiky z napTvWebResizeSystemMirror (bez Android volani)
    // vraci null = "neni co delat" (uz na cili), jinak [cw,ch]
    int[] decideResize(int sw, int sh) {
        boolean landscape = sw > sh;
        int[] qv0 = napTvWebQualityFor(true, false, landscape);
        int maxSide = qv0[0];
        float scale = Math.min(1.0f, (float) maxSide / Math.max(sw, sh));
        int cw = Math.max(2, (int) (sw * scale)) & ~1;
        int ch = Math.max(2, (int) (sh * scale)) & ~1;
        if (cw == napTvWebSystemWidth && ch == napTvWebSystemHeight) return null;
        return new int[]{cw, ch};
    }

    public static void main(String[] a) {
        ResizeLogicCheck t = new ResizeLogicCheck();
        int sw = 2960, sh = 1440; // S8-typicke landscape rozliseni

        System.out.println("=== Stav po startu na LOW (system Width/Height = 960/466) ===");
        System.out.println("Pozadavek na LOW (zadna zmena ocekavana) -> " + java.util.Arrays.toString(t.decideResize(sw, sh)) + "  (cekano: null = nic nedelat)");

        t.napTvWebQualityTier = 2; // uzivatel prepne na HIGH
        int[] r = t.decideResize(sw, sh);
        System.out.println("Pozadavek na HIGH -> " + java.util.Arrays.toString(r) + "  (cekano: [1918, 934] nebo blizko - realne prekonfigurovani)");

        // simuluj ze VirtualDisplay byl skutecne prestaven na tuhle velikost
        if (r != null) { t.napTvWebSystemWidth = r[0]; t.napTvWebSystemHeight = r[1]; }

        System.out.println("Znovu HIGH (uz na cili) -> " + java.util.Arrays.toString(t.decideResize(sw, sh)) + "  (cekano: null = zadny zbytecny restart VirtualDisplay)");

        t.napTvWebQualityTier = 1;
        System.out.println("Prepnuti na MEDIUM -> " + java.util.Arrays.toString(t.decideResize(sw, sh)) + "  (cekano: nova mensi velikost, ne null)");
    }
}
