// B319: skutecna trida NativePs1CoreBridge z appky v JVM na PC se SKUTECNYM nap_ps1_native.cpp
// (knihovna libnapps1core.so z ../../b308/ps1_harness.cpp - misto jadra PCSX "hra" 320x240, jinak
// vse jako v telefonu), -Xcheck:jni. Nova native metoda proudOtevri2Native (volby stranky WEB TV
// B319: kopie pri posunu obrazu, polovicni rozliseni) se vola primo pres reflexi (presne typy
// int(int)) a pak pres obal proudOtevri2Safe, ktery vola MainActivity (/ps1.stream?v=319).
// Proud z bezici "hry" musi chodit obema prohlizecum (B319 i staremu) a log Hubu rict, ktery
// prohlizec ma volby B319.
import eu.atarihelp.emu10.NativePs1CoreBridge;
import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class TestB319Proud {
    static int chyb = 0;

    static void over(boolean ok, String co) {
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
        if (!ok) chyb++;
    }

    static int paketu(int id, byte[] b) throws Exception {
        int n = 0;
        long t0 = System.currentTimeMillis();
        while (n < 5 && System.currentTimeMillis() - t0 < 5000) {
            int r = NativePs1CoreBridge.proudDalsiSafe(id, b, 200, 0);
            if (r > 0 && b[4] == 'N' && b[5] == 'A' && b[6] == 3) n++;
        }
        return n;
    }

    public static void main(String[] a) throws Exception {
        String prac = a.length > 0 ? a[0] : "/tmp/nap_b319_jvm";
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Dune 2000.cue"))) {
            w.write("FILE \"Dune 2000.bin\" BINARY\n  TRACK 01 MODE2/2352\n");
        }
        over(NativePs1CoreBridge.isLoaded(), "knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        int nativnich = 0;
        for (Method m : NativePs1CoreBridge.class.getDeclaredMethods()) if (Modifier.isNative(m.getModifiers())) nativnich++;
        over(nativnich == 29, "native metod ve tride: " + nativnich + " (28 z B318 + proudOtevri2Native B319)");

        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Dune 2000.cue");
        over("PS1_HRA_OK".equals(r), "bootGameSafe (ps1BootDoMonitoru) -> " + r);
        Thread.sleep(300);
        NativePs1CoreBridge.proudLogSafe();   // starsi radky pryc

        Method m = NativePs1CoreBridge.class.getDeclaredMethod("proudOtevri2Native", int.class);
        m.setAccessible(true);
        over(m.getReturnType() == int.class && Modifier.isStatic(m.getModifiers()) && Modifier.isPrivate(m.getModifiers()),
             "proudOtevri2Native(int) -> int, private static native (jako ostatni proud*Native)");
        int id3 = (Integer) m.invoke(null, 3);
        int id0 = NativePs1CoreBridge.proudOtevriSafe();
        int idS = NativePs1CoreBridge.proudOtevri2Safe(3);
        int idS0 = NativePs1CoreBridge.proudOtevri2Safe(0);
        over(id3 > 0 && id0 > 0 && idS > 0 && idS0 > 0 && id3 != id0 && idS != idS0,
             "prohlizece: B319 primo " + id3 + ", stary " + id0 + ", B319 pres obal " + idS + ", obal s volbami 0 = stary " + idS0);
        byte[] b = new byte[1 << 20];
        int p3 = paketu(id3, b), p0 = paketu(id0, b), pS = paketu(idS, b), pS0 = paketu(idS0, b);
        over(p3 == 5 && p0 == 5 && pS == 5 && pS0 == 5, "proud z bezici hry chodi vsem (paketu verze 3: " + p3 + " / " + p0 + " / " + pS + " / " + pS0 + ")");
        String log = NativePs1CoreBridge.proudLogSafe();
        log = log == null ? "" : log;
        boolean b3 = false, b0 = false, bS = false, bS0 = false;
        for (String radek : log.split("\n")) {
            if (radek.contains("prohlizec " + id3 + " pripojen")) b3 = radek.contains("| B319: posun obrazu");
            if (radek.contains("prohlizec " + id0 + " pripojen")) b0 = !radek.contains("B319");
            if (radek.contains("prohlizec " + idS + " pripojen")) bS = radek.contains("| B319: posun obrazu");
            if (radek.contains("prohlizec " + idS0 + " pripojen")) bS0 = !radek.contains("B319");
        }
        over(b3 && b0 && bS && bS0, "log Hubu: prohlizec s volbami B319 \"... | B319: posun obrazu = kopie v prohlizeci, ...\", stary bez B319");
        for (String radek : log.split("\n")) if (radek.contains("pripojen")) System.out.println("   " + radek);
        NativePs1CoreBridge.proudZavriSafe(id3);
        NativePs1CoreBridge.proudZavriSafe(id0);
        NativePs1CoreBridge.proudZavriSafe(idS);
        NativePs1CoreBridge.proudZavriSafe(idS0);
        String log2 = NativePs1CoreBridge.proudLogSafe();
        over(log2 != null && log2.contains("prohlizec " + id3 + " odpojen") && log2.contains("B319: posun obrazu"),
             "odpojeni: souhrn prohlizece B319 s radkem \"B319: posun obrazu ... x, casti v polovicnim rozliseni ...\"");
        System.out.println("stop: " + NativePs1CoreBridge.stopSafe());
        System.out.println("VYSLEDEK: " + (chyb == 0 ? "0 chyb" : chyb + " CHYB"));
        System.exit(chyb == 0 ? 0 : 1);
    }
}
