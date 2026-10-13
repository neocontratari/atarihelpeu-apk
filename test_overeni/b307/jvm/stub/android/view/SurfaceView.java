package android.view;
/** B307 test: nahrada SurfaceView pro javac i beh v JVM (velikost nastavi test). */
public class SurfaceView {
    private final SurfaceHolder holder = new SurfaceHolder() {
        public void setFormat(int f) {}
        public void addCallback(Callback c) {}
        public Surface getSurface() { return null; }
    };
    private int w = 0, h = 0;
    public SurfaceView(android.content.Context c) {}
    public SurfaceHolder getHolder() { return holder; }
    public void setZOrderOnTop(boolean b) {}
    public void setKeepScreenOn(boolean b) {}
    public void setFocusable(boolean b) {}
    public void setFocusableInTouchMode(boolean b) {}
    public void setClickable(boolean b) {}
    public boolean onTouchEvent(MotionEvent e) { return false; }
    public final int getWidth() { return w; }
    public final int getHeight() { return h; }
    /** jen test: velikost view (jako po rozvrzeni na telefonu) */
    public void testVelikost(int w, int h) { this.w = w; this.h = h; }
}
