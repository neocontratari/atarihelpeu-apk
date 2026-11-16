package eu.atarihelp.emu10;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Process;
import android.os.SystemClock;
import android.view.PixelCopy;
import android.view.View;
import android.view.ViewTreeObserver;

import java.nio.ByteBuffer;

/**
 * B333: STRANKY APPKY NA TV / PC CELE Z C++ - BEZ JAVY (Rene: "tady budu nekompromisni - vypni Javu").
 *
 * Hry, uvodni demo a hlavni menu jdou na TV proudem z C++ (B303-B311). Stranky (OPTIONS, napoveda,
 * seznam her PS1, prehravac MP3, YouTube, web ...) sly starou cestou: Java fotila celou obrazovku
 * (1440x2672) na hlavnim vlakne a kodovala ji do H.264 (115 Mbit/s) / JPEG; prohlizec si pro H.264
 * stahoval prehravac z internetu. Log B332: v OPTIONS 896 z 900 snimku stejnych, ~7 ms na snimek
 * na hlavnim vlakne, "enkoder nestiha".
 *
 * Ted Java stranku jen VYFOTI a preda do C++ (nap_tv_stranka.h): kodovani, posilani zmenenych casti
 * a zvuk dela C++ stejne jako u menu (/stranka.stream). Foti se:
 *  - jen kdyz se na strance neco zmenilo (OnDrawListener), nejvys ~15x za sekundu,
 *  - beze zmeny 2x za sekundu (proud zustane zivy, ukaze se i to, co OnDraw nevidi),
 *  - zmensene na nejvys 1920 x 1080 (vic TV neukaze) - PixelCopy zmensuje na grafickem cipu,
 *  - na vlastnim vlakne (vysledek PixelCopy a predani do C++ mimo hlavni vlakno),
 *  - jen kdyz se nekdo diva a na obrazovce neni hra / menu / demo (ty maji vlastni proud).
 */
final class NapTvStranka {

    static final int ZMENA_MS = 66;      // nejvys ~15 snimku/s, kdyz se stranka meni
    static final int KLID_MS = 500;      // beze zmeny 2x za sekundu
    static final int MAX_S = 1920, MAX_V = 1080;

    /** Je na obrazovce stranka (zadny jiny proud z C++)? - MainActivity */
    interface Kde { boolean strankaNaObrazovce(); }

    private final Activity a;
    private final Kde kde;
    private final NapAktualizace.Hlaska log;
    private final Handler ui = new Handler(Looper.getMainLooper());

    private HandlerThread vlakno;
    private Handler h;
    private volatile boolean bezi = false, spina = true, cekam = false;
    private volatile long posledniMs = 0;
    private int divaku = 0;
    private Bitmap bmp;
    private ByteBuffer buf;
    private long snimku = 0, chyb = 0, logMs = 0, snimkuOd = 0;
    private String rozmer = "?";
    private View koren;
    private final ViewTreeObserver.OnDrawListener kresleni = () -> spina = true;

    NapTvStranka(Activity a, Kde kde, NapAktualizace.Hlaska log) {
        this.a = a;
        this.kde = kde;
        this.log = log;
    }

    private void log(String s) { try { if (log != null) log.rekni("B333 TV STRANKA: " + s); } catch (Throwable ignored) {} }

    /** Vypocet zmenseni (testuje se v JVM): nejvys 1920 x 1080, sude rozmery. {sirka, vyska} */
    static int[] zmensi(int sw, int sh) {
        if (sw <= 0 || sh <= 0) return new int[]{0, 0};
        final double s = Math.min(1.0, Math.min((double) MAX_S / sw, (double) MAX_V / sh));
        int w = (int) Math.round(sw * s) & ~1, v = (int) Math.round(sh * s) & ~1;
        return new int[]{Math.max(2, w), Math.max(2, v)};
    }

    /** Pripojil se / odpojil divak stranky (vlakno serveru TV). */
    synchronized void divak(boolean pripojen) {
        divaku = Math.max(0, divaku + (pripojen ? 1 : -1));
        if (divaku > 0 && !bezi) start();
        else if (divaku == 0 && bezi) stop();
    }

