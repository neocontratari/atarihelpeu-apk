package android.view;
public interface SurfaceHolder {
    interface Callback { void surfaceCreated(SurfaceHolder h); void surfaceChanged(SurfaceHolder h, int f, int w, int hh); void surfaceDestroyed(SurfaceHolder h); }
    void setFormat(int f); void addCallback(Callback c); Surface getSurface();
}
