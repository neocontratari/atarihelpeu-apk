package eu.atarihelp.emu10;

import android.content.res.AssetManager;
import android.view.Surface;

/**
 * B304: MOST NA NOVE UVODNI DEMO N&P (cele v C++).
 *
 * Demo podle Reneho scenare (45 s, 5 scen; B309: 59 s - delsi ukazky her) se kresli v C++ pres OpenGL ES
 * na vlastnim vlakne primo do plochy NapDemoView - zadne WebView, zadne
 * video. Pro TV / PC kresli C++ zvlast obraz 16:9.
 *
 * Zvuk je cela stopa spocitana v C++ a prehrava ji C++ (OpenSL ES). Na TV / PC
 * jde demo proudem z C++ (/intro.stream - obraz JPEG + zvuk v jednom paketu,
 * jako Sega a PS1 v B305). C++ vi, kolik uz reproduktor opravdu prehral -
 * podle toho bezi obraz, takze sedi se zvukem i na pomalejsim telefonu.
 *
 * B307: Java z intra ven, co nejvic to jde - zalozni AudioTrack je pryc
 * (zvuk startuje a konci C++ samo v introStart / introStop), tlacitko SKIP
 * kresli C++ a o dotyku rozhoduje C++ (introDotyk). Java uz jen preda
 * plochu, dotyk a bajty proudu na TV (a pro stare prohlizece bez
 * ReadableStream hotovy snimek z C++ - introGrabFrame).
 */
public final class NativeIntroBridge {

    private NativeIntroBridge() {}

    private static boolean nactena = false;
    private static String chyba = "";

    static {
        try {
            System.loadLibrary("napintro");
            nactena = true;
        } catch (Throwable t) {
            nactena = false;
            chyba = t.getClass().getSimpleName() + ": " + t.getMessage();
        }
    }

    /** Je knihovna napintro v telefonu? (bez ni se pusti puvodni intro) */
    public static boolean jeNactena() { return nactena; }
    public static String chybaNacteni() { return chyba; }

    /** Dotyk na tlacitku SKIP / jinam (vysledek introDotyk). */
    public static final int DOTYK_NIC = 0, DOTYK_SKIP = 1, DOTYK_JINAM = 2;

    // ---- C++ ----
    public static native void introNastavAssety(AssetManager am);
    /**
     * Novy beh dema; C++ rovnou pusti i zvuk (OpenSL ES).
     * Bity: 1 = zvuk hraje C++, 2 = zvuk Reneho hry (Pitt-Kitt) je ve stope.
     */
    public static native int introStart();
    /** Konec dema: C++ zastavi kresleni i zvuk. */
    public static native void introStop();
    public static native void introSetDisplaySurface(Surface s);
    public static native boolean introHotovo();
    public static native boolean introPripraveno();
    public static native long introSnimku();
    public static native int introCasMs();
    /**
     * B307: dotyk (zvednuti prstu) v bodech plochy o velikosti w x h.
     * 0 = nic (hned po startu), 1 = tlacitko SKIP, 2 = jinam.
     */
    public static native int introDotyk(float x, float y, int w, int h);
    /**
     * Posledni snimek pro TV (16:9) - stejny tvar jako Ps1GlTextureView.borrowFrame:
     * (sirka&lt;&lt;16)|vyska, zaporne kdyz je pole male, 0 kdyz nic noveho.
     * Jen pro stare prohlizece bez ReadableStream (ostatni berou /intro.stream).
     */
    public static native int introGrabFrame(int[] ven);
    /** Kratka zprava do logu: GPU, priprava, rychlost kresleni, zvuk. */
    public static native String introInfo();
    /** B314: 1 = anglicke titulky, 0 = ceske (+ anglicke pod nimi). */
    private static native void introJazyk(int en);
    // B306: TV / PC proudem z C++ (stejne jako NativeSegaCoreBridge / NativePs1CoreBridge)
    private static native int proudOtevriNative();
    private static native int proudDalsiNative(int id, byte[] out, int cekejMs, int zapisMs);
    private static native void proudZavriNative(int id);
    private static native int proudStavNative();
    private static native String proudLogNative();

    public static int proudOtevriSafe() {
        if (!nactena) return 0;
        try { return proudOtevriNative(); } catch (Throwable t) { return 0; }
    }
    /** pocet bajtu (4 B delka + paket), 0 = nic noveho, zaporne = male pole, MIN_VALUE = chyba */
    public static int proudDalsiSafe(int id, byte[] out, int cekejMs, int zapisMs) {
        if (!nactena) return Integer.MIN_VALUE;
        try { return proudDalsiNative(id, out, cekejMs, zapisMs); } catch (Throwable t) { return Integer.MIN_VALUE; }
    }
    public static void proudZavriSafe(int id) {
        if (!nactena) return;
        try { proudZavriNative(id); } catch (Throwable ignored) {}
    }
    /** bit0 = demo kresli, bit1 = nekdo se diva */
    public static int proudStavSafe() {
        if (!nactena) return 0;
        try { return proudStavNative(); } catch (Throwable t) { return 0; }
    }
    public static String proudLogSafe() {
        if (!nactena) return null;
        try { return proudLogNative(); } catch (Throwable t) { return null; }
    }

    // ---- bezpecne obaly (kdyz knihovna chybi, nic se nestane) ----
    public static int grabFrameSafe(int[] ven) {
        if (!nactena) return 0;
        try { return introGrabFrame(ven); } catch (Throwable t) { return 0; }
    }
    public static void plochaSafe(Surface s) {
        if (!nactena) return;
        try { introSetDisplaySurface(s); } catch (Throwable ignored) {}
    }
    /** B307: o dotyku rozhoduje C++ (tlacitko SKIP kresli C++). */
    public static int dotykSafe(float x, float y, int w, int h) {
        if (!nactena) return DOTYK_JINAM;
        try { return introDotyk(x, y, w, h); } catch (Throwable t) { return DOTYK_JINAM; }
    }
    public static boolean hotovoSafe() {
        if (!nactena) return true;
        try { return introHotovo(); } catch (Throwable t) { return true; }
    }
    public static boolean pripravenoSafe() {
        if (!nactena) return false;
        try { return introPripraveno(); } catch (Throwable t) { return false; }
    }
    public static long snimkuSafe() {
        if (!nactena) return 0;
        try { return introSnimku(); } catch (Throwable t) { return 0; }
    }
    public static int casMsSafe() {
        if (!nactena) return 0;
        try { return introCasMs(); } catch (Throwable t) { return 0; }
    }
    public static String infoSafe() {
        if (!nactena) return "knihovna napintro chybi";
        try { return introInfo(); } catch (Throwable t) { return "?"; }
    }
    public static void stopSafe() {
        if (!nactena) return;
        try { introStop(); } catch (Throwable ignored) {}
    }
    /**
     * B314: jazyk titulku intra - cesky: titulky cesky + anglicky pod nimi (jak to Rene
     * navrhl), anglicky: jen anglicke titulky a anglicke zaverecne titulky. false = knihovna chybi.
     */
    public static boolean jazykSafe(boolean en) {
        if (!nactena) return false;
        try { introJazyk(en ? 1 : 0); return true; } catch (Throwable t) { return false; }
    }
}
