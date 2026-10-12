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
 * vlakne) - tahle trida jen preda plochu a dotek prstem = preskocit.
 *
 * Lezi NAD strankou (setZOrderOnTop), takze pod ni se mezitim muze
 * nacist menu - po konci dema je hned videt, bez cekani.
 */
final class NapDemoView extends SurfaceView implements SurfaceHolder.Callback {

    private final Runnable preskocit;
    private final long vznik = System.currentTimeMillis();
    private Surface predana = null;

    NapDemoView(Context c, Runnable preskocit) {
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
        if (e.getActionMasked() == MotionEvent.ACTION_UP && System.currentTimeMillis() - vznik > 700) {
            if (preskocit != null) preskocit.run();
        }
        return true;
    }
}
