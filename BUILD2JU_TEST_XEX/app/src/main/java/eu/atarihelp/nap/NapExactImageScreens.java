package eu.atarihelp.nap;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.net.Uri;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * N&P VISION EXACT IMAGE SCREENS
 *
 * CIL:
 * - presna graficka kopie navrhu = obrazek jako full-screen UI
 * - funkcni neviditelne hit-zony pres tlacitka
 * - bez XML, cista Java
 *
 * DULEZITE:
 * - assety musi byt v: app/src/main/assets/nap_ui/
 *   nap_intro_command_center.png
 *   nap_player_cassette.png
 *   nap_original_logo.png
 *
 * Integrace:
 *   NapExactImageScreens.showIntro(this, new NapExactImageScreens.Actions() { ... });
 *
 * Pokud je v aplikaci jiny package, uprav prvni radek tohoto souboru
 * z "package eu.atarihelp.nap;" na package aplikace, nebo importuj tuto tridu.
 */
public final class NapExactImageScreens {

    private NapExactImageScreens() {}

    public interface Actions {
        void openEmulator();
        void openPlayer();
        void openAtariHelp();
        void openHelp();
        void openYouTube();
        void openMyPlaylists();
        void openLibrary();
        void openFavorites();
        void openSettings();
        void onPlayerTransport(String command);
    }

    public static class SimpleActions implements Actions {
        protected final Activity activity;

        public SimpleActions(Activity activity) {
            this.activity = activity;
        }

        @Override public void openEmulator() {
            Toast.makeText(activity, "EMULATOR - napojit na existujici emulator", Toast.LENGTH_SHORT).show();
        }

        @Override public void openPlayer() {
            showPlayer(activity, this);
        }

        @Override public void openAtariHelp() {
            openExternalUrl(activity, "https://atarihelp.eu/");
        }

        @Override public void openHelp() {
            Toast.makeText(activity, "HELP - napojit na napovedu aplikace", Toast.LENGTH_SHORT).show();
        }

        @Override public void openYouTube() {
            openExternalUrl(activity, "https://www.youtube.com/");
        }

        @Override public void openMyPlaylists() {
            Toast.makeText(activity, "MY PLAYLISTS - napojit na playlist", Toast.LENGTH_SHORT).show();
        }

        @Override public void openLibrary() {
            Toast.makeText(activity, "LIBRARY - napojit na knihovnu", Toast.LENGTH_SHORT).show();
        }

        @Override public void openFavorites() {
            Toast.makeText(activity, "FAVORITES - napojit na oblibene", Toast.LENGTH_SHORT).show();
        }

        @Override public void openSettings() {
            Toast.makeText(activity, "SETTINGS - napojit na nastaveni", Toast.LENGTH_SHORT).show();
        }

        @Override public void onPlayerTransport(String command) {
            Toast.makeText(activity, command, Toast.LENGTH_SHORT).show();
        }
    }

    public static void showIntro(final Activity activity, final Actions actions) {
        List<Zone> zones = new ArrayList<>();

        // Souradnice jsou normalizovane podle obrazku 941x1672.
        // Funguje i pri jinem rozliseni, protoze se klik prepocitava na obrazek.
        zones.add(new Zone("EMULATOR", 0.055f, 0.490f, 0.500f, 0.655f, new Runnable() {
            @Override public void run() { actions.openEmulator(); }
        }));
        zones.add(new Zone("PREHRAVAC", 0.500f, 0.490f, 0.945f, 0.655f, new Runnable() {
            @Override public void run() { actions.openPlayer(); }
        }));
        zones.add(new Zone("ATARIHELP.EU", 0.055f, 0.655f, 0.500f, 0.820f, new Runnable() {
            @Override public void run() { actions.openAtariHelp(); }
        }));
        zones.add(new Zone("HELP", 0.500f, 0.655f, 0.945f, 0.820f, new Runnable() {
            @Override public void run() { actions.openHelp(); }
        }));

        activity.setContentView(new ExactImageMenuView(activity, "nap_ui/nap_intro_command_center.png", zones));
    }

