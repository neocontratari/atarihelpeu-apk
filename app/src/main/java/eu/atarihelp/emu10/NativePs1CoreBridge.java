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
    private static native String ps1Boot(String systemDir, String saveDir, String gamePath);
    private static native String ps1Status();
    private static native String ps1Stop();
    private static native int ps1GrabFrame(int[] out);
    private static native int ps1PullAudio(short[] out, int frames);
    private static native void ps1SetInput(int id, boolean down);
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
}
