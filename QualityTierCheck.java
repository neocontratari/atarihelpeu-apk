class WebView {
    String url = null;
    String getUrl() { return url; }
}

public class QualityTierCheck {
    volatile int napTvWebQualityTier = 0;

    // presna kopie pridane funkce ze skutecneho MainActivity.java
    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        if (!landscape)         table = new int[][]{{1120,72,55},{1280,82,50},{1440,90,45}};
        else if (djScreen)      table = new int[][]{{1120,72,65},{1280,82,55},{1440,90,45}};
        else if (hqLiteScreen)  table = new int[][]{{860,62,75}, {1000,72,65},{1180,80,55}};
        else                    table = new int[][]{{760,54,75}, {900,66,70}, {1080,74,60}};
        return table[t];
    }

    static String fmt(int[] q) { return q[0] + "px / Q" + q[1] + " / " + q[2] + "ms"; }

    public static void main(String[] a) {
        QualityTierCheck t = new QualityTierCheck();

        System.out.println("=== REGRESNI KONTROLA: LOW (tier=0) MUSI presne odpovidat SK13 ===");
        t.napTvWebQualityTier = 0;
        System.out.println("DJ landscape    LOW -> " + fmt(t.napTvWebQualityFor(true,  false, true))  + "  (cekano: 1120px / Q72 / 65ms)");
        System.out.println("EMU landscape   LOW -> " + fmt(t.napTvWebQualityFor(false, true,  true))  + "  (cekano: 860px / Q62 / 75ms)");
        System.out.println("OTHER landscape LOW -> " + fmt(t.napTvWebQualityFor(false, false, true))  + "  (cekano: 760px / Q54 / 75ms)");
        System.out.println("Portrait (jakekoli) LOW -> " + fmt(t.napTvWebQualityFor(true, false, false)) + "  (cekano: 1120px / Q72 / 55ms)");

        System.out.println();
        System.out.println("=== VSECH 9 KOMBINACI (3 typy x 3 urovne) + portret ===");
        for (int tier = 0; tier <= 2; tier++) {
            t.napTvWebQualityTier = tier;
            String label = tier == 0 ? "LOW   " : tier == 1 ? "MEDIUM" : "HIGH  ";
            System.out.println(label + " DJ      -> " + fmt(t.napTvWebQualityFor(true,  false, true)));
            System.out.println(label + " EMU     -> " + fmt(t.napTvWebQualityFor(false, true,  true)));
            System.out.println(label + " OTHER   -> " + fmt(t.napTvWebQualityFor(false, false, true)));
            System.out.println(label + " PORTRAIT-> " + fmt(t.napTvWebQualityFor(false, false, false)));
        }

        System.out.println();
        System.out.println("=== KONTROLA MONOTONIE: kazda dalsi uroven musi byt >= predchozi (nikdy hor s vyssim tier) ===");
        boolean ok = true;
        for (boolean dj : new boolean[]{true, false}) {
            for (boolean hq : new boolean[]{true, false}) {
                for (boolean land : new boolean[]{true, false}) {
                    int[] low; t.napTvWebQualityTier = 0; low = t.napTvWebQualityFor(dj, hq, land);
                    int[] med; t.napTvWebQualityTier = 1; med = t.napTvWebQualityFor(dj, hq, land);
                    int[] high; t.napTvWebQualityTier = 2; high = t.napTvWebQualityFor(dj, hq, land);
                    if (!(low[0] <= med[0] && med[0] <= high[0] && low[1] <= med[1] && med[1] <= high[1])) {
                        ok = false;
                        System.out.println("CHYBA neni monotonni: dj=" + dj + " hq=" + hq + " land=" + land);
                    }
                }
            }
        }
        System.out.println(ok ? "MONOTONIE OK - vyssi uroven je vzdy >= nizsi" : "MONOTONIE SELHALA");
    }
}
