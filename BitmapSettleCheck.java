public class BitmapSettleCheck {
    int bmW = 0, bmH = 0; // simulace napTvWebBitmap rozmeru (0,0 = null)
    long pendingSince = 0;
    int pendingW = 0, pendingH = 0;
    int drawCalls = 0;
    long now = 0;

    // presna kopie nove rozhodovaci logiky (bez skutecneho Bitmap/Canvas)
    boolean tick(int bw, int bh) {
        if (bmW != bw || bmH != bh) {
            if (pendingW != bw || pendingH != bh) {
                pendingW = bw; pendingH = bh; pendingSince = now;
                return false; // zadne kresleni tenhle tik
            }
            if (now - pendingSince < 150L) return false; // porad cekame
            bmW = bw; bmH = bh; // "vytvorena" nova bitmapa
        }
        drawCalls++;
        return true;
    }

    public static void main(String[] a) {
        BitmapSettleCheck t = new BitmapSettleCheck();

        System.out.println("=== Scenar 1: JIZ existujici bitmapa, stabilni velikost (zadna zmena) - kazdy tik kresli ===");
        t.bmW = 800; t.bmH = 600; // simulace: bitmapa uz existuje na teto velikosti z drivejska
        t.now = 0; boolean r1 = t.tick(800, 600);
        t.now = 50; boolean r2 = t.tick(800, 600);
        System.out.println("tik1=" + r1 + " tik2=" + r2 + " drawCalls=" + t.drawCalls + "  (cekano: true,true,2)");

        System.out.println();
        System.out.println("=== Scenar 2: zmena velikosti (napr. HIGH prechod DJ->emu) - freeze behem settle ===");
        BitmapSettleCheck t2 = new BitmapSettleCheck();
        t2.bmW = 1920; t2.bmH = 934; // byli jsme na DJ HIGH
        t2.now = 0;
        boolean s1 = t2.tick(1680, 819); // prechod na emu HIGH - jina velikost
        t2.now = 40;
        boolean s2 = t2.tick(1680, 819); // porad stejna cilova velikost, ale jeste < 150ms
        t2.now = 90;
        boolean s3 = t2.tick(1680, 819); // porad < 150ms
        t2.now = 160;
        boolean s4 = t2.tick(1680, 819); // uz > 150ms - konecne kresli
        t2.now = 200;
        boolean s5 = t2.tick(1680, 819); // uz stabilni, kresli normalne
        System.out.println("s1=" + s1 + " s2=" + s2 + " s3=" + s3 + " s4=" + s4 + " s5=" + s5);
        System.out.println("drawCalls=" + t2.drawCalls + "  (cekano: false,false,false,true,true = 2 kresleni)");
        boolean ok2 = !s1 && !s2 && !s3 && s4 && s5 && t2.drawCalls == 2;
        System.out.println(ok2 ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Scenar 3: velikost se zmeni VICKRAT za sebou (nestabilni cil) - odpocet se resetuje ===");
        BitmapSettleCheck t3 = new BitmapSettleCheck();
        t3.bmW = 800; t3.bmH = 600;
        t3.now = 0; t3.tick(1000, 750);   // novy cil A
        t3.now = 100; t3.tick(1200, 900); // ZMENA cile na B driv nez A stihlo settle - odpocet se MUSI resetovat
        t3.now = 200; boolean beforeSettle = t3.tick(1200, 900); // jen 100ms od zmeny na B, jeste ne
        t3.now = 260; boolean afterSettle = t3.tick(1200, 900);  // 160ms od zmeny na B - uz ano
        System.out.println("pred settle (100ms od posledni zmeny)=" + beforeSettle + " po settle (160ms)=" + afterSettle);
        System.out.println((!beforeSettle && afterSettle) ? "OK - odpocet se spravne resetoval pri zmene cile" : "CHYBA");
    }
}
