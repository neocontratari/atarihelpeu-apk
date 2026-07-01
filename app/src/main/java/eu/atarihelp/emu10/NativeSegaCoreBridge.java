package eu.atarihelp.emu10;

/**
 * BUILD2RM: Java -> JNI bridge for C++ core directly inside the normal Sega screen.
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

    // BUILD2RM: real-core adapter slot + C++ only QT-audio-kept + blackscreen guard lifecycle.
    // Without the real C/C++ Sega core source dropped into app/src/main/cpp/vendor, it must NOT fake gameplay.
    public static native String realCoreStatus();
    public static native String realCoreLoadRom(byte[] romBytes);
    public static native String realCoreStep();
    public static native int pullAudio(short[] pcmOut, int frames);
    public static native int pullAudioStereo(short[] pcmOut, int stereoFrames);
    public static native String setPerformanceMode(String mode);

    // BUILD2RM: tvrdy lifecycle stop pro normalni app stranky.
    // Vypne native worker + audio FIFO, aby Sega obraz/zvuk nezustaval pres Intro/VBXE/Atari/PS1.
    public static native String shutdown();
}
