package eu.atarihelp.emu10;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ContentValues;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.content.ClipData;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Build;
import android.os.Debug; // BUILD2RW: passive heap/GC audit only
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.provider.MediaStore;
import android.util.Base64;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.view.View;
import android.view.TextureView;
import android.graphics.SurfaceTexture;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceError;
import android.webkit.WebResourceResponse;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLDecoder;

/**
 * AtariHelp.eu EMU-10 BUILD2BM
 * - file chooser (NAHRAJ XEX/ATR/ZIP)
 * - AHSAVE (ulozeni logu)
 * - DownloadListener: ZIP/XEX/ATR z webu se stahne a rovnou spusti v emulatoru
 * - BUILD2AG UI: NET HRY + XC12 WAV/MP3 real seek pres REW/F.FWD
 * - BUILD2AQ INTRO MP3: MP3 PRIDAT nacita/pripojuje skladby do playlistu + EJECT reset
 * - BUILD2BM XC12: cycle-accurate tape tone for CLOAD + quieter WAV + touch zone alignment
 */
public class MainActivity extends Activity {
    private static final int PICK_FILE = 1;
    private static final int PICK_BRIDGE = 2;
    private static final int PICK_PS1_GAME = 7; // BUILD2SA2
    private static final String ATARIHELP_BROWSER_UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"; // BUILD2SA5K
    private static final long ATARIHELP_MIN_REQUEST_GAP_MS = 30000L; // BUILD2SA5M: no accidental hammering.
    private static final long ATARIHELP_FAIL_COOLDOWN_MS = 15L * 60L * 1000L;
    private static final int ATARI_INJECT_MAX_ATTEMPTS = 80; // BUILD2SA5W: wait until 130XE page exposes AHRECV_* before sending a web game.
    private static final long ATARI_INJECT_RETRY_MS = 250L;
    private static final int ATARI_INJECT_CHUNK_CHARS = 65536; // BUILD2SA5X: ordered AHRECV transfer, smaller chunks for WebView.
    private static final String SEGA_URL = "file:///android_asset/emu_sega/index.html"; // BUILD2SA2
    private static byte[] pendingSegaGame = null;   // BUILD2SA2: hra ze SBIRKY cekajici na nacteni Sega stranky
    private static String pendingSegaName = null;
    private static android.os.ParcelFileDescriptor ps1GamePfd = null; // BUILD2SA2: drzi fd otevrene hry
    private static volatile String ps1LastBootResult = "not_booted";
    private static volatile int ps1AudioGen = 0; // BUILD2SA3
    private volatile int ps1LifecycleGen = 0; // BUILD2SA5I: cancels stale PS1 boots/audio after leaving PS1.
    private volatile boolean ps1BootActive = false;
    private volatile boolean ps1SessionActive = false;
    private volatile long atariHelpLastRequestAtMs = 0L; // BUILD2SA5M
    private volatile long atariHelpBlockedUntilMs = 0L;
    private volatile AudioTrack ps1CurrentAudioTrack = null; // BUILD2SA3B: hard-stop pri prepnuti PS1 hry
    private Thread ps1AudioThread = null;
    private volatile String ps1CurrentGameLabel = "ps1_game";
    private static final String EMU_URL = "file:///android_asset/emu/index.html";
    private WebView web;
    private FrameLayout rootFrame;
    private NativeInPlaceView nativeInPlaceView;
    private boolean nativeInPlaceEnabled = false;
    private String nativeLastRomInfo = "C++ ROM zatim nenactena";
    private String nativeLastStatus = "NATIVE_OFF";
    private int nativeInputEvents = 0;
    private final StringBuilder nativeLog = new StringBuilder();
    private volatile boolean nativeCoreAudioRun = false;
    private volatile int nativeAudioGeneration = 0; // BUILD2RV: kills stale AudioTrack threads after every ROM change; prevents cumulative slowdown.
    private Thread nativeCoreAudioThread;
    private volatile AudioTrack nativeCurrentAudioTrack;
    private volatile int nativeActiveAudioTracks = 0; // BUILD2RV: proof that only one Sega AudioTrack path is alive.
    private ValueCallback<Uri[]> pendingChooser;
    private byte[] pendingGame;
    private String pendingName;
    private int pendingGameInjectSeq = 0;
    private String pendingBridgeKind;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private volatile int nativeRomLoadGeneration = 0;
    private volatile long nativeViewDrawCounter = 0;
    private volatile long nativeViewDrawCounterAtRomLoad = 0;
    private volatile boolean nativeLandscapeFullVideo = false;
    private volatile long nativeLastRenderCostNs = 0;
    private volatile long nativeRenderPerfWindowStartMs = 0;
    private volatile long nativeRenderPerfWindowFrames = 0;
    private volatile long nativeRenderPerfSlowFrames = 0;
    private volatile String nativePerformanceMode = "HIGH"; // BUILD2RV: SBIRKA selects HIGH QUALITY / LOW PERFORMANCE.
    private volatile long nativeLastSaveLogAtMs = 0; // BUILD2RV: one tap must not create 4 log files.

    private String safeFileName(String name) {
        if (name == null || name.trim().length() == 0) name = "AtariHelp_file.bin";
        String safe = name.replaceAll("[^A-Za-z0-9._-]", "_");
        if (safe.length() > 96) safe = safe.substring(0, 96);
        return safe;
    }

    private File getPublicAtariHelpDownloadsDir() {
        File base = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        File dir = new File(base, "AtariHelp");
        if (!dir.exists()) dir.mkdirs();
        return dir;
    }

    private String mimeForName(String name) {
        String lower = name == null ? "" : name.toLowerCase();
        if (lower.endsWith(".cas")) return "application/octet-stream";
        if (lower.endsWith(".wav")) return "audio/wav";
        if (lower.endsWith(".txt") || lower.endsWith(".log")) return "text/plain";
        return "application/octet-stream";
    }

    private String writeBytesToDownloads(String name, byte[] data) throws IOException {
        String safe = safeFileName(name);

        // Android 10+ spravne pres MediaStore do viditelne slozky Downloads/AtariHelp.
        if (Build.VERSION.SDK_INT >= 29) {
            ContentValues values = new ContentValues();
            values.put(MediaStore.MediaColumns.DISPLAY_NAME, safe);
            values.put(MediaStore.MediaColumns.MIME_TYPE, mimeForName(safe));
            values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/AtariHelp");
            Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
            if (uri == null) throw new IOException("MediaStore insert vratil null");
            OutputStream out = getContentResolver().openOutputStream(uri);
            if (out == null) throw new IOException("MediaStore openOutputStream vratil null");
            try { out.write(data); } finally { out.close(); }
            return "Downloads/AtariHelp/" + safe;
        }

        // Android 9 a starsi / NOX: klasicka verejna slozka Downloads/AtariHelp.
        File dir = getPublicAtariHelpDownloadsDir();
        File f = new File(dir, safe);
        int n = 1;
        String base = safe;
        String ext = "";
        int dot = safe.lastIndexOf('.');
        if (dot > 0) { base = safe.substring(0, dot); ext = safe.substring(dot); }
        while (f.exists()) {
            f = new File(dir, base + "_" + n + ext);
            n++;
        }
        FileOutputStream out = new FileOutputStream(f);
        try { out.write(data); } finally { out.close(); }
        return f.getAbsolutePath();
    }