    public static void showPlayer(final Activity activity, final Actions actions) {
        List<Zone> zones = new ArrayList<>();

        // Transportni tlacitka na kazetaku / prehravaci
        zones.add(new Zone("REWIND", 0.050f, 0.710f, 0.200f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("REWIND"); }
        }));
        zones.add(new Zone("PLAY", 0.200f, 0.710f, 0.355f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("PLAY"); }
        }));
        zones.add(new Zone("STOP", 0.355f, 0.710f, 0.510f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("STOP"); }
        }));
        zones.add(new Zone("PAUSE", 0.510f, 0.710f, 0.665f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("PAUSE"); }
        }));
        zones.add(new Zone("FAST_FWD", 0.665f, 0.710f, 0.820f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("FAST FWD"); }
        }));
        zones.add(new Zone("RECORD", 0.820f, 0.710f, 0.955f, 0.770f, new Runnable() {
            @Override public void run() { actions.onPlayerTransport("RECORD"); }
        }));

        // Playlist / YouTube zony
        zones.add(new Zone("MY_PLAYLISTS", 0.045f, 0.785f, 0.390f, 0.850f, new Runnable() {
            @Override public void run() { actions.openMyPlaylists(); }
        }));
        zones.add(new Zone("YOUTUBE", 0.630f, 0.785f, 0.955f, 0.850f, new Runnable() {
            @Override public void run() { actions.openYouTube(); }
        }));

        // Spodni navigace
        zones.add(new Zone("PLAYER", 0.000f, 0.930f, 0.200f, 1.000f, new Runnable() {
            @Override public void run() { showPlayer(activity, actions); }
        }));
        zones.add(new Zone("PLAYLISTS", 0.200f, 0.930f, 0.400f, 1.000f, new Runnable() {
            @Override public void run() { actions.openMyPlaylists(); }
        }));
        zones.add(new Zone("LIBRARY", 0.400f, 0.930f, 0.600f, 1.000f, new Runnable() {
            @Override public void run() { actions.openLibrary(); }
        }));
        zones.add(new Zone("FAVORITES", 0.600f, 0.930f, 0.800f, 1.000f, new Runnable() {
            @Override public void run() { actions.openFavorites(); }
        }));
        zones.add(new Zone("SETTINGS", 0.800f, 0.930f, 1.000f, 1.000f, new Runnable() {
            @Override public void run() { actions.openSettings(); }
        }));

        activity.setContentView(new ExactImageMenuView(activity, "nap_ui/nap_player_cassette.png", zones));
    }

    public static void openExternalUrl(Activity activity, String url) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            intent.addCategory(Intent.CATEGORY_BROWSABLE);
            activity.startActivity(intent);
        } catch (Exception e) {
            Toast.makeText(activity, "Odkaz nejde otevrit: " + url, Toast.LENGTH_LONG).show();
        }
    }

    private static final class Zone {
        final String name;
        final RectF norm;
        final Runnable action;

        Zone(String name, float l, float t, float r, float b, Runnable action) {
            this.name = name;
            this.norm = new RectF(l, t, r, b);
            this.action = action;
        }

        boolean hit(float nx, float ny) {
            return norm.contains(nx, ny);
        }
    }

    public static final class ExactImageMenuView extends View {
        private final Activity activity;
        private final String assetPath;
        private final List<Zone> zones;
        private final Paint imagePaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
        private final Paint debugPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Rect src = new Rect();
        private final RectF dst = new RectF();
        private Bitmap bitmap;
        private boolean debugZones = false;

        public ExactImageMenuView(Activity activity, String assetPath, List<Zone> zones) {
            super(activity);
            this.activity = activity;
            this.assetPath = assetPath;
            this.zones = zones;
            setFocusable(true);
            setClickable(true);
            loadBitmap();

            debugPaint.setStyle(Paint.Style.STROKE);
            debugPaint.setStrokeWidth(3f);
            debugPaint.setColor(Color.argb(160, 255, 200, 40));
        }

        public void setDebugZones(boolean enabled) {
            debugZones = enabled;
            invalidate();
        }

        private void loadBitmap() {
            try {
                InputStream is = activity.getAssets().open(assetPath);
                bitmap = BitmapFactory.decodeStream(is);
                is.close();
                if (bitmap != null) {
                    src.set(0, 0, bitmap.getWidth(), bitmap.getHeight());
                }
            } catch (Exception e) {
                bitmap = null;
            }
        }

        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            canvas.drawColor(Color.rgb(3, 3, 3));

            if (bitmap == null) {
                Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
                p.setColor(Color.rgb(255, 200, 80));
                p.setTextSize(34f);
                canvas.drawText("CHYBI ASSET: " + assetPath, 30, 80, p);
                return;
            }

            computeFitCenterRect(getWidth(), getHeight(), bitmap.getWidth(), bitmap.getHeight(), dst);
            canvas.drawBitmap(bitmap, src, dst, imagePaint);

            if (debugZones) {
                for (Zone z : zones) {
                    RectF r = zoneToScreen(z.norm);
                    canvas.drawRect(r, debugPaint);
                }
            }
        }

        @Override public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() != MotionEvent.ACTION_UP) {
                return true;
            }
            if (bitmap == null) return true;
            if (!dst.contains(event.getX(), event.getY())) return true;

            float nx = (event.getX() - dst.left) / dst.width();
            float ny = (event.getY() - dst.top) / dst.height();

            for (Zone z : zones) {
                if (z.hit(nx, ny)) {
                    if (z.action != null) z.action.run();
                    return true;
                }
            }
            return true;
        }

        private RectF zoneToScreen(RectF n) {
            return new RectF(
                    dst.left + n.left * dst.width(),
                    dst.top + n.top * dst.height(),
                    dst.left + n.right * dst.width(),
                    dst.top + n.bottom * dst.height()
            );
        }

        private static void computeFitCenterRect(int viewW, int viewH, int imgW, int imgH, RectF out) {
            float scale = Math.min((float) viewW / (float) imgW, (float) viewH / (float) imgH);
            float w = imgW * scale;
            float h = imgH * scale;
            float l = (viewW - w) * 0.5f;
            float t = (viewH - h) * 0.5f;
            out.set(l, t, l + w, t + h);
        }
    }
}
