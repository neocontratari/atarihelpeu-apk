public class FreezeControlFlowCheck {
    boolean napTvWebRunning = true;
    boolean napTvWebResizeInProgress = false;
    long napTvWebResizeStartedMs = 0;
    long now = 0; // simulovany cas

    int captureRuns = 0;      // kolikrat probehla "zachytavaci" cast (za normalnich okolnosti)
    int fastReschedules = 0;  // kolikrat se naplanoval rychly 40ms retry
    int normalReschedules = 0; // kolikrat se naplanoval normalni tik (konec metody)

    // presna kopie nove kontrolni logiky ze zacatku napTvWebFrameTick.run()
    void tick() {
        if (napTvWebResizeInProgress) {
            if (now - napTvWebResizeStartedMs > 4000L) {
                napTvWebResizeInProgress = false;
            } else {
                if (napTvWebRunning) fastReschedules++; // simulace ui.postDelayed(this, 40)
                return; // KRITICKE: tohle presne simuluje realny "return;" v puvodni metode
            }
        }
        captureRuns++; // simulace zbytku metody (zachytavani)
        if (napTvWebRunning) normalReschedules++; // simulace radku na konci metody
    }

    public static void main(String[] a) {
        FreezeControlFlowCheck t = new FreezeControlFlowCheck();

        System.out.println("=== Scenar 1: normalni provoz (zadny resize) - 5 ticku ===");
        for (int i = 0; i < 5; i++) { t.now += 50; t.tick(); }
        System.out.println("captureRuns=" + t.captureRuns + " normalReschedules=" + t.normalReschedules + " fastReschedules=" + t.fastReschedules);
        System.out.println((t.captureRuns == 5 && t.normalReschedules == 5 && t.fastReschedules == 0) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 2: zacne resize, trva 200ms (3 tiky po 40-70ms), pak dorazi cerstvy snimek ===");
        FreezeControlFlowCheck t2 = new FreezeControlFlowCheck();
        t2.now = 1000;
        t2.napTvWebResizeInProgress = true;
        t2.napTvWebResizeStartedMs = 1000;
        t2.now = 1040; t2.tick(); // stale probiha (40ms < 4000ms)
        t2.now = 1080; t2.tick(); // stale probiha
        t2.now = 1120; t2.tick(); // stale probiha
        System.out.println("Behem cekani: captureRuns=" + t2.captureRuns + " fastReschedules=" + t2.fastReschedules + "  (cekano: captureRuns=0, fastReschedules=3 - ZADNY zachyt behem zmrazeni, ale KAZDY tik naplanoval dalsi)");
        // ted "dorazi cerstvy snimek" - simulace napTvWebHandleSystemImage, ktere nastavi flag na false
        t2.napTvWebResizeInProgress = false;
        t2.now = 1160; t2.tick();
        System.out.println("Po prijeti snimku: captureRuns=" + t2.captureRuns + " normalReschedules=" + t2.normalReschedules + "  (cekano: 1, 1 - normalni provoz obnoven)");

        System.out.println();
        System.out.println("=== Scenar 3: resize NIKDY nedorazi (edge case) - musi se po 4s vzdat, NE zamrznout navzdy ===");
        FreezeControlFlowCheck t3 = new FreezeControlFlowCheck();
        t3.now = 5000;
        t3.napTvWebResizeInProgress = true;
        t3.napTvWebResizeStartedMs = 5000;
        int ticks = 0;
        for (long elapsed = 40; elapsed <= 4100; elapsed += 40) {
            t3.now = 5000 + elapsed;
            t3.tick();
            ticks++;
        }
        System.out.println("Po " + ticks + " ticich (celkem ~4.1s): resizeInProgress=" + t3.napTvWebResizeInProgress + " captureRuns=" + t3.captureRuns);
        System.out.println(!t3.napTvWebResizeInProgress && t3.captureRuns > 0 ? "OK - pojistka zabrala, obraz se NEzamrzl navzdy" : "CHYBA - obraz by zustal zmrazeny navzdy!");
    }
}
