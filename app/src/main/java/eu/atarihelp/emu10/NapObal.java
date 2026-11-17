package eu.atarihelp.emu10;

import android.content.Context;
import android.graphics.Color;
import android.view.View;
import android.widget.FrameLayout;

/**
 * B342: cerny obal cele appky (obsah okna). Normalne ma appka (rootFrame: stranka a vsechny plochy C++)
 * celou obrazovku jako dosud. Na televizi s volbou NA VYSKU je appka pruh 9:16 uprostred (jako telefon),
 * po stranach cerno - televize se otocit neumi (NapObrazovka).
 *
 * Vsechny plochy (menu C++, Atari, Sega, PS1, demo) jsou deti rootFrame a maji jeho velikost, stranka
 * hlasi polohu obrazu konzole v rootFrame - proto staci zmenit velikost rootFrame, nic jineho.
 */
final class NapObal extends FrameLayout {
    private boolean pruh = false;

    NapObal(Context c) {
        super(c);
        setBackgroundColor(Color.BLACK);
    }

    boolean pruh() { return pruh; }

    /** Zapnout / vypnout pruh uprostred (vraci true, kdyz se neco zmenilo). */
    boolean pruh(boolean p) {
        if (p == pruh) return false;
        pruh = p;
        requestLayout();
        invalidate();
        return true;
    }

    @Override
    protected void onMeasure(int ws, int hs) {
        final int W = MeasureSpec.getSize(ws), H = MeasureSpec.getSize(hs);
        setMeasuredDimension(W, H);
        final int w = pruh ? NapObrazovka.sirkaPruhu(W, H) : W;
        for (int i = 0; i < getChildCount(); i++) {
            final View c = getChildAt(i);
            if (c.getVisibility() == GONE) continue;
            c.measure(MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(H, MeasureSpec.EXACTLY));
        }
    }

    @Override
    protected void onLayout(boolean zmena, int l, int t, int r, int b) {
        final int W = r - l, H = b - t;
        for (int i = 0; i < getChildCount(); i++) {
            final View c = getChildAt(i);
            if (c.getVisibility() == GONE) continue;
            final int w = c.getMeasuredWidth();
            final int x = (W - w) / 2;
            c.layout(x, 0, x + w, H);
        }
    }
}
