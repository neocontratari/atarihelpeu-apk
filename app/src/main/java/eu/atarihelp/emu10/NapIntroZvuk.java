package eu.atarihelp.emu10;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * BUILD2SA41: ZVUK INTRA V JAVE.
 *
 * PROC: intro si zvuk delalo samo v prohlizeci (Web Audio). Na telefonu
 * to hralo, ale NA TV NE - tam se zvuk bere z jadra pres
 * napTvWebAudioPush() a do WebView se nedosahne.
 *
 * Ted se vzorky pocitaji tady a posilaji na OBE strany zaroven:
 *   AudioTrack          -> reproduktor telefonu
 *   napTvWebAudioPush   -> WEB TV
 * Presne tou cestou, kterou uz pouziva Atari, Sega i PS1.
 *
 * Zvuky Segy ani Sony tu nejsou - ty si spocitaji jejich jadra sama.
 */
public final class NapIntroZvuk {

    public interface NaTv { void vzorky(short[] pcm, int ramcu, int vzorkovani); }

    private static final int SR = 44100;
    private static final int HLASU = 20;

    // ------------------------------------------------------------------
    //  Hlasy
    // ------------------------------------------------------------------
    private static final int CTVEREC = 0, TROJUHELNIK = 1, PILA = 2, SINUS = 3, SUM = 4;

    private static final class Hlas {
        boolean zni;
        int tvar;
        float f, a, faze, t, delka, ostrost;
    }

    private final Hlas[] hlasy = new Hlas[HLASU];
    private Thread vlakno;
    private volatile boolean bezi;
    private volatile NaTv naTv;
    private int sem = 1, takt = 0;
    private double taktDo = 0.0, cas = 0.0;

    public NapIntroZvuk() {
        for (int i = 0; i < HLASU; i++) hlasy[i] = new Hlas();
    }

    // ------------------------------------------------------------------
    //  Ovladani
    // ------------------------------------------------------------------
    public synchronized void start(NaTv naTv) {
        if (bezi) return;
        this.naTv = naTv;
        for (Hlas h : hlasy) h.zni = false;
        takt = 0; taktDo = 0.0; cas = 0.0;
        bezi = true;
        vlakno = new Thread(this::smycka, "nap-intro-zvuk");
        vlakno.setPriority(Thread.MAX_PRIORITY);
        vlakno.start();
    }

    public synchronized void stop() {
        bezi = false;
        if (vlakno != null) {
            try { vlakno.join(300); } catch (Throwable ignored) {}
            vlakno = null;
        }
    }

    public boolean bezi() { return bezi; }

    /** Klapnuti klavesy - kratky cvak, jako kdyz OS klapne reproduktorem. */
    public void klapnuti(boolean konecRadku) {
        if (konecRadku) {
            pridej(SUM, 0, 0.014f, 0.50f, 2f);
            pridej(CTVEREC, 210, 0.032f, 0.24f, 2f);
        } else {
            pridej(SUM, 0, 0.012f, 0.42f, 2f);
            pridej(CTVEREC, 150 + (nahoda() % 40), 0.018f, 0.20f, 2f);
        }
    }

    /** Dunive nabehnuti televize. */
    public void zapnutiTv() {
        pridej(SINUS, 55, 0.55f, 0.45f, 1f);
        pridej(SINUS, 15600, 0.90f, 0.05f, 1f);
    }

    // ------------------------------------------------------------------
    //  Vnitrek
    // ------------------------------------------------------------------
    private synchronized boolean pridej(int tvar, float f, float delka, float a, float ostrost) {
        for (Hlas h : hlasy) {
            if (!h.zni) {
                h.zni = true; h.tvar = tvar; h.f = f; h.a = a;
                h.faze = 0f; h.t = 0f; h.delka = delka; h.ostrost = ostrost;
                return true;
            }
        }
        return false;
    }

    private int nahoda() { sem = sem * 1664525 + 1013904223; return (sem >>> 8) & 0x7FFFFF; }
    private static float pul(int p) { return (float) (220.0 * Math.pow(2.0, p / 12.0)); }

