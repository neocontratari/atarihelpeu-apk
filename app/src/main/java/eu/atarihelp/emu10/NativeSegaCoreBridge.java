package eu.atarihelp.emu10;

/**
 * BUILD2PT_SEGA_NATIVE_CPP_IN_PLACE_NORMAL_UI_STAGE84
 * Small JNI bridge used directly by the normal Sega WebView UI.
 * No separate native test Activity is required for the in-place path.
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
}
