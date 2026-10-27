package eu.atarihelp.emu10;
// BUILD2SA2: samostatna .so - kdyz selze, Sega jede dal. Vsechna volani chranena.
public final class NativePs1CoreBridge {
    private static boolean loaded = false;
    private static String loadError = "";
    static {
        try { System.loadLibrary("napps1core"); loaded = true; }
        catch (Throwable t) { loadError = String.valueOf(t.getMessage()); }
    }
    private static native String ps1CoreInfo();
    // Jedina cesta pro obraz PS1: jadro kresli pres OpenGL ES a snimek jde do
    // monitoru. Prazdna cesta ke hre = start bez disku (menu BIOSu).
    private static native String ps1BootDoMonitoru(String systemDir, String saveDir, String gamePath);   // start bez disku -> menu BIOSu
    private static native String ps1Status();
    private static native String ps1MemCardInfo();
    private static native String ps1Stop();
    private static native int ps1GrabFrame(int[] out);
    // PRIME KRESLENI: predame jadru plochu z aplikace, ono na ni kresli samo.
    // null = odpojit.
    private static native void ps1SetDisplaySurface(android.view.Surface surface);
    // Prime stisky z klavesnice a joysticku - nejdou pres WebView, takze
    // maji nejnizsi moznou odezvu.
    private static native void ps1SetButton(int id, boolean down);
    private static native void ps1SetStick(int x, int y);
    // TV: snimek jde z C PRIMO do enkoderu H.264, bez Javy.
    private static native void ps1SetTvSurface(android.view.Surface surface);
    private static native int ps1PullAudio(short[] out, int frames);
    private static native int ps1PullTvAudio(short[] out);   // zvuk pro TV (kopie prehravaneho)
    private static native void ps1SetInput(int id, boolean down);
    private static native String ps1SaveState(String path);
    private static native String ps1LoadState(String path);
    // BUILD2SK99: rekne nativnimu kodu, kam prubezne (synchronne, hned) zapisovat
    // pohotovostni diagnostiku (napr. GLES/EGL inicializace u gpu-gles, SK98) -
    // primo do STEJNEHO souboru, ktery uz appka servíruje pres /log. Driv
    // nativni diagnostika (__android_log_print) mizela v Android logcatu, kam
    // Rene nema pristup - tohle to opravuje. Volat JEDNOU brzy (napr. onCreate),
    // pred jakymkoli PS1 bootem.
    private static native void ps1SetDiagLogPath(String path);
    public static void setDiagLogPathSafe(String path) {
        if (!loaded) return;
        try { ps1SetDiagLogPath(path); } catch (Throwable ignored) {}
    }
    // ===== B305: PS1 NA TV / PC CELA V C++ (jako Atari 130XE v B303) =====
    // C++ zabali kazdy snimek (JPEG jen zmenenych casti, puvodni rozliseni
    // hry) i zvuk do jednoho paketu, Java (/ps1.stream) jen posila bajty.
    private static native int proudOtevriNative();
    /** 4 B delka + paket; 0 = nic noveho, zaporne = male pole (-n), MIN_VALUE = chyba */
    private static native int proudDalsiNative(int id, byte[] out, int cekejMs, int zapisMs);
    private static native void proudZavriNative(int id);
    /** bit0 = jadro posila snimky, bit1 = nekdo se diva */
    private static native int proudStavNative();
    private static native String proudLogNative();

    public static boolean isLoaded() { return loaded; }
    public static int proudOtevriSafe() {
        if (!loaded) return 0;
        try { return proudOtevriNative(); } catch (Throwable t) { return 0; }
    }
    public static int proudDalsiSafe(int id, byte[] out, int cekejMs, int zapisMs) {
        if (!loaded) return Integer.MIN_VALUE;
        try { return proudDalsiNative(id, out, cekejMs, zapisMs); } catch (Throwable t) { return Integer.MIN_VALUE; }
    }
    public static void proudZavriSafe(int id) {
        if (!loaded) return;
        try { proudZavriNative(id); } catch (Throwable ignored) {}
    }
    public static int proudStavSafe() {
        if (!loaded) return 0;
        try { return proudStavNative(); } catch (Throwable t) { return 0; }
    }
    public static String proudLogSafe() {
        if (!loaded) return null;
        try { return proudLogNative(); } catch (Throwable t) { return null; }
    }

    public static int pullAudioSafe(short[] out, int frames) {
        if (!loaded) return 0;
        try { return ps1PullAudio(out, frames); } catch (Throwable t) { return 0; }
    }
    // Vyzvedne zvuk pro TV (kopii toho, co se prave prehrava). Vraci pocet shortu.
    public static int pullTvAudioSafe(short[] out) {
        if (!loaded) return 0;
        try { return ps1PullTvAudio(out); } catch (Throwable t) { return 0; }
    }
    // Spusti PS1 BEZ disku - jako skutecna konzole po zapnuti: nabehne BIOS
    // a jeho menu (MEMORY CARD / CD PLAYER). Obraz jde do monitoru v appce.
    public static String bootBiosSafe(String systemDir, String saveDir) {
        if (!loaded) return "PS1_BIOS_FAIL knihovna";
        try { return ps1BootDoMonitoru(systemDir, saveDir, ""); } catch (Throwable t) { return "PS1_BIOS_FAIL " + t; }
    }
    /** Spusti HRU do TEHOZ monitoru jako BIOS - zadne druhe platno. */
    public static String bootGameSafe(String systemDir, String saveDir, String gamePath) {
        if (!loaded) return "PS1_HRA_FAIL knihovna";
        if (gamePath == null || gamePath.isEmpty()) return "PS1_HRA_FAIL prazdna cesta";
        try { return ps1BootDoMonitoru(systemDir, saveDir, gamePath); }
        catch (Throwable t) { return "PS1_HRA_FAIL " + t; }
    }