    /** Skladba - ctyri casti po osmi taktech, kazda jinak. */
    private void hudba() {
        if (cas < taktDo) return;
        final double D = 0.155;
        taktDo = cas + D * 4;
        int cast = (takt >> 3) & 3;
        final int[][] AKORDY = {
            {  0,  0, -4, -4, -2, -2, -5, -5 },
            { -5, -5, -2, -2,  0,  0,  3,  3 },
            { -7, -7, -3, -3, -5, -5,  0,  0 },
            {  0, -2, -4, -5, -7, -5, -4, -2 },
        };
        final int[][] ARP = {
            { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 3, 7, 12 }, { 0, 5, 9, 12 },
        };
        final int[][] LEAD = {
            { 12, 10,  7, 10 }, { 15, 12, 19, 12 }, { 19, 17, 15, 12 }, { 24, 22, 19, 17 },
        };
        int akord = AKORDY[cast][takt % 8];

        pridej(cast == 0 ? TROJUHELNIK : (cast == 1 ? PILA : SINUS),
               pul(akord - 24), (float) (D * 3.4), 0.34f, 1f);

        int kolik = (cast == 1) ? 6 : 4;
        for (int i = 0; i < kolik; i++) {
            int nota = akord + ARP[cast][i % 4] + (i >= 4 ? 12 : 0);
            pridej(cast == 0 ? CTVEREC : SINUS, pul(nota),
                   (float) ((D * 4 / kolik) * 0.8), 0.11f, 1f);
        }
        if ((takt % 8) >= 4) {
            pridej(cast == 0 ? PILA : TROJUHELNIK, pul(akord + LEAD[cast][takt % 4]),
                   (float) (D * 1.7), 0.09f, 1f);
        }
        pridej(SUM, 0, 0.05f, 0.14f, 2f);
        if ((takt % 8) == 7) pridej(SUM, 0, 0.13f, 0.16f, 2f);
        takt++;
    }

    private synchronized int naplni(short[] ven, int ramcu) {
        hudba();
        final float dt = 1f / SR;
        for (int i = 0; i < ramcu; i++) {
            float v = 0f;
            for (Hlas h : hlasy) {
                if (!h.zni) continue;
                if (h.t >= h.delka) { h.zni = false; continue; }
                float obal = 1f - (h.t / h.delka);
                if (h.ostrost > 1.5f) obal *= obal;
                else                  obal = 0.25f + 0.75f * obal;
                float vz;
                if (h.tvar == SUM) {
                    vz = (nahoda() & 0xFFFF) / 32768f - 1f;
                } else {
                    h.faze += h.f * dt;
                    if (h.faze >= 1f) h.faze -= 1f;
                    if (h.tvar == CTVEREC)          vz = (h.faze < 0.5f) ? 1f : -1f;
                    else if (h.tvar == TROJUHELNIK) vz = 4f * Math.abs(h.faze - 0.5f) - 1f;
                    else if (h.tvar == PILA)        vz = 2f * h.faze - 1f;
                    else                            vz = (float) Math.sin(h.faze * 6.2831853);
                }
                v += vz * h.a * obal;
                h.t += dt;
            }
            v *= 0.45f;
            if (v > 1f) v = 1f;
            if (v < -1f) v = -1f;
            short s = (short) (v * 32000f);
            ven[i * 2] = s; ven[i * 2 + 1] = s;
            cas += dt;
        }
        return ramcu;
    }

    private void smycka() {
        AudioTrack at = null;
        try {
            int min = AudioTrack.getMinBufferSize(SR,
                    AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
            int buf = Math.max(min, SR / 8 * 4);
            at = new AudioTrack(AudioManager.STREAM_MUSIC, SR,
                    AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                    buf, AudioTrack.MODE_STREAM);
            at.play();
            final int RAMCU = 1024;
            short[] davka = new short[RAMCU * 2];
            while (bezi) {
                int n = naplni(davka, RAMCU);

                // ===== NA TV NEJDRIV, AZ POTOM DO REPRODUKTORU =====
                // Dve veci, ktere jsem mel spatne a ktere u Segy v kodu
                // uz davno stoji napsane:
                //
                // 1) POSLAT SE MUSI POCET SHORTU, NE RAMCU. Pole je stereo,
                //    takze na n ramcu pripada n*2 shortu. Posilal jsem n
                //    a na TV sla POLOVINA vzorku - odtud to kousani.
                //
                // 2) POSLAT SE MUSI DRIV, NEZ SE PREHRAJE. at.write() ceka,
                //    az reproduktor davku dohraje - kdyz se na TV posila
                //    az potom, prijde zvuk pozde a televize ho zahodi.
                NaTv t = naTv;
                if (t != null) {
                    try { t.vzorky(davka, n * 2, SR); } catch (Throwable ignored) {}
                }
                at.write(davka, 0, n * 2);
            }
        } catch (Throwable ignored) {
        } finally {
            if (at != null) { try { at.stop(); at.release(); } catch (Throwable ignored) {} }
        }
    }
}
