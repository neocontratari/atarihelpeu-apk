// B306: skutecne tridy NativeIntroBridge, NativeSegaCoreBridge a NativePs1CoreBridge z appky
// v JVM na PC. Knihovny = skutecne C++ appky prelozene pro PC (../tv_intro.cpp jako
// libnapintro.so, ../tv_sega6.cpp jako libnapsega_native_proof.so, ../tv_ps1_6.cpp jako
// libnapps1core.so). Vola NOVE native metody B306 (demo: zvuk v C++ a proud na TV) primo
// (pres reflexi - jsou private) i pres verejne obaly - kdyby nazev nebo typ nesedel,
// spadne to tady (UnsatisfiedLinkError), ne az na telefonu. Sega a PS1: native metody
// prenosu z B305 znovu (C++ se v B306 menilo).
import eu.atarihelp.emu10.NativeIntroBridge;
import eu.atarihelp.emu10.NativePs1CoreBridge;
import eu.atarihelp.emu10.NativeSegaCoreBridge;
import java.lang.reflect.Method;

public class TestPodpisu {
    static int chyb = 0;
    static Object volej(Class<?> c, String jm, Class<?>[] t, Object... a) throws Exception {
        Method m = c.getDeclaredMethod(jm, t);
        m.setAccessible(true);
        return m.invoke(null, a);
    }
    public static void main(String[] args) throws Exception {
        // ---- demo ----
        if (!NativeIntroBridge.jeNactena()) { System.out.println("CHYBA napintro: " + NativeIntroBridge.chybaNacteni()); System.exit(1); }
        NativeIntroBridge.introNastavAssety(new android.content.res.AssetManager());
        NativeIntroBridge.introStart();
        boolean nat = (Boolean) volej(NativeIntroBridge.class, "introZvukStartNative", new Class<?>[0]);
        System.out.println("introZvukStartNative -> " + nat + " (C++ OpenSL)");
        if (!nat) chyb++;
        Thread.sleep(300);
        int id = (Integer) volej(NativeIntroBridge.class, "proudOtevriNative", new Class<?>[0]);
        byte[] b = new byte[1 << 20];
        int n = (Integer) volej(NativeIntroBridge.class, "proudDalsiNative", new Class<?>[]{int.class, byte[].class, int.class, int.class}, id, b, 100, 0);
        int st = (Integer) volej(NativeIntroBridge.class, "proudStavNative", new Class<?>[0]);
        String log = (String) volej(NativeIntroBridge.class, "proudLogNative", new Class<?>[0]);
        System.out.println("proudOtevriNative -> " + id + ", proudDalsiNative -> " + n + " (bez plochy zadny snimek), proudStavNative -> " + st
                + ", proudLogNative -> " + (log == null ? "null" : log.trim()));
        if (id <= 0 || n != 0 || (st & 2) == 0 || log == null || !log.contains("TV PROUD INTRO")) chyb++;
        volej(NativeIntroBridge.class, "proudZavriNative", new Class<?>[]{int.class}, id);
        volej(NativeIntroBridge.class, "introZvukStopNative", new Class<?>[0]);
        // verejne obaly (jak je vola MainActivity)
        int id2 = NativeIntroBridge.proudOtevriSafe();
        int n2 = NativeIntroBridge.proudDalsiSafe(id2, b, 50, 0);
        int st2 = NativeIntroBridge.proudStavSafe();
        NativeIntroBridge.proudZavriSafe(id2);
        String log2 = NativeIntroBridge.proudLogSafe();
        NativeIntroBridge.zvukStart(null);
        String kdo = NativeIntroBridge.zvukKdo();
        NativeIntroBridge.zvukStop();
        String kdo2 = NativeIntroBridge.zvukKdo();
        System.out.println("obaly: proudOtevriSafe " + id2 + ", proudDalsiSafe " + n2 + ", proudStavSafe " + st2
                + ", zvukKdo po zvukStart '" + kdo + "', po zvukStop '" + kdo2 + "'");
        if (id2 <= 0 || n2 != 0 || log2 == null || !"C++ OpenSL ES".equals(kdo) || !"nic".equals(kdo2)) chyb++;
        NativeIntroBridge.stopSafe();
        // ---- Sega a PS1 (native metody prenosu, C++ z B306) ----
        int sId = NativeSegaCoreBridge.proudOtevriSafe();
        Thread.sleep(200);
        int sN = NativeSegaCoreBridge.proudDalsiSafe(sId, b, 250, 0);
        int sSt = NativeSegaCoreBridge.proudStavSafe();
        NativeSegaCoreBridge.proudZavriSafe(sId);
        String sLog = NativeSegaCoreBridge.proudLogSafe();
        System.out.println("Sega: proudOtevriSafe " + sId + ", proudDalsiSafe " + sN + " B, proudStavSafe " + sSt + ", log " + (sLog == null ? 0 : sLog.split("\n").length) + " radky");
        if (sId <= 0 || sN <= 32 || (sSt & 1) == 0 || sLog == null) chyb++;
        if (NativePs1CoreBridge.isLoaded()) {
            int pId = NativePs1CoreBridge.proudOtevriSafe();
            int pN = 0;
            for (int i = 0; i < 20 && pN <= 0; i++) pN = NativePs1CoreBridge.proudDalsiSafe(pId, b, 250, 0);
            int pSt = NativePs1CoreBridge.proudStavSafe();
            NativePs1CoreBridge.proudZavriSafe(pId);
            String pLog = NativePs1CoreBridge.proudLogSafe();
            System.out.println("PS1: proudOtevriSafe " + pId + ", proudDalsiSafe " + pN + " B, proudStavSafe " + pSt + ", log " + (pLog == null ? 0 : pLog.split("\n").length) + " radky");
            if (pId <= 0 || pN <= 32 || (pSt & 1) == 0 || pLog == null) chyb++;
        } else { System.out.println("CHYBA PS1 knihovna"); chyb++; }
        System.out.println(chyb == 0 ? "VYSLEDEK: nove native metody B306 (demo: 7) + prenos Sega / PS1 (10) - nazvy i typy sedi (0 chyb)" : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
