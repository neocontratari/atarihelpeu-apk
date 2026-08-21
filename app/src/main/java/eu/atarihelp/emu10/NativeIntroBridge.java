package eu.atarihelp.emu10;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.view.Surface;

import java.io.File;
import java.io.FileInputStream;

/**
 * BUILD2SA26: MUSTEK NA NATIVNI INTRO.
 *
 * Intro se kresli STEJNOU CESTOU JAKO PS1 - vlastni plocha, vlastni
 * vlakno, OpenGL ES pres EGL. Ne ve WebView.
 *
 * PROC: puvodni intro bezelo jako HTML+JavaScript na HLAVNIM VLAKNE
 * a TV si od nej musela obraz fotit pres PixelCopy celeho okna. Pri
 * zapnute WEB TV se pak kousalo uplne vsechno. PS1 a Sega to delaji
 * spravne - obraz jde primo z jadra a okno se nesnima vubec.
 *
 * Dve plochy, dve vlakna, spolecny cas:
 *   introSetDisplaySurface  -> obraz na telefonu
 *   introSetTvSurface       -> obraz do enkoderu pro TV
 */
public final class NativeIntroBridge {

    private NativeIntroBridge() {}

    static {
        try { System.loadLibrary("napintro"); }
        catch (Throwable t) { /* knihovna muze chybet - intro se proste nespusti */ }
    }

    // ---- obraz ----
    public static native void introStart();
    public static native void introSetDisplaySurface(Surface s);
    public static native void introSetTvSurface(Surface s);
    public static native boolean introHotovo();
    public static native long introSnimku();
    /**
     * Pujci posledni snimek pro TV - stejny tvar jako
     * Ps1GlTextureView.borrowFrame a NativeSegaCoreBridge.grabFrame.
     * Vraci (sirka&lt;&lt;16)|vyska, zaporne kdyz je pole male, 0 kdyz nic noveho.
     */
    public static native int introGrabFrame(int[] ven);

    /** Bezpecne - kdyz knihovna chybi, jen se nic nestane. */
    public static int grabFrameSafe(int[] ven) {
        try { return introGrabFrame(ven); } catch (Throwable t) { return 0; }
    }

    // ---- zvuk ----
    /** Naplni davku vzorku. Vraci pocet ramcu. */
    public static native int introNaplnZvuk(short[] buf, int ramcu);
    public static native long introVzorku();
    public static native void introKlapnuti(boolean konecRadku);
    /** Znelka ze zarizeni misto vlastni. ktera: 0 = Sega, 1 = PS1. */
    public static native void introNactiZnelku(int ktera, short[] vzorky, int vzorkovani);

    // ==================================================================
    //  ZVUK - AudioTrack krmeny z nativni syntezy
    // ==================================================================
    private static Thread zvukVlakno = null;
    private static volatile boolean zvukBezi = false;

