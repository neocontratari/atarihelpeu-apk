public class HysteresisCheck {
    boolean napTvWebSystemInFallback = false;
    int napTvWebSystemFreshStreak = 0;

    // presna kopie nove rozhodovaci logiky
    boolean decide(long age) {
        if (age <= 250L) napTvWebSystemFreshStreak++; else napTvWebSystemFreshStreak = 0;
        boolean appCapture;
        if (!napTvWebSystemInFallback) {
            appCapture = age > 3500L;
            if (appCapture) napTvWebSystemInFallback = true;
        } else {
            appCapture = napTvWebSystemFreshStreak < 3;
            if (!appCapture) napTvWebSystemInFallback = false;
        }
        return appCapture;
    }

    public static void main(String[] a) {
        HysteresisCheck h = new HysteresisCheck();
        // simulace turbulentniho prechodu: system-mirror snimky chodi NEPRAVIDELNE,
        // "age" jednou pod prahem, jednou nad, pak zas cerstve - presne to, co by
        // se stalo behem prechodu z PS1 na jinou obrazovku
        long[] ageSequence = {
            50, 100, 3600,   // prvni kratky vypadek pres prah - MEL by spustit fallback
            50, 3700,        // hned zase cerstvy snimek, pak zas vypadek - PUVODNI KOD by tu kmital
            50, 50, 3800,    // dalsi kolisani
            50, 50, 50, 50,  // ted uz porad cerstve snimky - po chvili by se melo vratit na mirror
            50, 50
        };
        int flips = 0;
        boolean prev = false; // zacinme v mirror rezimu (appCapture=false)
        System.out.println("tick  age    appCapture  (zmena?)");
        for (int i = 0; i < ageSequence.length; i++) {
            boolean cur = h.decide(ageSequence[i]);
            boolean changed = cur != prev;
            if (changed) flips++;
            System.out.println(String.format("%3d   %5d  %-5s       %s", i, ageSequence[i], cur, changed ? "ZMENA" : ""));
            prev = cur;
        }
        System.out.println();
        System.out.println("Celkem zmen stavu: " + flips + " (puvodni kod bez hystereze by mel vic zmen presne na kazdem prekroceni prahu)");

        // porovnani: STARA logika (bez hystereze) na stejne sekvenci
        System.out.println();
        System.out.println("=== Pro srovnani: STARA logika (bez hystereze) na stejne sekvenci ===");
        int oldFlips = 0;
        boolean oldPrev = false;
        for (int i = 0; i < ageSequence.length; i++) {
            boolean oldCapture = ageSequence[i] > 3500L;
            if (oldCapture != oldPrev) oldFlips++;
            oldPrev = oldCapture;
        }
        System.out.println("Stara logika zmen stavu: " + oldFlips);
        System.out.println((flips < oldFlips) ? "HYSTEREZE SNIZILA POCET KMITU: " + oldFlips + " -> " + flips : "POZOR: hystereze nepomohla");
    }
}
