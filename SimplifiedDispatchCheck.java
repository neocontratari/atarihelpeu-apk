public class SimplifiedDispatchCheck {
    boolean pixelCopyPending = false;
    long pixelCopyPendingAtMs = 0;
    int drawCalls = 0;
    int pixelCopyAttempts = 0;
    long now = 0;

    // presna kopie nove zjednodusene dispatch logiky (bez skutecnych Android API)
    void tick(boolean hqLiteScreen) {
        if (pixelCopyPending && pixelCopyPendingAtMs > 0 && now - pixelCopyPendingAtMs > 900L) {
            pixelCopyPending = false;
            pixelCopyPendingAtMs = 0;
        }
        if (hqLiteScreen) {
            boolean pixelCopyAllowed = !pixelCopyPending; // (SDK/handler check vynechano - vzdy true v testu)
            if (pixelCopyAllowed) {
                pixelCopyPending = true;
                pixelCopyPendingAtMs = now;
                pixelCopyAttempts++;
                // (asynchronni vysledek prijde pozdeji - simulujeme zvlast)
            }
        } else {
            drawCalls++; // VZDY, bez ohledu na cokoli jineho
        }
    }

    void resolvePixelCopySuccess() {
        pixelCopyPending = false;
        pixelCopyPendingAtMs = 0;
    }

    public static void main(String[] a) {
        System.out.println("=== Scenar 1: ne-hqLite obsah (DJ/menu) - draw() VZDY, bez ohledu na cokoli ===");
        SimplifiedDispatchCheck t1 = new SimplifiedDispatchCheck();
        long b1=1_753_000_000_000L; for (int i = 0; i < 10; i++) { t1.now = b1 + i * 50; t1.tick(false); }
        System.out.println("drawCalls=" + t1.drawCalls + " pixelCopyAttempts=" + t1.pixelCopyAttempts + "  (cekano: 10, 0)");
        System.out.println((t1.drawCalls == 10 && t1.pixelCopyAttempts == 0) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 2: hqLite obsah, PixelCopy vzdy uspesne a rychle - pravidelne pokusy ===");
        SimplifiedDispatchCheck t2 = new SimplifiedDispatchCheck();
        long b2=1_753_000_000_000L; for (int i = 0; i < 5; i++) {
            t2.now = b2 + i * 50;
            t2.tick(true);
            t2.resolvePixelCopySuccess(); // simulace rychle async odpovedi driv nez dalsi tik
        }
        System.out.println("pixelCopyAttempts=" + t2.pixelCopyAttempts + " drawCalls=" + t2.drawCalls + "  (cekano: 5, 0 - zadny draw() pro hqLite)");
        System.out.println((t2.pixelCopyAttempts == 5 && t2.drawCalls == 0) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 3: hqLite obsah, PixelCopy 'visi' (async odpoved nikdy neprijde) - 900ms timeout ho uvolni ===");
        SimplifiedDispatchCheck t3 = new SimplifiedDispatchCheck();
        long base3 = 1_753_000_000_000L;
        t3.now = base3 + 0; t3.tick(true); // prvni pokus, zustane "pending" (nikdy nevyresime)
        t3.now = base3 + 40; t3.tick(true); // pending, mensi nez 900ms -> nic
        t3.now = base3 + 400; t3.tick(true); // porad pending
        t3.now = base3 + 950; t3.tick(true); // > 900ms od prvniho pokusu -> timeout uvolni, novy pokus
        System.out.println("pixelCopyAttempts=" + t3.pixelCopyAttempts + "  (cekano: 2 - prvni pokus + jeden novy po timeoutu)");
        System.out.println((t3.pixelCopyAttempts == 2) ? "OK - zadne trvale zaseknuti" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 4: prechod mezi typy obsahu (presne to, co se deje pri opusteni PS1) ===");
        SimplifiedDispatchCheck t4 = new SimplifiedDispatchCheck();
        long b4=1_753_000_000_000L; t4.now = b4 + 0; t4.tick(true); // jeste na PS1, hqLite=true, zacne PixelCopy
        t4.now = b4 + 50; t4.tick(false); // uz na DJ pultu, hqLite=false -> IHNED draw(), bez ohledu na predchozi PixelCopy stav
        System.out.println("po prepnuti: drawCalls=" + t4.drawCalls + " (cekano: 1 - draw() funguje OKAMZITE, zadny cooldown/cekani)");
        System.out.println((t4.drawCalls == 1) ? "OK - zadna zavislost na predchozim hqLite stavu" : "CHYBA");
    }
}
