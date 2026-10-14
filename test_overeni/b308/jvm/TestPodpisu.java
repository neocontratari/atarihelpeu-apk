// B308: skutecna trida NativePs1CoreBridge z appky v JVM na PC se SKUTECNYM
// nap_ps1_native.cpp (knihovna libnapps1core.so z ../ps1_harness.cpp - misto jadra PCSX
// "hra" 320x240, jinak vse jako v telefonu). Vola NOVE native metody B308 primo pres
// reflexi (obaly *Safe by UnsatisfiedLinkError schovaly) i pres obaly, ktere pouziva
// MainActivity. Kdyby nesedel nazev nebo typ (napr. byte[] u ps1Denik), na telefonu by
// to spadlo nebo mlcelo.
import eu.atarihelp.emu10.NativePs1CoreBridge;
import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;

public class TestPodpisu {
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
        String prac = a.length > 0 ? a[0] : "/tmp/nap_b308_jvm";
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Doom.cue"))) {
            w.write("FILE \"Doom.bin\" BINARY\n  TRACK 01 MODE2/2352\n");
        }
        over(NativePs1CoreBridge.isLoaded(), "knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        int nativnich = 0;
        for (Method m : NativePs1CoreBridge.class.getDeclaredMethods()) if (Modifier.isNative(m.getModifiers())) nativnich++;
        System.out.println("native metod ve tride: " + nativnich);

        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Doom.cue");
        over("PS1_HRA_OK".equals(r), "bootGameSafe (ps1BootDoMonitoru) -> " + r);
        Thread.sleep(300);

        // nove B308 primo (reflexe) - nazev a typ musi sedet
        String k = (String) zavolej("ps1PcKlavesa", new Class<?>[]{String.class, boolean.class}, "KeyK", true);
        over("PS1:KRIZEK".equals(k), "ps1PcKlavesa(String, boolean) -> String: " + k);
        Object d = zavolej("ps1Denik", new Class<?>[0]);
        over(d instanceof byte[] && ((byte[]) d).length > 100, "ps1Denik() -> byte[] (" + (d instanceof byte[] ? ((byte[]) d).length : -1) + " B)");
        // obaly, ktere vola MainActivity
        over("PS1:KRIZEK".equals(NativePs1CoreBridge.pcKlavesaSafe("KeyK", false)), "pcKlavesaSafe (MainActivity /klavesa)");
        over("PS1:PUSTENO".equals(NativePs1CoreBridge.pcKlavesaSafe("PustVse", false)), "pcKlavesaSafe PustVse");
        over(NativePs1CoreBridge.pcKlavesaSafe(null, true) != null, "pcKlavesaSafe(null) nespadne");
        String den = NativePs1CoreBridge.denikSafe();
        over(den.contains("PS1 START: HRA") && den.contains("CUE: 1 stop") && den.contains("TV KLAVESNICE: KeyK dolu -> KRIZEK")
             && den.contains("BEZI HRA"), "denikSafe (AHPS1.ps1Denik, LOG appky): start, CUE, klavesnice");
        String s = NativePs1CoreBridge.saveStateSafe(prac + "/pozice.state");
        over(s.startsWith("PS1_STATE_SAVE_OK"), "saveStateSafe -> " + s.replaceAll("path=.*", "path=..."));
        String l = NativePs1CoreBridge.loadStateSafe(prac + "/pozice.state");
        over(l.startsWith("PS1_STATE_LOAD_OK"), "loadStateSafe -> " + l.replaceAll("path=.*", "path=..."));
        String st = NativePs1CoreBridge.statusSafe();
        over(st.startsWith("PS1_RUN running=YES"), "statusSafe (hra bezi -> running=YES) -> " + st.substring(0, Math.min(40, st.length())));
        over(NativePs1CoreBridge.memCardInfoSafe().startsWith("PS1_MEMCARD"), "memCardInfoSafe");
        over("PS1_STOPPED".equals(NativePs1CoreBridge.stopSafe()), "stopSafe");
        over(NativePs1CoreBridge.statusSafe().startsWith("PS1_RUN running=NO"), "statusSafe po stopSafe -> running=NO");
        String bios = NativePs1CoreBridge.bootBiosSafe(prac, prac);
        over("PS1_BIOS_OK".equals(bios), "bootBiosSafe -> " + bios);
        NativePs1CoreBridge.stopSafe();
        den = NativePs1CoreBridge.denikSafe();
        over(den.contains("PS1 KONEC") && den.contains("PS1 START: BEZ DISKU"), "denik: konec hry a start BIOSu");
        byte[] utf = den.getBytes(StandardCharsets.UTF_8);
        System.out.println("denik: " + den.split("\n").length + " radku, " + utf.length + " B");
        System.out.println(chyb == 0 ? "VYSLEDEK: nove native metody B308 (ps1PcKlavesa, ps1Denik) i obaly sedi, 0 chyb"
                                     : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
