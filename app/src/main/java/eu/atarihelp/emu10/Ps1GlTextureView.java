package eu.atarihelp.emu10;

// ============================================================
//  KROK C2: plynuly PS1 obraz pres TextureView + vlastni OpenGL.
//
//  PROC TextureView a ne GLSurfaceView:
//  GLSurfaceView je samostatna hardwarova vrstva, kterou snimani
//  obrazovky (PixelCopy) NEVIDI -> TV cast posilal cernou plochu
//  (v logu brightAvg=0). TextureView se kresli do okna aplikace,
//  takze ho cast zachyti. Rene to mel v puvodni apce spravne.
//
//  Zaroven si drzime vsechno dobre z naseho rendereru: vlastni EGL
//  kontext, double buffering, vsync (eglSwapInterval 1), nahravani
//  obrazu do GL textury a roztazeni na GPU. Zadny lockCanvas,
//  zadny JPEG, zadne blikani.
// ============================================================

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.view.TextureView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

public class Ps1GlTextureView extends TextureView implements TextureView.SurfaceTextureListener {

    public interface LogSink { void log(String msg); }

    private volatile boolean running = false;
    private Thread thread;
    private LogSink sink;

    public Ps1GlTextureView(Context ctx, LogSink logSink) {
        super(ctx);
        this.sink = logSink;
        setOpaque(true);
        setSurfaceTextureListener(this);
    }

    private void say(String m) {
        try { if (sink != null) sink.log(m); } catch (Throwable ignored) {}
        try { android.util.Log.i("EGLRender", m); } catch (Throwable ignored) {}
    }

