package eu.atarihelp.emu10;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * B291: plocha, na kterou C++ PRIMO kresli cele zarizeni Atari 130XE ze
 * schvaleneho navrhu (obrazovka s obrazem Atari, klavesnice, konzole,
 * kazetak, POWER, servisni tlacitka).
 *
 * Tahle trida nic nekresli a nic nerozhoduje - jen:
 *  - preda C++ plochu displeje (surfaceChanged/surfaceDestroyed),
 *  - preda C++ kazdy dotek prstu (vcetne vice prstu najednou),
 *  - kdyz C++ rekne, ze bylo pusteno servisni tlacitko (XEX, TXT, LOG,
 *    HELP, MENU...), preda to MainActivity (vyber souboru, dialog...).
 *
 * Proc ne WebView jako v B289/B290: tam se kazdy dotek posilal do JS jako
 * ~3,6 MB obrazek a "zmacknute" tlacitko se na telefonu nestihlo ukazat.
 * Tady C++ kresli do displeje sam, stisk je videt do jednoho snimku.
 */
final class AtariDeviceView extends SurfaceView implements SurfaceHolder.Callback {

    interface Akce { void naServisniAkci(int kod); }

    private final Akce akce;

    AtariDeviceView(Context c, Akce akce) {
        super(c);
        this.akce = akce;
        getHolder().setFormat(PixelFormat.RGBA_8888);
        getHolder().addCallback(this);
        // Plocha lezi NAD strankou HELP (stejne jako PS1 na vysku - overeny
        // zpusob v teto appce). Schovava se pres setVisibility (LOG/CHYBA).
        setZOrderOnTop(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        setKeepScreenOn(true);
    }

    @Override public void surfaceCreated(SurfaceHolder h) {
        // velikost prijde hned v surfaceChanged
    }

    @Override public void surfaceChanged(SurfaceHolder h, int format, int w, int hh) {
        NativeAtariCoreBridge.devSurfaceSafe(h.getSurface(), w, hh);
    }

    @Override public void surfaceDestroyed(SurfaceHolder h) {
        // C++ po navratu uz do plochy nekresli (ceka na kreslici vlakno)
        NativeAtariCoreBridge.devSurfaceSafe(null, 0, 0);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override public boolean onTouchEvent(MotionEvent e) {
        final int am = e.getActionMasked();
        switch (am) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                final int i = e.getActionIndex();
                NativeAtariCoreBridge.devTouchSafe(e.getPointerId(i), 0, e.getX(i), e.getY(i));
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP: {
                final int i = e.getActionIndex();
                final int r = NativeAtariCoreBridge.devTouchSafe(e.getPointerId(i), 1, e.getX(i), e.getY(i));
                if (r != 0 && akce != null) akce.naServisniAkci(r);
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                // B296: posun prstu - obrazovka Atari je joystick (leva pulka smer)
                final int n = e.getPointerCount();
                for (int i = 0; i < n; i++) NativeAtariCoreBridge.devTouchSafe(e.getPointerId(i), 3, e.getX(i), e.getY(i));
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                NativeAtariCoreBridge.devTouchSafe(0, 2, 0f, 0f);
                return true;
            default:
                return true;
        }
    }
}