    public static void zvukStart() {
        if (zvukBezi) return;
        zvukBezi = true;
        zvukVlakno = new Thread(() -> {
            AudioTrack at = null;
            try {
                final int SR = 44100;
                int min = AudioTrack.getMinBufferSize(SR,
                        AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                int buf = Math.max(min, SR / 8 * 4);      // asi 125 ms
                at = new AudioTrack(AudioManager.STREAM_MUSIC, SR,
                        AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                        buf, AudioTrack.MODE_STREAM);
                at.play();
                final int RAMCU = 1024;
                short[] davka = new short[RAMCU * 2];
                while (zvukBezi) {
                    int n = introNaplnZvuk(davka, RAMCU);
                    if (n <= 0) { Thread.sleep(5); continue; }
                    at.write(davka, 0, n * 2);
                }
            } catch (Throwable ignored) {
            } finally {
                if (at != null) { try { at.stop(); at.release(); } catch (Throwable ignored) {} }
            }
        }, "nap-intro-zvuk");
        zvukVlakno.setPriority(Thread.MAX_PRIORITY);
        zvukVlakno.start();
    }

    public static void zvukStop() {
        zvukBezi = false;
        if (zvukVlakno != null) {
            try { zvukVlakno.join(300); } catch (Throwable ignored) {}
            zvukVlakno = null;
        }
    }

    // ==================================================================
    //  ZNELKA ZE ZARIZENI
    //
    //  Uplne stejne, jako se hleda BIOS pro PS1: podivame se do slozky
    //  stazenych souboru. Kdyz tam soubor je, prehraje se; kdyz ne,
    //  zahraje se vlastni znelka. Do aplikace se nic nebali a nic se
    //  nikam nestahuje.
    // ==================================================================
    private static final String[] JMENA_SEGA = { "sega_start.wav", "SEGA_START.WAV", "sega.wav" };
    private static final String[] JMENA_PS1  = { "ps1_start.wav",  "PS1_START.WAV",  "ps1.wav"  };

    /** @param slozky kam se ma koukat (typicky Download/AtariHelp) */
    public static String nactiZnelkyZeZarizeni(File[] slozky) {
        StringBuilder zprava = new StringBuilder();
        zprava.append(zkusJednu(slozky, JMENA_SEGA, 0, "SEGA"));
        zprava.append(' ');
        zprava.append(zkusJednu(slozky, JMENA_PS1, 1, "PS1"));
        return zprava.toString();
    }

    private static String zkusJednu(File[] slozky, String[] jmena, int ktera, String popis) {
        if (slozky == null) return popis + "=bez-slozky";
        for (File d : slozky) {
            if (d == null || !d.isDirectory()) continue;
            for (String jm : jmena) {
                File f = new File(d, jm);
                if (!f.isFile() || f.length() < 64) continue;
                try {
                    Wav w = nactiWav(f);
                    if (w == null) continue;
                    introNactiZnelku(ktera, w.vzorky, w.vzorkovani);
                    return popis + "=" + jm + "(" + w.vzorky.length + "vz," + w.vzorkovani + "Hz)";
                } catch (Throwable t) {
                    return popis + "=chyba:" + t.getClass().getSimpleName();
                }
            }
        }
        return popis + "=vlastni";
    }

    private static final class Wav { short[] vzorky; int vzorkovani; }

    /**
     * Nacte jednoduchy WAV (PCM 16 bitu). Stereo se smicha na mono -
     * znelka nepotrebuje sirku a je to o polovinu mene pameti.
     */
    private static Wav nactiWav(File f) throws Exception {
        if (f.length() > 12 * 1024 * 1024) return null;      // pojistka
        byte[] d = new byte[(int) f.length()];
        FileInputStream in = new FileInputStream(f);
        try {
            int cti = 0;
            while (cti < d.length) {
                int k = in.read(d, cti, d.length - cti);
                if (k < 0) break;
                cti += k;
            }
        } finally { in.close(); }

        if (d.length < 44) return null;
        if (!(d[0]=='R' && d[1]=='I' && d[2]=='F' && d[3]=='F')) return null;
        if (!(d[8]=='W' && d[9]=='A' && d[10]=='V' && d[11]=='E')) return null;

        int kanalu = 0, vzorkovani = 0, bitu = 0;
        int datOd = -1, datDelka = 0;
        int p = 12;
        while (p + 8 <= d.length) {
            int velikost = le32(d, p + 4);
            if (velikost < 0) break;
            if (d[p]=='f' && d[p+1]=='m' && d[p+2]=='t' && d[p+3]==' ' && p + 8 + 16 <= d.length) {
                kanalu     = le16(d, p + 10);
                vzorkovani = le32(d, p + 12);
                bitu       = le16(d, p + 22);
            } else if (d[p]=='d' && d[p+1]=='a' && d[p+2]=='t' && d[p+3]=='a') {
                datOd = p + 8;
                // Nekterym souborum sedi v hlavicce nesmyslna delka
                // (napr. ze streamu). Bereme, co je opravdu v souboru.
                datDelka = Math.min(velikost, d.length - datOd);
                if (datDelka < 0) datDelka = d.length - datOd;
                break;
            }
            p += 8 + velikost + (velikost & 1);
        }
        if (datOd < 0 || bitu != 16 || kanalu < 1 || vzorkovani < 8000) return null;

        int ramcu = datDelka / (2 * kanalu);
        if (ramcu <= 0) return null;
        short[] mono = new short[ramcu];
        for (int i = 0; i < ramcu; i++) {
            int o = datOd + i * 2 * kanalu;
            int soucet = 0;
            for (int c = 0; c < kanalu; c++) soucet += (short) (le16(d, o + c * 2));
            mono[i] = (short) (soucet / kanalu);
        }
        Wav w = new Wav();
        w.vzorky = mono; w.vzorkovani = vzorkovani;
        return w;
    }

    private static int le16(byte[] d, int i) {
        return (d[i] & 0xFF) | ((d[i+1] & 0xFF) << 8);
    }
    private static int le32(byte[] d, int i) {
        return (d[i] & 0xFF) | ((d[i+1] & 0xFF) << 8)
             | ((d[i+2] & 0xFF) << 16) | ((d[i+3] & 0xFF) << 24);
    }
}
