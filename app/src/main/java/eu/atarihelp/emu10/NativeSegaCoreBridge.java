package eu.atarihelp.emu10;

/**
 * BUILD2PU: tiny Java -> JNI bridge for C++ proof directly inside the normal Sega screen.
 * This is not Sega gameplay yet; it only exposes ROM header/checksum, input, pattern render and PCM tone.
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
