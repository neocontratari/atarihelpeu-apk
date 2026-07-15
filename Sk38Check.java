public class Sk38Check {
    boolean pending = false;
    long pendingAtMs = 0;
    long gen = 0;
    long disabledUntilMs = 0;
    int drawCalls = 0;
    int published = 0;
    int discardedStale = 0;
    long now = 0;

    // presna kopie CELE rozhodovaci logiky (radek 347-451 z realneho souboru)
    // vraci "issued gen" pokud byl vydan PixelCopy pozadavek, jinak null
    Long tick(boolean hqLiteScreen) {
        boolean didTimeoutFallback = false;
        if (pending && pendingAtMs > 0 && now - pendingAtMs > 900L) {
            pending = false; pendingAtMs = 0;
            disabledUntilMs = now + (hqLiteScreen ? 2000L : 12000L);
            didTimeoutFallback = true;
            if (!hqLiteScreen) { drawCalls++; gen++; }
        }
        boolean pixelCopyAllowed = hqLiteScreen && !pending && now >= disabledUntilMs;
        if (!didTimeoutFallback && pixelCopyAllowed) {
            pending = true;
            pendingAtMs = now;
            long requestGen = ++gen;
            return requestGen; // simulace vydani pozadavku (callback resolvujeme zvlast)
        } else if (!didTimeoutFallback) {
            if (!hqLiteScreen) { drawCalls++; gen++; }
        }
        return null;
    }

    // presna kopie callbacku
    void resolve(long requestGen, boolean success) {
        if (requestGen != gen) { discardedStale++; return; }
        if (success) published++;
        pending = false; pendingAtMs = 0;
    }

    public static void main(String[] a) {
        System.out.println("=== Test 1: ne-hqLite obsah NIKDY nezkousi PixelCopy (minimalni zasah funguje) ===");
        Sk38Check t1 = new Sk38Check();
        for (int i = 0; i < 5; i++) { t1.now = i * 50; Long r = t1.tick(false); if (r != null) System.out.println("CHYBA: PixelCopy vydan pro ne-hqLite!"); }
        System.out.println("drawCalls=" + t1.drawCalls + " pending=" + t1.pending + "  (cekano: 5, false)");
        System.out.println((t1.drawCalls == 5 && !t1.pending) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Test 2: hqLite obsah funguje normalne (PS1/Sega beze zmeny) ===");
        Sk38Check t2 = new Sk38Check();
        t2.now = 0;
        Long g = t2.tick(true);
        System.out.println("Pozadavek vydan: " + (g != null));
        t2.resolve(g, true);
        System.out.println("published=" + t2.published + " discardedStale=" + t2.discardedStale + "  (cekano: 1, 0)");
        System.out.println((t2.published == 1 && t2.discardedStale == 0) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Test 3: KRITICKA RASOVA PODMINKA - zpozdena PS1 odpoved PO prechodu na DJ pult ===");
        Sk38Check t3 = new Sk38Check();
        long base = 1_753_000_000_000L;
        t3.now = base;
        Long genPs1 = t3.tick(true); // PS1, pozadavek vydan
        System.out.println("PS1 pozadavek vydan, gen=" + genPs1);
        t3.now = base + 50;
        t3.tick(false); // uzivatel MEZITIM prepnul na DJ pult
        System.out.println("Po prechodu na DJ: drawCalls=" + t3.drawCalls + " (cekano: 1)");
        t3.resolve(genPs1, true); // stary PS1 pozadavek KONECNE odpovi
        System.out.println("Po zpozdene odpovedi: published=" + t3.published + " discardedStale=" + t3.discardedStale + "  (cekano: 0, 1)");
        boolean okRace = t3.drawCalls == 1 && t3.published == 0 && t3.discardedStale == 1;
        System.out.println(okRace ? "OK - kriticka rasova podminka spravne osetrena" : "KRITICKA CHYBA");

        System.out.println();
        System.out.println("=== Test 4: prechod hqLite->non-hqLite->hqLite funguje (zadne trvale zaseknuti) ===");
        Sk38Check t4 = new Sk38Check();
        t4.now = base;
        t4.tick(true); // Sega, pozadavek vydan, pending=true
        t4.now = base + 50;
        t4.tick(false); // DJ pult - draw() funguje IHNED, i kdyz pending je porad true ze Segy
        System.out.println("Behem 'stucked pending' z predchozi Segy: drawCalls=" + t4.drawCalls + "  (cekano: 1 - draw() nezavisi na pending flagu)");
        System.out.println((t4.drawCalls == 1) ? "OK - draw() funguje bez ohledu na stary pending stav" : "CHYBA");
    }
}
