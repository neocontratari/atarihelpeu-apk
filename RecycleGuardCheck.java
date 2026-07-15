public class RecycleGuardCheck {
    int bmW = 0, bmH = 0;
    boolean recycled = false;
    boolean pixelCopyPending = false;
    int recycleCalls = 0;
    int createCalls = 0;

    // presna kopie nove logiky (bez skutecneho Bitmap)
    void resizeIfNeeded(int bw, int bh) {
        boolean mismatch = (bmW == 0 && bmH == 0) || bmW != bw || bmH != bh; // 0,0 = "null"
        if (mismatch) {
            boolean hadOld = !(bmW == 0 && bmH == 0);
            if (hadOld && !pixelCopyPending) {
                recycleCalls++;
            }
            bmW = bw; bmH = bh;
            createCalls++;
        }
    }

    public static void main(String[] a) {
        System.out.println("=== Test 1: bezny prechod (zadny PixelCopy) - musi se recyklovat ===");
        RecycleGuardCheck t1 = new RecycleGuardCheck();
        t1.resizeIfNeeded(800, 600); // prvni - zadna stara bitmapa k recyklaci
        t1.resizeIfNeeded(1200, 900); // zmena velikosti - stara SE MUSI recyklovat
        System.out.println("createCalls=" + t1.createCalls + " recycleCalls=" + t1.recycleCalls + "  (cekano: 2, 1)");
        System.out.println((t1.createCalls == 2 && t1.recycleCalls == 1) ? "OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Test 2: PixelCopy prave bezi - NESMI se recyklovat (bezpecnost) ===");
        RecycleGuardCheck t2 = new RecycleGuardCheck();
        t2.resizeIfNeeded(800, 600);
        t2.pixelCopyPending = true;
        t2.resizeIfNeeded(1200, 900); // zmena velikosti, ale PixelCopy bezi
        System.out.println("createCalls=" + t2.createCalls + " recycleCalls=" + t2.recycleCalls + "  (cekano: 2, 0 - PRESKOCENO kvuli bezpecnosti)");
        System.out.println((t2.createCalls == 2 && t2.recycleCalls == 0) ? "OK - bezpecnostni pojistka funguje" : "CHYBA");

        System.out.println();
        System.out.println("=== Test 3: opakovane prechody bez PixelCopy - kazdy se recykluje ===");
        RecycleGuardCheck t3 = new RecycleGuardCheck();
        int[][] sizes = {{800,600},{1200,900},{760,1120},{1920,934},{860,672}};
        for (int[] s : sizes) t3.resizeIfNeeded(s[0], s[1]);
        System.out.println("createCalls=" + t3.createCalls + " recycleCalls=" + t3.recycleCalls + "  (cekano: 5, 4 - vsechny krome prvni)");
        System.out.println((t3.createCalls == 5 && t3.recycleCalls == 4) ? "OK" : "CHYBA");
    }
}