    private void start() {
        bezi = true;
        spina = true;
        cekam = false;
        snimkuOd = snimku;
        logMs = SystemClock.uptimeMillis();
        vlakno = new HandlerThread("nap-tv-stranka", Process.THREAD_PRIORITY_DISPLAY);
        vlakno.start();
        h = new Handler(vlakno.getLooper());
        ui.post(() -> {
            try {
                koren = a.getWindow().getDecorView();
                koren.getViewTreeObserver().addOnDrawListener(kresleni);
            } catch (Throwable t) { log("hlidani zmen nejde (" + t.getClass().getSimpleName() + ") - fotim 2x za sekundu"); }
        });
        h.post(tik);
        log("divak pripojen - stranky appky jdou na TV z C++ (bez Javy: zadne H.264 / MJPEG), fotim jen pri zmene, nejvys "
                + (1000 / ZMENA_MS) + "/s, beze zmeny 2/s");
    }

    private void stop() {
        bezi = false;
        ui.post(() -> {
            try { if (koren != null) koren.getViewTreeObserver().removeOnDrawListener(kresleni); } catch (Throwable ignored) {}
            koren = null;
        });
        final HandlerThread v = vlakno;
        vlakno = null;
        h = null;
        if (v != null) v.quitSafely();
        log("divaci odesli - nefotim (snimku celkem " + snimku + ", chyb " + chyb + ")");
    }

    private final Runnable tik = new Runnable() {
        @Override public void run() {
            final Handler hh = h;
            if (!bezi || hh == null) return;
            final long ted = SystemClock.uptimeMillis();
            final long od = ted - posledniMs;
            if (!cekam && od >= ZMENA_MS && (spina || od >= KLID_MS)) {
                try { if (kde.strankaNaObrazovce()) snimej(ted); } catch (Throwable ignored) {}
            }
            if (ted - logMs >= 10000L) {
                final long n = snimku - snimkuOd;
                log("snimku " + n + " za " + ((ted - logMs) / 1000) + " s (" + rozmer + "), chyb " + chyb);
                logMs = ted;
                snimkuOd = snimku;
                try { final String s = NativeMenuBridge.strankaLogSafe(); if (s != null) for (String r : s.split("\n")) if (!r.isEmpty()) log.rekni(r); } catch (Throwable ignored) {}
            }
            hh.postDelayed(this, ZMENA_MS / 2);
        }
    };

    private void snimej(long ted) {
        cekam = true;
        spina = false;
        posledniMs = ted;
        ui.post(() -> {
            try {
                final View v = a.getWindow().getDecorView();
                final int sw = v.getWidth(), sh = v.getHeight();
                if (!bezi || sw <= 0 || sh <= 0 || !v.isAttachedToWindow()) { cekam = false; return; }
                final int[] z = zmensi(sw, sh);
                if (bmp == null || bmp.getWidth() != z[0] || bmp.getHeight() != z[1]) {
                    bmp = Bitmap.createBitmap(z[0], z[1], Bitmap.Config.ARGB_8888);
                    buf = ByteBuffer.allocateDirect(z[0] * z[1] * 4);
                    rozmer = sw + "x" + sh + " -> " + z[0] + "x" + z[1];
                }
                final Bitmap cil = bmp;
                final ByteBuffer b = buf;
                final Handler hh = h;
                if (hh == null) { cekam = false; return; }
                if (Build.VERSION.SDK_INT >= 26) {
                    // graficky cip zkopiruje a zmensi okno; vysledek prijde na vlakno stranky
                    PixelCopy.request(a.getWindow(), new Rect(0, 0, sw, sh), cil, vysledek -> {
                        if (vysledek == PixelCopy.SUCCESS) posli(cil, b); else chyb++;
                        cekam = false;
                    }, hh);
                } else {
                    // Android 7: PixelCopy okna neni - nakreslit stranku zmensenou (hlavni vlakno, jen pri zmene)
                    final Canvas c = new Canvas(cil);
                    c.scale((float) z[0] / sw, (float) z[1] / sh);
                    v.draw(c);
                    hh.post(() -> { posli(cil, b); cekam = false; });
                }
            } catch (Throwable t) {
                chyb++;
                cekam = false;
            }
        });
    }

    private void posli(Bitmap cil, ByteBuffer b) {
        try {
            b.rewind();
            cil.copyPixelsToBuffer(b);
            NativeMenuBridge.strankaSnimekSafe(b, cil.getWidth(), cil.getHeight(), cil.getWidth() * 4);
            snimku++;
        } catch (Throwable t) {
            chyb++;
        }
    }
}