    // ---------------- TextureView zivotni cyklus ----------------

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture st, int w, int h) {
        startRender(st, w, h);
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture st, int w, int h) {
        viewW = Math.max(1, w);
        viewH = Math.max(1, h);
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture st) {
        stopRender();
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture st) { }

    public void stopRender() {
        running = false;
        Thread t = thread;
        thread = null;
        if (t != null) {
            try { t.join(800); } catch (Throwable ignored) {}
        }
    }

    // ---------------- render vlakno ----------------

    private volatile int viewW = 1, viewH = 1;

    private void startRender(final SurfaceTexture st, int w, int h) {
        viewW = Math.max(1, w);
        viewH = Math.max(1, h);
        running = true;
        thread = new Thread(new Runnable() {
            @Override public void run() { renderLoop(st); }
        }, "Ps1GlTextureView");
        thread.setPriority(Thread.NORM_PRIORITY + 1);
        thread.start();
    }

    private EGLDisplay eglDisplay = EGL14.EGL_NO_DISPLAY;
    private EGLContext eglContext = EGL14.EGL_NO_CONTEXT;
    private EGLSurface eglSurface = EGL14.EGL_NO_SURFACE;

    private int program, texId, aPos, aTex, uTex;
    private int texW = 0, texH = 0;
    private int[] argb = new int[1024 * 512];
    private IntBuffer pixels;
    private long frames = 0;
    private boolean loggedFirst = false;

    private boolean eglSetup(SurfaceTexture st) {
        eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (eglDisplay == EGL14.EGL_NO_DISPLAY) { say("C2 EGL: zadny display"); return false; }
        int[] ver = new int[2];
        if (!EGL14.eglInitialize(eglDisplay, ver, 0, ver, 1)) { say("C2 EGL: init selhal"); return false; }

        int[] attribs = {
            EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
            EGL14.EGL_SURFACE_TYPE,    EGL14.EGL_WINDOW_BIT,
            EGL14.EGL_RED_SIZE,   8,
            EGL14.EGL_GREEN_SIZE, 8,
            EGL14.EGL_BLUE_SIZE,  8,
            EGL14.EGL_ALPHA_SIZE, 8,
            EGL14.EGL_DEPTH_SIZE, 0,
            EGL14.EGL_NONE
        };
        EGLConfig[] cfgs = new EGLConfig[1];
        int[] num = new int[1];
        if (!EGL14.eglChooseConfig(eglDisplay, attribs, 0, cfgs, 0, 1, num, 0) || num[0] < 1) {
            say("C2 EGL: zadna konfigurace"); return false;
        }
        int[] ctxAttribs = { EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE };
        eglContext = EGL14.eglCreateContext(eglDisplay, cfgs[0], EGL14.EGL_NO_CONTEXT, ctxAttribs, 0);
        if (eglContext == EGL14.EGL_NO_CONTEXT) { say("C2 EGL: kontext selhal"); return false; }

        int[] surfAttribs = { EGL14.EGL_NONE };
        eglSurface = EGL14.eglCreateWindowSurface(eglDisplay, cfgs[0], st, surfAttribs, 0);
        if (eglSurface == EGL14.EGL_NO_SURFACE) { say("C2 EGL: surface selhal"); return false; }

        if (!EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
            say("C2 EGL: makeCurrent selhal"); return false;
        }
        // vsync: swap presne v rytmu displeje -> zadne blikani ani trhani
        EGL14.eglSwapInterval(eglDisplay, 1);
        say("C2 EGL pripraven (TextureView, double buffering, vsync ON)");
        return true;
    }

    private void eglRelease() {
        try {
            if (eglDisplay != EGL14.EGL_NO_DISPLAY) {
                EGL14.eglMakeCurrent(eglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
                if (eglSurface != EGL14.EGL_NO_SURFACE) EGL14.eglDestroySurface(eglDisplay, eglSurface);
                if (eglContext != EGL14.EGL_NO_CONTEXT) EGL14.eglDestroyContext(eglDisplay, eglContext);
                EGL14.eglTerminate(eglDisplay);
            }
        } catch (Throwable ignored) {}
        eglDisplay = EGL14.EGL_NO_DISPLAY;
        eglSurface = EGL14.EGL_NO_SURFACE;
        eglContext = EGL14.EGL_NO_CONTEXT;
        texW = 0; texH = 0;
    }

    private int compile(int type, String src) {
        int sh = GLES20.glCreateShader(type);
        GLES20.glShaderSource(sh, src);
        GLES20.glCompileShader(sh);
        return sh;
    }

    private FloatBuffer fbuf(float[] d) {
        FloatBuffer fb = ByteBuffer.allocateDirect(d.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        fb.put(d).position(0);
        return fb;
    }

    private boolean glSetup() {
        String vs = "attribute vec2 aPos; attribute vec2 aTex; varying vec2 vTex;"
                  + "void main(){ vTex=aTex; gl_Position=vec4(aPos,0.0,1.0); }";
        // jadro dava ARGB (0xAARRGGBB) -> v pameti B,G,R,A -> prohodit zpet
        String fs = "precision mediump float; varying vec2 vTex; uniform sampler2D uTex;"
                  + "void main(){ vec4 c=texture2D(uTex,vTex); gl_FragColor=vec4(c.b,c.g,c.r,1.0); }";
        program = GLES20.glCreateProgram();
        GLES20.glAttachShader(program, compile(GLES20.GL_VERTEX_SHADER, vs));
        GLES20.glAttachShader(program, compile(GLES20.GL_FRAGMENT_SHADER, fs));
        GLES20.glLinkProgram(program);
        int[] ok = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, ok, 0);
        if (ok[0] == 0) { say("C2 GL: program se neslinkoval"); return false; }
        aPos = GLES20.glGetAttribLocation(program, "aPos");
        aTex = GLES20.glGetAttribLocation(program, "aTex");
        uTex = GLES20.glGetUniformLocation(program, "uTex");
        int[] t = new int[1];
        GLES20.glGenTextures(1, t, 0);
        texId = t[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        return true;
    }

    private void ensureTex(int w, int h) {
        if (w == texW && h == texH) return;
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, w, h, 0,
                GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        texW = w; texH = h;
        say("C2 rozliseni hry: " + w + "x" + h);
    }

    private void renderLoop(SurfaceTexture st) {
        if (!eglSetup(st) || !glSetup()) { eglRelease(); return; }

        FloatBuffer pos = fbuf(new float[]{ -1,-1,  1,-1,  -1,1,  1,1 });
        FloatBuffer tex = fbuf(new float[]{  0, 1,  1, 1,   0,0,  1,0 });

        while (running) {
            try {
                GLES20.glClearColor(0.05f, 0.06f, 0.09f, 1f);
                GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

                int wh = NativePs1CoreBridge.grabFrameSafe(argb);
                if (wh < 0) {
                    int need = ((-wh) >> 16) * ((-wh) & 0xFFFF);
                    argb = new int[need + 1024];
                    wh = NativePs1CoreBridge.grabFrameSafe(argb);
                }
                int srcW = (wh > 0) ? (wh >> 16) : 0;
                int srcH = (wh > 0) ? (wh & 0xFFFF) : 0;

                if (srcW > 0 && srcH > 0) {
                    if (!loggedFirst) {
                        loggedFirst = true;
                        say("C2 prvni snimek: " + srcW + "x" + srcH);
                    }
                    ensureTex(srcW, srcH);
                    int need = srcW * srcH;
                    if (pixels == null || pixels.capacity() < need) {
                        pixels = ByteBuffer.allocateDirect(need * 4)
                                .order(ByteOrder.nativeOrder()).asIntBuffer();
                    }
                    pixels.clear();
                    pixels.put(argb, 0, need);
                    pixels.position(0);

                    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
                    GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, srcW, srcH,
                            GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, pixels);

                    // letterbox pri zachovani pomeru stran (roztazeni dela GPU)
                    int vw = viewW, vh = (viewW * srcH) / srcW;
                    if (vh > viewH) { vh = viewH; vw = (viewH * srcW) / srcH; }
                    GLES20.glViewport((viewW - vw) / 2, (viewH - vh) / 2, vw, vh);

                    GLES20.glUseProgram(program);
                    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
                    GLES20.glUniform1i(uTex, 0);
                    GLES20.glVertexAttribPointer(aPos, 2, GLES20.GL_FLOAT, false, 0, pos);
                    GLES20.glVertexAttribPointer(aTex, 2, GLES20.GL_FLOAT, false, 0, tex);
                    GLES20.glEnableVertexAttribArray(aPos);
                    GLES20.glEnableVertexAttribArray(aTex);
                    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
                }

                // preklopeni na obrazovku - vsync sam ohlida tempo
                if (!EGL14.eglSwapBuffers(eglDisplay, eglSurface)) {
                    say("C2 swap selhal, koncim smycku");
                    break;
                }

                frames++;
                if (frames % 300 == 0) say("C2 bezi: " + frames + " snimku, " + srcW + "x" + srcH);
            } catch (Throwable t) {
                say("C2 chyba ve smycce: " + t);
                break;
            }
        }
        eglRelease();
        say("C2 render ukoncen (" + frames + " snimku)");
    }
}
