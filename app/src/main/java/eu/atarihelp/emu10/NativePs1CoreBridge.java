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
    private static native String ps1Stop();
    private static native int ps1GrabFrame(int[] out);
    // PRIME KRESLENI: predame jadru plochu z aplikace, ono na ni kresli samo.
    // null = odpojit.
    private static native void ps1SetDisplaySurface(android.view.Surface surface);
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
    public static String stopSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Stop(); } catch (Throwable t) { return "PS1_STOP_CALL_FAIL " + t.getMessage(); }
    }
}
