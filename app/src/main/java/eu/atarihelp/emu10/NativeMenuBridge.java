package eu.atarihelp.emu10;

import android.content.res.AssetManager;
import android.view.Surface;

/**
 * B311: MOST NA HLAVNI MENU (karusel velkych karet, cele v C++).
 *
 * Rene: "cela ta uvodni stranka musi byt uplne jinak - zadny nudny staticky
 * ikony pod sebou. Plynule horizontalni skrolovani velkych obrazku a karet
 * ... napis to modularne a dynamicky ... cistej lehkej C++ kod pro grafiku,
 * at to lita s nulovou odezvou."
 *
 * Menu kresli C++ (OpenGL ES na vlastnim vlakne) do NapMenuView. Karty jsou
 * v assets/menu/karty.txt - novou masinu pridas obrazkem a jednim radkem,
 * v kodu se nic nemeni. Java jen preda plochu, kazdy bod prstu a klavesy;
 * C++ vrati, co se ma stat (spustit kartu, OPTIONS, nic) - Java pak prepne
 * stranku. Na TV / PC jde menu proudem z C++ (/menu.stream, jako uvodni demo).
 */
public final class NativeMenuBridge {

    private NativeMenuBridge() {}

    private static boolean nactena = false;
    private static String chyba = "";

    static {
        try {
            System.loadLibrary("napmenu");
            nactena = true;
        } catch (Throwable t) {
            nactena = false;
            chyba = t.getClass().getSimpleName() + ": " + t.getMessage();
        }
    }

    /** Je knihovna napmenu v telefonu? (bez ni zustane menu v HTML - index.html) */
    public static boolean jeNactena() { return nactena; }
    public static String chybaNacteni() { return chyba; }

    // vysledek dotyku a klaves (stejne jako napmenu::Akce v C++)
    public static final int AKCE_NIC = 0, AKCE_KARTA = 1000, AKCE_NASTAVENI = 2000, AKCE_ZAMCENO = 3000;
    // klavesy (napmenu::Klavesa)
    public static final int KL_VLEVO = 1, KL_VPRAVO = 2, KL_POTVRD = 3, KL_ZACATEK = 4, KL_KONEC = 5;
    // dotyk (akce pro menuDotyk)
    public static final int DOTYK_DOLU = 0, DOTYK_TAH = 1, DOTYK_NAHORU = 2, DOTYK_ZRUSEN = 3;
    /** Jak dlouho bezi animace spusteni karty (C++ SPUSTENI_S) - pak Java prepne stranku. */
    public static final int SPUSTENI_MS = 300;

    // ---- C++ ----
    private static native int menuNastavAssety(AssetManager am);
    private static native int menuKaret();
    private static native String menuKarta(int i);
    private static native void menuZamek(String klic, boolean zamceno, String text);
    private static native void menuPlocha(Surface s);
    private static native void menuUkaz(int jak);
    private static native void menuNastavPrah(float px);
    private static native int menuDotyk(int akce, float x, float y, long casMs, int w, int h);
    private static native int menuKlavesa(int kl);
    private static native int menuVybrana();
    private static native void menuSkoc(int i);
    private static native boolean menuPripraveno();
    private static native boolean menuChyba();
    private static native long menuSnimku();
    private static native String menuInfo();
    private static native String menuLog();
    private static native int menuGrabFrame(int[] ven);
    private static native void menuNastav(float hz, int tvKvalita);       // B313
    // TV / PC proudem z C++ (stejne jako NativeIntroBridge)
    private static native int proudOtevriNative();
    private static native int proudDalsiNative(int id, byte[] out, int cekejMs, int zapisMs);
    private static native void proudZavriNative(int id);
    private static native int proudStavNative();
    private static native String proudLogNative();

