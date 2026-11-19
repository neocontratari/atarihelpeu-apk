package eu.atarihelp.emu10;

import java.io.File;
import java.io.FileWriter;

/**
 * B355: mosty Java -> C++ pro TLACITKA OVLADACE v JVM s -Xcheck:jni (jako ../../b354/jvm/TestB354Most):
 *  A) NativePs1CoreBridge.ovladacMapaSafe se SKUTECNYM nap_ps1_native.cpp (libnapps1core.so z ../../b308/ps1_harness.cpp)
 *  B) mosty Atari bez knihovny: devOvladacMapaSafe / devOvladacKatalogSafe vraci "" a nespadnou
 * Preklad a spusteni: viz ../JAK_SPUSTIT.txt krok 5
 */
public class TestB355Most {
    static int ok = 0, chyb = 0;
    static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); }

    public static void main(String[] a) throws Exception {
        final String prac = a.length > 0 ? a[0] : "/tmp/nap_b355_jvm";
        System.out.println("B355 - mosty TLACITKA OVLADACE v JVM (-Xcheck:jni): NativePs1CoreBridge se skutecnym nap_ps1_native.cpp, Atari bez knihovny");
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Hra.cue"))) { w.write("FILE \"Hra.bin\" BINARY\n  TRACK 01 MODE2/2352\n"); }
        over(NativePs1CoreBridge.isLoaded(), "A1) knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        final String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Hra.cue");
        Thread.sleep(300);
        final String m1 = NativePs1CoreBridge.ovladacMapaSafe("A=KOLECKO;B=KRIZEK;MODE=SELECT;Q=1");
        final String m2 = NativePs1CoreBridge.ovladacMapaSafe(null);
        final String m3 = NativePs1CoreBridge.ovladacMapaSafe("");
        final int t = NativePs1CoreBridge.ovladacKlavesaSafe(0, 96, true);
        NativePs1CoreBridge.ovladacKlavesaSafe(0, 96, false);
        System.out.println("hra: " + r + " | mapa: " + m1 + " | null: " + m2);
        over("PS1_HRA_OK".equals(r) && m1.equals("A=KOLECKO;B=KRIZEK;X=CTVEREC;Y=TROJUHELNIK;L1=L1;R1=R1;L2=L2;R2=R2;L3=L3;R3=R3;START=START;SELECT=SELECT;MODE=SELECT")
                        && m2.equals(m3) && m3.endsWith("MODE=MENU") && t == 1,
                "A2) ps1OvladacMapa za behu hry: jadro vrati mapu (neznamy radek Q vynechan), null i \"\" = vychozi, klavesa ovladace dal jde");
        Thread.sleep(300);
        final String dn = NativePs1CoreBridge.denikSafe();
        over(dn.contains("OVLADAC TLACITKA (B355): A(spodni)=KOLECKO"), "A3) denik PS1: \"OVLADAC TLACITKA (B355): A(spodni)=KOLECKO ...\"");
        NativePs1CoreBridge.stopSafe();
        String am = "x", ak = "x";
        boolean pad = false;
        try { am = NativeAtariCoreBridge.devOvladacMapaSafe("A=K10"); ak = NativeAtariCoreBridge.devOvladacKatalogSafe(0, false); }
        catch (Throwable e) { pad = true; }
        over(!pad && am.isEmpty() && ak.isEmpty(), "B1) most Atari bez knihovny: devOvladacMapaSafe / devOvladacKatalogSafe vraci \"\" a nespadne");
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
