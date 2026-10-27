// B317: skutecna trida NativePs1CoreBridge z appky v JVM na PC se SKUTECNYM nap_ps1_native.cpp
// (knihovna libnapps1core.so z ../../b308/ps1_harness.cpp - misto jadra PCSX "hra" 320x240,
// jinak vse jako v telefonu), -Xcheck:jni. Nove native metody mysi B317 se volaji primo pres
// reflexi (obaly *Safe by UnsatisfiedLinkError schovaly) - nazev, typy parametru i navratovy typ
// musi sedet presne - a pak pres obaly, ktere vola MainActivity (AHPS1).
import eu.atarihelp.emu10.NativePs1CoreBridge;
import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.file.Files;

public class TestB317Mys {
    static int chyb = 0;

    static void over(boolean ok, String co) {
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
        if (!ok) chyb++;
    }

    static Object zavolej(String jm, Class<?>[] typy, Object... a) throws Exception {
        Method m = NativePs1CoreBridge.class.getDeclaredMethod(jm, typy);
        m.setAccessible(true);
        return m.invoke(null, a);
    }

    public static void main(String[] a) throws Exception {
        String prac = a.length > 0 ? a[0] : "/tmp/nap_b317_jvm";
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Dune 2000.cue"))) {
            w.write("FILE \"Dune 2000.bin\" BINARY\n  TRACK 01 MODE2/2352\n");
        }
        over(NativePs1CoreBridge.isLoaded(), "knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        int nativnich = 0;
        for (Method m : NativePs1CoreBridge.class.getDeclaredMethods()) if (Modifier.isNative(m.getModifiers())) nativnich++;
        over(nativnich == 27, "native metod ve tride: " + nativnich + " (23 z B316 + 4 mys)");

        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Dune 2000.cue");
        over("PS1_HRA_OK".equals(r), "bootGameSafe (ps1BootDoMonitoru) -> " + r);
        Thread.sleep(300);

        // nove B317 primo pres reflexi - presne typy
        String s = (String) zavolej("ps1MysStav", new Class<?>[0]);
        over(s != null && s.startsWith("0|100|0|"), "ps1MysStav() -> String: \"" + s + "\" (hra bez zaznamu = mys vypnuta)");
        s = (String) zavolej("ps1MysNastav", new Class<?>[]{int.class, int.class}, 1, 120);
        over(s != null && s.startsWith("1|120|1|"), "ps1MysNastav(int, int) -> String: \"" + s + "\"");
        Object o = zavolej("ps1MysDotyk", new Class<?>[]{int.class, int.class, float.class, float.class, float.class, float.class},
                0, 7, 100f, 100f, 732f, 412f);
        over(o instanceof Integer && ((Integer) o & 5) == 5, "ps1MysDotyk(int, int, float x4) -> int: " + o + " (bit0 mys, bit2 prst na obraze)");
        Thread.sleep(60);
        o = zavolej("ps1MysDotyk", new Class<?>[]{int.class, int.class, float.class, float.class, float.class, float.class},
                1, 7, 101f, 100f, 732f, 412f);
        over(o instanceof Integer && ((Integer) o & 8) == 8, "tuknuti: ps1MysDotyk vratil " + o + " (bit3 = klik levym -> stranka zavibruje)");
        o = zavolej("ps1PcMys", new Class<?>[]{int.class, int.class, int.class}, 5, -3, 0);
        over("PS1:MYS".equals(o), "ps1PcMys(int, int, int) -> String: " + o);

        // obaly, ktere vola MainActivity (AHPS1.ps1MysNastav / ps1MysStav / ps1MysDotyk)
        s = NativePs1CoreBridge.mysNastavSafe(3, 80);
        over(s.startsWith("3|80|1|"), "mysNastavSafe(3, 80) -> \"" + s + "\"");
        over(NativePs1CoreBridge.mysStavSafe().startsWith("3|80|"), "mysStavSafe -> " + NativePs1CoreBridge.mysStavSafe());
        int b = NativePs1CoreBridge.mysDotykSafe(0, 9, 10f, 10f, 732f, 412f);
        NativePs1CoreBridge.mysDotykSafe(3, 9, 0f, 0f, 1f, 1f);
        over((b & 1) == 1, "mysDotykSafe -> " + b);
        over("PS1:MYS".equals(NativePs1CoreBridge.pcMysSafe(1, 1, 0)), "pcMysSafe -> PS1:MYS");
        s = NativePs1CoreBridge.mysNastavSafe(0, 80);
        over(s.startsWith("0|80|0|"), "mysNastavSafe(0, 80): mys vypnuta -> \"" + s + "\"");
        over("PS1:MYS_VYP".equals(NativePs1CoreBridge.pcMysSafe(1, 1, 1)), "mys vypnuta: pcMysSafe -> PS1:MYS_VYP");

        // zapamatovano pro hru ve slozce karty (saveDir)
        File f = new File(prac, "PS1_mys_pro_hry.txt");
        String obsah = f.exists() ? new String(Files.readAllBytes(f.toPath()), "UTF-8") : "";
        over(obsah.contains("Dune 2000.cue\t0\t80\n"), "PS1_mys_pro_hry.txt ve slozce karty: \"Dune 2000.cue  0  80\" (falesne jadro nema ID disku -> jmeno souboru)");
        String denik = NativePs1CoreBridge.denikSafe();
        over(denik.contains("MYS: mys v portu 1, port 2 prazdny (citlivost 120 %)") && denik.contains("MYS Z PC: prvni zprava z prohlizece"),
                "denik PS1: radky MYS (zapnuti, mys z PC)");
        System.out.println("stop: " + NativePs1CoreBridge.stopSafe());
        System.out.println("VYSLEDEK: " + (chyb == 0 ? "0 chyb" : chyb + " CHYB"));
        System.exit(chyb == 0 ? 0 : 1);
    }
}
