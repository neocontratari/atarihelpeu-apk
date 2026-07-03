package eu.atarihelp.emu10;
// BUILD2SA1: samostatna .so knihovna napps1core - kdyz selze, Sega bezi dal nedotcena.
public final class NativePs1CoreBridge {
    private static boolean loaded = false;
    private static String loadError = "";
    static {
        try { System.loadLibrary("napps1core"); loaded = true; }
        catch (Throwable t) { loadError = String.valueOf(t.getMessage()); }
    }
    private static native String ps1CoreInfo();
    public static String coreInfoSafe() {
        if (!loaded) return "PS1_CORE_LOAD_FAIL " + loadError;
        try { return ps1CoreInfo(); } catch (Throwable t) { return "PS1_CORE_CALL_FAIL " + t.getMessage(); }
    }
}
