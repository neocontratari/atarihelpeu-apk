// B307: skutecna trida NativeIntroBridge z appky v JVM na PC. Knihovna = skutecny C++ appky
// (../tv_intro.cpp jako libnapintro.so: nap_intro_gl.cpp + Mesa bez okna + OpenSL "karta").
// Zavola VSECHNY native metody tridy (verejne primo, soukrome pres reflexi) i verejne
// obaly tak, jak je vola MainActivity / NapDemoView - kdyby nazev nebo typ nesedel,
// spadne to tady (UnsatisfiedLinkError / spatna hodnota), ne az na telefonu.
// A overi, ze metody zalozni Javy (AudioTrack) jsou z tridy pryc.
import eu.atarihelp.emu10.NativeIntroBridge;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;

public class TestPodpisu {
    static int chyb = 0;
    static void over(boolean ok, String co) {
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
        if (!ok) chyb++;
    }
    static Object volej(String jm, Class<?>[] t, Object... a) throws Exception {
        Method m = NativeIntroBridge.class.getDeclaredMethod(jm, t);
        m.setAccessible(true);
        return m.invoke(null, a);
    }
    public static void main(String[] args) throws Exception {
        if (!NativeIntroBridge.jeNactena()) { System.out.println("CHYBA napintro: " + NativeIntroBridge.chybaNacteni()); System.exit(1); }
        TreeSet<String> nativni = new TreeSet<>();
        for (Method m : NativeIntroBridge.class.getDeclaredMethods())
            if (Modifier.isNative(m.getModifiers())) nativni.add(m.getName());
        System.out.println("native metody NativeIntroBridge (" + nativni.size() + "): " + nativni);
        String[] pryc = {"introNaplnZvuk", "introCasZvuku", "introZvukStartNative", "introZvukStopNative", "zvukStart", "zvukStop", "zvukKdo"};
        List<String> zbylo = new ArrayList<>();
        for (Method m : NativeIntroBridge.class.getDeclaredMethods())
            for (String p : pryc) if (m.getName().equals(p)) zbylo.add(p);
        over(zbylo.isEmpty(), "zalozni Java zvuk (AudioTrack: introNaplnZvuk, introCasZvuku, zvukStart...) je z tridy pryc " + zbylo);

        TreeSet<String> zavolano = new TreeSet<>();
        NativeIntroBridge.introNastavAssety(new android.content.res.AssetManager()); zavolano.add("introNastavAssety");
        int st = NativeIntroBridge.introStart(); zavolano.add("introStart");
        over(st == 3, "introStart() -> " + st + " (1 = zvuk C++ OpenSL ES, 2 = zvuk Reneho hry ve stope)");
        int hned = NativeIntroBridge.introDotyk(305f, 38f, 360, 780); zavolano.add("introDotyk");
        Thread.sleep(800);
        // plocha zadna (JVM) -> C++ vezme velikost od Javy: 360 x 780, tlacitko SKIP [263,23]-[347,53]
        int skip = NativeIntroBridge.introDotyk(305f, 38f, 360, 780);
        int jinam = NativeIntroBridge.introDotyk(180f, 390f, 360, 780);
        int skip2 = NativeIntroBridge.dotykSafe(610f, 76f, 720, 1560);
        over(hned == NativeIntroBridge.DOTYK_NIC && skip == NativeIntroBridge.DOTYK_SKIP && jinam == NativeIntroBridge.DOTYK_JINAM
                && skip2 == NativeIntroBridge.DOTYK_SKIP,
             "introDotyk(float,float,int,int): hned po startu " + hned + ", na SKIP " + skip + ", jinam " + jinam + ", dotykSafe 2x " + skip2);
        boolean prip = NativeIntroBridge.introPripraveno(); zavolano.add("introPripraveno");
        boolean hot = NativeIntroBridge.introHotovo(); zavolano.add("introHotovo");
        long sn = NativeIntroBridge.introSnimku(); zavolano.add("introSnimku");
        int cms = NativeIntroBridge.introCasMs(); zavolano.add("introCasMs");
        String info = NativeIntroBridge.introInfo(); zavolano.add("introInfo");
        int[] pole = new int[854 * 480];
        int g = NativeIntroBridge.introGrabFrame(pole); zavolano.add("introGrabFrame");
        NativeIntroBridge.introSetDisplaySurface(null); zavolano.add("introSetDisplaySurface");
        over(!prip && !hot && sn == 0 && cms == 0 && info != null && g == 0,
             "bez plochy: pripraveno " + prip + ", hotovo " + hot + ", snimku " + sn + ", cas " + cms + " ms, grabFrame " + g + ", info '" + info + "'");
        int id = (Integer) volej("proudOtevriNative", new Class<?>[0]); zavolano.add("proudOtevriNative");
        byte[] b = new byte[1 << 20];
        int n = (Integer) volej("proudDalsiNative", new Class<?>[]{int.class, byte[].class, int.class, int.class}, id, b, 100, 0); zavolano.add("proudDalsiNative");
        int sv = (Integer) volej("proudStavNative", new Class<?>[0]); zavolano.add("proudStavNative");
        String log = (String) volej("proudLogNative", new Class<?>[0]); zavolano.add("proudLogNative");
        volej("proudZavriNative", new Class<?>[]{int.class}, id); zavolano.add("proudZavriNative");
        over(id > 0 && n == 0 && (sv & 2) != 0 && log != null && log.contains("TV PROUD INTRO"),
             "proud na TV: otevri " + id + ", dalsi " + n + " (bez plochy zadny snimek), stav " + sv + ", log '" + (log == null ? "null" : log.trim()) + "'");
        // verejne obaly (jak je vola MainActivity)
        int id2 = NativeIntroBridge.proudOtevriSafe();
        int n2 = NativeIntroBridge.proudDalsiSafe(id2, b, 50, 0);
        int st2 = NativeIntroBridge.proudStavSafe();
        NativeIntroBridge.proudZavriSafe(id2);
        String log2 = NativeIntroBridge.proudLogSafe();
        int g2 = NativeIntroBridge.grabFrameSafe(pole);
        over(id2 > 0 && n2 == 0 && (st2 & 2) != 0 && log2 != null && g2 == 0 && NativeIntroBridge.infoSafe() != null
                && !NativeIntroBridge.pripravenoSafe() && NativeIntroBridge.snimkuSafe() == 0 && NativeIntroBridge.casMsSafe() == 0,
             "obaly: proudOtevriSafe " + id2 + ", proudDalsiSafe " + n2 + ", proudStavSafe " + st2 + ", grabFrameSafe " + g2);
        NativeIntroBridge.plochaSafe(null);
        NativeIntroBridge.introStop(); zavolano.add("introStop");
        NativeIntroBridge.stopSafe();
        over(zavolano.equals(nativni), "zavolany VSECHNY native metody tridy (" + zavolano.size() + " z " + nativni.size() + ")"
                + (zavolano.equals(nativni) ? "" : " chybi " + rozdil(nativni, zavolano)));
        System.out.println(chyb == 0 ? "VYSLEDEK: native metody NativeIntroBridge B307 (" + nativni.size() + ") - nazvy i typy sedi (0 chyb)" : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
    static TreeSet<String> rozdil(TreeSet<String> a, TreeSet<String> b) { TreeSet<String> r = new TreeSet<>(a); r.removeAll(b); return r; }
}
