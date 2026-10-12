package eu.atarihelp.emu10;

import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.view.Surface;

/**
 * B304: MOST NA NOVE UVODNI DEMO N&P (cele v C++).
 *
 * Demo podle Reneho scenare (45 s, 5 scen) se kresli v C++ pres OpenGL ES
 * na vlastnim vlakne primo do plochy NapDemoView - zadne WebView, zadne
 * video. Pro TV / PC kresli C++ zvlast obraz 16:9 (introGrabFrame).
 *
 * Zvuk je cela stopa spocitana v C++ (bzuceni obrazovky, vrata, vitr,
 * ptaci, hromy, dest, hudba). Tady se jen prehrava pres AudioTrack a
 * stejne vzorky jdou na TV. C++ se po kazdem zapisu dozvi, kolik uz
 * reproduktor opravdu prehral - podle toho bezi obraz, takze sedi se
 * zvukem i na pomalejsim telefonu.
 */
public final class NativeIntroBridge {

    private NativeIntroBridge() {}

    private static boolean nactena = false;
    private static String chyba = "";

    static {
        try {
            System.loadLibrary("napintro");
            nactena = true;
        } catch (Throwable t) {
            nactena = false;
            chyba = t.getClass().getSimpleName() + ": " + t.getMessage();
        }
    }

    /** Je knihovna napintro v telefonu? (bez ni se pusti puvodni intro) */
    public static boolean jeNactena() { return nactena; }
    public static String chybaNacteni() { return chyba; }

    // ---- C++ ----
    public static native void introNastavAssety(AssetManager am);
    public static native void introStart();
    public static native void introStop();
    public static native void introSetDisplaySurface(Surface s);
    public static native boolean introHotovo();
    public static native boolean introPripraveno();
    public static native long introSnimku();
    public static native int introCasMs();
    /**
     * Posledni snimek pro TV (16:9) - stejny tvar jako Ps1GlTextureView.borrowFrame:
     * (sirka&lt;&lt;16)|vyska, zaporne kdyz je pole male, 0 kdyz nic noveho.
     */
    public static native int introGrabFrame(int[] ven);
    /** Davka zvuku (stereo 44,1 kHz). Vraci pocet ramcu, -1 = konec. */
    public static native int introNaplnZvuk(short[] buf, int ramcu);
    /** Kolik ramcu uz reproduktor prehral (AudioTrack.getPlaybackHeadPosition). */
    public static native void introCasZvuku(long hlava);
    /** Kratka zprava do logu: GPU, priprava, rychlost kresleni, zvuk. */
    public static native String introInfo();

    // ---- bezpecne obaly (kdyz knihovna chybi, nic se nestane) ----
    public static int grabFrameSafe(int[] ven) {
        if (!nactena) return 0;
        try { return introGrabFrame(ven); } catch (Throwable t) { return 0; }
    }
    public static void plochaSafe(Surface s) {
        if (!nactena) return;
        try { introSetDisplaySurface(s); } catch (Throwable ignored) {}
    }
    public static boolean hotovoSafe() {
        if (!nactena) return true;
        try { return introHotovo(); } catch (Throwable t) { return true; }
    }
    public static boolean pripravenoSafe() {
        if (!nactena) return false;
        try { return introPripraveno(); } catch (Throwable t) { return false; }
    }
    public static long snimkuSafe() {
        if (!nactena) return 0;
        try { return introSnimku(); } catch (Throwable t) { return 0; }
    }
    public static int casMsSafe() {
        if (!nactena) return 0;
        try { return introCasMs(); } catch (Throwable t) { return 0; }
    }
    public static String infoSafe() {
        if (!nactena) return "knihovna napintro chybi";
        try { return introInfo(); } catch (Throwable t) { return "?"; }
    }
    public static void stopSafe() {
        if (!nactena) return;
        try { introStop(); } catch (Throwable ignored) {}
    }

    // ==================================================================
    //  ZVUK - AudioTrack krmeny z C++ (a stejne vzorky na TV)
    // ==================================================================
    public interface NaTv { void vzorky(short[] pcm, int shortu, int vzorkovani); }

    private static Thread zvukVlakno = null;
    private static volatile boolean zvukBezi = false;
    private static volatile AudioTrack zvukTrack = null;

    public static synchronized void zvukStart(final NaTv naTv) {
        if (!nactena || zvukBezi) return;
        zvukBezi = true;
        zvukVlakno = new Thread(() -> {
            AudioTrack at = null;
            try {
                final int SR = 44100;
                int min = AudioTrack.getMinBufferSize(SR, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                int buf = Math.max(min, SR / 10 * 4);            // asi 100 ms
                at = new AudioTrack.Builder()
                        .setAudioAttributes(new AudioAttributes.Builder()
                                .setUsage(AudioAttributes.USAGE_GAME)
                                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                                .build())
                        .setAudioFormat(new AudioFormat.Builder()
                                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                                .setSampleRate(SR)
                                .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                                .build())
                        .setBufferSizeInBytes(buf)
                        .setTransferMode(AudioTrack.MODE_STREAM)
                        .build();
                zvukTrack = at;
                at.play();
                final int RAMCU = 1024;
                short[] davka = new short[RAMCU * 2];
                while (zvukBezi) {
                    int n = introNaplnZvuk(davka, RAMCU);
                    if (n < 0) break;                            // konec stopy / demo zastavene
                    if (n == 0) { Thread.sleep(5); continue; }
                    at.write(davka, 0, n * 2);
                    introCasZvuku(at.getPlaybackHeadPosition() & 0xFFFFFFFFL);
                    if (naTv != null) {
                        try { naTv.vzorky(davka, n * 2, SR); } catch (Throwable ignored) {}
                    }
                }
            } catch (Throwable ignored) {
            } finally {
                zvukTrack = null;
                if (at != null) {
                    try { at.pause(); at.flush(); } catch (Throwable ignored) {}
                    try { at.release(); } catch (Throwable ignored) {}
                }
            }
        }, "nap-demo-zvuk");
        zvukVlakno.setPriority(Thread.MAX_PRIORITY);
        zvukVlakno.start();
    }

    public static synchronized void zvukStop() {
        zvukBezi = false;
        AudioTrack at = zvukTrack;
        // blokujici zapis do AudioTrack se pauzou hned uvolni
        if (at != null) { try { at.pause(); } catch (Throwable ignored) {} }
        if (zvukVlakno != null) {
            try { zvukVlakno.join(400); } catch (Throwable ignored) {}
            zvukVlakno = null;
        }
    }
}
