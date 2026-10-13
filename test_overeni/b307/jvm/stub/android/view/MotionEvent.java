package android.view;
/** B307 test: nahrada MotionEvent (obtain jako v Androidu). */
public final class MotionEvent {
    public static final int ACTION_DOWN = 0, ACTION_UP = 1, ACTION_MOVE = 2;
    private final int akce; private final float x, y;
    private MotionEvent(int akce, float x, float y) { this.akce = akce; this.x = x; this.y = y; }
    public static MotionEvent obtain(long downTime, long eventTime, int action, float x, float y, int metaState) { return new MotionEvent(action, x, y); }
    public int getActionMasked() { return akce & 0xFF; }
    public float getX() { return x; }
    public float getY() { return y; }
}
