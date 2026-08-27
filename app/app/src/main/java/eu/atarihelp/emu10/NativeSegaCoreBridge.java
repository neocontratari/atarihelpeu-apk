package eu.atarihelp.emu10;

/**
 * BUILD2RV: Java -> JNI bridge for C++ core directly inside the normal Sega screen.
 * Sega WebView/Java wrapper is disabled on this page; this bridge is the only Sega runtime path.
 */
public final class NativeSegaCoreBridge {
    static {
        System.loadLibrary("napsega_native_proof");
    }

    private NativeSegaCoreBridge() {}

    public static native String buildString();
    public static native String romInfo(byte[] romBytes);
    public static native void renderPattern(int width, int height, int frame, int[] argbOut);
    public static native void setInput(int key, boolean pressed);
    public static native String inputStatus();
    public static native void makeAudioTone(short[] pcmOut, int sampleRate, double hz);

    // BUILD2RV: real-core adapter slot + C++ only RR recovery safe-audit lifecycle.
    // Without the real C/C++ Sega core source dropped into app/src/main/cpp/vendor, it must NOT fake gameplay.
    public static native String realCoreStatus();
    public static native String realCoreLoadRom(byte[] romBytes);
    public static native String realCoreStep();

    // ===== OBRAZ PRIMO NA PLOCHU (stejna cesta jako PS1, od B117) =====
    // Jadro uz snimek vyrabi; tohle ho dostane na obrazovku pres OpenGL ES
    // bez snimani okna aplikace.
    private static native void setDisplaySurface(android.view.Surface surface);
    // Snimek pro TV - vraci sirku<<16|vysku, 0 = neni, zaporne = male pole.
    private static native int grabFrame(int[] out);

    public static void setDisplaySurfaceSafe(android.view.Surface s) {
        try { setDisplaySurface(s); } catch (Throwable ignored) {}
    }
    public static int grabFrameSafe(int[] out) {
        try { return grabFrame(out); } catch (Throwable ignored) { return 0; }
    }
    public static native int pullAudio(short[] pcmOut, int frames);
    public static native int pullAudioStereo(short[] pcmOut, int stereoFrames);
    public static native String setPerformanceMode(String mode);
    public static native String configureRuntime(int sdk, int cores, String model);

    // BUILD2RV: tvrdy lifecycle stop pro normalni app stranky.
    // Vypne native worker + audio FIFO, aby Sega obraz/zvuk nezustaval pres Intro/VBXE/Atari/PS1.
    public static native String shutdown();
}
