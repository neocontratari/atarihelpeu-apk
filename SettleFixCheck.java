public class SettleFixCheck {
    int bmW = 0, bmH = 0;
    long pendingSince = 0;
    long struggleStart = 0;
    int pendingW = 0, pendingH = 0;
    int drawCalls = 0;
    long now = 0;

    // presna kopie OPRAVENE logiky (bez skutecneho Bitmap/Canvas)
    boolean tick(int bw, int bh) {
        if (bmW != bw || bmH != bh) {
            if (struggleStart == 0) struggleStart = now;
            boolean hardTimeout = (now - struggleStart) > 2000L;
            boolean withinTolerance = pendingSince > 0
                    && Math.abs(pendingW - bw) <= 4
                    && Math.abs(pendingH - bh) <= 4;
            if (!withinTolerance && !hardTimeout) {
                pendingW = bw; pendingH = bh; pendingSince = now;
                return false;
            }
            if (!hardTimeout && (now - pendingSince) < 150L) return false;
            bmW = bw; bmH = bh;
            pendingSince = 0; struggleStart = 0;
        }
        drawCalls++;
        return true;
    }

    public static void main(String[] a) {
        System.out.println("=== Scenar 1: stabilni cil (beze zmeny) - settle po 150ms presne jako SK28 ===");
        SettleFixCheck t1 = new SettleFixCheck();
        t1.bmW = 800; t1.bmH = 600;
        long base = 1_753_000_000_000L; // realisticky System.currentTimeMillis() rad velikosti, NIKDY 0
        t1.now = base; boolean s1a = t1.tick(1200, 900);
        t1.now = base + 75; boolean s1b = t1.tick(1200, 900);
        t1.now = base + 160; boolean s1c = t1.tick(1200, 900);
        System.out.println("s1a=" + s1a + " s1b=" + s1b + " s1c=" + s1c + " drawCalls=" + t1.drawCalls + "  (cekano: false,false,true,1)");
        System.out.println((!s1a && !s1b && s1c && t1.drawCalls == 1) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 2: KRITICKY TEST - drobny sum +-2px KAZDY tik (presne to, co SK28 rozbilo) ===");
        SettleFixCheck t2 = new SettleFixCheck();
        t2.bmW = 800; t2.bmH = 600;
        long base2 = 1_753_000_000_000L;
        int[][] jitter = {{1200,900},{1201,899},{1199,901},{1202,898},{1200,900},{1198,902}};
        long[] times = {base2+0, base2+40, base2+85, base2+130, base2+175, base2+220};
        boolean anyDrawn = false;
        for (int i = 0; i < jitter.length; i++) {
            t2.now = times[i];
            boolean r = t2.tick(jitter[i][0], jitter[i][1]);
            System.out.println("tik" + i + " (" + jitter[i][0] + "," + jitter[i][1] + ") t=" + times[i] + "ms -> kresli=" + r);
            if (r) anyDrawn = true;
        }
        System.out.println("KRITICKE: aspon jednou nakresleno prestoze cil furt kmita v ramci sumu?  " + (anyDrawn ? "ANO - OPRAVENO" : "NE - PORAD ZASEKNUTE!"));

        System.out.println();
        System.out.println("=== Scenar 3: PATOLOGICKY - cil se meni O VIC NEZ 4px KAZDY tik (nikdy v toleranci) ===");
        SettleFixCheck t3 = new SettleFixCheck();
        t3.bmW = 800; t3.bmH = 600;
        long base3 = 1_753_000_000_000L;
        int w = 1000;
        boolean hardTimeoutSaved = false;
        for (long tms = 0; tms <= 2100; tms += 50) {
            t3.now = base3 + tms;
            w += 20; // pokazde skoci o 20px - nikdy v 4px toleranci
            boolean r = t3.tick(w, w);
            if (r) { hardTimeoutSaved = true; System.out.println("Zachraneno tvrdym limitem v t=" + tms + "ms, drawCalls=" + t3.drawCalls); break; }
        }
        System.out.println(hardTimeoutSaved ? "OK - tvrdy limit 2000ms zabranil trvalemu zaseknuti i pri neustale genuine zmene cile" : "CHYBA - zasteklo se navzdy i s pojistkou!");
    }
}
