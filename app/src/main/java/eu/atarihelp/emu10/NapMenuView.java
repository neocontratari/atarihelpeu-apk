package eu.atarihelp.emu10;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewConfiguration;

/**
 * B311: plocha hlavniho menu. Kresli do ni C++ (OpenGL ES na vlastnim vlakne,
 * NativeMenuBridge) - tahle trida jen preda plochu, kazdy bod prstu a klavesy.
 *
 * NULOVA ODEZVA: dotyk se posila hned, jak prijde (requestUnbufferedDispatch -
 * Android pak udalosti nesbira do snimku), vcetne historickych bodu z jedne
 * udalosti (rychlost prstu); kresleni si pri kazdem snimku vezme nejnovejsi
 * polohu prstu. O tom, co se klepnutim stalo (spustit kartu, OPTIONS, nic),
 * rozhoduje C++ - MainActivity dostane jen vysledek.
 *
 * Lezi NAD strankou (setZOrderOnTop) - pod ni je index.html (OPTIONS a zaloha,
 * kdyby C++ menu neslo).
 */
final class NapMenuView extends SurfaceView implements SurfaceHolder.Callback {

    /** Co se ma stat (NativeMenuBridge.AKCE_*) a odkud (dotyk / klavesa). */
    interface Akce { void akce(int kod, String odkud); }

    private final Akce akce;
    private Surface predana = null;
    private int prst = -1;          // sledovany prst (id ukazatele)

    NapMenuView(Context c, Akce akce) {
        super(c);
        this.akce = akce;
        getHolder().setFormat(PixelFormat.RGBX_8888);     // nepruhledna - nic pod ni neprosvita
        getHolder().addCallback(this);
        setZOrderOnTop(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        try { NativeMenuBridge.prahSafe(ViewConfiguration.get(c).getScaledTouchSlop()); } catch (Throwable ignored) {}
    }

    @Override public void surfaceCreated(SurfaceHolder h) {
        // velikost prijde hned v surfaceChanged
    }

    @Override public void surfaceChanged(SurfaceHolder h, int format, int w, int hh) {
        // pri otoceni telefonu zustava stejna plocha (jen jina velikost) - C++ si
        // velikost cte samo kazdy snimek, plochu tedy predat jen jednou
        Surface s = h.getSurface();
        if (s != predana) {
            predana = s;
            NativeMenuBridge.plochaSafe(s);
        }
    }

    @Override public void surfaceDestroyed(SurfaceHolder h) {
        // C++ po navratu uz do plochy nekresli (pocka na sve kreslici vlakno)
        predana = null;
        NativeMenuBridge.plochaSafe(null);
    }

    private void posli(int typ, float x, float y, long cas) {
        final int r = NativeMenuBridge.dotykSafe(typ, x, y, cas, getWidth(), getHeight());
        if (r != NativeMenuBridge.AKCE_NIC && akce != null) akce.akce(r, "dotyk");
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override public boolean onTouchEvent(MotionEvent e) {
        final int a = e.getActionMasked();
        if (a == MotionEvent.ACTION_DOWN) {
            // udalosti tohoto prstu chodi hned, ne az se snimkem displeje (API 21+)
            try { requestUnbufferedDispatch(e); } catch (Throwable ignored) {}
            prst = e.getPointerId(0);
            posli(NativeMenuBridge.DOTYK_DOLU, e.getX(0), e.getY(0), e.getEventTime());
            return true;
        }
        final int i = prst < 0 ? -1 : e.findPointerIndex(prst);
        if (i < 0) return true;
        switch (a) {
            case MotionEvent.ACTION_MOVE: {
                // historicke body (prst se pohnul vic, nez je udalosti) - pro presnou rychlost
                final int nh = e.getHistorySize();
                for (int k = 0; k < nh; k++)
                    posli(NativeMenuBridge.DOTYK_TAH, e.getHistoricalX(i, k), e.getHistoricalY(i, k), e.getHistoricalEventTime(k));
                posli(NativeMenuBridge.DOTYK_TAH, e.getX(i), e.getY(i), e.getEventTime());
                break;
            }
            case MotionEvent.ACTION_UP:
                posli(NativeMenuBridge.DOTYK_NAHORU, e.getX(i), e.getY(i), e.getEventTime());
                prst = -1;
                break;
            case MotionEvent.ACTION_POINTER_UP:
                // zvednul se sledovany prst (druhy zustal) - bere se jako pusteni
                if (e.getPointerId(e.getActionIndex()) == prst) {
                    posli(NativeMenuBridge.DOTYK_NAHORU, e.getX(i), e.getY(i), e.getEventTime());
                    prst = -1;
                }
                break;
            case MotionEvent.ACTION_CANCEL:
                posli(NativeMenuBridge.DOTYK_ZRUSEN, e.getX(i), e.getY(i), e.getEventTime());
                prst = -1;
                break;
            default:
                break;
        }
        return true;
    }

    /** Klavesnice / ovladac / dalkove ovladani: sipky listuji, Enter / A spusti. */
    static int klavesaMenu(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_BUTTON_L1:
                return NativeMenuBridge.KL_VLEVO;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_BUTTON_R1:
                return NativeMenuBridge.KL_VPRAVO;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_BUTTON_A:
            case KeyEvent.KEYCODE_SPACE:
                return NativeMenuBridge.KL_POTVRD;
            case KeyEvent.KEYCODE_MOVE_HOME:
                return NativeMenuBridge.KL_ZACATEK;
            case KeyEvent.KEYCODE_MOVE_END:
                return NativeMenuBridge.KL_KONEC;
            default:
                return 0;
        }
    }

    @Override public boolean onKeyDown(int keyCode, KeyEvent ev) {
        final int kl = klavesaMenu(keyCode);
        if (kl == 0) return super.onKeyDown(keyCode, ev);
        final int r = NativeMenuBridge.klavesaSafe(kl);
        if (r != NativeMenuBridge.AKCE_NIC && akce != null) akce.akce(r, "klavesa");
        return true;
    }
}