    // ---- bezpecne obaly (kdyz knihovna chybi, nic se nestane) ----
    /** Nacte karty z menu/karty.txt; vraci pocet karet (0 = menu v C++ nejde). */
    public static int nastavAssetySafe(AssetManager am) {
        if (!nactena) return 0;
        try { return menuNastavAssety(am); } catch (Throwable t) { return 0; }
    }
    public static int karetSafe() {
        if (!nactena) return 0;
        try { return menuKaret(); } catch (Throwable t) { return 0; }
    }
    /** "id|NAZEV|cil|zamek" nebo null */
    public static String kartaSafe(int i) {
        if (!nactena) return null;
        try { return menuKarta(i); } catch (Throwable t) { return null; }
    }
    public static void zamekSafe(String klic, boolean zamceno, String text) {
        if (!nactena) return;
        try { menuZamek(klic, zamceno, text == null ? "" : text); } catch (Throwable ignored) {}
    }
    public static void plochaSafe(Surface s) {
        if (!nactena) return;
        try { menuPlocha(s); } catch (Throwable ignored) {}
    }
    /** jak: 1 = nastup (karty prileti), 0 = rychle prolnuti (navrat z emulatoru) */
    public static void ukazSafe(int jak) {
        if (!nactena) return;
        try { menuUkaz(jak); } catch (Throwable ignored) {}
    }
    public static void prahSafe(float px) {
        if (!nactena) return;
        try { menuNastavPrah(px); } catch (Throwable ignored) {}
    }
    public static int dotykSafe(int akce, float x, float y, long casMs, int w, int h) {
        if (!nactena) return AKCE_NIC;
        try { return menuDotyk(akce, x, y, casMs, w, h); } catch (Throwable t) { return AKCE_NIC; }
    }
    public static int klavesaSafe(int kl) {
        if (!nactena) return AKCE_NIC;
        try { return menuKlavesa(kl); } catch (Throwable t) { return AKCE_NIC; }
    }
    public static int vybranaSafe() {
        if (!nactena) return 0;
        try { return menuVybrana(); } catch (Throwable t) { return 0; }
    }
    public static void skocSafe(int i) {
        if (!nactena) return;
        try { menuSkoc(i); } catch (Throwable ignored) {}
    }
    public static boolean pripravenoSafe() {
        if (!nactena) return false;
        try { return menuPripraveno(); } catch (Throwable t) { return false; }
    }
    public static boolean chybaSafe() {
        if (!nactena) return true;
        try { return menuChyba(); } catch (Throwable t) { return true; }
    }
    public static long snimkuSafe() {
        if (!nactena) return 0;
        try { return menuSnimku(); } catch (Throwable t) { return 0; }
    }
    public static String infoSafe() {
        if (!nactena) return "knihovna napmenu chybi: " + chyba;
        try { return menuInfo(); } catch (Throwable t) { return "?"; }
    }
    /** Radky do logu appky (varovani karet, jake obrazky karty maji) - vyzvedne a smaze. */
    public static String logSafe() {
        if (!nactena) return null;
        try { return menuLog(); } catch (Throwable t) { return null; }
    }
    /**
     * B313: obnovovaci frekvence displeje (Display.getRefreshRate: C++ podle ni meri,
     * jestli menu stiha) a kvalita TV z OPTIONS (0 NIZKA 854x480, 1 STREDNI 1024x576,
     * 2 VYSOKA 1280x720 - velikost obrazu menu na TV / PC).
     */
    public static void nastavSafe(float hz, int tvKvalita) {
        if (!nactena) return;
        try { menuNastav(hz, tvKvalita); } catch (Throwable ignored) {}
    }
    /** Snimek pro stare prohlizece bez ReadableStream: (sirka<<16)|vyska, zaporne = male pole, 0 = nic. */
    public static int grabFrameSafe(int[] ven) {
        if (!nactena) return 0;
        try { return menuGrabFrame(ven); } catch (Throwable t) { return 0; }
    }
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
    /** bit0 = menu kresli, bit1 = nekdo se diva */
    public static int proudStavSafe() {
        if (!nactena) return 0;
        try { return proudStavNative(); } catch (Throwable t) { return 0; }
    }
    public static String proudLogSafe() {
        if (!nactena) return null;
        try { return proudLogNative(); } catch (Throwable t) { return null; }
    }
}
