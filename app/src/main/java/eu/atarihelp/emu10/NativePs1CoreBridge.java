package eu.atarihelp.emu10;
// BUILD2SA2: samostatna .so - kdyz selze, Sega jede dal. Vsechna volani chranena.
public final class NativePs1CoreBridge {
    private static boolean loaded = false;
    private static String loadError = "";
    static {
        try { System.loadLibrary("napps1core3"); loaded = true; }
        catch (Throwable t) { loadError = String.valueOf(t.getMessage()); }
    }
    private static native String ps1CoreInfo();
    private static native String ps1Boot(String systemDir, String saveDir, String gamePath);
    private static native String ps1Status();
    private static native String ps1BuildStamp();   // EMU10-O2
    private static native String ps1GlesStatus();   // EMU10-O2
    private static native String ps1Stop();
    private static native int ps1GrabFrame(int[] out);
    private static native int ps1PullAudio(short[] out, int frames);
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
    public static String bootSafe(String sys, String save, String game) {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Boot(sys, save, game); } catch (Throwable t) { return "PS1_BOOT_CALL_FAIL " + t.getMessage(); }
    }
    public static String statusSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Status(); } catch (Throwable t) { return "PS1_STATUS_CALL_FAIL " + t.getMessage(); }
    }
    public static String stopSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1Stop(); } catch (Throwable t) { return "PS1_STOP_CALL_FAIL " + t.getMessage(); }
    }

    // EMU10-O2: kdyz tyhle dve funkce v knihovne NEJSOU, je knihovna stara.
    // UnsatisfiedLinkError je tady informace, ne chyba - proto se nechytá tise.
    public static String buildStampSafe() {
        // EMU10-O3: knihovna se prejmenovala na napps1core3. Stara kopie
        // libnapps1core.so uz tenhle nazev nemuze obslouzit - takze kdyz se
        // tohle nacte, je to NUTNE cerstve prelozene jadro. A kdyz se
        // nenacte, CMake nic nevyrobil a je to videt.
        if (!loaded) return "JADRO SE VUBEC NENACETLO -> CMake nic neprelozil (" + loadError + ")";
        try { return ps1BuildStamp(); }
        catch (UnsatisfiedLinkError e) { return "STARE JADRO (razitko v knihovne chybi)"; }
        catch (Throwable t) { return "RAZITKO_FAIL " + t.getMessage(); }
    }
    public static String glesStatusSafe() {
        if (!loaded) return "JADRO SE VUBEC NENACETLO";
        try { return ps1GlesStatus(); }
        catch (UnsatisfiedLinkError e) { return "STARE JADRO (hlaseni gpu-gles v knihovne chybi)"; }
        catch (Throwable t) { return "GLES_STAV_FAIL " + t.getMessage(); }
    }
}
