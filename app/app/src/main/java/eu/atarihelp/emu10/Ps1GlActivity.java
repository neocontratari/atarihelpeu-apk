package eu.atarihelp.emu10;

// ============================================================
//  KROK C1: Nova celoobrazovkova PS1 obrazovka pres OpenGL ES.
//
//  Bere skutecny obraz z beziciho PS1 jadra (grabFrameSafe) a
//  kresli ho pres GLSurfaceView - tedy s vsync page-flipem,
//  presne jako nas overeny EGL renderer. Zadny lockCanvas,
//  zadny JPEG, zadne blikani.
//
//  Spousti se z menu (novy most AHRENDER.openPs1). Bezi VEDLE
//  stavajici cesty - nic v puvodnim zobrazovani nemeni. Az to
//  overime, v kroku C2 timhle nahradime starou cestu.
//
//  GLSurfaceView schvalne: je to nejcistsi a nejbezpecnejsi
//  zpusob, jak dostat vsync obraz, a je to prvni kamen prechodu
//  cele appky na C++/OpenGL, ktery Rene planuje.
// ============================================================

import android.app.Activity;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.WindowManager;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class Ps1GlActivity extends Activity {

    private GLSurfaceView glView;

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        // celoobrazovkove, drz obrazovku rozsvicenou
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

        glView = new GLSurfaceView(this);
        glView.setEGLContextClientVersion(2);
        // 8888 okno s double bufferingem (vsync resi GLSurfaceView sam)
        glView.setEGLConfigChooser(8, 8, 8, 8, 0, 0);
        glView.setRenderer(new Ps1Renderer());
        glView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        setContentView(glView);
    }

    @Override protected void onResume() { super.onResume(); if (glView != null) glView.onResume(); }
    @Override protected void onPause()  { super.onPause();  if (glView != null) glView.onPause();  }

    // ------------------------------------------------------------
    //  Renderer: kazdy snimek si vezme obraz z PS1 jadra a nahraje
    //  ho do GL textury, pak nakresli na celoobrazovkovy quad.
    // ------------------------------------------------------------
    private static final class Ps1Renderer implements GLSurfaceView.Renderer {
        private int program, texId, aPos, aTex, uTex;
        private int texW = 0, texH = 0;
        private int[] argb = new int[1024 * 512];
        private IntBuffer pixels;
        private int viewW = 1, viewH = 1;
        private long frames = 0;
        private boolean loggedFirst = false;

        private FloatBuffer quad(float[] d) {
            FloatBuffer fb = ByteBuffer.allocateDirect(d.length * 4)
                    .order(ByteOrder.nativeOrder()).asFloatBuffer();
            fb.put(d).position(0);
            return fb;
        }

        private int compile(int type, String src) {
            int sh = GLES20.glCreateShader(type);
            GLES20.glShaderSource(sh, src);
            GLES20.glCompileShader(sh);
            return sh;
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig cfg) {
            String vs =
                "attribute vec2 aPos;\n" +
                "attribute vec2 aTex;\n" +
                "varying vec2 vTex;\n" +
                "void main(){ vTex=aTex; gl_Position=vec4(aPos,0.0,1.0); }\n";
            // PS1 jadro dava pixely v ARGB (0xAARRGGBB). Kdyz je nahrajeme jako
            // GL_RGBA bajty, poradi v pameti je B,G,R,A -> ve fragmentu proto
            // prohodime zpet (.bgra -> spravne barvy). Alfa vzdy 1.
            String fs =
                "precision mediump float;\n" +
                "varying vec2 vTex;\n" +
                "uniform sampler2D uTex;\n" +
                "void main(){ vec4 c=texture2D(uTex,vTex); gl_FragColor=vec4(c.b,c.g,c.r,1.0); }\n";

            program = GLES20.glCreateProgram();
            GLES20.glAttachShader(program, compile(GLES20.GL_VERTEX_SHADER, vs));
            GLES20.glAttachShader(program, compile(GLES20.GL_FRAGMENT_SHADER, fs));
            GLES20.glLinkProgram(program);

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
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int w, int h) {
            viewW = Math.max(1, w);
            viewH = Math.max(1, h);
        }

        private void ensureTex(int w, int h) {
            if (w == texW && h == texH) return;
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, w, h, 0,
                    GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
            texW = w; texH = h;
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            GLES20.glClearColor(0.05f, 0.06f, 0.09f, 1f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

            // 1) obraz z PS1 jadra
            int wh = NativePs1CoreBridge.grabFrameSafe(argb);
            if (wh < 0) {
                int need = ((-wh) >> 16) * ((-wh) & 0xFFFF);
                argb = new int[need + 1024];
                wh = NativePs1CoreBridge.grabFrameSafe(argb);
            }
            if (wh <= 0) { frames++; return; }  // jadro jeste nema obraz

            int srcW = wh >> 16, srcH = wh & 0xFFFF;
            if (srcW <= 0 || srcH <= 0) { frames++; return; }

            if (!loggedFirst) {
                loggedFirst = true;
                try {
                    android.util.Log.i("EGLRender",
                        "C1 PS1 prvni snimek: " + srcW + "x" + srcH +
                        " p0=0x" + Integer.toHexString(argb[0]));
                } catch (Throwable ignored) {}
            }

            ensureTex(srcW, srcH);

            if (pixels == null || pixels.capacity() < srcW * srcH) {
                pixels = ByteBuffer.allocateDirect(srcW * srcH * 4)
                        .order(ByteOrder.nativeOrder()).asIntBuffer();
            }
            pixels.clear();
            pixels.put(argb, 0, srcW * srcH);
            pixels.position(0);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
            GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, srcW, srcH,
                    GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, pixels);

            // 2) letterbox pri zachovani pomeru stran
            int vw = viewW, vh = (viewW * srcH) / srcW;
            if (vh > viewH) { vh = viewH; vw = (viewH * srcW) / srcH; }
            GLES20.glViewport((viewW - vw) / 2, (viewH - vh) / 2, vw, vh);

            // 3) nakresli quad s texturou
            FloatBuffer pos = quad(new float[]{ -1,-1,  1,-1,  -1,1,  1,1 });
            FloatBuffer tex = quad(new float[]{ 0,1,  1,1,  0,0,  1,0 });  // v obracene

            GLES20.glUseProgram(program);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texId);
            GLES20.glUniform1i(uTex, 0);
            GLES20.glVertexAttribPointer(aPos, 2, GLES20.GL_FLOAT, false, 0, pos);
            GLES20.glVertexAttribPointer(aTex, 2, GLES20.GL_FLOAT, false, 0, tex);
            GLES20.glEnableVertexAttribArray(aPos);
            GLES20.glEnableVertexAttribArray(aTex);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            frames++;
            if (frames % 300 == 0) {
                try { android.util.Log.i("EGLRender",
                    "C1 bezi: " + frames + " snimku, " + srcW + "x" + srcH); } catch (Throwable ignored) {}
            }
        }
    }
}
