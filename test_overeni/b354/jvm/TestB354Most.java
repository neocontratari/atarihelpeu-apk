package eu.atarihelp.emu10;

import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * B354: mosty Java -> C++ pro hrace 2 v JVM s -Xcheck:jni (jako ../../b320/jvm TestB320Ovladac):
 *  A) NativePs1CoreBridge se SKUTECNYM nap_ps1_native.cpp (libnapps1core.so z ../../b308/ps1_harness.cpp):
 *     ps1OvladacKlavesaHrac / ps1OvladacOsyHrac za behu "hry" (hrac 1 i 2), stare metody beze zmeny, denik
 *  B) mosty Atari bez knihovny: devOvladac*Safe(hrac, ...) vraci 0 a nespadnou
 * Preklad a spusteni: viz ../JAK_SPUSTIT.txt
 */
public class TestB354Most {
    static int ok = 0, chyb = 0;
    static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); }

    public static void main(String[] a) throws Exception {
        final String prac = a.length > 0 ? a[0] : "/tmp/nap_b354_jvm";
        System.out.println("B354 - mosty pro hrace 2 v JVM (-Xcheck:jni): NativePs1CoreBridge se skutecnym nap_ps1_native.cpp, Atari bez knihovny");
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Hra 1 na 1.cue"))) {
            w.write("FILE \"Hra 1 na 1.bin\" BINARY\n  TRACK 01 MODE2/2352\n");
        }
        // ---------- A) PS1 ----------
        over(NativePs1CoreBridge.isLoaded(), "A1) knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        Method mk = NativePs1CoreBridge.class.getDeclaredMethod("ps1OvladacKlavesaHrac", int.class, int.class, boolean.class);
        Method mo = NativePs1CoreBridge.class.getDeclaredMethod("ps1OvladacOsyHrac", int.class, float[].class, int.class);
        over(mk.getReturnType() == int.class && mo.getReturnType() == void.class && Modifier.isNative(mk.getModifiers())
                        && Modifier.isPrivate(mk.getModifiers()) && Modifier.isStatic(mo.getModifiers()) && Modifier.isNative(mo.getModifiers()),
                "A2) podpisy: int ps1OvladacKlavesaHrac(int, int, boolean), void ps1OvladacOsyHrac(int, float[], int) - private static native");
        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        final String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Hra 1 na 1.cue");
        over("PS1_HRA_OK".equals(r), "A3) bootGameSafe -> " + r);
        Thread.sleep(300);
        final int t2a = NativePs1CoreBridge.ovladacKlavesaSafe(1, 96, true);     // hrac 2: A
        final int t2b = NativePs1CoreBridge.ovladacKlavesaSafe(1, 97, true);     // hrac 2: B
        final int t1a = NativePs1CoreBridge.ovladacKlavesaSafe(0, 96, true);     // hrac 1: A
        final int tst = NativePs1CoreBridge.ovladacKlavesaSafe(96, true);        // stara metoda = hrac 1
        final int t20 = NativePs1CoreBridge.ovladacKlavesaSafe(1, 4, true);      // ZPET neni tlacitko ovladace
        NativePs1CoreBridge.ovladacOsySafe(1, new float[]{0.9f, 0f, 0f, 0f, -1f, 0f, 0.8f, 0f}, 15);
        NativePs1CoreBridge.ovladacOsySafe(1, new float[3], 15);                 // kratke pole
        NativePs1CoreBridge.ovladacOsySafe(1, null, 15);                         // null nespadne
        NativePs1CoreBridge.ovladacOsySafe(0, new float[8], 15);
        Thread.sleep(200);
        NativePs1CoreBridge.ovladacKlavesaSafe(1, 96, false);
        NativePs1CoreBridge.ovladacKlavesaSafe(1, 97, false);
        NativePs1CoreBridge.ovladacKlavesaSafe(0, 96, false);
        NativePs1CoreBridge.ovladacPustSafe();
        over(t2a == 1 && t2b == 2 && t1a == 1 && tst == 1 && t20 == 0,
                "A4) za behu hry: hrac 2 A -> " + t2a + " (T_A), B -> " + t2b + " (T_B); hrac 1 A -> " + t1a + "; stara metoda -> " + tst
                        + "; ZPET -> " + t20 + "; osy hrace 2 (i kratke pole a null) a ovladacPustSafe bez chyby JNI");
        Thread.sleep(300);
        final String dn = NativePs1CoreBridge.denikSafe();
        over(dn.contains("OVLADAC 2 (hrac 2): prvni stisk -> PS1 port 2"), "A5) denik PS1: \"OVLADAC 2 (hrac 2): prvni stisk -> PS1 port 2\"");
        NativePs1CoreBridge.stopSafe();
        // ---------- B) Atari bez knihovny ----------
        int ra = -1;
        boolean pad = false;
        try {
            ra = NativeAtariCoreBridge.devOvladacKlavesaSafe(1, 96, true);
            NativeAtariCoreBridge.devOvladacOsySafe(1, new float[8], 15);
            NativeAtariCoreBridge.devOvladacPustSafe();
        } catch (Throwable t) { pad = true; }
        over(!pad && ra == 0, "B1) most Atari bez knihovny: devOvladac*Safe(hrac 2, ...) vraci 0 a nespadne");
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
