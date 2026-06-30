package eu.atarihelp.emu10;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;

/**
 * BUILD2PP_SEGA_NATIVE_CPP_PROOF_BUTTON_STAGE80
 *
 * POZOR: Tohle jeste NENI hotovy Sega emulator.
 * Je to poctivy native C++ proof vedle stavajici WebView Segy:
 * - Java Activity se otevre z tlacitka C++ TEST v emu_sega/index.html
 * - nacita .gen/.bin/.md ROM pres Android picker
 * - posle ROM bajty do C++ pres JNI
 * - C++ vrati realny header/checksum info
 * - C++ generuje testovaci native frame do Android Canvas bitmapy
 *
 * Cil: overit, ze projekt umi hybrid Java/WebView UI -> Android Activity -> JNI -> C++.
 * Zadny fake gameplay, zadne ROM v APK.
 */
public class NativeSegaProofActivity extends Activity {
    private static final int PICK_ROM = 80;
    private TextView info;
    private NativePatternView pattern;
    private boolean nativeOk = false;

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
        title.setText("SEGA C++ TEST / BUILD2PP");
        right.addView(title);

        Button pick = new Button(this);
        pick.setText("VYBRAT ROM PRO C++ TEST");
        pick.setOnClickListener(v -> openRomPicker());
        right.addView(pick);

        Button back = new Button(this);
        back.setText("ZPET DO APPKY");
        back.setOnClickListener(v -> finish());
        right.addView(back);

        info = new TextView(this);
        info.setTextColor(Color.rgb(210, 236, 255));
        info.setTextSize(13);
        info.setTypeface(android.graphics.Typeface.MONOSPACE);
        info.setText(defaultInfo());
        ScrollView scroll = new ScrollView(this);
        scroll.addView(info);
        right.addView(scroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1.0f));

        setContentView(root);
    }

    private String defaultInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append("STAV:\n");
        sb.append("- Toto neni fake Sega gameplay.\n");
        sb.append("- WebView Sega zustava beze zmen jako zaloha.\n");
        sb.append("- Tohle overuje JNI/C++ cestu pro budouci native core.\n\n");
        if (nativeOk) {
            try { sb.append(nativeCoreBuildString()).append("\n\n"); }
            catch (Throwable t) { sb.append("Native build string ERROR: ").append(t.getMessage()).append("\n\n"); }
        } else {
            sb.append("NATIVE LIBRARY NENAHRANA.\n");
            sb.append("Pokud build spadne, bude pravdepodobne chybet NDK/CMake.\n\n");
        }
        sb.append("KROK:\n");
        sb.append("1. Dej VYBRAT ROM PRO C++ TEST.\n");
        sb.append("2. Vyber .gen/.bin/.md.\n");
        sb.append("3. C++ vrati header/checksum info.\n\n");
        sb.append("CIL DALSI VETVE:\n");
        sb.append("realny Sega C++ core + native obraz + native audio.\n");
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
            try {
                nativeInfo = nativeRomInfo(bytes);
            } catch (Throwable t) {
                nativeInfo = "NATIVE ROM INFO ERROR: " + t.getMessage();
            }
            info.setText("ROM: " + name + "\n" + nativeInfo + "\n\n" + defaultInfo());
        } catch (Throwable t) {
            info.setText("CHYBA PRI CTENI ROM:\n" + t.getMessage() + "\n\n" + defaultInfo());
        }
    }

    public static class NativePatternView extends View {
        private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
        private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Handler handler = new Handler(Looper.getMainLooper());
        private Bitmap bmp;
        private int[] pixels;
        private int frame = 0;
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
            } catch (Throwable t) {
                canvas.drawColor(Color.rgb(8, 12, 24));
                canvas.drawText("NATIVE C++ LIB ERROR", 30, 70, textPaint);
                canvas.drawText(String.valueOf(t.getMessage()), 30, 112, textPaint);
            }
        }
    }
}
