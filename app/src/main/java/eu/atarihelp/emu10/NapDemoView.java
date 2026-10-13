package eu.atarihelp.emu10;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * B304: plocha uvodniho dema. Kresli do ni C++ (OpenGL ES na vlastnim
 * vlakne) - tahle trida jen preda plochu a dotek prstem.
 *
 * B307: tlacitko SKIP kresli C++ a C++ taky rozhodne, kam prst klepl
 * (NativeIntroBridge.introDotyk) - tady se jen predaji souradnice.
 * Klepnuti na SKIP i kamkoli jinam demo preskoci (jako drive).
 *
 * Lezi NAD strankou (setZOrderOnTop), takze pod ni se mezitim muze
 * nacist menu - po konci dema je hned videt, bez cekani.
 */
final class NapDemoView extends SurfaceView implements SurfaceHolder.Callback {

    /** Preskocit demo - proc (do logu): "tlacitko SKIP" / "dotyk mimo SKIP". */
    interface Preskocit { void preskoc(String proc); }

    private final Preskocit preskocit;
    private Surface predana = null;

    NapDemoView(Context c, Preskocit preskocit) {
        super(c);
        this.preskocit = preskocit;
        getHolder().setFormat(PixelFormat.RGBX_8888);     // nepruhledna - nic pod ni neprosvita
        getHolder().addCallback(this);
        setZOrderOnTop(true);
        setKeepScreenOn(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
    }

    @Override public void surfaceCreated(SurfaceHolder h) {
        // velikost prijde hned v surfaceChanged
    }

    @Override public void surfaceChanged(SurfaceHolder h, int format, int w, int hh) {
        // Pri otoceni telefonu zustava stejna plocha (jen jina velikost) -
        // C++ si velikost cte samo kazdy snimek, plochu tedy predat jen jednou.
        Surface s = h.getSurface();
        if (s != predana) {
            predana = s;
            NativeIntroBridge.plochaSafe(s);
        }
    }

    @Override public void surfaceDestroyed(SurfaceHolder h) {
        // C++ po navratu uz do plochy nekresli (pocka na sve kreslici vlakno)
        predana = null;
        NativeIntroBridge.plochaSafe(null);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override public boolean onTouchEvent(MotionEvent e) {
        if (e.getActionMasked() == MotionEvent.ACTION_UP && preskocit != null) {
            // B307: kam prst klepl, rozhodne C++ (tlacitko SKIP kresli C++);
            // hned po startu (prst, ktery appku spustil) C++ vrati "nic"
            final int r = NativeIntroBridge.dotykSafe(e.getX(), e.getY(), getWidth(), getHeight());
            if (r == NativeIntroBridge.DOTYK_SKIP) preskocit.preskoc("tlacitko SKIP");
            else if (r == NativeIntroBridge.DOTYK_JINAM) preskocit.preskoc("dotyk mimo SKIP");
        }
        return true;
    }
}