    public class AHSave {
        @JavascriptInterface
        public String save(String name, String text) {
            try {
                String path = writeBytesToDownloads(name, text.getBytes("UTF-8"));
                return "DOWNLOADS_OK:" + path;
            } catch (Exception e) {
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir == null) dir = getFilesDir();
                    File f = new File(dir, safeFileName(name));
                    FileOutputStream out = new FileOutputStream(f);
                    out.write(text.getBytes("UTF-8"));
                    out.close();
                    return "FALLBACK_APP_DIR:" + f.getAbsolutePath() + " | DOWNLOADS_CHYBA:" + e.getMessage();
                } catch (Exception e2) {
                    return "CHYBA: " + e.getMessage() + " / fallback: " + e2.getMessage();
                }
            }
        }

        @JavascriptInterface
        public String saveBase64(String name, String b64) {
            String clean = b64 == null ? "" : b64.replaceAll("\\s", "");
            int mod = clean.length() % 4;
            if (mod == 2) clean += "==";
            else if (mod == 3) clean += "=";
            try {
                byte[] data = Base64.decode(clean, Base64.DEFAULT);
                String path = writeBytesToDownloads(name, data);
                return "DOWNLOADS_OK:" + path;
            } catch (Exception e) {
                try {
                    File dir = getExternalFilesDir(null);
                    if (dir == null) dir = getFilesDir();
                    File f = new File(dir, safeFileName(name));
                    byte[] data = Base64.decode(clean, Base64.DEFAULT);
                    FileOutputStream out = new FileOutputStream(f);
                    out.write(data);
                    out.close();
                    return "FALLBACK_APP_DIR:" + f.getAbsolutePath() + " | DOWNLOADS_CHYBA:" + e.getMessage();
                } catch (Exception e2) {
                    return "CHYBA: " + e.getMessage() + " / fallback: " + e2.getMessage();
                }
            }
        }
    }

    private String ps1CleanBiosName(String name) {
        if (name == null) name = "bios.bin";
        return name.toLowerCase(Locale.US).replaceAll("[^a-z0-9._-]", "_");
    }

    private void ps1WriteBytes(File f, byte[] data) throws IOException {
        FileOutputStream fo = new FileOutputStream(f);
        try { fo.write(data); } finally { fo.close(); }
    }

    private String ps1WriteBiosAliases(File dir, String clean, byte[] data) throws IOException {
        if (dir == null || clean == null || data == null || data.length != 524288) return "";
        String alias = "";
        String compact = clean.replace("-", "").replace("_", "");
        String canonical = null;
        if (compact.matches("scph[0-9a-z]+\\.bin")) canonical = compact;
        else if ("psxonpsp660.bin".equals(clean) || "psxonpsp660.bin".equals(compact)) canonical = "PSXONPSP660.bin";
        else if (clean.contains("5500")) canonical = "scph5500.bin";
        else if (clean.contains("5501")) canonical = "scph5501.bin";
        else if (clean.contains("5502")) canonical = "scph5502.bin";
        else if (clean.contains("1001")) canonical = "scph1001.bin";
        else if (clean.contains("7502")) canonical = "scph7502.bin";
        else if (clean.contains("7001")) canonical = "scph7001.bin";
        else if (clean.contains("7003")) canonical = "scph7003.bin";

        if (canonical != null && !canonical.equals(clean)) {
            File cf = new File(dir, canonical);
            ps1WriteBytes(cf, data);
            alias += " alias=" + cf.getAbsolutePath();
        } else if (canonical == null && !clean.matches("scph[0-9a-z]+\\.bin")) {
            File cf = new File(dir, "scph5501.bin");
            ps1WriteBytes(cf, data);
            alias += " alias=" + cf.getAbsolutePath();
        }
        return alias;
    }

    public class AHPS1 {
        @JavascriptInterface
        public String ps1CoreInfo() { return NativePs1CoreBridge.coreInfoSafe(); }
        // BUILD2SA2/SA5P: ulozi BIOS .bin do systemove slozky jadra, pokud ho uzivatel sam vybere.
        @JavascriptInterface
        public String ps1SaveBios(String name, String b64) {
            try {
                if (name == null || b64 == null) return "PS1_BIOS_SAVE_FAIL empty";
                java.io.File dir = new java.io.File(getFilesDir(), "ps1_system");
                if (!dir.exists() && !dir.mkdirs()) return "PS1_BIOS_SAVE_FAIL mkdir";
                String clean = ps1CleanBiosName(name);
                byte[] data = Base64.decode(b64, Base64.DEFAULT);
                java.io.File f = new java.io.File(dir, clean);
                ps1WriteBytes(f, data);
                String alias = ps1WriteBiosAliases(dir, clean, data);
                return "PS1_BIOS_SAVED path=" + f.getAbsolutePath() + alias + " bytes=" + data.length;
            } catch (Throwable t) { return "PS1_BIOS_SAVE_FAIL " + t.getMessage(); }
        }
        // BUILD2SA2: nativni vyber hry -> fd -> /proc/self/fd (700MB bez kopirovani)
        @JavascriptInterface
        public void ps1PickGame() {
            ui.post(() -> {
                try {
                    Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                    i.setType("*/*");
                    i.addCategory(Intent.CATEGORY_OPENABLE);
                    startActivityForResult(Intent.createChooser(i, "PS1: vyber .bin / .iso / .img hry"), PICK_PS1_GAME);
                } catch (Throwable t) { ps1LastBootResult = "PS1_PICK_FAIL " + t.getMessage(); }
            });
        }
        @JavascriptInterface
        public String ps1Status() { return NativePs1CoreBridge.statusSafe() + " | lastBoot=" + ps1LastBootResult; }
        @JavascriptInterface
        public void ps1Input(String button, boolean down) {
            int id = ps1ButtonId(button);
            if (id >= 0) NativePs1CoreBridge.setInputSafe(id, down);
        }
        private File ps1StateFile() {
            File saveDir = new File(getFilesDir(), "ps1_saves");
            if (!saveDir.exists()) saveDir.mkdirs();
            String base = safeFileName(ps1CurrentGameLabel == null ? "ps1_game" : ps1CurrentGameLabel);
            if (base.endsWith(".bin") || base.endsWith(".iso") || base.endsWith(".img") || base.endsWith(".cue")) {
                int dot = base.lastIndexOf('.');
                if (dot > 0) base = base.substring(0, dot);
            }
            return new File(saveDir, base + ".slot0.state");
        }
        @JavascriptInterface
        public String ps1QuickSave() {
            try {
                File f = ps1StateFile();
                String r = NativePs1CoreBridge.saveStateSafe(f.getAbsolutePath());
                return r;
            } catch (Throwable t) {
                return "PS1_STATE_SAVE_EXCEPTION " + t.getMessage();
            }
        }
        @JavascriptInterface
        public String ps1QuickLoad() {
            try {
                File f = ps1StateFile();
                if (!f.exists()) return "PS1_STATE_LOAD_FAIL missing_file path=" + f.getAbsolutePath();
                String r = NativePs1CoreBridge.loadStateSafe(f.getAbsolutePath());
                return r;
            } catch (Throwable t) {
                return "PS1_STATE_LOAD_EXCEPTION " + t.getMessage();
            }
        }
        @JavascriptInterface
        public String ps1SaveInfo() {
            try {
                File f = ps1StateFile();
                return "PS1_SAVE_SLOT path=" + f.getAbsolutePath() + " exists=" + f.exists() + " bytes=" + (f.exists() ? f.length() : 0);
            } catch (Throwable t) {
                return "PS1_SAVE_SLOT_ERROR " + t.getMessage();
            }
        }
        // BUILD2SA5: nahled obrazu - realne snimky z jadra jako kvalitnejsi JPEG base64.
        // Neni to finalni render (ten pojede pres TextureView v SA2C), ale je to
        // OPRAVDOVY obraz z beziciho jadra, zadny fake.
        private int[] ps1PrevBuf = new int[1024 * 512];
        @JavascriptInterface
        public String ps1FramePreviewB64() {
            try {
                int wh = NativePs1CoreBridge.grabFrameSafe(ps1PrevBuf);
                if (wh < 0) { int need = ((-wh) >> 16) * ((-wh) & 0xFFFF); ps1PrevBuf = new int[need + 1024]; wh = NativePs1CoreBridge.grabFrameSafe(ps1PrevBuf); }
                if (wh <= 0) return "";
                int w = wh >> 16, h = wh & 0xFFFF;
                android.graphics.Bitmap bm = android.graphics.Bitmap.createBitmap(ps1PrevBuf, w, h, android.graphics.Bitmap.Config.ARGB_8888);
                java.io.ByteArrayOutputStream bo = new java.io.ByteArrayOutputStream();
                bm.compress(android.graphics.Bitmap.CompressFormat.JPEG, 95, bo);
                bm.recycle();
                return Base64.encodeToString(bo.toByteArray(), Base64.NO_WRAP);
            } catch (Throwable t) { return ""; }
        }
        @JavascriptInterface
        public String ps1Stop() {
            return stopPs1SessionHard("jsPs1Stop"); // BUILD2SA5I: one path stops audio + core + fd.
        }
    }
    private int ps1ButtonId(String button) {
        if (button == null) return -1;
        String b = button.toUpperCase(Locale.US);
        if ("CROSS".equals(b) || "X".equals(b)) return 0;       // RetroPad B -> PS1 Cross
        if ("SQUARE".equals(b)) return 1;                       // RetroPad Y -> PS1 Square
        if ("SELECT".equals(b)) return 2;
        if ("START".equals(b)) return 3;
        if ("UP".equals(b)) return 4;
        if ("DOWN".equals(b)) return 5;
        if ("LEFT".equals(b)) return 6;
        if ("RIGHT".equals(b)) return 7;
        if ("CIRCLE".equals(b) || "O".equals(b)) return 8;      // RetroPad A -> PS1 Circle
        if ("TRIANGLE".equals(b)) return 9;                     // RetroPad X -> PS1 Triangle
        if ("L1".equals(b) || "L".equals(b)) return 10;
        if ("R1".equals(b) || "R".equals(b)) return 11;
        if ("L2".equals(b)) return 12;
        if ("R2".equals(b)) return 13;
        return -1;
    }

    public class AHPick {
        @JavascriptInterface
        public void pickGame() {
            ui.post(() -> openBridgePicker("game"));
        }
        @JavascriptInterface
        public void pickAudio() {
            ui.post(() -> openBridgePicker("audio"));
        }
        @JavascriptInterface
        public void pickMp3() {
            ui.post(() -> openBridgePicker("mp3"));
        }
        @JavascriptInterface
        public void pickText() {
            ui.post(() -> openBridgePicker("text"));
        }
    }
    private String nowStamp() {
        try { return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US).format(new Date()); }
        catch (Throwable t) { return String.valueOf(System.currentTimeMillis()); }
    }

    private void appendNativeLog(String line) {
        synchronized (nativeLog) {
            nativeLog.append(nowStamp()).append("  ").append(line == null ? "" : line).append("\n");
            // BUILD2RW: bigger ring (~100 KB) so the 10s PASSIVE_AUDIT_RW rows survive a long S8
            // degradation test. Memory cost is trivial; the point is to SEE what grows over time.
            if (nativeLog.length() > 100000) nativeLog.delete(0, nativeLog.length() - 100000);
        }
    }

    private int mapNativeKey(String btn) {
        if (btn == null) return -1;
        String b = btn.toUpperCase(Locale.US);
        if ("UP".equals(b)) return 0;
        if ("DOWN".equals(b)) return 1;
        if ("LEFT".equals(b)) return 2;
        if ("RIGHT".equals(b)) return 3;
        if ("A".equals(b)) return 4;
        if ("B".equals(b)) return 5;
        if ("C".equals(b)) return 6;
        if ("START".equals(b)) return 7;
        return -1;
    }

    private String buildNativeInPlaceLog() {
        StringBuilder out = new StringBuilder();
        out.append("SEGA C++ IN-PLACE LOG / BUILD2RY\n");
        out.append("AtariHelp.eu EMU-10 BUILD2RY_SEGA_NATIVE_CPP_ONLY_CORE_O3_VDP_LEFT_BLANK_STAGE141\n\n");
        out.append("DEVICE sdk=").append(Build.VERSION.SDK_INT)
           .append(" release=").append(Build.VERSION.RELEASE)
           .append(" brand=").append(Build.BRAND)
           .append(" model=").append(Build.MODEL)
           .append(" cores=").append(Runtime.getRuntime().availableProcessors()).append("\n");
        out.append("nativeInPlaceEnabled=").append(nativeInPlaceEnabled).append("\n");
        out.append("nativeLastStatus=").append(nativeLastStatus).append("\n");
        out.append("nativeInputEvents=").append(nativeInputEvents).append("\n");
        out.append("nativeCoreAudioRun=").append(nativeCoreAudioRun).append("\n");
        out.append("nativeAudioGeneration=").append(nativeAudioGeneration).append("\n");
        out.append("nativePerformanceMode=").append(nativePerformanceMode).append("\n");
        out.append("nativeActiveAudioTracks=").append(nativeActiveAudioTracks).append("\n");
        try { out.append("realCoreStatus=").append(NativeSegaCoreBridge.realCoreStatus().replace('\n',' ')).append("\n"); } catch (Throwable t) { out.append("realCoreStatus=ERROR ").append(safeMsg(t)).append("\n"); }
        out.append("\nROM BLOCK:\n").append(nativeLastRomInfo == null ? "" : nativeLastRomInfo).append("\n\n");
        out.append("EVENTS:\n");
        synchronized (nativeLog) { out.append(nativeLog.toString()); }
        out.append("\nDULEZITE:\n- Tohle porad neni hotovy Sega gameplay.\n");
        out.append("- Toto overuje normalni Sega UI -> Java -> JNI -> C++ -> ROM/input/audio/render/log.\n");
        out.append("- Sega emulace je v BUILD2RV C++ only; Java/WebView wrapper se nespousti; C++ CORE UI tlacitko je odstranene; FM/PSG zeroed RR recovery safe-audit bez hard-drop orezavani; SBIRKA prepina LOW/HIGH vykon; SBIRKA prepina LOW/HIGH vykon; mobile visibility/orientation nesmi vypnout native video; C++ region respektuje ROM header; nulove recty se ignoruji.\n");
        return out.toString();
    }

    private synchronized void startNativeCoreAudioStream() {
        if (nativeCoreAudioRun || (nativeCoreAudioThread != null && nativeCoreAudioThread.isAlive()) || nativeCurrentAudioTrack != null) {
            appendNativeLog("NATIVE_AUDIO_STREAM_PRESTART_HARD_STOP_RV_RR_RECOVERY_SAFE_AUDIT_QT oldGen=" + nativeAudioGeneration);
            stopNativeCoreAudioStream();
        }
        final int audioGen = ++nativeAudioGeneration; // BUILD2RV: one valid AudioTrack writer per ROM session.
        nativeCoreAudioRun = true;
        nativeCoreAudioThread = new Thread(() -> {
            try { android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); } catch (Throwable ignored) {}
            final int sampleRate = 48000;
            final boolean s8NoStarve = (Build.VERSION.SDK_INT <= 28) || ((Build.MODEL == null ? "" : Build.MODEL).toUpperCase(Locale.US).contains("SM-G950"));
            final int chunk = 384; // BUILD2RV: stable stereo chunk; RP 192-frame low-latency cracked; keep 384.
            AudioTrack track = null;
            try {
                int min = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                // BUILD2RV: S8 RR recovery profile gets a real reservoir; Nox keeps the clean RQ-sized path.
                int wantedFrames = s8NoStarve ? 8192 : 3072;
                int wantedBytes = wantedFrames * 2 * 2;
                int bufferBytes = Math.max(min > 0 ? (s8NoStarve ? min * 3 : min * 2) : 0, wantedBytes);
                AudioTrack.Builder builder = null;
                if (Build.VERSION.SDK_INT >= 21) {
                    builder = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                            .setBufferSizeInBytes(bufferBytes)
                            .setTransferMode(AudioTrack.MODE_STREAM);
                    if (Build.VERSION.SDK_INT >= 26) {
                        /* BUILD2RV: do not force Android low-latency profile; on S8/Nox it caused crackle when the core needed a little reservoir. */
                    }
                    track = builder.build();
                } else {
                    track = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufferBytes, AudioTrack.MODE_STREAM);
                }
                int setFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) {
                    try { setFrames = track.setBufferSizeInFrames(s8NoStarve ? 8192 : Math.max(3072, chunk * 8)); } catch (Throwable ignored) {}
                }
                int actualFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) { try { actualFrames = track.getBufferSizeInFrames(); } catch (Throwable ignored) {} }
                short[] pcm = new short[chunk * 2];
                nativeCurrentAudioTrack = track;
                nativeActiveAudioTracks = 1;
                appendNativeLog("NATIVE_AUDIO_STREAM_START_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " sampleRate=" + sampleRate + " chunk=" + chunk + " minBytes=" + min + " bufferBytes=" + bufferBytes + " setFrames=" + setFrames + " mode=" + nativePerformanceMode + " forcedLowLatency=false actualFrames=" + actualFrames + " s8NoStarve=" + s8NoStarve + " singleAudioPath=YES activeAudioTracks=" + nativeActiveAudioTracks + " bufferMsApprox=" + ((actualFrames > 0 ? actualFrames : (bufferBytes / 4)) * 1000 / sampleRate));

                int prefilled = 0;
                int preLoops = 0;
                long preDeadline = System.currentTimeMillis() + (s8NoStarve ? 900 : 260);
                int prefillTarget = s8NoStarve ? 6144 : 3072;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration && prefilled < prefillTarget && System.currentTimeMillis() < preDeadline) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PREFILL_PULL_ERROR_RV " + safeMsg(pullErr)); got = 0; }
                    if (got > 0) {
                        int wr = track.write(pcm, 0, pcm.length);
                        if (wr > 0) prefilled += wr;
                    } else {
                        try { Thread.sleep(4); } catch (Throwable ignored) {}
                    }
                    preLoops++;
                }

                if (!nativeCoreAudioRun || audioGen != nativeAudioGeneration) {
                    appendNativeLog("NATIVE_AUDIO_START_CANCELLED_RV staleGen=" + audioGen + " current=" + nativeAudioGeneration);
                    return;
                }
                track.play();
                appendNativeLog("NATIVE_AUDIO_PREFILL_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " frames=" + prefilled + " loops=" + preLoops + " playState=" + track.getPlayState());
                int loops = 0;
                int underrunLoops = 0;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PULL_ERROR_RV " + safeMsg(pullErr)); got = 0; }
                    if (got <= chunk / 4) {
                        underrunLoops++;
                        // BUILD2RV: S8 no-starve waits briefly for native core instead of writing immediate silence crackles.
                        int retries = s8NoStarve ? 5 : 1;
                        for (int rr = 0; rr < retries && got <= chunk / 2 && nativeCoreAudioRun && audioGen == nativeAudioGeneration; rr++) {
                            try { Thread.sleep(s8NoStarve ? 3 : 2); } catch (Throwable ignored) {}
                            try { int got2 = NativeSegaCoreBridge.pullAudioStereo(pcm, chunk); if (got2 > got) got = got2; } catch (Throwable ignored) {}
                        }
                    }
                    if (loops < 16 || loops % 180 == 0) {
                        // BUILD2RW passive: also report the real Android AudioTrack underrun counter (API >= 24).
                        int trackUnderruns = -1;
                        try { if (Build.VERSION.SDK_INT >= 24) trackUnderruns = track.getUnderrunCount(); } catch (Throwable ignored) {}
                        appendNativeLog("NATIVE_AUDIO_PULL_RW_PASSIVE_AUDIT gen=" + audioGen + " got=" + got + " loop=" + loops + " underrunLoops=" + underrunLoops + " audioTrackUnderruns=" + trackUnderruns + " mode=" + nativePerformanceMode);
                    }
                    // BUILD2RV: feed AudioTrack one small full clock chunk. Native produces real samples or true silence only; RR uses balanced FIFO governor, not RP tiny buffer drops or RO huge delay.
                    int framesToWrite = pcm.length;
                    int off = 0;
                    while (off < framesToWrite && nativeCoreAudioRun && audioGen == nativeAudioGeneration) {
                        int wr;
                        if (Build.VERSION.SDK_INT >= 23) wr = track.write(pcm, off, framesToWrite - off, AudioTrack.WRITE_BLOCKING);
                        else wr = track.write(pcm, off, framesToWrite - off);
                        if (wr <= 0) break;
                        off += wr;
                    }
                    loops++;
                }
            } catch (Throwable t) {
                appendNativeLog("NATIVE_AUDIO_STREAM_ERROR_RV gen=" + audioGen + " " + safeMsg(t));
            } finally {
                try { if (track != null) { track.pause(); track.flush(); track.stop(); track.release(); } } catch (Throwable ignored) {}
                if (nativeCurrentAudioTrack == track) nativeCurrentAudioTrack = null;
                if (nativeActiveAudioTracks > 0) nativeActiveAudioTracks = 0;
                appendNativeLog("NATIVE_AUDIO_STREAM_STOP_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + audioGen + " current=" + nativeAudioGeneration + " activeAudioTracks=" + nativeActiveAudioTracks + " singleAudioPath=YES");
            }
        }, "AtariHelpSegaLowLatencyAudioRR_" + audioGen);
        nativeCoreAudioThread.setDaemon(true);
        try { nativeCoreAudioThread.setPriority(Thread.MAX_PRIORITY); } catch (Throwable ignored) {}
        nativeCoreAudioThread.start();
    }

    private synchronized void stopNativeCoreAudioStream() {
        final int stopGen = ++nativeAudioGeneration; // BUILD2RV: invalidate stale AudioTrack writers before starting another ROM.
        nativeCoreAudioRun = false;
        AudioTrack at = nativeCurrentAudioTrack;
        if (at != null) {
            try { at.pause(); } catch (Throwable ignored) {}
            try { at.flush(); } catch (Throwable ignored) {}
            try { at.stop(); } catch (Throwable ignored) {}
            // BUILD2RV: on S8 old AudioTrack.write() could stay blocked after ROM change.
            // Release immediately; the audio thread finally-block tolerates the already released track.
            try { at.release(); } catch (Throwable ignored) {}
            nativeCurrentAudioTrack = null;
        }
        Thread t = nativeCoreAudioThread;
        if (t != null && t.isAlive() && Thread.currentThread() != t) {
            try { t.interrupt(); } catch (Throwable ignored) {}
            try { t.join(1100); } catch (Throwable ignored) {}
            if (t.isAlive()) appendNativeLog("NATIVE_AUDIO_THREAD_STILL_ALIVE_RV invalidatedGen=" + stopGen + " releasedTrack=YES will exit on generation guard");
        }
        if (nativeCoreAudioThread == t) nativeCoreAudioThread = null;
        nativeActiveAudioTracks = 0;
        appendNativeLog("NATIVE_AUDIO_STREAM_STOP_REQUEST_RV_RR_RECOVERY_SAFE_AUDIT_QT gen=" + stopGen + " hardReleaseTrack=" + (at != null) + " activeAudioTracks=" + nativeActiveAudioTracks + " oldAudioKilled=YES javaAudioReleased=" + (at != null) + " singleAudioPath=YES");
    }

    private boolean isSegaNativeOwnerUrl(String url) {
        if (url == null) return false;
        String u = url.toLowerCase(Locale.US);
        // Native C++ view smi zit jen nad normalni Sega strankou. Jakmile WebView jde na Intro/VBXE/Atari/PS1/Web,
        // musi se view i worker vypnout, jinak presne vznikne chaos "Sonic okno vsude".
        return u.startsWith("file:///android_asset/emu_sega/index.html") || u.startsWith("file:///android_asset/emu_sega/");
    }

    private String compactUrl(String url) {
        if (url == null) return "null";
        String u = url.replace("file:///android_asset/", "asset:/");
        return u.length() > 96 ? u.substring(0, 96) : u;
    }

    private static class FetchResult {
        byte[] data;
        String contentType;
        String contentDisposition;
        String via;
        int relayMode = -1;
    }

    private static class SegaExtract {
        byte[] data;
        String name;
    }

    private static class AtariExtract {
        byte[] data;
        String name;
    }

    private boolean isAtariHelpUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("atarihelp.eu") || host.equals("www.atarihelp.eu") || host.endsWith(".atarihelp.eu");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean isProviderBlockedUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase(Locale.US);
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase(Locale.US);
            return host.equals("atarihelp.eu") || host.equals("www.atarihelp.eu") || host.endsWith(".atarihelp.eu")
                    || host.equals("vedos.cz") || host.equals("www.vedos.cz") || host.endsWith(".vedos.cz");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private void applyWebViewVisualMode(String url, String reason) {
        if (web == null) return;
        boolean normalWeb = isProviderBlockedUrl(url);
        try { web.setBackgroundColor(normalWeb ? Color.WHITE : Color.TRANSPARENT); } catch (Throwable ignored) {}
        try { if (rootFrame != null) rootFrame.setBackgroundColor(normalWeb ? Color.WHITE : Color.BLACK); } catch (Throwable ignored) {}
        try { web.setLayerType(normalWeb ? View.LAYER_TYPE_NONE : View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
        if (normalWeb) appendNativeLog("BUILD2SA5K WEBVIEW_NORMAL_WEB reason=" + reason + " url=" + compactUrl(url));
    }

    private synchronized boolean markAtariHelpRequestAllowed(String url, String reason) {
        if (isProviderBlockedUrl(url)) {
            appendNativeLog("BUILD2SA5S PROVIDER_RELAY_REQUEST reason=" + reason + " url=" + compactUrl(url));
            return true;
        }
        if (!isAtariHelpUrl(url)) return true;
        long now = System.currentTimeMillis();
        if (atariHelpBlockedUntilMs > now) {
            showAtariHelpSafetyStop(url, reason, atariHelpBlockedUntilMs - now);
            appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_COOLDOWN reason=" + reason + " waitMs=" + (atariHelpBlockedUntilMs - now) + " url=" + compactUrl(url));
            return false;
        }
        boolean userDownload = reason != null && reason.startsWith("download");
        long elapsed = now - atariHelpLastRequestAtMs;
        if (!userDownload && atariHelpLastRequestAtMs > 0 && elapsed < ATARIHELP_MIN_REQUEST_GAP_MS) {
            long waitMs = ATARIHELP_MIN_REQUEST_GAP_MS - elapsed;
            showAtariHelpSafetyStop(url, reason, waitMs);
            appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_RATE reason=" + reason + " waitMs=" + waitMs + " url=" + compactUrl(url));
            return false;
        }
        atariHelpLastRequestAtMs = now;
        appendNativeLog("BUILD2SA5M ATARIHELP_REQUEST_ALLOWED reason=" + reason + " url=" + compactUrl(url));
        return true;
    }

    private void loadAtariHelpGuarded(String url, String reason) {
        if (web == null || url == null) return;
        if (isProviderBlockedUrl(url)) {
            applyWebViewVisualMode(url, "relayLoad");
            appendNativeLog("BUILD2SA5S ATARIHELP_RELAY_LOAD reason=" + reason + " url=" + compactUrl(url));
            web.loadUrl(url);
            return;
        }
        if (!markAtariHelpRequestAllowed(url, reason)) return;
        applyWebViewVisualMode(url, reason);
        web.loadUrl(url);
    }

    private String providerRelayUrl(String url, int mode) throws IOException {
        String enc = java.net.URLEncoder.encode(url, "UTF-8");
        if (mode == 0) return "https://proxy.cors.sh/" + url; // BUILD2SA5S: tested HTML + binary ZIP relay.
        if (mode == 1) return "https://api.allorigins.win/raw?url=" + enc;
        if (mode == 2) return "https://corsproxy.io/?url=" + enc;
        return "https://r.jina.ai/" + url; // Reader fallback: pages only, not binary downloads.
    }

    private FetchResult fetchViaProviderRelay(String url, int maxBytes, boolean allowReader, String reason) throws IOException {
        IOException last = null;
        int attempts = allowReader ? 4 : 3;
        for (int i = 0; i < attempts; i++) {
            String relay = providerRelayUrl(url, i);
            HttpURLConnection c = null;
            try {
                c = (HttpURLConnection) new URL(relay).openConnection();
                c.setInstanceFollowRedirects(true);
                c.setConnectTimeout(18000);
                c.setReadTimeout(50000);
                try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
                try { c.setRequestProperty("Accept", "*/*"); } catch (Throwable ignored) {}
                if (i == 0) {
                    try { c.setRequestProperty("Origin", "https://atarihelp.eu"); } catch (Throwable ignored) {}
                    try { c.setRequestProperty("Referer", "https://atarihelp.eu/"); } catch (Throwable ignored) {}
                }
                int code = c.getResponseCode();
                if (code < 200 || code >= 400) throw new IOException("relay HTTP " + code + " via=" + relay);
                FetchResult out = new FetchResult();
                out.data = readStreamLimited(c.getInputStream(), maxBytes);
                out.contentType = c.getContentType();
                out.contentDisposition = c.getHeaderField("Content-Disposition");
                out.via = relay;
                out.relayMode = i;
                if (out.data == null || out.data.length == 0) throw new IOException("relay empty via=" + relay);
                appendNativeLog("BUILD2SA5S PROVIDER_RELAY_OK reason=" + reason + " mode=" + i + " bytes=" + out.data.length + " via=" + compactUrl(relay) + " target=" + compactUrl(url));
                return out;
            } catch (IOException ex) {
                last = ex;
                appendNativeLog("BUILD2SA5S PROVIDER_RELAY_FAIL reason=" + reason + " try=" + i + " err=" + safeMsg(ex) + " target=" + compactUrl(url));
            } finally {
                try { if (c != null) c.disconnect(); } catch (Throwable ignored) {}
            }
        }
        throw last == null ? new IOException("relay failed") : last;
    }

    private FetchResult fetchUrlBytes(String url, int maxBytes, String reason, boolean allowReader) throws IOException {
        if (isProviderBlockedUrl(url)) return fetchViaProviderRelay(url, maxBytes, allowReader, reason);
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setInstanceFollowRedirects(true);
        configureGameHttpConnection(c, url);
        c.connect();
        int code = c.getResponseCode();
        if (code < 200 || code >= 400) throw new IOException("HTTP " + code + " " + c.getResponseMessage());
        FetchResult out = new FetchResult();
        out.data = readStreamLimited(c.getInputStream(), maxBytes);
        out.contentType = c.getContentType();
        out.contentDisposition = c.getHeaderField("Content-Disposition");
        out.via = url;
        try { c.disconnect(); } catch (Throwable ignored) {}
        return out;
    }

    private byte[] readStreamLimited(InputStream in, int maxBytes) throws IOException {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[16384];
        int n;
        while ((n = in.read(buf)) > 0 && bos.size() < maxBytes) {
            int take = Math.min(n, maxBytes - bos.size());
            if (take > 0) bos.write(buf, 0, take);
            if (take < n) break;
        }
        try { in.close(); } catch (Throwable ignored) {}
        return bos.toByteArray();
    }

    private String responseMime(String url, String contentType, boolean mainFrame) {
        if (contentType != null) {
            int semi = contentType.indexOf(';');
            String mime = (semi >= 0 ? contentType.substring(0, semi) : contentType).trim();
            if (mime.length() > 0) return mime;
        }
        String guess = null;
        try { guess = java.net.URLConnection.guessContentTypeFromName(Uri.parse(url).getPath()); } catch (Throwable ignored) {}
        if (guess != null && guess.length() > 0) return guess;
        return mainFrame ? "text/html" : "application/octet-stream";
    }

    private WebResourceResponse interceptProviderBlockedResource(String url, boolean mainFrame) {
        if (!isProviderBlockedUrl(url)) return null;
        try {
            FetchResult r = fetchViaProviderRelay(url, mainFrame ? 4 * 1024 * 1024 : 12 * 1024 * 1024, mainFrame, mainFrame ? "webMain" : "webResource");
            if (mainFrame && r.relayMode == 3) {
                byte[] html = renderReaderRelayHtml(url, r.data);
                return new WebResourceResponse("text/html", "UTF-8", new java.io.ByteArrayInputStream(html));
            }
            String mime = responseMime(url, r.contentType, mainFrame);
            String enc = (mime.startsWith("text/") || mime.contains("javascript") || mime.contains("json") || mime.contains("xml")) ? "UTF-8" : null;
            return new WebResourceResponse(mime, enc, new java.io.ByteArrayInputStream(r.data));
        } catch (Throwable t) {
            appendNativeLog("BUILD2SA5S PROVIDER_INTERCEPT_FAIL main=" + mainFrame + " err=" + safeMsg(t) + " url=" + compactUrl(url));
            return null;
        }
    }

    private byte[] renderReaderRelayHtml(String url, byte[] data) throws IOException {
        String md = new String(data, "UTF-8");
        StringBuilder b = new StringBuilder();
        b.append("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>")
                .append("<style>body{background:#fff;color:#111;font:16px/1.45 sans-serif;margin:0;padding:18px}a{color:#0645ad;font-weight:700;text-decoration:none}.top{border-left:4px solid #111;background:#f2f2f2;padding:10px 12px;margin:0 0 14px}h1{font-size:22px;margin:8px 0 14px}.line{margin:8px 0}.bullet{padding-left:18px}.mono{font-family:monospace;white-space:pre-wrap}</style>")
                .append("</head><body><div class='top'><b>AtariHelp relay reader</b><br>Provider cesta spadla, tahle stranka jde pres cteci relay. ZIP odkazy zustavaji klikatelne pro emulator.</div>");
        b.append("<div class='line'><b>URL:</b> <span class='mono'>").append(escapeHtml(url)).append("</span></div>");
        String[] lines = md.split("\\r?\\n");
        boolean inBody = false;
        for (String raw : lines) {
            String line = raw == null ? "" : raw;
            String trim = line.trim();
            if (trim.length() == 0) continue;
            if (trim.equals("Markdown Content:")) { inBody = true; continue; }
            if (trim.startsWith("Title: ")) {
                b.append("<h1>").append(escapeHtml(trim.substring(7))).append("</h1>");
                continue;
            }
            if (trim.startsWith("URL Source:")) continue;
            if (trim.startsWith("URL Source")) continue;
            if (!inBody && trim.startsWith("Markdown Content")) continue;
            if (trim.startsWith("*")) {
                b.append("<div class='line bullet'>").append(markdownLinksToHtml(trim.replaceFirst("^\\*+\\s*", ""))).append("</div>");
            } else if (trim.startsWith("#")) {
                b.append("<h1>").append(markdownLinksToHtml(trim.replaceFirst("^#+\\s*", ""))).append("</h1>");
            } else {
                b.append("<div class='line'>").append(markdownLinksToHtml(trim)).append("</div>");
            }
        }
        b.append("</body></html>");
        return b.toString().getBytes("UTF-8");
    }

    private String markdownLinksToHtml(String s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder();
        int pos = 0;
        while (pos < s.length()) {
            int a = s.indexOf('[', pos);
            if (a < 0) {
                out.append(linkifyPlainUrls(escapeHtml(s.substring(pos))));
                break;
            }
            int b = s.indexOf("](", a);
            int c = b < 0 ? -1 : s.indexOf(')', b + 2);
            if (b < 0 || c < 0) {
                out.append(linkifyPlainUrls(escapeHtml(s.substring(pos))));
                break;
            }
            out.append(linkifyPlainUrls(escapeHtml(s.substring(pos, a))));
            String label = s.substring(a + 1, b);
            String href = s.substring(b + 2, c);
            out.append("<a href='").append(escapeHtml(href)).append("'>").append(escapeHtml(label)).append("</a>");
            pos = c + 1;
        }
        return out.toString();
    }

    private String linkifyPlainUrls(String escaped) {
        if (escaped == null || escaped.indexOf("http") < 0) return escaped == null ? "" : escaped;
        StringBuilder out = new StringBuilder();
        int pos = 0;
        while (pos < escaped.length()) {
            int h1 = escaped.indexOf("https://", pos);
            int h2 = escaped.indexOf("http://", pos);
            int h;
            if (h1 < 0) h = h2;
            else if (h2 < 0) h = h1;
            else h = Math.min(h1, h2);
            if (h < 0) {
                out.append(escaped.substring(pos));
                break;
            }
            out.append(escaped.substring(pos, h));
            int e = h;
            while (e < escaped.length()) {
                char ch = escaped.charAt(e);
                if (ch <= ' ' || ch == '&' || ch == '<' || ch == '>' || ch == '\'' || ch == '"') break;
                e++;
            }
            String href = escaped.substring(h, e);
            out.append("<a href='").append(href).append("'>").append(href).append("</a>");
            pos = e;
        }
        return out.toString();
    }

    private void showAtariHelpProviderBridge(final String url, final String reason, final String detail) {
        Runnable r = () -> {
            if (web == null) return;
            applyWebViewVisualMode(url, "providerBridge");
            String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    + "<style>body{background:#fff;color:#111;font-family:sans-serif;padding:22px;line-height:1.45}h1{font-size:22px;margin:0 0 12px}.box{padding:12px;border-left:4px solid #111;background:#f3f3f3;margin:12px 0}.warn{border-left-color:#b80;background:#fff7e6}.btn{display:block;margin:10px 0;padding:12px 14px;border:1px solid #111;border-radius:6px;background:#111;color:#fff;text-decoration:none;text-align:center;font-weight:700}.alt{background:#fff;color:#111}input{box-sizing:border-box;width:100%;padding:12px;border:1px solid #777;border-radius:6px;font:16px sans-serif}code{word-break:break-all}</style></head>"
                    + "<body><h1>AtariHelp lokalni bridge</h1>"
                    + "<div class='box warn'>Provider blokuje nebo zavira spojeni na AtariHelp/WEDOS. Appka proto tuhle domenu automaticky nenacita a neopakuje pozadavky.</div>"
                    + "<p><b>Puvodni URL:</b> <code>" + escapeHtml(url) + "</code></p>"
                    + "<p><b>Duvod:</b> <code>" + escapeHtml(reason) + " / " + escapeHtml(detail) + "</code></p>"
                    + "<a class='btn' href='#' onclick='try{AHPICK.pickGame();}catch(e){}return false;'>Vybrat hru z telefonu</a>"
                    + "<a class='btn alt' href='file:///android_asset/index.html'>Zpet do aplikace</a>"
                    + "<div class='box'><b>Primy neblokovany odkaz</b><br><p>Sem jde vlozit odkaz na ZIP/XEX/ATR/GEN z jineho hostingu nebo mirroru.</p>"
                    + "<input id='u' placeholder='https://mirror.example/hra.zip'>"
                    + "<a class='btn' href='#' onclick='var u=document.getElementById(\"u\").value;if(u){try{AHNET.runGameUrl(u);}catch(e){location.href=u;}}return false;'>Spustit z odkazu</a></div>"
                    + "<p>Tahle obrazovka je uvnitr APK, takze neni zavisla na providerovi ani na tom, jestli AtariHelp/WEDOS zrovna projde.</p>"
                    + "</body></html>";
            try { web.loadDataWithBaseURL("file:///android_asset/atarihelp_bridge.html", html, "text/html", "UTF-8", null); } catch (Throwable ignored) {}
            appendNativeLog("BUILD2SA5Q ATARIHELP_PROVIDER_BRIDGE reason=" + reason + " detail=" + detail + " url=" + compactUrl(url));
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private void showAtariHelpSafetyStop(final String url, final String reason, final long waitMs) {
        Runnable r = () -> {
            if (web == null) return;
            applyWebViewVisualMode(url, "safetyStop");
            long sec = Math.max(1L, (waitMs + 999L) / 1000L);
            String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    + "<style>body{background:#fff;color:#111;font-family:sans-serif;padding:22px;line-height:1.45}h1{font-size:22px}.btn{display:block;margin:10px 0;padding:12px 14px;border:1px solid #111;border-radius:6px;background:#111;color:#fff;text-decoration:none;text-align:center;font-weight:700}.alt{background:#fff;color:#111}.warn{padding:10px;border-left:4px solid #b80;background:#fff7e6}code{word-break:break-all}</style></head>"
                    + "<body><h1>AtariHelp ochranna brzda</h1>"
                    + "<div class='warn'>Aplikace ted neposila dalsi pozadavky na web, aby nezhorsila IP ban / rate limit.</div>"
                    + "<p><b>URL:</b> <code>" + escapeHtml(url) + "</code></p>"
                    + "<p><b>Duvod:</b> <code>" + escapeHtml(reason) + "</code></p>"
                    + "<p>Zkusit znovu nejdrive za " + sec + " s.</p>"
                    + "<a class='btn alt' href='#' onclick='try{AHPICK.pickGame();}catch(e){}return false;'>Vybrat ZIP z telefonu</a>"
                    + "</body></html>";
            try { web.loadDataWithBaseURL("about:blank", html, "text/html", "UTF-8", null); } catch (Throwable ignored) {}
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private void showAtariHelpLoadError(String url, String detail) {
        if (!isProviderBlockedUrl(url) || web == null) return;
        showAtariHelpProviderBridge(url, "loadError", detail);
    }

    private void configureGameHttpConnection(HttpURLConnection c, String url) {
        if (c == null) return;
        try { c.setConnectTimeout(20000); } catch (Throwable ignored) {}
        try { c.setReadTimeout(45000); } catch (Throwable ignored) {}
        if (isAtariHelpUrl(url)) {
            try { c.setRequestProperty("User-Agent", ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,application/zip,application/octet-stream,*/*;q=0.8"); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Accept-Language", "cs-CZ,cs;q=0.9,en;q=0.8"); } catch (Throwable ignored) {}
            try { c.setRequestProperty("Referer", "https://atarihelp.eu/?page_id=207"); } catch (Throwable ignored) {}
        }
    }

    private String escapeHtml(String s) {
        if (s == null) return "";
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;").replace("'", "&#39;");
    }

    private void stopNativeIfLeavingSega(String url, String source) {
        if (!nativeInPlaceEnabled && !nativeCoreAudioRun && nativeInPlaceView == null) return;
        if (isSegaNativeOwnerUrl(url)) return;
        stopNativeInPlaceHard(source + ":" + compactUrl(url));
    }

    private boolean isPs1OwnerUrl(String url) {
        if (url == null) return false;
        String u = url.toLowerCase(Locale.US);
        return u.startsWith("file:///android_asset/emu_ps1/index.html") || u.startsWith("file:///android_asset/emu_ps1/");
    }

    private void stopPs1IfLeaving(String url, String source) {
        if (!ps1BootActive && !ps1SessionActive && ps1CurrentAudioTrack == null && ps1AudioThread == null) return;
        if (isPs1OwnerUrl(url)) return;
        stopPs1SessionHard(source + ":" + compactUrl(url));
    }

    private synchronized String stopPs1SessionHard(String reason) {
        boolean hadSession = ps1BootActive || ps1SessionActive || ps1CurrentAudioTrack != null || ps1AudioThread != null;
        if (!hadSession) return "PS1_ALREADY_STOPPED";
        ps1LifecycleGen++;
        ps1BootActive = false;
        ps1SessionActive = false;
        stopPs1Audio();
        String r;
        try {
            r = NativePs1CoreBridge.stopSafe();
        } catch (Throwable t) {
            r = "PS1_STOP_EXCEPTION " + safeMsg(t);
        }
        closePs1GamePfdQuietly();
        ps1LastBootResult = "PS1_STOPPED " + reason;
        appendNativeLog("BUILD2SA5I PS1_SESSION_STOP reason=" + reason + " core=" + (r == null ? "null" : r.replace('\n', ' ')));
        return r;
    }

    private void closePs1GamePfdQuietly() {
        synchronized (MainActivity.class) {
            try {
                if (ps1GamePfd != null) {
                    ps1GamePfd.close();
                    ps1GamePfd = null;
                }
            } catch (Throwable ignored) {}
        }
    }

    private boolean isUiThread() {
        return Looper.myLooper() == Looper.getMainLooper();
    }

    private void removeNativeViewOnUi(final String reason) {
        final NativeInPlaceView old = nativeInPlaceView;
        nativeInPlaceView = null;
        Runnable r = () -> {
            try {
                if (old != null) {
                    try { old.stop(); } catch (Throwable ignored) {}
                    try { old.setVisibility(View.GONE); } catch (Throwable ignored) {}
                    try { if (old.getParent() instanceof ViewGroup) ((ViewGroup) old.getParent()).removeView(old); } catch (Throwable t) { appendNativeLog("NATIVE_VIEW_REMOVE_UI_ERR_RV " + safeMsg(t)); }
                }
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_OK_RV reason=" + reason);
            } catch (Throwable t) {
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_FAIL_RV reason=" + reason + " " + safeMsg(t));
            }
        };
        if (isUiThread()) r.run(); else ui.post(r);
    }

    private synchronized String stopNativeInPlaceHard(String reason) {
        if (!nativeInPlaceEnabled && !nativeCoreAudioRun && nativeInPlaceView == null) {
            nativeLastStatus = "NATIVE_ALREADY_OFF " + reason;
            return nativeLastStatus;
        }
        StringBuilder res = new StringBuilder();
        try { appendNativeLog("NATIVE_LIFECYCLE_STOP_BEGIN reason=" + reason); } catch (Throwable ignored) {}
        nativeRomLoadGeneration++; // BUILD2RV: cancel stale delayed audio/render watchdogs when leaving Sega/Atari/VBXE.
        nativeInPlaceEnabled = false;
        try { stopNativeCoreAudioStream(); nativeCurrentAudioTrack = null; res.append("audioStop=OK "); } catch (Throwable t) { res.append("audioStop=ERR:").append(safeMsg(t)).append(' '); }
        try {
            for (int i = 0; i < 8; i++) NativeSegaCoreBridge.setInput(i, false);
        } catch (Throwable ignored) {}
        try {
            String nativeStop = NativeSegaCoreBridge.shutdown();
            res.append("nativeStop=").append(nativeStop == null ? "null" : nativeStop.replace('\n', ' ')).append(' ');
        } catch (Throwable t) { res.append("nativeStop=ERR:").append(safeMsg(t)).append(' '); }
        try {
            removeNativeViewOnUi(reason);
            res.append("viewRemovePostedUI=YES");
        } catch (Throwable t) { res.append("viewRemovePostedUI=ERR:").append(safeMsg(t)); }
        nativeLastStatus = "NATIVE_OFF_LIFECYCLE_STOP " + reason + " " + res;
        appendNativeLog(nativeLastStatus);
        return nativeLastStatus;
    }

    public class AHNative {
        @JavascriptInterface
        public String enableInPlace() {
            try {
                final String build = NativeSegaCoreBridge.buildString();
                nativeLastStatus = "NATIVE_IN_PLACE_OK " + build.replace('\n', ' ');
                appendNativeLog("ENABLE_IN_PLACE " + nativeLastStatus);
                ui.post(() -> {
                    try {
                        if (rootFrame == null || web == null) return;
                        // BUILD2RV: enableInPlace is now idempotent. Rotation/resize/visibility on S8 called it many times;
                        // recreating TextureView each time slowly killed mobile video. Reuse the existing native view when it is alive.
                        if (nativeInPlaceView != null && nativeInPlaceView.getParent() == rootFrame) {
                            nativeInPlaceEnabled = true;
                            try { nativeInPlaceView.setAlpha(1f); nativeInPlaceView.setTranslationX(0f); nativeInPlaceView.setTranslationY(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.start(); nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_VIEW_REUSE_RV parent=rootFrame noRecreate=YES");
                            return;
                        }
                        if (nativeInPlaceView != null) {
                            try { nativeInPlaceView.stop(); } catch (Throwable ignored) {}
                            try { if (nativeInPlaceView.getParent() != null) ((ViewGroup) nativeInPlaceView.getParent()).removeView(nativeInPlaceView); } catch (Throwable ignored) {}
                            nativeInPlaceView = null;
                            appendNativeLog("NATIVE_VIEW_RECREATE_RV oldViewRemoved=YES detachedOrBadParent");
                        }
                        nativeInPlaceView = new NativeInPlaceView(MainActivity.this);
                        nativeInPlaceView.setClickable(false);
                        nativeInPlaceView.setEnabled(false);
                        nativeInPlaceView.setFocusable(false);
                        nativeInPlaceView.setFocusableInTouchMode(false);
                        // BUILD2RV: never show native view before JS sends a valid monitor/landscape rect.
                        nativeInPlaceView.setVisibility(View.INVISIBLE);
                        try { nativeInPlaceView.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
                        rootFrame.addView(nativeInPlaceView, new FrameLayout.LayoutParams(1, 1));
                        appendNativeLog("NATIVE_VIEW_REATTACH_RV parent=rootFrame freshView=YES hiddenUntilValidRect=YES");
                        nativeInPlaceEnabled = true;
                        nativeInPlaceView.setAlpha(1f);
                        nativeInPlaceView.setTranslationX(0f);
                        nativeInPlaceView.setTranslationY(0f);
                    } catch (Throwable t) {
                        nativeLastStatus = "NATIVE_VIEW_ERROR " + safeMsg(t);
                        appendNativeLog(nativeLastStatus);
                    }
                });
                return nativeLastStatus;
            } catch (Throwable t) {
                nativeLastStatus = "NATIVE_ENABLE_ERROR " + safeMsg(t);
                appendNativeLog(nativeLastStatus);
                return nativeLastStatus;
            }
        }

        @JavascriptInterface
        public String setNativeRect(final int x, final int y, final int w, final int h) {
            try {
                ui.post(() -> {
                    try {
                        if (nativeInPlaceView == null || rootFrame == null) return;
                        // BUILD2RV: Samsung/WebView can send a transient 0x0 rect when returning from landscape.
                        // Ignoring that rect prevents the native video from becoming black after portrait return.
                        if (w < 120 || h < 80) { appendNativeLog("SET_RECT_SKIP_SMALL_RV x=" + x + " y=" + y + " w=" + w + " h=" + h); return; }
                        int ww = Math.max(120, w);
                        int hh = Math.max(80, h);
                        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(ww, hh);
                        lp.leftMargin = Math.max(0, x);
                        lp.topMargin = Math.max(0, y);
                        nativeInPlaceView.setLayoutParams(lp);
                        nativeInPlaceView.setVisibility(nativeInPlaceEnabled ? View.VISIBLE : View.GONE);
                        if (nativeInPlaceEnabled) nativeInPlaceView.start();
                        boolean landscapeFull = ww > (hh * 1.12f) && x <= 4 && y <= 4;
                        nativeLandscapeFullVideo = landscapeFull;
                        if (landscapeFull && web != null) {
                            // BUILD2RV: on real phones the C++ native view is the video layer, WebView is only transparent controls/log.
                            // This gives full-screen Sonic/Aladdin with transparent joystick/buttons over the picture.
                            try { nativeInPlaceView.setZ(0f); } catch (Throwable ignored) {}
                            try { web.setZ(10f); } catch (Throwable ignored) {}
                            try { rootFrame.bringChildToFront(web); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_Z_ORDER_RV landscapeWebControlsOverNative=YES topLabelsHidden=YES");
                        } else {
                            try { if (web != null) web.setZ(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.setZ(20f); } catch (Throwable ignored) {}
                            nativeInPlaceView.bringToFront();
                        }
                        nativeInPlaceView.requestLayout();
                        nativeInPlaceView.invalidate();
                        appendNativeLog("SET_RECT_OK_RV x=" + Math.max(0, x) + " y=" + Math.max(0, y) + " w=" + ww + " h=" + hh + " visible=" + nativeInPlaceEnabled + " landscapeFull=" + landscapeFull);
                    } catch (Throwable t) { appendNativeLog("SET_RECT_ERROR " + safeMsg(t)); }
                });
                return "RECT_OK x=" + x + " y=" + y + " w=" + w + " h=" + h;
            } catch (Throwable t) {
                return "RECT_ERROR " + safeMsg(t);
            }
        }

        @JavascriptInterface
        public String loadRomBase64(String name, String b64) {
            try {
                if (b64 == null) b64 = "";
                String clean = b64.replaceAll("\\s", "");
                long decodeStart = System.currentTimeMillis();
                byte[] data = Base64.decode(clean, Base64.DEFAULT);
                long decodeMs = System.currentTimeMillis() - decodeStart;
                long t0 = System.currentTimeMillis();
                String info = NativeSegaCoreBridge.romInfo(data);
                long dt = System.currentTimeMillis() - t0;
                try {
                    String rt = NativeSegaCoreBridge.configureRuntime(Build.VERSION.SDK_INT, Runtime.getRuntime().availableProcessors(), Build.MODEL == null ? "" : Build.MODEL);
                    appendNativeLog("NATIVE_RUNTIME_CONFIG_RV " + rt);
                } catch (Throwable rtErr) {
                    appendNativeLog("NATIVE_RUNTIME_CONFIG_RV_ERROR " + safeMsg(rtErr));
                }

                // BUILD2RV: audio starts only after visible native frame; zeroed balanced audio clock/music timing lives in C++ core.
                // Tvrdý fresh start pred kazdou ROM brani stavu: Atari 130XE -> Sega -> nova ROM -> cerna obrazovka + zvuk.
                final int loadGen = ++nativeRomLoadGeneration;
                appendNativeLog("FRESH_ROM_GENERATION_RV gen=" + loadGen + " oldAudioGen=" + nativeAudioGeneration + " oldDraw=" + nativeViewDrawCounter);
                stopNativeCoreAudioStream();
                try { NativeSegaCoreBridge.shutdown(); appendNativeLog("FRESH_ROM_HARD_STOP_BEFORE_LOAD_RV gen=" + loadGen); } catch (Throwable ignored) {}
                nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                nativeRenderPerfWindowStartMs = 0; nativeRenderPerfWindowFrames = 0; nativeRenderPerfSlowFrames = 0;
                String realCore = NativeSegaCoreBridge.realCoreLoadRom(data);

                nativeLastRomInfo = "ROM: " + safeFileName(name) + "\n" + info + "\n\nREAL CORE SLOT:\n" + realCore;
                nativeLastStatus = "ROM_REAL_CORE_LOAD_READY bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt;
                appendNativeLog("ROM_REAL_CORE_LOAD_READY name=" + safeFileName(name) + " bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt + " gen=" + loadGen);
                appendNativeLog("REAL_CORE_RENDER_ACTIVE_RV after ROM load audio=WAIT_FRAME_AND_VIEW_DRAW");
                forceNativeViewRedrawBurst("afterRomLoad_RV");
                scheduleNativeAudioAfterFrameAndViewDraw(name, data, loadGen, 1);
                scheduleNativeRenderWatchdog(name, data, loadGen, 1);
                schedulePassiveAuditRW(loadGen, 1); // BUILD2RW: passive 10s audit rows, measure only
                appendNativeLog("PASSIVE_AUDIT_RX_START gen=" + loadGen + " intervalMs=10000 changesNothing=YES");
                return nativeLastStatus + "\n" + info + "\n\nREAL CORE SLOT:\n" + realCore;
            } catch (Throwable t) {
                nativeLastStatus = "ROM_TO_CPP_ERROR " + safeMsg(t);
                appendNativeLog(nativeLastStatus);
                return nativeLastStatus;
            }
        }

        @JavascriptInterface
        public String coreStep() {
            try {
                appendNativeLog("CORE_STEP_REQUEST");
                String res = NativeSegaCoreBridge.realCoreStep();
                nativeLastStatus = res == null ? "CORE_STEP_NULL" : res;
                appendNativeLog("CORE_STEP_RESULT " + nativeLastStatus.replace('\n', ' '));
                return nativeLastStatus;
            } catch (Throwable t) {
                String e = "CORE_STEP_ERROR " + safeMsg(t);
                nativeLastStatus = e;
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String input(String btn, boolean down) {
            try {
                int key = mapNativeKey(btn);
                if (key < 0) return "INPUT_UNKNOWN " + btn;
                NativeSegaCoreBridge.setInput(key, down);
                nativeInputEvents++;
                String st = NativeSegaCoreBridge.inputStatus();
                if (nativeInputEvents <= 16 || nativeInputEvents % 20 == 0) appendNativeLog("INPUT " + btn + " " + (down ? "DOWN" : "UP") + " count=" + nativeInputEvents + " " + st);
                return st;
            } catch (Throwable t) {
                String e = "INPUT_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String disableInPlace(String reason) {
            return stopNativeInPlaceHard(reason == null ? "js" : reason);
        }

        @JavascriptInterface
        public String status() {
            try { return NativeSegaCoreBridge.realCoreStatus() + "\nviewDrawCounter=" + nativeViewDrawCounter + "\nviewEnabled=" + nativeInPlaceEnabled + "\nperfMode=" + nativePerformanceMode; }
            catch (Throwable t) { return "STATUS_ERROR " + safeMsg(t); }
        }


        @JavascriptInterface
        public String setPerformanceMode(String mode) {
            try {
                String m = mode == null ? "HIGH" : mode.trim().toUpperCase(Locale.US);
                if (!m.startsWith("LOW")) m = "HIGH"; else m = "LOW";
                nativePerformanceMode = m;
                String nativePerf = "nativePerf=not-called";
                try { NativeSegaCoreBridge.configureRuntime(Build.VERSION.SDK_INT, Runtime.getRuntime().availableProcessors(), Build.MODEL == null ? "" : Build.MODEL); } catch (Throwable ignored) {}
                try { nativePerf = NativeSegaCoreBridge.setPerformanceMode(m); } catch (Throwable nt) { nativePerf = "nativePerfError=" + safeMsg(nt); }
                appendNativeLog("NATIVE_PERF_MODE_RV mode=" + nativePerformanceMode + " sdk=" + Build.VERSION.SDK_INT + " " + nativePerf);
                if (nativeInPlaceView != null) {
                    try { nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                }
                return "NATIVE_PERF_MODE_OK_RV mode=" + nativePerformanceMode + " " + nativePerf;
            } catch (Throwable t) {
                return "NATIVE_PERF_MODE_ERROR_RV " + safeMsg(t);
            }
        }

        @JavascriptInterface
        public String audioTest() {
            try {
                int sampleRate = 48000;
                int samples = 16000;
                short[] pcm = new short[samples];
                NativeSegaCoreBridge.makeAudioTone(pcm, sampleRate, 440.0);
                AudioTrack track;
                if (Build.VERSION.SDK_INT >= 21) {
                    track = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
                            .setBufferSizeInBytes(samples * 2)
                            .setTransferMode(AudioTrack.MODE_STATIC)
                            .build();
                } else {
                    track = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, samples * 2, AudioTrack.MODE_STATIC);
                }
                int written = track.write(pcm, 0, pcm.length);
                track.play();
                appendNativeLog("AUDIO_TEST_PLAY sampleRate=" + sampleRate + " written=" + written + " state=" + track.getPlayState());
                ui.postDelayed(() -> { try { track.stop(); track.release(); } catch (Throwable ignored) {} }, 850);
                return "AUDIO_TEST_OK sampleRate=" + sampleRate + " written=" + written;
            } catch (Throwable t) {
                String e = "AUDIO_TEST_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }

        @JavascriptInterface
        public String saveLog() {
            try {
                long now = System.currentTimeMillis();
                if (now - nativeLastSaveLogAtMs < 2200) {
                    appendNativeLog("SAVE_LOG_DEDUP_RV ignored deltaMs=" + (now - nativeLastSaveLogAtMs));
                    return "SAVE_LOG_DEDUP_RV";
                }
                nativeLastSaveLogAtMs = now;
                String fn = "AtariHelp_SEGA_CPP_INPLACE_LOG_BUILD2SA2_" + new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(new Date()) + ".txt";
                String path = writeBytesToDownloads(fn, buildNativeInPlaceLog().getBytes("UTF-8"));
                appendNativeLog("SAVE_LOG_OK " + path);
                return "SAVE_LOG_OK " + path;
            } catch (Throwable t) {
                String e = "SAVE_LOG_ERROR " + safeMsg(t);
                appendNativeLog(e);
                return e;
            }
        }
    }

    private String safeMsg(Throwable t) { return t == null ? "null" : (t.getMessage() == null ? t.toString() : t.getMessage()); }

    private void forceNativeViewRedrawBurst(final String reason) {
        ui.post(() -> {
            try {
                if (nativeInPlaceView != null) {
                    nativeInPlaceView.setVisibility(nativeInPlaceEnabled ? View.VISIBLE : View.GONE);
                    if (!nativeLandscapeFullVideo) nativeInPlaceView.bringToFront();
                    nativeInPlaceView.start();
                    nativeInPlaceView.forceRedrawOnce();
                    appendNativeLog("NATIVE_TEXTURE_REDRAW_BURST " + reason);
                }
            } catch (Throwable t) { appendNativeLog("NATIVE_VIEW_REDRAW_BURST_ERROR " + safeMsg(t)); }
        });
        // BUILD2RV: keep only a short burst; long delayed bursts from old ROMs added work after multiple ROM loads.
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 120);
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 420);
    }

    private void scheduleNativeAudioAfterFrameAndViewDraw(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 220 : 320;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_AUDIO_WAIT_CANCELLED_RV staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RV attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(430, st.length()))));
                if (hasFrame && viewReady) {
                    startNativeCoreAudioStream();
                    appendNativeLog("NATIVE_AUDIO_START_AFTER_FRAME_VIEW_RV name=" + safeFileName(romName) + " gen=" + gen + " audioProfile=QT_RR_RECOVERY_SAFE_AUDIT");
                    return;
                }
                if (attempt < 10 && nativeInPlaceEnabled) {
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, attempt + 1);
                    return;
                }
                stopNativeCoreAudioStream();
                nativeLastStatus = "NATIVE_AUDIO_NOT_STARTED_NO_VISIBLE_FRAME_RV name=" + safeFileName(romName);
                appendNativeLog(nativeLastStatus);
                forceNativeViewRedrawBurst("audioNoVisibleFrame_RV");
            } catch (Throwable t) {
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RV_ERROR " + safeMsg(t));
            }
        }, delay);
    }

    // BUILD2RX passive: battery temperature (sticky intent, no permission) + current CPU freq.
    // RW data showed S8 degradation with FLAT heap/GC => prime suspect is thermal throttling.
    // These two read-only values will prove or kill that theory.
    private String readBatteryTempC() {
        try {
            Intent i = registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            if (i == null) return "na";
            int t = i.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1);
            if (t < 0) return "na";
            return String.format(Locale.US, "%.1f", t / 10.0);
        } catch (Throwable t) { return "err"; }
    }
    private String readCpuFreqKHz(int cpu) {
        try {
            File f = new File("/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_cur_freq");
            if (!f.canRead()) return "na";
            java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(f));
            String line = br.readLine();
            br.close();
            return line == null ? "na" : line.trim();
        } catch (Throwable t) { return "na"; }
    }
    private String pickField(String flat, String key) {
        try {
            int i = flat.indexOf(key);
            if (i < 0) return "na";
            int end = flat.indexOf(' ', i);
            if (end < 0) end = Math.min(flat.length(), i + 40);
            return flat.substring(i + key.length(), end);
        } catch (Throwable t) { return "na"; }
    }

    // BUILD2RW PASSIVE AUDIT: every 10s log one row with Java heap, native heap, GC, AudioTrack
    // underruns, generations and the C++ audit block. It only reads and logs; it never changes
    // region, FIFO, clocks, gain or render. Goal: after 10-15 min on S8 the log itself shows
    // WHAT grows (heap? FIFO backlog? underruns? pull cost?) before we touch any fix.
    private void schedulePassiveAuditRW(final int gen, final int tick) {
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration || !nativeInPlaceEnabled) {
                    appendNativeLog("PASSIVE_AUDIT_RX_STOP gen=" + gen + " current=" + nativeRomLoadGeneration + " enabled=" + nativeInPlaceEnabled);
                    return;
                }
                Runtime rt = Runtime.getRuntime();
                long jUsedKb = (rt.totalMemory() - rt.freeMemory()) / 1024;
                long jTotalKb = rt.totalMemory() / 1024;
                long jMaxKb = rt.maxMemory() / 1024;
                long nAllocKb = 0, nTotalKb = 0;
                try { nAllocKb = Debug.getNativeHeapAllocatedSize() / 1024; nTotalKb = Debug.getNativeHeapSize() / 1024; } catch (Throwable ignored) {}
                String gcCount = "na", gcTime = "na";
                try {
                    if (Build.VERSION.SDK_INT >= 23) {
                        gcCount = Debug.getRuntimeStat("art.gc.gc-count");
                        gcTime = Debug.getRuntimeStat("art.gc.gc-time");
                    }
                } catch (Throwable ignored) {}
                int trackUnderruns = -1;
                try {
                    AudioTrack at = nativeCurrentAudioTrack;
                    if (at != null && Build.VERSION.SDK_INT >= 24) trackUnderruns = at.getUnderrunCount();
                } catch (Throwable ignored) {}
                String st;
                try { st = NativeSegaCoreBridge.realCoreStatus(); } catch (Throwable t) { st = "coreStatusErr=" + safeMsg(t); }
                String flat = st == null ? "null" : st.replace('\n', ' ');
                // BUILD2RX: RW truncation cut coreAvgMs off the row; extract the key fields explicitly.
                String coreAvg = pickField(flat, "coreAvgMs=");
                String coreMax = pickField(flat, "coreMaxMs=");
                String stress = pickField(flat, "sceneStress=");
                String backlog = pickField(flat, "audioBacklog=");
                appendNativeLog("PASSIVE_AUDIT_RX tick=" + tick + " gen=" + gen
                        + " upMin=" + ((tick * 10) / 60) + "." + String.format(Locale.US, "%02d", (tick * 10) % 60)
                        + " batteryTempC=" + readBatteryTempC()
                        + " cpu0KHz=" + readCpuFreqKHz(0) + " cpu4KHz=" + readCpuFreqKHz(4)
                        + " coreAvgMs=" + coreAvg + " coreMaxMs=" + coreMax + " sceneStress=" + stress + " audioBacklog=" + backlog
                        + " javaHeapUsedKB=" + jUsedKb + " javaHeapTotalKB=" + jTotalKb + " javaHeapMaxKB=" + jMaxKb
                        + " nativeHeapAllocKB=" + nAllocKb + " nativeHeapTotalKB=" + nTotalKb
                        + " gcCount=" + gcCount + " gcTimeMs=" + gcTime
                        + " audioTrackUnderruns=" + trackUnderruns
                        + " activeAudioTracks=" + nativeActiveAudioTracks
                        + " audioGen=" + nativeAudioGeneration + " romGen=" + nativeRomLoadGeneration
                        + " drawCounter=" + nativeViewDrawCounter
                        + " lastRenderCostMs=" + (nativeLastRenderCostNs / 1000000.0)
                        + " perfMode=" + nativePerformanceMode
                        + " core=" + flat.substring(0, Math.min(1400, flat.length())));
                schedulePassiveAuditRW(gen, tick + 1);
            } catch (Throwable t) {
                appendNativeLog("PASSIVE_AUDIT_RX_ERROR " + safeMsg(t));
            }
        }, 10000);
    }

    private void scheduleNativeRenderWatchdog(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 1350 : 2600;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_RENDER_WATCHDOG_CANCELLED_RV staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RV attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(520, st.length()))));
                if (hasFrame && viewReady) {
                    forceNativeViewRedrawBurst("watchdogFrameViewOK_RV");
                    return;
                }
                if (attempt <= 1 && romData != null && romData.length > 0 && nativeInPlaceEnabled) {
                    appendNativeLog("NATIVE_RENDER_WATCHDOG_FRESH_RELOAD_RV name=" + safeFileName(romName) + " bytes=" + romData.length + " gen=" + gen);
                    try { stopNativeCoreAudioStream(); } catch (Throwable ignored) {}
                    try { NativeSegaCoreBridge.shutdown(); } catch (Throwable ignored) {}
                    nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                    String reload = NativeSegaCoreBridge.realCoreLoadRom(romData);
                    nativeLastStatus = "NATIVE_RENDER_WATCHDOG_RELOAD_RV " + (reload == null ? "null" : reload.replace('\n',' '));
                    appendNativeLog(nativeLastStatus.substring(0, Math.min(900, nativeLastStatus.length())));
                    forceNativeViewRedrawBurst("watchdogFreshReload_RV");
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, 1);
                    scheduleNativeRenderWatchdog(romName, romData, gen, attempt + 1);
                    return;
                }
                if (!hasFrame || !viewReady) {
                    stopNativeCoreAudioStream();
                    nativeLastStatus = "NATIVE_RENDER_NO_VISIBLE_FRAME_AUDIO_STOPPED_RV name=" + safeFileName(romName);
                    appendNativeLog(nativeLastStatus);
                    forceNativeViewRedrawBurst("watchdogNoVisibleFrameAudioStopped_RV");
                }
            } catch (Throwable t) {
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RV_ERROR " + safeMsg(t));
            }
        }, delay);
    }

    private class NativeInPlaceView extends TextureView implements TextureView.SurfaceTextureListener {
        private final Paint paint = new Paint();
        private int frame = 0;
        private static final int SRC_W = 320;
        private static final int SRC_H = 224;
        private int[] argb = new int[SRC_W * SRC_H];
        private Bitmap bitmap;
        private volatile boolean running = false;
        private Thread renderThread;

        NativeInPlaceView(Activity a) {
            super(a);
            try { setOpaque(true); } catch (Throwable ignored) {}
            try { paint.setFilterBitmap(false); paint.setDither(false); } catch (Throwable ignored) {}
            setSurfaceTextureListener(this);
        }

        void start() {
            running = true;
            startRenderThreadIfReady("start");
        }

        void stop() {
            running = false;
            Thread t = renderThread;
            renderThread = null;
            if (t != null && t.isAlive() && Thread.currentThread() != t) {
                try { t.interrupt(); } catch (Throwable ignored) {}
                try { t.join(220); } catch (Throwable ignored) {}
            }
        }

        void forceRedrawOnce() {
            if (!running) running = true;
            startRenderThreadIfReady("force");
        }

        private void startRenderThreadIfReady(String reason) {
            if (!isAvailable()) return;
            Thread t = renderThread;
            if (t != null && t.isAlive()) return;
            renderThread = new Thread(() -> renderLoop(reason), "AtariHelpSegaTextureRG");
            renderThread.setDaemon(true);
            try { renderThread.setPriority(Build.VERSION.SDK_INT <= 28 ? Thread.NORM_PRIORITY : Thread.NORM_PRIORITY + 1); } catch (Throwable ignored) {}
            renderThread.start();
            appendNativeLog("NATIVE_TEXTURE_THREAD_START_RV reason=" + reason + " view=" + getWidth() + "x" + getHeight());
        }

        private void renderLoop(String reason) {
            long next = System.nanoTime();
            int noSurfaceLoops = 0;
            while (running) {
                if (!isAvailable()) {
                    if (++noSurfaceLoops > 20) break;
                    try { Thread.sleep(8); } catch (Throwable ignored) {}
                    continue;
                }
                noSurfaceLoops = 0;
                long startNs = System.nanoTime();
                Canvas c = null;
                try {
                    c = lockCanvas();
                    if (c != null) drawTextureFrame(c);
                } catch (Throwable t) {
                    appendNativeLog("NATIVE_TEXTURE_RENDER_ERROR_RV " + safeMsg(t));
                } finally {
                    try { if (c != null) unlockCanvasAndPost(c); } catch (Throwable t) { appendNativeLog("NATIVE_TEXTURE_UNLOCK_ERROR_RV " + safeMsg(t)); }
                }
                long cost = System.nanoTime() - startNs;
                nativeLastRenderCostNs = cost;
                // BUILD2RV: SBIRKA performance switch. HIGH keeps full-speed render for Nox/new phones.
                // LOW intentionally lowers only the render presentation pace on S8/A12; core/audio timing stays real.
                boolean lowMode = "LOW".equals(nativePerformanceMode);
                long period = lowMode ? 33333333L : 16666667L;
                if (lowMode && Build.VERSION.SDK_INT <= 28 && cost > 22000000L) period = 40000000L;
                next += period;
                long wait = next - System.nanoTime();
                if (wait > 1000000L) {
                    try { Thread.sleep(wait / 1000000L, (int)(wait % 1000000L)); } catch (Throwable ignored) {}
                } else if (wait < -period) {
                    next = System.nanoTime();
                    try { Thread.yield(); } catch (Throwable ignored) {}
                }
            }
            appendNativeLog("NATIVE_TEXTURE_THREAD_STOP_RV reason=" + reason + " drawCounter=" + nativeViewDrawCounter + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0));
        }

        private void drawTextureFrame(Canvas canvas) {
            int w = Math.max(16, getWidth());
            int h = Math.max(16, getHeight());
            if (bitmap == null || bitmap.getWidth() != SRC_W || bitmap.getHeight() != SRC_H) {
                argb = new int[SRC_W * SRC_H];
                bitmap = Bitmap.createBitmap(SRC_W, SRC_H, Bitmap.Config.ARGB_8888);
            }
            try {
                NativeSegaCoreBridge.renderPattern(SRC_W, SRC_H, frame++, argb);
                bitmap.setPixels(argb, 0, SRC_W, 0, 0, SRC_W, SRC_H);
                canvas.drawColor(Color.BLACK);
                Rect dst;
                if (nativeLandscapeFullVideo) {
                    // BUILD2RV: never crop the 320x224 game image. User reported Sonic lives/HUD cut off in landscape.
                    // Use aspect-fit with a small bottom/top safety margin; black side bars are acceptable, missing HUD is not.
                    float safeW = (float) w;
                    float safeH = (float) h * 0.94f;
                    float scale = Math.min(safeW / (float) SRC_W, safeH / (float) SRC_H);
                    int dw = Math.max(1, Math.round(SRC_W * scale));
                    int dh = Math.max(1, Math.round(SRC_H * scale));
                    int dx = Math.max(0, (w - dw) / 2);
                    int dy = Math.max(0, (h - dh) / 2);
                    dst = new Rect(dx, dy, dx + dw, dy + dh);
                } else {
                    dst = new Rect(0, 0, w, h);
                }
                Rect src = new Rect(0, 0, SRC_W, SRC_H); // BUILD2RV: left-edge is fixed in C++ scanline boundary mapping, not by Java crop.
                canvas.drawBitmap(bitmap, src, dst, paint);
                nativeViewDrawCounter++;
                long nowMs = System.currentTimeMillis();
                if (nativeRenderPerfWindowStartMs == 0) nativeRenderPerfWindowStartMs = nowMs;
                nativeRenderPerfWindowFrames++;
                if (nativeLastRenderCostNs > 22000000L) nativeRenderPerfSlowFrames++;
                if (nowMs - nativeRenderPerfWindowStartMs >= 5000) {
                    appendNativeLog("NATIVE_RENDER_PERF_RV frames5s=" + nativeRenderPerfWindowFrames + " slow22ms=" + nativeRenderPerfSlowFrames + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0) + " android=" + Build.VERSION.SDK_INT + " perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo);
                    nativeRenderPerfWindowStartMs = nowMs;
                    nativeRenderPerfWindowFrames = 0;
                    nativeRenderPerfSlowFrames = 0;
                }
                if (nativeViewDrawCounter <= 4 || nativeViewDrawCounter % 300 == 0) {
                    appendNativeLog("NATIVE_TEXTURE_FRAME_RV count=" + nativeViewDrawCounter + " view=" + w + "x" + h + " gameDst=" + dst.toShortString() + " srcCrop=0px leftBoundaryShift=YES perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo + " costMs=" + (nativeLastRenderCostNs / 1000000.0));
                }
            } catch (Throwable t) {
                try { canvas.drawColor(Color.rgb(20, 0, 0)); } catch (Throwable ignored) {}
                appendNativeLog("NATIVE_TEXTURE_FRAME_ERROR_RV " + safeMsg(t));
            }
        }

        @Override public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) { startRenderThreadIfReady("surfaceAvailable"); }
        @Override public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) { forceRedrawOnce(); }
        @Override public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) { stop(); return true; }
        @Override public void onSurfaceTextureUpdated(SurfaceTexture surface) { }
    }

    public class AHNet {
        @JavascriptInterface
        public void openGames() {
            // BUILD2SA5J: AtariHelp sbirka musi zustat uvnitr WebView.
            // Jen tak muze klik na ZIP/XEX/GEN projit pres AHNET.runGameUrl() a rovnou spustit emu.
            ui.post(() -> {
                String url = "https://atarihelp.eu/?page_id=207";
                loadAtariHelpGuarded(url, "openGames");
            });
        }
        @JavascriptInterface
        public void openInBrowser(String url) {
            ui.post(() -> openRawExternalBrowserUrl(url));
        }
        @JavascriptInterface
        public void runGameUrl(String url) {
            ui.post(() -> {
                if (url == null || url.length() == 0) return;
                if (hasSegaExtension(url)) downloadAndRunSega(url); // BUILD2SA2
                else downloadAndRun(url);
            });
        }
    }


    // BUILD2GH: normalni WWW odkazy nesmi spadnout do emulator NET loaderu.
    // Hlavni chyba byla .com v domenach facebook.com / youtube.com:
    // stary isGameUrl bral ".com" kdekoliv v URL jako Atari COM soubor.
    private boolean isExternalBrowserUrl(String url) {
        if (url == null) return false;
        String u = url.trim().toLowerCase();
        if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
        try {
            Uri uri = Uri.parse(url);
            String host = uri.getHost();
            if (host == null) return false;
            host = host.toLowerCase();

            // Tyhle normalni weby musi jit ven pres browser Intent,
            // nikdy do downloadAndRun / BOOTANY / NAHRAJ XEX.
            if (host.equals("youtube.com") || host.equals("www.youtube.com") || host.endsWith(".youtube.com")) return true;
            if (host.equals("facebook.com") || host.equals("www.facebook.com") || host.endsWith(".facebook.com")) return true;
        } catch (Throwable ignored) {}
        return false;
    }

    private boolean openRawExternalBrowserUrl(String url) {
        if (url == null || url.trim().length() == 0) return false;
        if (isProviderBlockedUrl(url)) {
            loadAtariHelpGuarded(url, "openBrowserRelay");
            return true;
        }
        try {
            Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            i.addCategory(Intent.CATEGORY_BROWSABLE);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(i);
            return true;
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean openExternalBrowserUrl(String url) {
        if (!isExternalBrowserUrl(url)) return false;
        try { openRawExternalBrowserUrl(url); } catch (Throwable ignored) {
            // DULEZITE: i kdyz by browser Intent selhal,
            // nevracet false, jinak by to sebral emulator loader.
        }
        return true;
    }

    private boolean hasGameExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".zip") || v.endsWith(".xex") || v.endsWith(".atr")
                || v.endsWith(".com") || v.endsWith(".exe");
    }

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= 23 && Build.VERSION.SDK_INT < 29 &&
                checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE}, 10);
        }
        web = new WebView(this);
        // BUILD2RV: WebView must be transparent in landscape; HTML is controls-only over native C++ video.
        try { web.setBackgroundColor(Color.TRANSPARENT); } catch (Throwable ignored) {}
        try { web.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        try { s.setUserAgentString(ATARIHELP_BROWSER_UA); } catch (Throwable ignored) {}
        s.setAllowFileAccess(true);
        s.setAllowUniversalAccessFromFileURLs(true);
        s.setAllowContentAccess(true);
        s.setMediaPlaybackRequiresUserGesture(false);
        try { s.setLoadsImagesAutomatically(true); } catch (Throwable ignored) {}
        try { s.setBlockNetworkImage(false); } catch (Throwable ignored) {}
        try { s.setBlockNetworkLoads(false); } catch (Throwable ignored) {}
        if (Build.VERSION.SDK_INT >= 21) {
            try { s.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW); } catch (Throwable ignored) {}
        }
        web.addJavascriptInterface(new AHSave(), "AHSAVE");
        web.addJavascriptInterface(new AHPick(), "AHPICK");
        web.addJavascriptInterface(new AHPS1(), "AHPS1"); // BUILD2SA1
        web.addJavascriptInterface(new AHNet(), "AHNET");
        web.addJavascriptInterface(new AHNative(), "AHNATIVE");
        web.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onShowFileChooser(WebView v, ValueCallback<Uri[]> cb,
                                             FileChooserParams params) {
                if (pendingChooser != null) pendingChooser.onReceiveValue(null);
                pendingChooser = cb;
                Intent i = new Intent(Intent.ACTION_GET_CONTENT);
                i.addCategory(Intent.CATEGORY_OPENABLE);
                i.setType("*/*");
                startActivityForResult(Intent.createChooser(i, "Vyber XEX / ATR / ZIP"), PICK_FILE);
                return true;
            }
        });
        web.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView v, String url, Bitmap favicon) {
                super.onPageStarted(v, url, favicon);
                applyWebViewVisualMode(url, "onPageStarted");
                stopNativeIfLeavingSega(url, "onPageStarted");
                stopPs1IfLeaving(url, "onPageStarted");
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView v, String url) {
                if (openExternalBrowserUrl(url)) return true;
                if (handleMaybeGameUrl(url)) return true;
                if (isProviderBlockedUrl(url)) return false;
                return false;
            }
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest request) {
                if (request != null && request.getUrl() != null) {
                    String url = request.getUrl().toString();
                    if (openExternalBrowserUrl(url)) return true;
                    if (handleMaybeGameUrl(url)) return true;
                    if (isProviderBlockedUrl(url)) return false;
                    return false;
                }
                return false;
            }
            @Override
            public void onPageFinished(WebView v, String url) {
                applyWebViewVisualMode(url, "onPageFinished");
                stopNativeIfLeavingSega(url, "onPageFinished");
                stopPs1IfLeaving(url, "onPageFinished");
                if (pendingGame != null && url != null && url.startsWith(EMU_URL)) {
                    schedulePendingAtariGameInjection("onPageFinished");
                }
                if (url != null && url.toLowerCase().contains("atarihelp.eu")) {
                    injectGameLinkBridge();
                }
            }
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView v, String url) {
                WebResourceResponse rr = interceptProviderBlockedResource(url, false);
                return rr != null ? rr : super.shouldInterceptRequest(v, url);
            }
            @Override
            public WebResourceResponse shouldInterceptRequest(WebView v, WebResourceRequest request) {
                if (Build.VERSION.SDK_INT >= 21 && request != null && request.getUrl() != null) {
                    WebResourceResponse rr = interceptProviderBlockedResource(request.getUrl().toString(), request.isForMainFrame());
                    if (rr != null) return rr;
                }
                return super.shouldInterceptRequest(v, request);
            }
            @Override
            public void onReceivedError(WebView v, int errorCode, String description, String failingUrl) {
                super.onReceivedError(v, errorCode, description, failingUrl);
                String current = v == null ? null : v.getUrl();
                if (current == null || current.equals(failingUrl)) {
                    showAtariHelpLoadError(failingUrl, "code=" + errorCode + " " + description);
                }
            }
            @Override
            public void onReceivedError(WebView v, WebResourceRequest request, WebResourceError error) {
                super.onReceivedError(v, request, error);
                if (Build.VERSION.SDK_INT >= 23 && request != null && request.isForMainFrame() && request.getUrl() != null) {
                    CharSequence d = error == null ? "" : error.getDescription();
                    showAtariHelpLoadError(request.getUrl().toString(), String.valueOf(d));
                }
            }
            @Override
            public void onReceivedHttpError(WebView v, WebResourceRequest request, WebResourceResponse errorResponse) {
                super.onReceivedHttpError(v, request, errorResponse);
                if (Build.VERSION.SDK_INT >= 21 && request != null && request.isForMainFrame() && request.getUrl() != null && isProviderBlockedUrl(request.getUrl().toString())) {
                    int code = errorResponse == null ? 0 : errorResponse.getStatusCode();
                    String reason = errorResponse == null ? "" : errorResponse.getReasonPhrase();
                    if (code >= 400) showAtariHelpLoadError(request.getUrl().toString(), "HTTP " + code + " " + reason);
                }
            }
        });
        web.setDownloadListener((url, userAgent, contentDisposition, mimetype, contentLength) -> {
            if (openExternalBrowserUrl(url)) return;
            if (isGameUrl(url, contentDisposition, mimetype)) {
                if (hasSegaExtension(url)) downloadAndRunSega(url); // BUILD2SA5U: raw Sega ROM only.
                else downloadAndRun(url); // ZIP decides by content after download.
            } else if (isProviderBlockedUrl(url)) {
                loadAtariHelpGuarded(url, "downloadListenerPageRelay");
            } else {
                try {
                    Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                    i.addCategory(Intent.CATEGORY_BROWSABLE);
                    startActivity(i);
                } catch (Exception ignored) {}
            }
        });
        rootFrame = new FrameLayout(this);
        try { rootFrame.setBackgroundColor(Color.BLACK); } catch (Throwable ignored) {}
        rootFrame.addView(web, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        setContentView(rootFrame);
        applyWebViewVisualMode("file:///android_asset/index.html", "startup");
        web.loadUrl("file:///android_asset/index.html");
    }

    private void openBridgePicker(String kind) {
        pendingBridgeKind = kind;
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);  // NOX je s ACTION_GET_CONTENT spolehlivejsi nez ACTION_OPEN_DOCUMENT
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        if ("mp3".equals(kind)) {
            i.setType("audio/*");
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"audio/mpeg", "audio/mp3", "audio/*", "application/octet-stream"});
            i.putExtra("android.content.extra.SHOW_ADVANCED", true);
            i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            i.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            startActivityForResult(Intent.createChooser(i, "MP3 PRIDAT - vyber pisnicku nebo vice pisnicek"), PICK_BRIDGE);
        } else if ("audio".equals(kind)) {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"audio/*", "audio/wav", "audio/x-wav", "audio/mpeg", "application/octet-stream"});
            startActivityForResult(Intent.createChooser(i, "VLOZIT KAZETU - vyber WAV / MP3 / CAS z mobilu"), PICK_BRIDGE);
        } else if ("text".equals(kind)) {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"text/*", "application/octet-stream", "*/*"});
            startActivityForResult(Intent.createChooser(i, "Vyber BASIC / Turbo BASIC TXT z mobilu"), PICK_BRIDGE);
        } else {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/zip", "application/octet-stream", "application/x-msdos-program", "*/*"});
            startActivityForResult(Intent.createChooser(i, "Vyber XEX / ATR / ZIP z mobilu"), PICK_BRIDGE);
        }
    }

    private String getDisplayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) {
                    String n = c.getString(idx);
                    if (n != null && n.length() > 0) return n;
                }
            }
        } catch (Exception ignored) {}
        String p = uri.getLastPathSegment();
        return (p == null || p.length() == 0) ? "vybrany_soubor" : p;
    }

    private byte[] readUriBytes(Uri uri, int maxBytes) throws Exception {
        InputStream in = getContentResolver().openInputStream(uri);
        if (in == null) throw new Exception("Nelze otevrit soubor");
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[16384];
        int n;
        while ((n = in.read(buf)) > 0) {
            bos.write(buf, 0, n);
            if (bos.size() > maxBytes) { in.close(); throw new Exception("Soubor je moc velky"); }
        }
        in.close();
        return bos.toByteArray();
    }

    private boolean isGameUrl(String url, String contentDisposition, String mimetype) {
        // BUILD2GH FIX:
        // NIKDY nehledat ".com" v cele URL, protoze facebook.com / youtube.com
        // by se pak povazoval za Atari COM soubor.
        try {
            if (url != null) {
                Uri uri = Uri.parse(url);
                String path = uri.getPath();
                if (hasGameExtension(path)) return true;
            }
        } catch (Throwable ignored) {}

        // Content-Disposition muze obsahovat skutecny filename souboru.
        if (contentDisposition != null) {
            String cd = contentDisposition.toLowerCase();
            int p = cd.indexOf("filename=");
            if (p >= 0) {
                String n = cd.substring(p + 9).replace("\"", "").replace("'", "").trim();
                int semi = n.indexOf(';');
                if (semi >= 0) n = n.substring(0, semi).trim();
                if (hasGameExtension(n)) return true;
            }
        }

        // MIME bereme jen jako slabou pomucku pro ZIP/octet, ne pro .com domenu.
        String mt = mimetype == null ? "" : mimetype.toLowerCase();
        if (mt.contains("application/zip")) return true;

        return false;
    }

    // BUILD2SA2: hry ze SBIRKY se musi routovat podle typu - Sega ROM do emu_sega,
    // ne do Atari. Atari cesta zustava pro xex/atr/com/exe/zip beze zmeny.
    private boolean hasSegaExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".gen") || v.endsWith(".md") || v.endsWith(".smd") || v.endsWith(".sms") || v.endsWith(".68k") || v.endsWith(".sgd");
    }

    private boolean hasAtariPayloadExtension(String value) {
        if (value == null) return false;
        String v = value.toLowerCase();
        int q = v.indexOf('?'); if (q >= 0) v = v.substring(0, q);
        int h = v.indexOf('#'); if (h >= 0) v = v.substring(0, h);
        return v.endsWith(".xex") || v.endsWith(".atr") || v.endsWith(".com") || v.endsWith(".exe");
    }

    private boolean isSegaCollectionContext() {
        try {
            String cur = web == null ? null : web.getUrl();
            if (cur == null) return false;
            String u = cur.toLowerCase(Locale.US);
            return u.startsWith(SEGA_URL) || u.contains("page_id=1003");
        } catch (Throwable ignored) {
            return false;
        }
    }

    private boolean shouldRouteAsSegaDownload(String url) {
        if (hasSegaExtension(url)) return true;
        if (!isGameUrl(url, null, null)) return false;
        return isSegaCollectionContext();
    }

    private SegaExtract extractSegaRomFromMaybeZip(String name, byte[] data) {
        if (data == null || data.length == 0) return null;
        if (hasSegaExtension(name)) {
            SegaExtract ex = new SegaExtract();
            ex.data = data;
            ex.name = name == null ? "sega_game.md" : name;
            return ex;
        }
        try {
            java.util.zip.ZipInputStream zi = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
            java.util.zip.ZipEntry ze;
            while ((ze = zi.getNextEntry()) != null) {
                String en = ze.getName() == null ? "" : ze.getName();
                if (hasSegaExtension(en)) {
                    ByteArrayOutputStream ro = new ByteArrayOutputStream();
                    byte[] rb = new byte[16384]; int rn;
                    while ((rn = zi.read(rb)) > 0 && ro.size() < 16 * 1024 * 1024) ro.write(rb, 0, rn);
                    zi.close();
                    SegaExtract ex = new SegaExtract();
                    ex.data = ro.toByteArray();
                    int sl = en.lastIndexOf('/');
                    ex.name = sl >= 0 ? en.substring(sl + 1) : en;
                    return ex;
                }
                zi.closeEntry();
            }
            zi.close();
        } catch (Throwable ignored) {}
        return null;
    }

    private AtariExtract extractAtariPayloadFromMaybeZip(String name, byte[] data) {
        if (data == null || data.length == 0) return null;
        if (hasAtariPayloadExtension(name)) {
            AtariExtract ex = new AtariExtract();
            ex.data = data;
            ex.name = name == null ? "atari_game.xex" : name;
            return ex;
        }
        try {
            java.util.zip.ZipInputStream zi = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
            java.util.zip.ZipEntry ze;
            AtariExtract atrFallback = null;
            while ((ze = zi.getNextEntry()) != null) {
                String en = ze.getName() == null ? "" : ze.getName();
                boolean atari = hasAtariPayloadExtension(en);
                if (atari) {
                    ByteArrayOutputStream ro = new ByteArrayOutputStream();
                    byte[] rb = new byte[16384]; int rn;
                    while ((rn = zi.read(rb)) > 0 && ro.size() < 16 * 1024 * 1024) ro.write(rb, 0, rn);
                    AtariExtract ex = new AtariExtract();
                    ex.data = ro.toByteArray();
                    int sl = en.lastIndexOf('/');
                    ex.name = sl >= 0 ? en.substring(sl + 1) : en;
                    if (en.toLowerCase(Locale.US).endsWith(".atr")) atrFallback = ex;
                    else {
                        zi.close();
                        return ex;
                    }
                }
                zi.closeEntry();
            }
            zi.close();
            if (atrFallback != null) return atrFallback;
        } catch (Throwable ignored) {}
        return null;
    }

    private void openSegaRomBytes(byte[] data, String name, String reason) {
        if (data == null || data.length == 0) return;
        pendingSegaGame = data;
        pendingSegaName = (name == null || name.length() == 0) ? "sega_game.md" : name;
        String cur = web == null ? null : web.getUrl();
        if (cur != null && cur.startsWith(SEGA_URL)) injectPendingSegaGame();
        else { web.loadUrl(SEGA_URL); web.postDelayed(MainActivity.this::injectPendingSegaGame, 1800); }
        appendNativeLog("BUILD2SA5T SEGA_ROUTE_OPEN reason=" + reason + " name=" + pendingSegaName + " bytes=" + data.length);
    }

    private void downloadAndRunSega(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadSega")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadSega", false);
                final String cdName = fetched.contentDisposition;
                final byte[] dataArr = fetched.data;
                final String name = guessDownloadName(url, cdName);
                appendNativeLog("BUILD2SA5S SEGA_WEB_ROM_DOWNLOADED name=" + name + " bytes=" + dataArr.length + " via=" + compactUrl(fetched.via));
                ui.post(() -> {
                    openSegaRomBytes(dataArr, name, "rawSega");
                });
            } catch (Exception ex) {
                appendNativeLog("BUILD2SA2 SEGA_WEB_ROM_FAIL " + ex.getMessage());
            }
        }).start();
    }

    private void downloadAndRunSegaArchive(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadSegaArchive")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadSegaArchive", false);
                final String cdName = fetched.contentDisposition;
                final byte[] dataArr = fetched.data;
                final String name = guessDownloadName(url, cdName);
                final SegaExtract sega = extractSegaRomFromMaybeZip(name, dataArr);
                if (sega == null || sega.data == null || sega.data.length == 0) {
                    throw new IOException("Sega ZIP neobsahuje .gen/.md/.smd/.sms ROM: " + name);
                }
                appendNativeLog("BUILD2SA5T SEGA_ARCHIVE_DOWNLOADED zip=" + name + " rom=" + sega.name + " romBytes=" + sega.data.length + " via=" + compactUrl(fetched.via));
                ui.post(() -> openSegaRomBytes(sega.data, sega.name, "segaArchive"));
            } catch (Exception ex) {
                appendNativeLog("BUILD2SA5T SEGA_ARCHIVE_FAIL " + safeMsg(ex));
                // BUILD2SA5T: Sega sbirka nesmi pri chybe prepnout do 130XE.
            }
        }).start();
    }
    // BUILD2SA3/SA5P: PS1 zvuk - dedikovane vlakno, Sega-style 384f chunks,
    // retry misto okamziteho ticha, generation guard a hard release pri prepnuti.
    private synchronized void startPs1Audio() {
        stopPs1Audio(); // BUILD2SA3B: pred novou hrou zabit stare vlakno a uvolnit stary AudioTrack
        final int gen = ++ps1AudioGen;
        ps1AudioThread = new Thread(() -> {
            AudioTrack at = null;
            try {
                try { android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO); } catch (Throwable ignored) {}
                final int sampleRate = 44100;
                final boolean s8NoStarve = (Build.VERSION.SDK_INT <= 28) || ((Build.MODEL == null ? "" : Build.MODEL).toUpperCase(Locale.US).contains("SM-G950"));
                final int chunkFrames = 384;
                int min = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                int wantedFrames = s8NoStarve ? 8192 : Math.max(4096, chunkFrames * 8);
                int bufBytes = Math.max(min > 0 ? (s8NoStarve ? min * 3 : min * 2) : 0, wantedFrames * 2 * 2);
                if (Build.VERSION.SDK_INT >= 21) {
                    at = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
                            .setBufferSizeInBytes(bufBytes)
                            .setTransferMode(AudioTrack.MODE_STREAM)
                            .build();
                } else {
                    at = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufBytes, AudioTrack.MODE_STREAM);
                }
                int setFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) {
                    try { setFrames = at.setBufferSizeInFrames(wantedFrames); } catch (Throwable ignored) {}
                }
                ps1CurrentAudioTrack = at;
                short[] buf = new short[chunkFrames * 2];
                int prefillFrames = 0;
                int prefillTarget = s8NoStarve ? 6144 : 3072;
                long prefillDeadline = System.currentTimeMillis() + (s8NoStarve ? 900 : 320);
                while (gen == ps1AudioGen && prefillFrames < prefillTarget && System.currentTimeMillis() < prefillDeadline) {
                    int got = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                    if (got > 0) {
                        softenPs1Pcm(buf, got * 2);
                        prefillFrames += writePs1AudioTrack(at, buf, got * 2, gen) / 2;
                    } else {
                        try { Thread.sleep(2); } catch (InterruptedException ignored) { break; }
                    }
                }
                at.play();
                int actualFrames = 0;
                try { if (Build.VERSION.SDK_INT >= 23) actualFrames = at.getBufferSizeInFrames(); } catch (Throwable ignored) {}
                appendNativeLog("BUILD2SA5P PS1_AUDIO_START gen=" + gen + " bufBytes=" + bufBytes + " minBytes=" + min + " setFrames=" + setFrames + " actualFrames=" + actualFrames + " prefillFrames=" + prefillFrames + " prefillTarget=" + prefillTarget + " gain=7/8 chunk=" + chunkFrames + " s8NoStarve=" + s8NoStarve + " retryNoSilence=ON previewThrottle=80ms cdAsync=ON biosAudit=ON");
                int idle = 0;
                int writes = 0;
                int loops = 0;
                int underrunLoops = 0;
                while (gen == ps1AudioGen) {
                    int got = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                    loops++;
                    if (got <= chunkFrames / 4) {
                        underrunLoops++;
                        int retries = s8NoStarve ? 5 : 2;
                        for (int rr = 0; rr < retries && got <= chunkFrames / 2 && gen == ps1AudioGen; rr++) {
                            try { Thread.sleep(s8NoStarve ? 3 : 2); } catch (InterruptedException ignored) { break; }
                            int got2 = NativePs1CoreBridge.pullAudioSafe(buf, chunkFrames);
                            if (got2 > got) got = got2;
                        }
                    }
                    if (got > 0) {
                        softenPs1Pcm(buf, got * 2);
                        int wrote = writePs1AudioTrack(at, buf, got * 2, gen);
                        idle = 0;
                        if (wrote > 0) writes++;
                        if (writes <= 6 || writes % 240 == 0) {
                            int underruns = -1;
                            try { if (Build.VERSION.SDK_INT >= 24) underruns = at.getUnderrunCount(); } catch (Throwable ignored) {}
                            appendNativeLog("BUILD2SA5P PS1_AUDIO_WRITE gen=" + gen + " gotFrames=" + got + " wroteShorts=" + wrote + " writes=" + writes + " loops=" + loops + " underrunLoops=" + underrunLoops + " underruns=" + underruns);
                        }
                    }
                    else {
                        idle++;
                        if (idle > 5000) break;
                        try { Thread.sleep(2); } catch (InterruptedException ignored) { break; }
                    }
                }
            } catch (Throwable t) {
                appendNativeLog("BUILD2SA5P PS1_AUDIO_ERROR " + t.getMessage());
            } finally {
                if (at != null) {
                    try { at.pause(); } catch (Throwable ignored) {}
                    try { at.flush(); } catch (Throwable ignored) {}
                    try { at.stop(); } catch (Throwable ignored) {}
                    try { at.release(); } catch (Throwable ignored) {}
                }
                if (ps1CurrentAudioTrack == at) ps1CurrentAudioTrack = null;
                appendNativeLog("BUILD2SA5P PS1_AUDIO_STOP gen=" + gen + " current=" + ps1AudioGen);
            }
        }, "nap-ps1-audio");
        ps1AudioThread.start();
    }
    private synchronized void stopPs1Audio() {
        final int gen = ++ps1AudioGen;
        AudioTrack at = ps1CurrentAudioTrack;
        ps1CurrentAudioTrack = null;
        if (at != null) {
            try { at.pause(); } catch (Throwable ignored) {}
            try { at.flush(); } catch (Throwable ignored) {}
            try { at.stop(); } catch (Throwable ignored) {}
            try { at.release(); } catch (Throwable ignored) {}
        }
        Thread t = ps1AudioThread;
        if (t != null && t != Thread.currentThread()) {
            try { t.interrupt(); } catch (Throwable ignored) {}
            try { t.join(250); } catch (Throwable ignored) {}
        }
        if (ps1AudioThread == t) ps1AudioThread = null;
        appendNativeLog("BUILD2SA5P PS1_AUDIO_STOP_REQUEST gen=" + gen + " hadTrack=" + (at != null));
    }
    private int writePs1AudioTrack(AudioTrack at, short[] pcm, int shorts, int gen) {
        if (at == null || pcm == null || shorts <= 0) return 0;
        int limit = Math.min(shorts, pcm.length);
        int off = 0;
        while (off < limit && gen == ps1AudioGen) {
            int wr = Build.VERSION.SDK_INT >= 23 ? at.write(pcm, off, limit - off, AudioTrack.WRITE_BLOCKING) : at.write(pcm, off, limit - off);
            if (wr <= 0) break;
            off += wr;
        }
        return off;
    }
    private void softenPs1Pcm(short[] pcm, int shorts) {
        if (pcm == null) return;
        int n = Math.min(shorts, pcm.length);
        for (int i = 0; i < n; i++) pcm[i] = (short)((pcm[i] * 7) / 8);
    }
    private void injectPendingSegaGame() {
        try {
            if (pendingSegaGame == null || pendingSegaName == null) return;
            String b64 = Base64.encodeToString(pendingSegaGame, Base64.NO_WRAP);
            String js = "try{napInjectRomBase64(" + jsQuote(pendingSegaName) + "," + jsQuote(b64) + ");}catch(e){console.log('napInject fail '+e);}";
            web.evaluateJavascript(js, null);
            appendNativeLog("BUILD2SA2 SEGA_WEB_ROM_INJECTED name=" + pendingSegaName + " bytes=" + pendingSegaGame.length);
            pendingSegaGame = null; pendingSegaName = null;
        } catch (Throwable t) { appendNativeLog("BUILD2SA2 SEGA_WEB_ROM_INJECT_FAIL " + t.getMessage()); }
    }

    private void runLocalPickedGame(String name, byte[] data) {
        if (name == null) name = "local_game.zip";
        if (data == null) return;
        if (hasSegaExtension(name)) {
            pendingSegaGame = data;
            pendingSegaName = name;
            String cur = web == null ? null : web.getUrl();
            if (cur != null && cur.startsWith(SEGA_URL)) injectPendingSegaGame();
            else { web.loadUrl(SEGA_URL); web.postDelayed(MainActivity.this::injectPendingSegaGame, 1800); }
            appendNativeLog("BUILD2SA5Q LOCAL_PICK_SEGA name=" + name + " bytes=" + data.length);
            return;
        }
        try {
            java.util.zip.ZipInputStream zi = new java.util.zip.ZipInputStream(new java.io.ByteArrayInputStream(data));
            java.util.zip.ZipEntry ze;
            while ((ze = zi.getNextEntry()) != null) {
                String en = ze.getName() == null ? "" : ze.getName();
                if (hasSegaExtension(en)) {
                    ByteArrayOutputStream ro = new ByteArrayOutputStream();
                    byte[] rb = new byte[16384]; int rn;
                    while ((rn = zi.read(rb)) > 0 && ro.size() < 16 * 1024 * 1024) ro.write(rb, 0, rn);
                    pendingSegaGame = ro.toByteArray();
                    int sl = en.lastIndexOf('/');
                    pendingSegaName = sl >= 0 ? en.substring(sl + 1) : en;
                    zi.close();
                    String cur = web == null ? null : web.getUrl();
                    if (cur != null && cur.startsWith(SEGA_URL)) injectPendingSegaGame();
                    else { web.loadUrl(SEGA_URL); web.postDelayed(MainActivity.this::injectPendingSegaGame, 1800); }
                    appendNativeLog("BUILD2SA5Q LOCAL_PICK_ZIP_CONTAINS_SEGA name=" + pendingSegaName + " bytes=" + pendingSegaGame.length);
                    return;
                }
                zi.closeEntry();
            }
            zi.close();
        } catch (Throwable ignored) {}
        queueAtariGameFor130xe(name, data, "localPick");
        appendNativeLog("BUILD2SA5Q LOCAL_PICK_ATARI name=" + name + " bytes=" + data.length);
    }

    private boolean handleMaybeGameUrl(String url) {
        if (openExternalBrowserUrl(url)) return true;
        if (hasSegaExtension(url)) { downloadAndRunSega(url); return true; } // BUILD2SA2: Sega ma prednost
        if (isGameUrl(url, null, null)) {
            downloadAndRun(url);
            return true;
        }
        return false;
    }

    private String guessDownloadName(String url, String contentDisposition) {
        try {
            if (contentDisposition != null) {
                String cd = contentDisposition;
                int p = cd.toLowerCase().indexOf("filename=");
                if (p >= 0) {
                    String n = cd.substring(p + 9).replace("\"", "").replace("'", "").trim();
                    int semi = n.indexOf(';');
                    if (semi >= 0) n = n.substring(0, semi).trim();
                    if (n.length() > 0) return n;
                }
            }
            Uri u = Uri.parse(url);
            String path = u.getLastPathSegment();
            if (path != null && path.length() > 0) return URLDecoder.decode(path, "UTF-8");
        } catch (Exception ignored) {}
        return "stazeno_z_webu.zip";
    }

    private void injectGameLinkBridge() {
        String js = "(function(){"
                + "if(window.__AH_GAME_BRIDGE)return;window.__AH_GAME_BRIDGE=1;"
                + "document.addEventListener('click',function(e){"
                + "var a=e.target;while(a&&a.tagName!=='A')a=a.parentElement;if(!a||!a.href)return;"
                + "var h=a.href;"
                + "if(/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd)([?#].*)?$/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd)/i.test(h)){"
                + "e.preventDefault();e.stopPropagation();try{AHNET.runGameUrl(h);}catch(err){location.href=h;}"
                + "}"
                + "},true);"
                + "document.addEventListener('click',function(e){"
                + "if(e.defaultPrevented)return;"
                + "var n=e.target,fig=null;while(n&&n!==document){if(n.tagName==='FIGURE'||(n.className&&String(n.className).indexOf('wp-block-image')>=0)){fig=n;break;}n=n.parentElement;}"
                + "if(!fig)return;var links=fig.getElementsByTagName('a');for(var i=0;i<links.length;i++){var h=links[i].href||'';"
                + "if(/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd)([?#].*)?$/i.test(h)||/\\.(xex|zip|atr|com|exe|gen|md|smd|sms|68k|sgd)/i.test(h)){"
                + "e.preventDefault();e.stopPropagation();try{AHNET.runGameUrl(h);}catch(err){location.href=h;}return;"
                + "}}"
                + "},true);"
                + "})();";
        web.evaluateJavascript(js, null);
    }

    private void downloadAndRun(final String url) {
        new Thread(() -> {
            try {
                if (!markAtariHelpRequestAllowed(url, "downloadGame")) return;
                FetchResult fetched = fetchUrlBytes(url, 16 * 1024 * 1024, "downloadGame", false);
                final String cdName = fetched.contentDisposition;
                final byte[] data = fetched.data;
                final String name = guessDownloadName(url, cdName);
                appendNativeLog("BUILD2SA5U WEB_GAME_DOWNLOADED name=" + name + " bytes=" + data.length + " via=" + compactUrl(fetched.via) + " zipContentDetect=ON");
                // BUILD2SA2B: Reneho web umi hostovat jen ZIPy. Kouknem DOVNITR zipu:
                // kdyz je uvnitr Sega ROM (.gen/.md/.smd/.sms), rozbalime a posleme
                // do EMU SEGA. Jinak jede stara Atari cesta beze zmeny.
                final SegaExtract sega = extractSegaRomFromMaybeZip(name, data);
                if (sega != null && sega.data != null && sega.data.length > 0) {
                    appendNativeLog("BUILD2SA5U ZIP_CONTAINS_SEGA name=" + sega.name + " bytes=" + sega.data.length + " -> EMU_SEGA");
                    ui.post(() -> openSegaRomBytes(sega.data, sega.name, "zipAutoDetect"));
                    return;
                }
                final AtariExtract atari = extractAtariPayloadFromMaybeZip(name, data);
                final byte[] atariData = (atari != null && atari.data != null && atari.data.length > 0) ? atari.data : data;
                final String atariName = (atari != null && atari.name != null && atari.name.length() > 0) ? atari.name : name;
                if (atari != null && atari.data != null && atari.data.length > 0) {
                    appendNativeLog("BUILD2SA5V ZIP_CONTAINS_ATARI name=" + atari.name + " bytes=" + atari.data.length + " -> EMU_130XE");
                }
                ui.post(() -> queueAtariGameFor130xe(atariName, atariData, "webDownload"));
            } catch (Exception ex) {
                appendNativeLog("BUILD2SA5U WEB_GAME_DOWNLOAD_FAIL " + safeMsg(ex));
                ui.post(() -> {
                    try {
                        final String msg = ex.getMessage() == null ? "neznamá chyba" : ex.getMessage();
                        String cur = web == null ? null : web.getUrl();
                        if (cur != null && cur.startsWith(EMU_URL)) {
                            web.postDelayed(() -> web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote("NET HRY: download selhal - " + msg) + ")", null), 500);
                        }
                    } catch (Exception ignored) {}
                });
            }
        }).start();
    }

    private String jsQuote(String text) {
        if (text == null) text = "";
        return "'" + text.replace("\\", "\\\\").replace("'", "\\'").replace("\n", " ").replace("\r", " ") + "'";
    }

    private void queueAtariGameFor130xe(String name, byte[] data, String reason) {
        if (data == null || data.length == 0) {
            appendNativeLog("BUILD2SA5W EMU130_QUEUE_SKIP_EMPTY reason=" + reason + " name=" + name);
            return;
        }
        pendingGame = data;
        pendingName = (name == null || name.length() == 0) ? "atarihelp_game.xex" : name;
        appendNativeLog("BUILD2SA5W EMU130_QUEUE reason=" + reason + " name=" + pendingName + " bytes=" + pendingGame.length);
        String cur = web == null ? null : web.getUrl();
        if (cur != null && cur.startsWith(EMU_URL)) {
            schedulePendingAtariGameInjection(reason + ":alreadyOn130xe");
        } else if (web != null) {
            web.loadUrl(EMU_URL + "?autorun=1");
        }
    }

    private void schedulePendingAtariGameInjection(final String reason) {
        if (pendingGame == null || web == null) return;
        final int seq = ++pendingGameInjectSeq;
        ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, 0), 120L);
    }

    private void tryInjectPendingAtariGame(final int seq, final String reason, final int attempt) {
        if (seq != pendingGameInjectSeq || pendingGame == null || web == null) return;
        String cur = web.getUrl();
        if (cur == null || !cur.startsWith(EMU_URL)) return;
        try {
            web.evaluateJavascript("(typeof window.AHRECV_BEGIN==='function'&&typeof window.AHRECV_PART==='function'&&typeof window.AHRECV_END==='function')", value -> {
                if (seq != pendingGameInjectSeq || pendingGame == null) return;
                boolean ready = "true".equals(String.valueOf(value)) || "\"true\"".equals(String.valueOf(value));
                if (ready) {
                    byte[] data = pendingGame;
                    String name = pendingName;
                    appendNativeLog("BUILD2SA5W EMU130_INJECT_READY reason=" + reason + " attempt=" + attempt + " name=" + name + " bytes=" + data.length);
                    try {
                        injectGame(name, data);
                        pendingGame = null;
                        pendingName = null;
                    } catch (Throwable t) {
                        appendNativeLog("BUILD2SA5W EMU130_INJECT_SEND_ERROR reason=" + reason + " " + safeMsg(t));
                        if (attempt < ATARI_INJECT_MAX_ATTEMPTS) {
                            ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_INJECT_RETRY_MS);
                        }
                    }
                    return;
                }
                if (attempt < ATARI_INJECT_MAX_ATTEMPTS) {
                    if (attempt == 0 || attempt == 8 || attempt == 20) {
                        appendNativeLog("BUILD2SA5W EMU130_INJECT_WAIT reason=" + reason + " attempt=" + attempt + " ready=" + value);
                    }
                    ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_INJECT_RETRY_MS);
                } else {
                    appendNativeLog("BUILD2SA5W EMU130_INJECT_TIMEOUT reason=" + reason + " name=" + pendingName + " bytes=" + pendingGame.length);
                }
            });
        } catch (Throwable t) {
            if (attempt < ATARI_INJECT_MAX_ATTEMPTS) {
                ui.postDelayed(() -> tryInjectPendingAtariGame(seq, reason, attempt + 1), ATARI_INJECT_RETRY_MS);
            } else {
                appendNativeLog("BUILD2SA5W EMU130_INJECT_JS_ERROR reason=" + reason + " " + safeMsg(t));
            }
        }
    }

    private void injectGame(String name, byte[] data) {
        appendNativeLog("BUILD2SA5X EMU130_INJECT_SEND_ORDERED name=" + name + " bytes=" + (data == null ? 0 : data.length));
        if (data == null || data.length == 0 || web == null) return;
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        final String safeName = (name == null || name.length() == 0) ? "atarihelp_game.xex" : name;
        web.evaluateJavascript("try{AHRECV_BEGIN(" + jsQuote(safeName) + ");'OK'}catch(e){'ERR:'+e.message}", value -> {
            if (!isJsOk(value)) {
                appendNativeLog("BUILD2SA5X EMU130_BEGIN_FAIL name=" + safeName + " result=" + value);
                return;
            }
            appendNativeLog("BUILD2SA5X EMU130_BEGIN_OK name=" + safeName + " b64Chars=" + b64.length());
            injectGamePartOrdered(safeName, b64, 0, 0);
        });
    }

    private boolean isJsOk(String value) {
        if (value == null) return false;
        String v = String.valueOf(value);
        return "\"OK\"".equals(v) || "OK".equals(v);
    }

    private void injectGamePartOrdered(final String name, final String b64, final int offset, final int partIndex) {
        if (web == null) return;
        if (offset >= b64.length()) {
            web.evaluateJavascript("try{AHRECV_END();'OK'}catch(e){'ERR:'+e.message}", value -> {
                if (isJsOk(value)) appendNativeLog("BUILD2SA5X EMU130_END_OK name=" + name + " parts=" + partIndex);
                else appendNativeLog("BUILD2SA5X EMU130_END_FAIL name=" + name + " result=" + value);
            });
            return;
        }
        final int end = Math.min(offset + ATARI_INJECT_CHUNK_CHARS, b64.length());
        final String part = b64.substring(offset, end);
        web.evaluateJavascript("try{AHRECV_PART('" + part + "');'OK'}catch(e){'ERR:'+e.message}", value -> {
            if (!isJsOk(value)) {
                appendNativeLog("BUILD2SA5X EMU130_PART_FAIL name=" + name + " part=" + partIndex + " result=" + value);
                return;
            }
            if (partIndex == 0 || end >= b64.length() || (partIndex % 16) == 0) {
                appendNativeLog("BUILD2SA5X EMU130_PART_OK name=" + name + " part=" + partIndex + " chars=" + end + "/" + b64.length());
            }
            injectGamePartOrdered(name, b64, end, partIndex + 1);
        });
    }

    private void injectAudio(String name, byte[] data) {
        web.evaluateJavascript("AHLOCAL_AUDIO_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHLOCAL_AUDIO_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHLOCAL_AUDIO_END()", null);
    }

    private void injectText(String name, byte[] data) {
        web.evaluateJavascript("AHLOCAL_TEXT_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHLOCAL_TEXT_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHLOCAL_TEXT_END()", null);
    }

    private String ps1BiosAudit(File sysDir) {
        try {
            if (sysDir == null) return "sysdir=null";
            StringBuilder sb = new StringBuilder();
            sb.append("sysdir=").append(sysDir.getAbsolutePath());
            if (!sysDir.exists()) return sb.append(" missing").toString();
            String[] known = new String[]{
                    "PSXONPSP660.bin", "psxonpsp660.bin",
                    "scph101.bin", "scph7001.bin", "scph5501.bin", "scph1001.bin",
                    "scph5500.bin", "scph5502.bin", "scph7003.bin", "scph7502.bin"
            };
            int found = 0;
            StringBuilder knownFound = new StringBuilder();
            for (String name : known) {
                File f = new File(sysDir, name);
                if (f.exists() && f.isFile()) {
                    found++;
                    if (knownFound.length() > 0) knownFound.append(",");
                    knownFound.append(f.getName()).append(":").append(f.length());
                }
            }
            File[] bins = sysDir.listFiles(new java.io.FilenameFilter() {
                @Override public boolean accept(File dir, String name) {
                    return name != null && name.toLowerCase(Locale.US).endsWith(".bin");
                }
            });
            sb.append(" knownFound=").append(found);
            if (knownFound.length() > 0) sb.append(" known=").append(knownFound);
            sb.append(" binCount=").append(bins == null ? -1 : bins.length);
            if (bins != null && bins.length > 0) {
                sb.append(" bins=");
                int max = Math.min(8, bins.length);
                for (int i = 0; i < max; i++) {
                    if (i > 0) sb.append(",");
                    sb.append(bins[i].getName()).append(":").append(bins[i].length());
                }
                if (bins.length > max) sb.append(",...");
            }
            return sb.toString();
        } catch (Throwable t) {
            return "audit_error=" + safeMsg(t);
        }
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (req == PICK_PS1_GAME) { // BUILD2SA2
            if (res != RESULT_OK || data == null || data.getData() == null) { ps1LastBootResult = "PS1_PICK_CANCELLED"; return; }
            final Uri uri = data.getData();
            final String pickedName = safeFileName(getDisplayName(uri));
            new Thread(() -> {
                int bootGen = 0;
                try {
                    android.os.ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
                    if (pfd == null) { ps1LastBootResult = "PS1_PICK_FAIL pfd=null"; return; }
                    synchronized (MainActivity.class) {
                        try { if (ps1GamePfd != null) ps1GamePfd.close(); } catch (Throwable ignored) {}
                        ps1GamePfd = pfd;
                    }
                    synchronized (MainActivity.this) {
                        bootGen = ++ps1LifecycleGen;
                        ps1BootActive = true;
                        ps1SessionActive = false;
                    }
                    String fdPath = "/proc/self/fd/" + pfd.getFd();
                    java.io.File sysDir = new java.io.File(getFilesDir(), "ps1_system");
                    java.io.File saveDir = new java.io.File(getFilesDir(), "ps1_saves");
                    if (!sysDir.exists()) sysDir.mkdirs();
                    if (!saveDir.exists()) saveDir.mkdirs();
                    ps1CurrentGameLabel = pickedName;
                    stopPs1Audio(); // BUILD2SA3B: cisty audio restart pri prepnuti PS1 hry
                    ps1LastBootResult = "PS1_BOOTING...";
                    appendNativeLog("BUILD2SA5P PS1_BIOS_AUDIT " + ps1BiosAudit(sysDir));
                    ps1LastBootResult = NativePs1CoreBridge.bootSafe(sysDir.getAbsolutePath(), saveDir.getAbsolutePath(), fdPath);
                    boolean ok = ps1LastBootResult != null && ps1LastBootResult.startsWith("PS1_BOOT_OK");
                    boolean stillWanted = ok && bootGen == ps1LifecycleGen && ps1BootActive;
                    ps1BootActive = false;
                    if (stillWanted) {
                        ps1SessionActive = true;
                        startPs1Audio(); // BUILD2SA3
                    } else {
                        ps1SessionActive = false;
                        stopPs1Audio();
                        if (ok) {
                            try { NativePs1CoreBridge.stopSafe(); } catch (Throwable ignored) {}
                            closePs1GamePfdQuietly();
                            ps1LastBootResult = "PS1_BOOT_CANCELLED_AFTER_LEAVE";
                        }
                    }
                } catch (Throwable t) {
                    ps1BootActive = false;
                    ps1SessionActive = false;
                    stopPs1Audio();
                    closePs1GamePfdQuietly();
                    ps1LastBootResult = "PS1_BOOT_EXCEPTION " + safeMsg(t);
                }
            }).start();
            return;
        }
        if (req == PICK_FILE && pendingChooser != null) {
            Uri[] out = (res == RESULT_OK && data != null && data.getData() != null)
                    ? new Uri[]{ data.getData() } : null;
            pendingChooser.onReceiveValue(out);
            pendingChooser = null;
            return;
        }
        if (req == PICK_BRIDGE) {
            if (res == RESULT_OK && data != null) {
                try {
                    int max = ("audio".equals(pendingBridgeKind) || "mp3".equals(pendingBridgeKind)) ? 64 * 1024 * 1024 : ("text".equals(pendingBridgeKind) ? 2 * 1024 * 1024 : 16 * 1024 * 1024);
                    if ("mp3".equals(pendingBridgeKind)) {
                        web.evaluateJavascript("AHLOCAL_MP3_PLAYLIST_BEGIN()", null);
                        ClipData clip = data.getClipData();
                        if (clip != null && clip.getItemCount() > 0) {
                            for (int k = 0; k < clip.getItemCount(); k++) {
                                Uri uri = clip.getItemAt(k).getUri();
                                if (uri == null) continue;
                                String name = getDisplayName(uri);
                                byte[] bytes = readUriBytes(uri, max);
                                injectAudio(name, bytes);
                            }
                        } else if (data.getData() != null) {
                            Uri uri = data.getData();
                            String name = getDisplayName(uri);
                            byte[] bytes = readUriBytes(uri, max);
                            injectAudio(name, bytes);
                        }
                        web.evaluateJavascript("AHLOCAL_MP3_PLAYLIST_END()", null);
                    } else if (data.getData() != null) {
                        Uri uri = data.getData();
                        String name = getDisplayName(uri);
                        byte[] bytes = readUriBytes(uri, max);
                        if ("audio".equals(pendingBridgeKind)) injectAudio(name, bytes);
                        else if ("text".equals(pendingBridgeKind)) injectText(name, bytes);
                        else runLocalPickedGame(name, bytes);
                    }
                } catch (Exception e) {
                    web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote(e.getMessage()) + ")", null);
                }
            }
            pendingBridgeKind = null;
        }
    }

    @Override
    public void onBackPressed() {
        if (web != null && web.canGoBack()) {
            stopNativeInPlaceHard("backPressedBeforeGoBack");
            stopPs1SessionHard("backPressedBeforeGoBack");
            web.goBack();
        }
        else super.onBackPressed();
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopNativeInPlaceHard("activityPause");
        stopPs1SessionHard("activityPause");
        if (web != null) web.onPause();
    }

    @Override
    protected void onDestroy() {
        stopNativeInPlaceHard("activityDestroy");
        stopPs1SessionHard("activityDestroy");
        super.onDestroy();
    }

    @Override
    protected void onResume() { super.onResume(); if (web != null) web.onResume(); }
}
