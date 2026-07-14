class WebView {
    String url = null;
    String getUrl() { return url; }
}

public class Sk16Check {
    volatile int napTvWebQualityTier = 0;

    // presna kopie ze skutecneho souboru
    private int[] napTvWebQualityFor(boolean djScreen, boolean hqLiteScreen, boolean landscape) {
        int t = napTvWebQualityTier; if (t < 0) t = 0; if (t > 2) t = 2;
        int[][] table;
        if (!landscape)         table = new int[][]{{1120,72,55},{1360,84,48},{1600,92,40}};
        else if (djScreen)      table = new int[][]{{1120,72,65},{1360,84,52},{1600,92,42}};
        else if (hqLiteScreen)  table = new int[][]{{860,62,75}, {1120,76,62},{1360,84,50}};
        else                    table = new int[][]{{760,54,75}, {1000,70,68},{1200,78,55}};
        return table[t];
    }

    // simulace vypoctu cw/ch pri startu system mirroru (presne dle SK16 zmeny)
    int[] simulateMirrorStart(int sw, int sh) {
        boolean landscape = sw > sh;
        int[] qv0 = napTvWebQualityFor(true, false, landscape);
        int maxSide = qv0[0];
        float scale = Math.min(1.0f, (float) maxSide / Math.max(sw, sh));
        int cw = Math.max(2, (int) (sw * scale)) & ~1;
        int ch = Math.max(2, (int) (sh * scale)) & ~1;
        return new int[]{cw, ch, maxSide};
    }

    public static void main(String[] a) {
        Sk16Check t = new Sk16Check();

        System.out.println("=== MIRROR-START ROZLISENI PODLE UROVNE (S8-like 2960x1440 landscape) ===");
        for (int tier = 0; tier <= 2; tier++) {
            t.napTvWebQualityTier = tier;
            int[] r = t.simulateMirrorStart(2960, 1440);
            String label = tier == 0 ? "LOW   " : tier == 1 ? "MEDIUM" : "HIGH  ";
            System.out.println(label + " -> cw=" + r[0] + " ch=" + r[1] + " (maxSide cil=" + r[2] + ")");
        }
        System.out.println("POCTIVE ZAZNAM ZMENY: puvodne bylo VZDY natvrdo cw~960 bez ohledu na uroven.");
        System.out.println("Ted i LOW dava vyssi zaklad (1120) - viz testplan pro vysvetleni proc.");

        System.out.println();
        System.out.println("=== MONOTONIE mirror-start rozliseni (HIGH >= MEDIUM >= LOW) ===");
        t.napTvWebQualityTier = 0; int low = t.simulateMirrorStart(2960, 1440)[0];
        t.napTvWebQualityTier = 1; int med = t.simulateMirrorStart(2960, 1440)[0];
        t.napTvWebQualityTier = 2; int high = t.simulateMirrorStart(2960, 1440)[0];
        System.out.println((low <= med && med <= high) ? "MONOTONIE OK (" + low + " <= " + med + " <= " + high + ")" : "SELHALA");

        System.out.println();
        System.out.println("=== FALLBACK PRAH (jen informativne, hodnota v kodu) ===");
        long oldThreshold = 1600L, newThreshold = 3500L;
        System.out.println("Puvodni prah: " + oldThreshold + "ms, novy prah: " + newThreshold + "ms (" + (newThreshold > oldThreshold ? "zvyseno - min casteji fallback" : "CHYBA") + ")");
    }
}
