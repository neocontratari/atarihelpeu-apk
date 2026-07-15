public class StaleRaceCheck {
    boolean pixelCopyPending = false;
    long pixelCopyPendingAtMs = 0;
    long requestGen = 0;
    int drawCalls = 0;
    int publishedFromPixelCopy = 0;
    int discardedStale = 0;
    long now = 0;

    // presna kopie nove logiky (cast 1: co se stane KDYZ se pozadavek VYDA)
    Long issuePixelCopyIfHqLite(boolean hqLiteScreen) {
        if (pixelCopyPending && pixelCopyPendingAtMs > 0 && now - pixelCopyPendingAtMs > 900L) {
            pixelCopyPending = false; pixelCopyPendingAtMs = 0;
        }
        if (hqLiteScreen) {
            if (!pixelCopyPending) {
                pixelCopyPending = true;
                pixelCopyPendingAtMs = now;
                return ++requestGen; // vraci gen tohoto konkretniho pozadavku
            }
            return null; // uz jeden bezi, nic noveho nevydano
        } else {
            drawCalls++;
            requestGen++; // BUILD2SK35: zvys generaci = oznac predchozi hqLite pozadavky jako zastarale
            return null;
        }
    }

    // presna kopie logiky uvnitr callbacku
    void resolvePixelCopy(long myGen, boolean success) {
        if (myGen != requestGen) {
            discardedStale++;
            return; // ZAHOZENO - nepublikuje se
        }
        if (success) publishedFromPixelCopy++;
        pixelCopyPending = false;
        pixelCopyPendingAtMs = 0;
    }

    public static void main(String[] a) {
        System.out.println("=== KRITICKY TEST: zastaraly pozdni PixelCopy vysledek PO prechodu na jinou obrazovku ===");
        StaleRaceCheck t = new StaleRaceCheck();
        long base = 1_753_000_000_000L;

        // Tik 1: jeste na PS1 (hqLite=true), vyda se PixelCopy pozadavek
        t.now = base + 0;
        Long gen1 = t.issuePixelCopyIfHqLite(true);
        System.out.println("Tik1 (PS1): pozadavek vydan, gen=" + gen1);

        // Tik 2: uzivatel MEZITIM prepnul na DJ pult (hqLite=false) - DRIV nez PS1 pozadavek odpovedel!
        t.now = base + 50;
        t.issuePixelCopyIfHqLite(false); // draw() se zavola, generace se zvysi
        System.out.println("Tik2 (DJ pult): draw() zavolan, drawCalls=" + t.drawCalls + ", generace nyni=" + t.requestGen);

        // Tik 3: PS1 pozadavek KONECNE odpovi (pozde, po prechodu) - MUSI se zahodit
        t.resolvePixelCopy(gen1, true);
        System.out.println("Po zpozdene odpovedi PS1 pozadavku: publishedFromPixelCopy=" + t.publishedFromPixelCopy
                + " discardedStale=" + t.discardedStale + "  (cekano: 0, 1 - MUSI byt zahozeno)");
        boolean ok1 = t.publishedFromPixelCopy == 0 && t.discardedStale == 1;
        System.out.println(ok1 ? "OK - zastaraly vysledek NEPREPSAL cerstvy obraz z DJ pultu" : "KRITICKA CHYBA - presne tohle zpusobilo SK32 katastrofu!");

        System.out.println();
        System.out.println("=== Normalni provoz beze zmeny obrazovky - PixelCopy funguje spravne ===");
        StaleRaceCheck t2 = new StaleRaceCheck();
        t2.now = base;
        Long g = t2.issuePixelCopyIfHqLite(true);
        t2.resolvePixelCopy(g, true);
        System.out.println("publishedFromPixelCopy=" + t2.publishedFromPixelCopy + " discardedStale=" + t2.discardedStale + "  (cekano: 1, 0)");
        System.out.println((t2.publishedFromPixelCopy == 1 && t2.discardedStale == 0) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Ne-hqLite obsah nikdy nezkousi PixelCopy, jen draw() ===");
        StaleRaceCheck t3 = new StaleRaceCheck();
        for (int i = 0; i < 5; i++) { t3.now = base + i * 50; t3.issuePixelCopyIfHqLite(false); }
        System.out.println("drawCalls=" + t3.drawCalls + " pixelCopyPending=" + t3.pixelCopyPending + "  (cekano: 5, false)");
        System.out.println((t3.drawCalls == 5 && !t3.pixelCopyPending) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Prepnuti hqLite->non-hqLite->hqLite - pending flag se nezasekne ===");
        StaleRaceCheck t4 = new StaleRaceCheck();
        t4.now = base;
        t4.issuePixelCopyIfHqLite(true); // PS1, pozadavek vydan, pending=true
        t4.now = base + 50;
        t4.issuePixelCopyIfHqLite(false); // DJ pult - pending flag NENI cisten timhle volanim primo,
                                            // ale je zvysena generace. Overime ze pri navratu na hqLite
                                            // po 900ms timeoutu jde znovu vydat pozadavek.
        t4.now = base + 1000; // > 900ms od puvodniho pozadavku
        Long gNew = t4.issuePixelCopyIfHqLite(true); // zpet na Segu napr.
        System.out.println("Novy pozadavek po navratu na hqLite: gen=" + gNew + "  (cekano: neni null - 900ms timeout uvolnil pending)");
        System.out.println((gNew != null) ? "OK - zadne trvale zaseknuti pending flagu" : "CHYBA");
    }
}
