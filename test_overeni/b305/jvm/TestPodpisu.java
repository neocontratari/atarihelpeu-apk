// B305: skutecne tridy NativeSegaCoreBridge a NativePs1CoreBridge z appky v JVM na PC
// (knihovny = skutecny nap_sega_native_proof.cpp a nap_ps1_native.cpp, viz ../tv_sega.cpp,
// ../tv_ps1.cpp). Vola NOVE native metody B305 primo (pres reflexi - obaly *Safe by
// UnsatisfiedLinkError schovaly) i pres obaly, ktere pouziva MainActivity.
// Kdyby nesedel nazev nebo typ, na telefonu by to spadlo / mlcelo.
import eu.atarihelp.emu10.NativePs1CoreBridge;
import eu.atarihelp.emu10.NativeSegaCoreBridge;
import java.lang.reflect.Method;

public class TestPodpisu {
    static int chyb = 0;

    static Object zavolej(Class<?> c, String jm, Class<?>[] typy, Object... a) throws Exception {
        Method m = c.getDeclaredMethod(jm, typy);
        m.setAccessible(true);
        return m.invoke(null, a);
    }

    static void jedno(Class<?> c, String druh, int verze, int vzork) throws Exception {
        System.out.println("=== " + c.getSimpleName() + " ===");
        int id = (Integer) zavolej(c, "proudOtevriNative", new Class<?>[0]);
        byte[] b = new byte[16];                          // schvalne male pole: paket musi pockat
        int n = 0, malo = 0;
        long t0 = System.currentTimeMillis();
        while (System.currentTimeMillis() - t0 < 3000) {
            n = (Integer) zavolej(c, "proudDalsiNative", new Class<?>[]{int.class, byte[].class, int.class, int.class}, id, b, 250, 0);
            if (n < 0 && n != Integer.MIN_VALUE) { malo = -n; b = new byte[-n + 65536]; continue; }
            if (n > 0) break;
        }
        int delka = (b[0] & 255) | ((b[1] & 255) << 8) | ((b[2] & 255) << 16) | ((b[3] & 255) << 24);
        int ver = b[6] & 255;
        int sr = (b[28] & 255) | ((b[29] & 255) << 8) | ((b[30] & 255) << 16) | ((b[31] & 255) << 24);
        System.out.println("proudOtevriNative -> " + id + ", proudDalsiNative s malym polem -> potreba " + malo + " B, pak paket " + n
                + " B: znacka " + (char) b[4] + (char) b[5] + ", verze " + ver + ", delka " + delka + ", zvuk " + sr + " Hz");
        if (id <= 0 || malo <= 0 || n != malo || delka != n - 4 || b[4] != 'N' || b[5] != 'A' || ver != verze || sr != vzork) { chyb++; System.out.println("CHYBA paket"); }
        int st = (Integer) zavolej(c, "proudStavNative", new Class<?>[0]);
        System.out.println("proudStavNative -> " + st + " (bit0 jadro bezi, bit1 nekdo se diva)");
        if (st != 3) { chyb++; System.out.println("CHYBA stav"); }
        zavolej(c, "proudZavriNative", new Class<?>[]{int.class}, id);
        String log = (String) zavolej(c, "proudLogNative", new Class<?>[0]);
        System.out.print("proudLogNative ->\n" + log);
        if (log == null || !log.contains("TV PROUD " + druh) || !log.contains("odpojen")) { chyb++; System.out.println("CHYBA log"); }
        int n2 = (Integer) zavolej(c, "proudDalsiNative", new Class<?>[]{int.class, byte[].class, int.class, int.class}, id, b, 10, 0);
        System.out.println("proudDalsiNative po zavreni -> " + n2 + " (ma byt Integer.MIN_VALUE)");
        if (n2 != Integer.MIN_VALUE) chyb++;
        if (zavolej(c, "proudLogNative", new Class<?>[0]) != null) { chyb++; System.out.println("CHYBA log podruhe neni null"); }
    }

    public static void main(String[] a) throws Exception {
        jedno(NativeSegaCoreBridge.class, "SEGA", 2, 48000);
        // obaly, ktere vola MainActivity
        int id = NativeSegaCoreBridge.proudOtevriSafe();
        byte[] b = new byte[1 << 20];
        int n = 0;
        for (int i = 0; i < 20 && n <= 0; i++) n = NativeSegaCoreBridge.proudDalsiSafe(id, b, 250, 3);
        System.out.println("Sega Safe: otevri " + id + ", dalsi " + n + " B, stav " + NativeSegaCoreBridge.proudStavSafe());
        NativeSegaCoreBridge.proudZavriSafe(id);
        if (id <= 0 || n <= 0 || NativeSegaCoreBridge.proudLogSafe() == null) chyb++;
        short[] pcm = new short[2048];
        int got = NativeSegaCoreBridge.pullAudioStereo(pcm, 1024);
        System.out.println("pullAudioStereo (B305: kopie zvuku do prenosu) -> " + got);

        System.out.println("PS1 knihovna nactena: " + NativePs1CoreBridge.isLoaded());
        if (!NativePs1CoreBridge.isLoaded()) { chyb++; }
        else {
            jedno(NativePs1CoreBridge.class, "PS1", 3, 44100);
            id = NativePs1CoreBridge.proudOtevriSafe();
            n = 0;
            for (int i = 0; i < 20 && n <= 0; i++) n = NativePs1CoreBridge.proudDalsiSafe(id, b, 250, 3);
            System.out.println("PS1 Safe: otevri " + id + ", dalsi " + n + " B, stav " + NativePs1CoreBridge.proudStavSafe());
            NativePs1CoreBridge.proudZavriSafe(id);
            if (id <= 0 || n <= 0 || NativePs1CoreBridge.proudLogSafe() == null) chyb++;
        }
        System.out.println(chyb == 0 ? "VYSLEDEK: vsech 10 novych native metod B305 (Sega i PS1) se zavolalo, nazvy i typy sedi (0 chyb)" : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
