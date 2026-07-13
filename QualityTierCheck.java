class WebView {
    String url = null;
    String getUrl() { return url; }
}

public class QualityTierCheck {
    volatile int napTvWebQualityTier = 0;

    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        if (!landscape)         table = new int[][]{{1120,72,55},{1360,84,48},{1600,92,40}};
        else if (djScreen)      table = new int[][]{{1120,72,65},{1360,84,52},{1600,92,42}};
        else if (hqLiteScreen)  table = new int[][]{{860,62,75}, {1120,76,62},{1360,84,50}};
        else                    table = new int[][]{{760,54,75}, {1000,70,68},{1200,78,55}};
        return table[t];
    }

    static String fmt(int[] q) { return q[0] + "px / Q" + q[1] + " / " + q[2] + "ms"; }
    static int clamp(int q) { return Math.max(35, Math.min(94, q)); }

    public static void main(String[] a) {
        QualityTierCheck t = new QualityTierCheck();

        System.out.println("=== REGRESNI KONTROLA: LOW (tier=0) MUSI zustat presne stejny jako SK11-14 ===");
        t.napTvWebQualityTier = 0;
        System.out.println("DJ landscape    LOW -> " + fmt(t.napTvWebQualityFor(true,  false, true))  + "  (cekano: 1120px / Q72 / 65ms)");
        System.out.println("EMU landscape   LOW -> " + fmt(t.napTvWebQualityFor(false, true,  true))  + "  (cekano: 860px / Q62 / 75ms)");
        System.out.println("OTHER landscape LOW -> " + fmt(t.napTvWebQualityFor(false, false, true))  + "  (cekano: 760px / Q54 / 75ms)");
        System.out.println("Portrait        LOW -> " + fmt(t.napTvWebQualityFor(true, false, false))  + "  (cekano: 1120px / Q72 / 55ms)");

        System.out.println();
        System.out.println("=== NOVE MEDIUM/HIGH hodnoty + kontrola stropu 94 ===");
        boolean clampOk = true;
        for (int tier = 0; tier <= 2; tier++) {
            t.napTvWebQualityTier = tier;
            String label = tier == 0 ? "LOW   " : tier == 1 ? "MEDIUM" : "HIGH  ";
            int[][] combos = {
                t.napTvWebQualityFor(true, false, true),
                t.napTvWebQualityFor(false, true, true),
                t.napTvWebQualityFor(false, false, true),
                t.napTvWebQualityFor(false, false, false)
            };
            String[] names = {"DJ", "EMU", "OTHER", "PORTRAIT"};
            for (int i = 0; i < combos.length; i++) {
                int q = combos[i][1];
                int clamped = clamp(q);
                if (clamped != q) { clampOk = false; System.out.println("POZOR: " + label + " " + names[i] + " kvalita " + q + " by se oriznula na " + clamped); }
                System.out.println(label + " " + names[i] + " -> " + fmt(combos[i]) + (clamped != q ? "  *** OSEKNUTO ***" : ""));
            }
        }
        System.out.println(clampOk ? "STROP OK - zadna nastavena kvalita se neosekava" : "POZOR - neco se osekava");

        System.out.println();
        System.out.println("=== MONOTONIE (HIGH >= MEDIUM >= LOW ve vsech kombinacich) ===");
        boolean ok = true;
        for (boolean dj : new boolean[]{true, false}) {
            for (boolean hq : new boolean[]{true, false}) {
                for (boolean land : new boolean[]{true, false}) {
                    t.napTvWebQualityTier = 0; int[] low = t.napTvWebQualityFor(dj, hq, land);
                    t.napTvWebQualityTier = 1; int[] med = t.napTvWebQualityFor(dj, hq, land);
                    t.napTvWebQualityTier = 2; int[] high = t.napTvWebQualityFor(dj, hq, land);
                    if (!(low[0] <= med[0] && med[0] <= high[0] && low[1] <= med[1] && med[1] <= high[1])) {
                        ok = false;
                        System.out.println("CHYBA: dj=" + dj + " hq=" + hq + " land=" + land);
                    }
                }
            }
        }
        System.out.println(ok ? "MONOTONIE OK" : "MONOTONIE SELHALA");
    }
}
