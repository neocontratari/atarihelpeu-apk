package eu.atarihelp.emu10;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;

/**
 * BUILD2PQ_SEGA_NATIVE_CPP_INPUT_AUDIO_PROOF_STAGE81
 *
 * POZOR: Tohle jeste NENI hotovy Sega emulator.
 * Je to druhy native C++ proof vedle stavajici WebView Segy:
 * - PP uz overil Activity -> JNI -> C++ -> ROM header/checksum
 * - PQ navic overuje C++ input stav a C++ generovany PCM audio test
 * - WebView Sega zustava beze zmen jako zaloha
 * - zadny fake gameplay, zadna ROM v APK
 */
public class NativeSegaProofActivity extends Activity {
    private static final int PICK_ROM = 80;
    private TextView info;
    private TextView live;
    private NativePatternView pattern;
    private boolean nativeOk = false;
    private String lastRomBlock = "ROM: zatim nevybrana";
    private final Handler uiHandler = new Handler(Looper.getMainLooper());

    static {
        try {
            System.loadLibrary("napsega_native_proof");
        } catch (Throwable ignored) {
            // Native chyba se ukaze uzivateli v onCreate.
        }
    }

    private static native String nativeCoreBuildString();
    private static native String nativeRomInfo(byte[] romBytes);
    private static native void nativeRenderPattern(int width, int height, int frame, int[] argbOut);
    private static native void nativeSetInput(int key, boolean pressed);
    private static native String nativeGetInputStatus();
    private static native void nativeMakeAudioTone(short[] pcmOut, int sampleRate, double hz);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            String build = nativeCoreBuildString();
            nativeOk = build != null && build.length() > 0;
        } catch (Throwable t) {
            nativeOk = false;
        }

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.HORIZONTAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(18, 18, 18, 18);
        root.setBackgroundColor(Color.rgb(2, 5, 10));

        pattern = new NativePatternView(this);
        LinearLayout.LayoutParams p1 = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1.35f);
        root.addView(pattern, p1);

        LinearLayout right = new LinearLayout(this);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(18, 0, 0, 0);
        LinearLayout.LayoutParams p2 = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1.0f);
        root.addView(right, p2);

        TextView title = new TextView(this);
        title.setTextColor(Color.rgb(255, 225, 122));
        title.setTextSize(18);
        title.setTypeface(android.graphics.Typeface.MONOSPACE, android.graphics.Typeface.BOLD);
        title.setText("SEGA C++ TEST / BUILD2PQ");
        right.addView(title);

        LinearLayout row1 = new LinearLayout(this);
        row1.setOrientation(LinearLayout.HORIZONTAL);
        right.addView(row1);

        Button pick = new Button(this);
        pick.setText("VYBRAT ROM");
        pick.setOnClickListener(v -> openRomPicker());
        row1.addView(pick, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));

        Button audio = new Button(this);
        audio.setText("C++ AUDIO TEST");
        audio.setOnClickListener(v -> playNativeAudioTest());
        row1.addView(audio, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));

        Button back = new Button(this);
        back.setText("ZPET DO APPKY");
        back.setOnClickListener(v -> finish());
        right.addView(back);

        live = new TextView(this);
        live.setTextColor(Color.rgb(150, 255, 180));
        live.setTextSize(12);
        live.setTypeface(android.graphics.Typeface.MONOSPACE);
        live.setText("LIVE: cekam...");
        right.addView(live);

        right.addView(makePadBlock());

        info = new TextView(this);
        info.setTextColor(Color.rgb(210, 236, 255));
        info.setTextSize(12);
        info.setTypeface(android.graphics.Typeface.MONOSPACE);
        info.setText(defaultInfo());
        ScrollView scroll = new ScrollView(this);
        scroll.addView(info);
        right.addView(scroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1.0f));

        setContentView(root);
        startLiveMonitor();
    }

    private LinearLayout makePadBlock() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(0, 8, 0, 8);

        TextView label = new TextView(this);
        label.setText("C++ INPUT PROOF - ovlada jen native test pattern");
        label.setTextColor(Color.rgb(255, 225, 122));
        label.setTextSize(11);
        label.setTypeface(android.graphics.Typeface.MONOSPACE, android.graphics.Typeface.BOLD);
        box.addView(label);

        LinearLayout r0 = new LinearLayout(this); r0.setGravity(Gravity.CENTER);
        r0.addView(holdButton("UP", 0), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        box.addView(r0);

        LinearLayout r1 = new LinearLayout(this); r1.setOrientation(LinearLayout.HORIZONTAL);
        r1.addView(holdButton("LEFT", 2), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        r1.addView(holdButton("DOWN", 1), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        r1.addView(holdButton("RIGHT", 3), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        box.addView(r1);

        LinearLayout r2 = new LinearLayout(this); r2.setOrientation(LinearLayout.HORIZONTAL);
        r2.addView(holdButton("A", 4), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        r2.addView(holdButton("B", 5), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        r2.addView(holdButton("C", 6), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        r2.addView(holdButton("START", 7), new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        box.addView(r2);
        return box;
    }

    private Button holdButton(String text, int key) {
        Button b = new Button(this);
        b.setText(text);
        b.setOnTouchListener((v, ev) -> {
            int a = ev.getActionMasked();
            boolean down = a == MotionEvent.ACTION_DOWN || a == MotionEvent.ACTION_POINTER_DOWN;
            boolean up = a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL || a == MotionEvent.ACTION_POINTER_UP;
            if (down || up) {
                try { nativeSetInput(key, down); } catch (Throwable ignored) {}
                v.setPressed(down);
                updateLiveOnce();
                return true;
            }
            return false;
        });
        return b;
    }

    private void startLiveMonitor() {
        uiHandler.postDelayed(new Runnable() {
            @Override public void run() {
                updateLiveOnce();
                uiHandler.postDelayed(this, 500);
            }
        }, 500);
    }

    private void updateLiveOnce() {
        String s;
        try { s = nativeGetInputStatus(); }
        catch (Throwable t) { s = "INPUT native error: " + t.getMessage(); }
        live.setText("LIVE: " + pattern.getFpsText() + " | " + s);
    }

    private String defaultInfo() {
        StringBuilder sb = new StringBuilder();
        if (nativeOk) {
            try { sb.append(nativeCoreBuildString()).append("\n\n"); }
            catch (Throwable t) { sb.append("Native build string ERROR: ").append(t.getMessage()).append("\n\n"); }
        } else {
            sb.append("NATIVE LIBRARY NENAHRANA. Pokud build spadne, chybi NDK/CMake.\n\n");
        }
        sb.append("STAV BUILD2PQ:\n");
        sb.append("- PP proof OK: Java -> JNI -> C++ -> ROM header.\n");
        sb.append("- PQ pridal C++ input stav a C++ PCM audio tone.\n");
        sb.append("- Toto stale NENI fake Sega gameplay.\n");
        sb.append("- WebView Sega zustava zaloha beze zmen.\n\n");
        sb.append("KROK:\n");
        sb.append("1. Vyber ROM - C++ vrati header/checksum.\n");
        sb.append("2. Zmackni C++ AUDIO TEST - ma pipnout kratky cisty ton.\n");
        sb.append("3. Drz DPAD/A/B/C/START - barevny pattern musi reagovat.\n\n");
        sb.append(lastRomBlock).append("\n");
        return sb.toString();
    }

    private void openRomPicker() {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream", "application/zip", "*/*"});
        startActivityForResult(Intent.createChooser(i, "SEGA C++ TEST - vyber ROM"), PICK_ROM);
    }

    private String displayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) {
                    String n = c.getString(idx);
                    if (n != null && n.length() > 0) return n;
                }
            }
        } catch (Throwable ignored) {}
        String p = uri.getLastPathSegment();
        return p == null ? "ROM" : p;
    }

    private byte[] readBytes(Uri uri, int maxBytes) throws Exception {
        InputStream in = getContentResolver().openInputStream(uri);
        if (in == null) throw new Exception("Nelze otevrit ROM");
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[32768];
        int n;
        while ((n = in.read(buf)) > 0) {
            bos.write(buf, 0, n);
            if (bos.size() > maxBytes) {
                in.close();
                throw new Exception("ROM je moc velka pro proof test: " + bos.size());
            }
        }
        in.close();
        return bos.toByteArray();
    }

    private void playNativeAudioTest() {
        try {
            final int sampleRate = 48000;
            final int samples = sampleRate / 3; // cca 0.33s
            short[] pcm = new short[samples];
            nativeMakeAudioTone(pcm, sampleRate, 440.0);
            AudioTrack track = new AudioTrack.Builder()
                    .setAudioAttributes(new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_GAME)
                            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                            .build())
                    .setAudioFormat(new AudioFormat.Builder()
                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                            .setSampleRate(sampleRate)
                            .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                            .build())
                    .setTransferMode(AudioTrack.MODE_STATIC)
                    .setBufferSizeInBytes(pcm.length * 2)
                    .build();
            track.write(pcm, 0, pcm.length);
            track.play();
            info.setText("C++ AUDIO TEST: pusten kratky 440Hz PCM ton z C++ dat.\n\n" + defaultInfo());
            uiHandler.postDelayed(() -> {
                try { track.stop(); track.release(); } catch (Throwable ignored) {}
            }, 700);
        } catch (Throwable t) {
            info.setText("C++ AUDIO TEST ERROR:\n" + t.getMessage() + "\n\n" + defaultInfo());
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_ROM) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
        Uri uri = data.getData();
        try {
            String name = displayName(uri);
            byte[] bytes = readBytes(uri, 8 * 1024 * 1024);
            String nativeInfo;
            try { nativeInfo = nativeRomInfo(bytes); }
            catch (Throwable t) { nativeInfo = "NATIVE ROM INFO ERROR: " + t.getMessage(); }
            lastRomBlock = "ROM: " + name + "\n" + nativeInfo;
            info.setText(defaultInfo());
        } catch (Throwable t) {
            lastRomBlock = "CHYBA PRI CTENI ROM:\n" + t.getMessage();
            info.setText(defaultInfo());
        }
    }

    public static class NativePatternView extends View {
        private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
        private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Handler handler = new Handler(Looper.getMainLooper());
        private Bitmap bmp;
        private int[] pixels;
        private int frame = 0;
        private long lastFpsT = 0;
        private int fpsFrames = 0;
        private String fpsText = "FPS ?";
        private final Runnable tick = new Runnable() {
            @Override public void run() {
                frame++;
                invalidate();
                handler.postDelayed(this, 33);
            }
        };

        public NativePatternView(android.content.Context ctx) {
            super(ctx);
            setBackgroundColor(Color.BLACK);
            textPaint.setColor(Color.rgb(255, 225, 122));
            textPaint.setTextSize(28f);
            textPaint.setTypeface(android.graphics.Typeface.MONOSPACE);
        }

        public String getFpsText() { return fpsText; }

        @Override protected void onAttachedToWindow() {
            super.onAttachedToWindow();
            handler.post(tick);
        }

        @Override protected void onDetachedFromWindow() {
            handler.removeCallbacks(tick);
            super.onDetachedFromWindow();
        }

        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int w = 320, h = 224;
            if (bmp == null) {
                bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
                pixels = new int[w * h];
            }
            try {
                nativeRenderPattern(w, h, frame, pixels);
                bmp.setPixels(pixels, 0, w, 0, 0, w, h);
                android.graphics.Rect src = new android.graphics.Rect(0, 0, w, h);
                android.graphics.Rect dst = new android.graphics.Rect(0, 0, getWidth(), getHeight());
                canvas.drawBitmap(bmp, src, dst, paint);
                fpsFrames++;
                long now = android.os.SystemClock.uptimeMillis();
                if (lastFpsT == 0) lastFpsT = now;
                long dt = now - lastFpsT;
                if (dt >= 1000) {
                    fpsText = "nativePatternFPS=" + Math.round((fpsFrames * 1000f) / Math.max(1, dt));
                    fpsFrames = 0;
                    lastFpsT = now;
                }
            } catch (Throwable t) {
                canvas.drawColor(Color.rgb(8, 12, 24));
                canvas.drawText("NATIVE C++ LIB ERROR", 30, 70, textPaint);
                canvas.drawText(String.valueOf(t.getMessage()), 30, 112, textPaint);
            }
        }
    }
}
