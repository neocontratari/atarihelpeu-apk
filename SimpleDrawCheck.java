public class SimpleDrawCheck {
    int bmW = 0, bmH = 0;
    boolean pixelCopyPending = false;
    int recycleCalls = 0;
    int drawCalls = 0;

    // presna kopie zjednodusene logiky
    void captureByDraw(int bw, int bh) {
        boolean mismatch = (bmW == 0 && bmH == 0) || bmW != bw || bmH != bh;
        if (mismatch) {
            boolean hadOld = !(bmW == 0 && bmH == 0);
            if (hadOld && !pixelCopyPending) recycleCalls++;
            bmW = bw; bmH = bh;
        }
        drawCalls++; // VZDY se nakresli, bez ohledu na cokoli - zadne cekani
    }

    public static void main(String[] a) {
        System.out.println("=== Test: OKAMZITA prestavba pri KAZDE zmene velikosti, zadne zpozdeni ===");
        SimpleDrawCheck t = new SimpleDrawCheck();
        int[][] sizes = {{800,600},{800,600},{1200,900},{800,600},{1200,900}}; // simulace "nestabilnich" rozmeru jako u animovanych stranek
        for (int[] s : sizes) t.captureByDraw(s[0], s[1]);
        System.out.println("drawCalls=" + t.drawCalls + "  (cekano: 5 - KAZDY tik nakresli, i pri opakovane meniciho se rozmeru)");
        System.out.println((t.drawCalls == 5) ? "OK - zadne 'nikdy nenakresli' zaseknuti mozne, i pri neustale se menicich rozmerech" : "CHYBA");

        System.out.println();
        System.out.println("=== Test: recyklace stale probiha spravne (pojistka ze SK40 zachovana) ===");
        System.out.println("recycleCalls=" + t.recycleCalls + "  (cekano: 3 - tri zmeny velikosti v sekvenci 800->1200->800->1200)");
        System.out.println((t.recycleCalls == 3) ? "OK" : "CHYBA");
    }
}