    public static void setDisplaySurfaceSafe(android.view.Surface s) {
        if (!loaded) return;
        try { ps1SetDisplaySurface(s); } catch (Throwable ignored) {}
    }

    public static void setTvSurfaceSafe(android.view.Surface s) {
        if (!loaded) return;
        try { ps1SetTvSurface(s); } catch (Throwable ignored) {}
    }

    public static void setButtonSafe(int id, boolean down) {
        if (!loaded) return;
        try { ps1SetButton(id, down); } catch (Throwable ignored) {}
    }
    public static void setStickSafe(int x, int y) {
        if (!loaded) return;
        try { ps1SetStick(x, y); } catch (Throwable ignored) {}
    }

    public static int grabFrameSafe(int[] out) {
        if (!loaded) return 0;
        try { return ps1GrabFrame(out); } catch (Throwable t) { return 0; }
    }
    public static void setInputSafe(int id, boolean down) {
        if (!loaded) return;
        try { ps1SetInput(id, down); } catch (Throwable ignored) {}
    }
    public static String saveStateSafe(String path) {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1SaveState(path); } catch (Throwable t) { return "PS1_STATE_SAVE_CALL_FAIL " + t.getMessage(); }
    }
    public static String loadStateSafe(String path) {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1LoadState(path); } catch (Throwable t) { return "PS1_STATE_LOAD_CALL_FAIL " + t.getMessage(); }
    }
    public static String coreInfoSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1CoreInfo(); } catch (Throwable t) { return "PS1_CORE_CALL_FAIL " + t.getMessage(); }
    }
    public static String statusSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Status(); } catch (Throwable t) { return "PS1_STATUS_CALL_FAIL " + t.getMessage(); }
    }
    public static String memCardInfoSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1MemCardInfo(); } catch (Throwable t) { return "PS1_MEMCARD_CALL_FAIL " + t.getMessage(); }
    }
    public static String stopSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Stop(); } catch (Throwable t) { return "PS1_STOP_CALL_FAIL " + t.getMessage(); }
    }

    // ===== B308: KLAVESNICE PC V PROHLIZECI (TV / PC) A DENIK PS1 =====
    // Klavesa z prohlizece (e.code: KeyW, ArrowUp, Space ...) jde rovnou do C++,
    // ktere rozhodne, co udela (mapovani v nap_ps1_native.cpp). "PustVse" pusti
    // vse, "Zije" = prohlizec je tu (drzena klavesa), "STAV" = PS1 bezi.
    private static native String ps1PcKlavesa(String code, boolean dolu);
    // Denik PS1 z C++ (poslednich 800 radku: start, BIOS, disk, obraz, zdravi,
    // tlacitka, hlasky jadra, chyby) - UTF-8 bajty.
    private static native byte[] ps1Denik();

    public static String pcKlavesaSafe(String code, boolean dolu) {
        if (!loaded) return null;
        try { return ps1PcKlavesa(code == null ? "" : code, dolu); } catch (Throwable t) { return null; }
    }
    public static String denikSafe() {
        if (!loaded) return "";
        try {
            byte[] b = ps1Denik();
            return b == null ? "" : new String(b, java.nio.charset.StandardCharsets.UTF_8);
        } catch (Throwable t) { return "PS1_DENIK_CALL_FAIL " + t.getMessage(); }
    }

    // ===== B317: MYS PLAYSTATION (Dune 2000 ...) - cela logika v C++ (nap_ps1_mys.h) =====
    // Dotyk na obraze hry jako touchpad: akce 0 dolu, 1 nahoru, 2 pohyb, 3 zruseno;
    // x, y v obrazu, w, h = velikost obrazu (body stranky). Vraci stav (bit0 mys zapnuta,
    // bit1 leve tlacitko drzi, bit2 prst na obraze, bit3 / bit4 = tenhle dotyk byl klik
    // levym / pravym tlacitkem).
    private static native int ps1MysDotyk(int akce, int id, float x, float y, float w, float h);
    // rezim 0 vyp, 1 port 1, 2 port 1 + ovladac v portu 2, 3 port 2 + ovladac v portu 1;
    // citlivost v %. Zapamatuje se pro hru. Vraci "rezim|citlivost|stav|klic hry".
    private static native String ps1MysNastav(int rezim, int citlivost);
    private static native String ps1MysStav();
    // mys pocitace z prohlizece na TV / PC: posun a tlacitka (bit0 leve, bit1 prave)
    private static native String ps1PcMys(int dx, int dy, int tlacitka);

    public static int mysDotykSafe(int akce, int id, float x, float y, float w, float h) {
        if (!loaded) return 0;
        try { return ps1MysDotyk(akce, id, x, y, w, h); } catch (Throwable t) { return 0; }
    }
    public static String mysNastavSafe(int rezim, int citlivost) {
        if (!loaded) return "";
        try { String r = ps1MysNastav(rezim, citlivost); return r == null ? "" : r; } catch (Throwable t) { return ""; }
    }
    public static String mysStavSafe() {
        if (!loaded) return "";
        try { String r = ps1MysStav(); return r == null ? "" : r; } catch (Throwable t) { return ""; }
    }
    public static String pcMysSafe(int dx, int dy, int tlacitka) {
        if (!loaded) return null;
        try { return ps1PcMys(dx, dy, tlacitka); } catch (Throwable t) { return null; }
    }
}
