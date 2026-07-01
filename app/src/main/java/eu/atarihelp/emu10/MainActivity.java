package eu.atarihelp.emu10;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ContentValues;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.content.Intent;
import android.content.ClipData;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Build;
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
    private volatile int nativeAudioGeneration = 0; // BUILD2RI: kills stale AudioTrack threads after every ROM change; prevents cumulative slowdown.
    private Thread nativeCoreAudioThread;
    private volatile AudioTrack nativeCurrentAudioTrack;
    private ValueCallback<Uri[]> pendingChooser;
    private byte[] pendingGame;
    private String pendingName;
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
    private volatile String nativePerformanceMode = "HIGH"; // BUILD2RI: SBIRKA selects HIGH QUALITY / LOW PERFORMANCE.
    private volatile long nativeLastSaveLogAtMs = 0; // BUILD2RI: one tap must not create 4 log files.

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
            if (nativeLog.length() > 20000) nativeLog.delete(0, nativeLog.length() - 20000);
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
        out.append("SEGA C++ IN-PLACE LOG / BUILD2RI\n");
        out.append("AtariHelp.eu EMU-10 BUILD2RI_SEGA_NATIVE_CPP_ONLY_FM_MUSIC_TIMING_BASS_CLAMP_STAGE125\n\n");
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
        try { out.append("realCoreStatus=").append(NativeSegaCoreBridge.realCoreStatus().replace('\n',' ')).append("\n"); } catch (Throwable t) { out.append("realCoreStatus=ERROR ").append(safeMsg(t)).append("\n"); }
        out.append("\nROM BLOCK:\n").append(nativeLastRomInfo == null ? "" : nativeLastRomInfo).append("\n\n");
        out.append("EVENTS:\n");
        synchronized (nativeLog) { out.append(nativeLog.toString()); }
        out.append("\nDULEZITE:\n- Tohle porad neni hotovy Sega gameplay.\n");
        out.append("- Toto overuje normalni Sega UI -> Java -> JNI -> C++ -> ROM/input/audio/render/log.\n");
        out.append("- Sega emulace je v BUILD2RI C++ only; Java/WebView wrapper se nespousti; C++ CORE UI tlacitko je odstranene; FM hudba ma bass clamp a bez FM/PSG desync dropu; SBIRKA prepina LOW/HIGH vykon; mobile visibility/orientation nesmi vypnout native video; C++ region respektuje ROM header; nulove recty se ignoruji.\n");
        return out.toString();
    }

    private synchronized void startNativeCoreAudioStream() {
        if (nativeCoreAudioRun && nativeCoreAudioThread != null && nativeCoreAudioThread.isAlive()) {
            appendNativeLog("NATIVE_AUDIO_STREAM_ALREADY_RUNNING_RI_AUDIO_MUSIC_TIMING_QT gen=" + nativeAudioGeneration);
            return;
        }
        final int audioGen = ++nativeAudioGeneration; // BUILD2RI: one valid AudioTrack writer per ROM session.
        nativeCoreAudioRun = true;
        nativeCoreAudioThread = new Thread(() -> {
            try { android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO); } catch (Throwable ignored) {}
            final int sampleRate = 48000;
            final int chunk = 384; // BUILD2RI: stable AudioTrack clock; full chunks with bass-clamped native output.
            AudioTrack track = null;
            try {
                int min = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
                // BUILD2RI: keep the AudioTrack clock stable. Bigger buffer hides short FM music stalls without changing emulation speed.
                int wantedBytes = chunk * 2 * 16;
                int bufferBytes = Math.max(min > 0 ? min * 3 : 0, wantedBytes);
                AudioTrack.Builder builder = null;
                if (Build.VERSION.SDK_INT >= 21) {
                    builder = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
                            .setBufferSizeInBytes(bufferBytes)
                            .setTransferMode(AudioTrack.MODE_STREAM);
                    if (Build.VERSION.SDK_INT >= 26) {
                        try { builder.setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY); } catch (Throwable ignored) {}
                    }
                    track = builder.build();
                } else {
                    track = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferBytes, AudioTrack.MODE_STREAM);
                }
                int setFrames = -1;
                if (Build.VERSION.SDK_INT >= 23) {
                    try { setFrames = track.setBufferSizeInFrames(Math.max(4096, chunk * 12)); } catch (Throwable ignored) {}
                }
                short[] pcm = new short[chunk];
                nativeCurrentAudioTrack = track;
                appendNativeLog("NATIVE_AUDIO_STREAM_START_RI_AUDIO_MUSIC_TIMING_QT gen=" + audioGen + " sampleRate=" + sampleRate + " chunk=" + chunk + " minBytes=" + min + " bufferBytes=" + bufferBytes + " setFrames=" + setFrames + " mode=" + nativePerformanceMode + " perfLowLatency=" + (Build.VERSION.SDK_INT >= 26));

                int prefilled = 0;
                int preLoops = 0;
                long preDeadline = System.currentTimeMillis() + 500;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration && prefilled < Math.min(3072, chunk * 8) && System.currentTimeMillis() < preDeadline) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudio(pcm, pcm.length); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PREFILL_PULL_ERROR_RI " + safeMsg(pullErr)); got = 0; }
                    if (got > 0) {
                        int wr = track.write(pcm, 0, pcm.length);
                        if (wr > 0) prefilled += wr;
                    } else {
                        try { Thread.sleep(4); } catch (Throwable ignored) {}
                    }
                    preLoops++;
                }

                if (!nativeCoreAudioRun || audioGen != nativeAudioGeneration) {
                    appendNativeLog("NATIVE_AUDIO_START_CANCELLED_RI staleGen=" + audioGen + " current=" + nativeAudioGeneration);
                    return;
                }
                track.play();
                appendNativeLog("NATIVE_AUDIO_PREFILL_RI_AUDIO_MUSIC_TIMING_QT gen=" + audioGen + " frames=" + prefilled + " loops=" + preLoops + " playState=" + track.getPlayState());
                int loops = 0;
                int underrunLoops = 0;
                while (nativeCoreAudioRun && audioGen == nativeAudioGeneration) {
                    int got = 0;
                    try { got = NativeSegaCoreBridge.pullAudio(pcm, pcm.length); }
                    catch (Throwable pullErr) { appendNativeLog("NATIVE_AUDIO_PULL_ERROR_RI " + safeMsg(pullErr)); got = 0; }
                    if (got <= chunk / 4) { underrunLoops++; }
                    if (loops < 16 || loops % 180 == 0) appendNativeLog("NATIVE_AUDIO_PULL_RI_AUDIO_MUSIC_TIMING_QT gen=" + audioGen + " got=" + got + " loop=" + loops + " underrunLoops=" + underrunLoops + " mode=" + nativePerformanceMode);
                    // BUILD2RI: always feed AudioTrack a full clock chunk. Native writes real samples and controlled decay only on real underrun;
                    // writing tiny partial chunks made the Android audio clock starve and made music sound slow.
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
                appendNativeLog("NATIVE_AUDIO_STREAM_ERROR_RI gen=" + audioGen + " " + safeMsg(t));
            } finally {
                try { if (track != null) { track.pause(); track.flush(); track.stop(); track.release(); } } catch (Throwable ignored) {}
                if (nativeCurrentAudioTrack == track) nativeCurrentAudioTrack = null;
                appendNativeLog("NATIVE_AUDIO_STREAM_STOP_RI_AUDIO_MUSIC_TIMING_QT gen=" + audioGen + " current=" + nativeAudioGeneration);
            }
        }, "AtariHelpSegaAudioMusicTimingRI_" + audioGen);
        nativeCoreAudioThread.setDaemon(true);
        try { nativeCoreAudioThread.setPriority(Thread.MAX_PRIORITY); } catch (Throwable ignored) {}
        nativeCoreAudioThread.start();
    }

    private synchronized void stopNativeCoreAudioStream() {
        final int stopGen = ++nativeAudioGeneration; // BUILD2RI: invalidate stale AudioTrack writers before starting another ROM.
        nativeCoreAudioRun = false;
        AudioTrack at = nativeCurrentAudioTrack;
        if (at != null) {
            try { at.pause(); } catch (Throwable ignored) {}
            try { at.flush(); } catch (Throwable ignored) {}
            try { at.stop(); } catch (Throwable ignored) {}
            // BUILD2RI: on S8 old AudioTrack.write() could stay blocked after ROM change.
            // Release immediately; the audio thread finally-block tolerates the already released track.
            try { at.release(); } catch (Throwable ignored) {}
            nativeCurrentAudioTrack = null;
        }
        Thread t = nativeCoreAudioThread;
        if (t != null && t.isAlive() && Thread.currentThread() != t) {
            try { t.interrupt(); } catch (Throwable ignored) {}
            try { t.join(1100); } catch (Throwable ignored) {}
            if (t.isAlive()) appendNativeLog("NATIVE_AUDIO_THREAD_STILL_ALIVE_RI invalidatedGen=" + stopGen + " releasedTrack=YES will exit on generation guard");
        }
        if (nativeCoreAudioThread == t) nativeCoreAudioThread = null;
        appendNativeLog("NATIVE_AUDIO_STREAM_STOP_REQUEST_RI_AUDIO_MUSIC_TIMING_QT gen=" + stopGen + " hardReleaseTrack=" + (at != null));
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

    private void stopNativeIfLeavingSega(String url, String source) {
        if (!nativeInPlaceEnabled && !nativeCoreAudioRun && nativeInPlaceView == null) return;
        if (isSegaNativeOwnerUrl(url)) return;
        stopNativeInPlaceHard(source + ":" + compactUrl(url));
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
                    try { if (old.getParent() instanceof ViewGroup) ((ViewGroup) old.getParent()).removeView(old); } catch (Throwable t) { appendNativeLog("NATIVE_VIEW_REMOVE_UI_ERR_RI " + safeMsg(t)); }
                }
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_OK_RI reason=" + reason);
            } catch (Throwable t) {
                appendNativeLog("NATIVE_VIEW_REMOVE_UI_FAIL_RI reason=" + reason + " " + safeMsg(t));
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
        nativeRomLoadGeneration++; // BUILD2RI: cancel stale delayed audio/render watchdogs when leaving Sega/Atari/VBXE.
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
                        // BUILD2RI: enableInPlace is now idempotent. Rotation/resize/visibility on S8 called it many times;
                        // recreating TextureView each time slowly killed mobile video. Reuse the existing native view when it is alive.
                        if (nativeInPlaceView != null && nativeInPlaceView.getParent() == rootFrame) {
                            nativeInPlaceEnabled = true;
                            try { nativeInPlaceView.setAlpha(1f); nativeInPlaceView.setTranslationX(0f); nativeInPlaceView.setTranslationY(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.start(); nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_VIEW_REUSE_RI parent=rootFrame noRecreate=YES");
                            return;
                        }
                        if (nativeInPlaceView != null) {
                            try { nativeInPlaceView.stop(); } catch (Throwable ignored) {}
                            try { if (nativeInPlaceView.getParent() != null) ((ViewGroup) nativeInPlaceView.getParent()).removeView(nativeInPlaceView); } catch (Throwable ignored) {}
                            nativeInPlaceView = null;
                            appendNativeLog("NATIVE_VIEW_RECREATE_RI oldViewRemoved=YES detachedOrBadParent");
                        }
                        nativeInPlaceView = new NativeInPlaceView(MainActivity.this);
                        nativeInPlaceView.setClickable(false);
                        nativeInPlaceView.setEnabled(false);
                        nativeInPlaceView.setFocusable(false);
                        nativeInPlaceView.setFocusableInTouchMode(false);
                        // BUILD2RI: never show native view before JS sends a valid monitor/landscape rect.
                        nativeInPlaceView.setVisibility(View.INVISIBLE);
                        try { nativeInPlaceView.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
                        rootFrame.addView(nativeInPlaceView, new FrameLayout.LayoutParams(1, 1));
                        appendNativeLog("NATIVE_VIEW_REATTACH_RI parent=rootFrame freshView=YES hiddenUntilValidRect=YES");
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
                        // BUILD2RI: Samsung/WebView can send a transient 0x0 rect when returning from landscape.
                        // Ignoring that rect prevents the native video from becoming black after portrait return.
                        if (w < 120 || h < 80) { appendNativeLog("SET_RECT_SKIP_SMALL_RI x=" + x + " y=" + y + " w=" + w + " h=" + h); return; }
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
                            // BUILD2RI: on real phones the C++ native view is the video layer, WebView is only transparent controls/log.
                            // This gives full-screen Sonic/Aladdin with transparent joystick/buttons over the picture.
                            try { nativeInPlaceView.setZ(0f); } catch (Throwable ignored) {}
                            try { web.setZ(10f); } catch (Throwable ignored) {}
                            try { rootFrame.bringChildToFront(web); } catch (Throwable ignored) {}
                            appendNativeLog("NATIVE_Z_ORDER_RI landscapeWebControlsOverNative=YES topLabelsHidden=YES");
                        } else {
                            try { if (web != null) web.setZ(0f); } catch (Throwable ignored) {}
                            try { nativeInPlaceView.setZ(20f); } catch (Throwable ignored) {}
                            nativeInPlaceView.bringToFront();
                        }
                        nativeInPlaceView.requestLayout();
                        nativeInPlaceView.invalidate();
                        appendNativeLog("SET_RECT_OK_RI x=" + Math.max(0, x) + " y=" + Math.max(0, y) + " w=" + ww + " h=" + hh + " visible=" + nativeInPlaceEnabled + " landscapeFull=" + landscapeFull);
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

                // BUILD2RI: audio starts only after visible native frame; bass clamp/music timing lives in C++ core.
                // Tvrdý fresh start pred kazdou ROM brani stavu: Atari 130XE -> Sega -> nova ROM -> cerna obrazovka + zvuk.
                final int loadGen = ++nativeRomLoadGeneration;
                appendNativeLog("FRESH_ROM_GENERATION_RI gen=" + loadGen + " oldAudioGen=" + nativeAudioGeneration + " oldDraw=" + nativeViewDrawCounter);
                stopNativeCoreAudioStream();
                try { NativeSegaCoreBridge.shutdown(); appendNativeLog("FRESH_ROM_HARD_STOP_BEFORE_LOAD_RI gen=" + loadGen); } catch (Throwable ignored) {}
                nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                nativeRenderPerfWindowStartMs = 0; nativeRenderPerfWindowFrames = 0; nativeRenderPerfSlowFrames = 0;
                String realCore = NativeSegaCoreBridge.realCoreLoadRom(data);

                nativeLastRomInfo = "ROM: " + safeFileName(name) + "\n" + info + "\n\nREAL CORE SLOT:\n" + realCore;
                nativeLastStatus = "ROM_REAL_CORE_LOAD_READY bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt;
                appendNativeLog("ROM_REAL_CORE_LOAD_READY name=" + safeFileName(name) + " bytes=" + data.length + " decodeMs=" + decodeMs + " parserMs=" + dt + " gen=" + loadGen);
                appendNativeLog("REAL_CORE_RENDER_ACTIVE_RI after ROM load audio=WAIT_FRAME_AND_VIEW_DRAW");
                forceNativeViewRedrawBurst("afterRomLoad_RI");
                scheduleNativeAudioAfterFrameAndViewDraw(name, data, loadGen, 1);
                scheduleNativeRenderWatchdog(name, data, loadGen, 1);
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
                try { nativePerf = NativeSegaCoreBridge.setPerformanceMode(m); } catch (Throwable nt) { nativePerf = "nativePerfError=" + safeMsg(nt); }
                appendNativeLog("NATIVE_PERF_MODE_RI mode=" + nativePerformanceMode + " sdk=" + Build.VERSION.SDK_INT + " " + nativePerf);
                if (nativeInPlaceView != null) {
                    try { nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {}
                }
                return "NATIVE_PERF_MODE_OK_RI mode=" + nativePerformanceMode + " " + nativePerf;
            } catch (Throwable t) {
                return "NATIVE_PERF_MODE_ERROR_RI " + safeMsg(t);
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
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
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
                    appendNativeLog("SAVE_LOG_DEDUP_RI ignored deltaMs=" + (now - nativeLastSaveLogAtMs));
                    return "SAVE_LOG_DEDUP_RI";
                }
                nativeLastSaveLogAtMs = now;
                String fn = "AtariHelp_SEGA_CPP_INPLACE_LOG_BUILD2RI_" + new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(new Date()) + ".txt";
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
        // BUILD2RI: keep only a short burst; long delayed bursts from old ROMs added work after multiple ROM loads.
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 120);
        ui.postDelayed(() -> { try { if (nativeInPlaceView != null) nativeInPlaceView.forceRedrawOnce(); } catch (Throwable ignored) {} }, 420);
    }

    private void scheduleNativeAudioAfterFrameAndViewDraw(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 220 : 320;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_AUDIO_WAIT_CANCELLED_RI staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RI attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(430, st.length()))));
                if (hasFrame && viewReady) {
                    startNativeCoreAudioStream();
                    appendNativeLog("NATIVE_AUDIO_START_AFTER_FRAME_VIEW_RI name=" + safeFileName(romName) + " gen=" + gen + " audioProfile=QT_BASS_CLAMP_MUSIC_TIMING");
                    return;
                }
                if (attempt < 10 && nativeInPlaceEnabled) {
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, attempt + 1);
                    return;
                }
                stopNativeCoreAudioStream();
                nativeLastStatus = "NATIVE_AUDIO_NOT_STARTED_NO_VISIBLE_FRAME_RI name=" + safeFileName(romName);
                appendNativeLog(nativeLastStatus);
                forceNativeViewRedrawBurst("audioNoVisibleFrame_RI");
            } catch (Throwable t) {
                appendNativeLog("NATIVE_AUDIO_WAIT_FRAME_VIEW_RI_ERROR " + safeMsg(t));
            }
        }, delay);
    }

    private void scheduleNativeRenderWatchdog(final String romName, final byte[] romData, final int gen, final int attempt) {
        final int delay = attempt <= 1 ? 1350 : 2600;
        ui.postDelayed(() -> {
            try {
                if (gen != nativeRomLoadGeneration) { appendNativeLog("NATIVE_RENDER_WATCHDOG_CANCELLED_RI staleGen=" + gen + " current=" + nativeRomLoadGeneration); return; }
                String st = NativeSegaCoreBridge.realCoreStatus();
                boolean hasFrame = st != null && st.indexOf("frameReady=YES") >= 0 && st.indexOf("frameCounter=0") < 0;
                boolean viewReady = nativeInPlaceEnabled && nativeInPlaceView != null && nativeInPlaceView.getParent() != null
                        && nativeInPlaceView.getVisibility() == View.VISIBLE && nativeViewDrawCounter > nativeViewDrawCounterAtRomLoad;
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RI attempt=" + attempt + " gen=" + gen + " hasFrame=" + hasFrame + " viewReady=" + viewReady + " draw=" + nativeViewDrawCounter + "/" + nativeViewDrawCounterAtRomLoad + " " + (st == null ? "null" : st.replace('\n',' ').substring(0, Math.min(520, st.length()))));
                if (hasFrame && viewReady) {
                    forceNativeViewRedrawBurst("watchdogFrameViewOK_RI");
                    return;
                }
                if (attempt <= 1 && romData != null && romData.length > 0 && nativeInPlaceEnabled) {
                    appendNativeLog("NATIVE_RENDER_WATCHDOG_FRESH_RELOAD_RI name=" + safeFileName(romName) + " bytes=" + romData.length + " gen=" + gen);
                    try { stopNativeCoreAudioStream(); } catch (Throwable ignored) {}
                    try { NativeSegaCoreBridge.shutdown(); } catch (Throwable ignored) {}
                    nativeViewDrawCounterAtRomLoad = nativeViewDrawCounter;
                    String reload = NativeSegaCoreBridge.realCoreLoadRom(romData);
                    nativeLastStatus = "NATIVE_RENDER_WATCHDOG_RELOAD_RI " + (reload == null ? "null" : reload.replace('\n',' '));
                    appendNativeLog(nativeLastStatus.substring(0, Math.min(900, nativeLastStatus.length())));
                    forceNativeViewRedrawBurst("watchdogFreshReload_RI");
                    scheduleNativeAudioAfterFrameAndViewDraw(romName, romData, gen, 1);
                    scheduleNativeRenderWatchdog(romName, romData, gen, attempt + 1);
                    return;
                }
                if (!hasFrame || !viewReady) {
                    stopNativeCoreAudioStream();
                    nativeLastStatus = "NATIVE_RENDER_NO_VISIBLE_FRAME_AUDIO_STOPPED_RI name=" + safeFileName(romName);
                    appendNativeLog(nativeLastStatus);
                    forceNativeViewRedrawBurst("watchdogNoVisibleFrameAudioStopped_RI");
                }
            } catch (Throwable t) {
                appendNativeLog("NATIVE_RENDER_WATCHDOG_RI_ERROR " + safeMsg(t));
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
            appendNativeLog("NATIVE_TEXTURE_THREAD_START_RI reason=" + reason + " view=" + getWidth() + "x" + getHeight());
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
                    appendNativeLog("NATIVE_TEXTURE_RENDER_ERROR_RI " + safeMsg(t));
                } finally {
                    try { if (c != null) unlockCanvasAndPost(c); } catch (Throwable t) { appendNativeLog("NATIVE_TEXTURE_UNLOCK_ERROR_RI " + safeMsg(t)); }
                }
                long cost = System.nanoTime() - startNs;
                nativeLastRenderCostNs = cost;
                // BUILD2RI: SBIRKA performance switch. HIGH keeps full-speed render for Nox/new phones.
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
            appendNativeLog("NATIVE_TEXTURE_THREAD_STOP_RI reason=" + reason + " drawCounter=" + nativeViewDrawCounter + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0));
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
                    // BUILD2RI: never crop the 320x224 game image. User reported Sonic lives/HUD cut off in landscape.
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
                canvas.drawBitmap(bitmap, null, dst, paint);
                nativeViewDrawCounter++;
                long nowMs = System.currentTimeMillis();
                if (nativeRenderPerfWindowStartMs == 0) nativeRenderPerfWindowStartMs = nowMs;
                nativeRenderPerfWindowFrames++;
                if (nativeLastRenderCostNs > 22000000L) nativeRenderPerfSlowFrames++;
                if (nowMs - nativeRenderPerfWindowStartMs >= 5000) {
                    appendNativeLog("NATIVE_RENDER_PERF_RI frames5s=" + nativeRenderPerfWindowFrames + " slow22ms=" + nativeRenderPerfSlowFrames + " lastCostMs=" + (nativeLastRenderCostNs / 1000000.0) + " android=" + Build.VERSION.SDK_INT + " perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo);
                    nativeRenderPerfWindowStartMs = nowMs;
                    nativeRenderPerfWindowFrames = 0;
                    nativeRenderPerfSlowFrames = 0;
                }
                if (nativeViewDrawCounter <= 4 || nativeViewDrawCounter % 300 == 0) {
                    appendNativeLog("NATIVE_TEXTURE_FRAME_RI count=" + nativeViewDrawCounter + " view=" + w + "x" + h + " gameDst=" + dst.toShortString() + " perfMode=" + nativePerformanceMode + " landscapeFull=" + nativeLandscapeFullVideo + " costMs=" + (nativeLastRenderCostNs / 1000000.0));
                }
            } catch (Throwable t) {
                try { canvas.drawColor(Color.rgb(20, 0, 0)); } catch (Throwable ignored) {}
                appendNativeLog("NATIVE_TEXTURE_FRAME_ERROR_RI " + safeMsg(t));
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
            ui.post(() -> web.loadUrl("https://atarihelp.eu/?page_id=207"));
        }
        @JavascriptInterface
        public void runGameUrl(String url) {
            ui.post(() -> {
                if (url != null && url.length() > 0) downloadAndRun(url);
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

    private boolean openExternalBrowserUrl(String url) {
        if (!isExternalBrowserUrl(url)) return false;
        try {
            Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            i.addCategory(Intent.CATEGORY_BROWSABLE);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(i);
        } catch (Throwable ignored) {
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
        // BUILD2RI: WebView must be transparent in landscape; HTML is controls-only over native C++ video.
        try { web.setBackgroundColor(Color.TRANSPARENT); } catch (Throwable ignored) {}
        try { web.setLayerType(View.LAYER_TYPE_HARDWARE, null); } catch (Throwable ignored) {}
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setAllowFileAccess(true);
        s.setAllowUniversalAccessFromFileURLs(true);
        s.setAllowContentAccess(true);
        s.setMediaPlaybackRequiresUserGesture(false);
        web.addJavascriptInterface(new AHSave(), "AHSAVE");
        web.addJavascriptInterface(new AHPick(), "AHPICK");
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
                stopNativeIfLeavingSega(url, "onPageStarted");
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView v, String url) {
                if (openExternalBrowserUrl(url)) return true;
                return handleMaybeGameUrl(url);
            }
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest request) {
                if (request != null && request.getUrl() != null) {
                    String url = request.getUrl().toString();
                    if (openExternalBrowserUrl(url)) return true;
                    return handleMaybeGameUrl(url);
                }
                return false;
            }
            @Override
            public void onPageFinished(WebView v, String url) {
                stopNativeIfLeavingSega(url, "onPageFinished");
                if (pendingGame != null && url.startsWith(EMU_URL)) {
                    injectGame(pendingName, pendingGame);
                    pendingGame = null;
                }
                if (url != null && url.toLowerCase().contains("atarihelp.eu")) {
                    injectGameLinkBridge();
                }
            }
        });
        web.setDownloadListener((url, userAgent, contentDisposition, mimetype, contentLength) -> {
            if (openExternalBrowserUrl(url)) return;
            if (isGameUrl(url, contentDisposition, mimetype)) {
                downloadAndRun(url);
            } else {
                try {
                    Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                    i.addCategory(Intent.CATEGORY_BROWSABLE);
                    startActivity(i);
                } catch (Exception ignored) {}
            }
        });
        web.loadUrl("file:///android_asset/index.html");
        rootFrame = new FrameLayout(this);
        try { rootFrame.setBackgroundColor(Color.BLACK); } catch (Throwable ignored) {}
        rootFrame.addView(web, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        setContentView(rootFrame);
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

    private boolean handleMaybeGameUrl(String url) {
        if (openExternalBrowserUrl(url)) return true;
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
                + "if(/\\.(xex|zip|atr|com|exe)([?#].*)?$/i.test(h)||/\\.(xex|zip|atr|com|exe)/i.test(h)){"
                + "e.preventDefault();try{AHNET.runGameUrl(h);}catch(err){location.href=h;}"
                + "}"
                + "},true);"
                + "})();";
        web.evaluateJavascript(js, null);
    }

    private void downloadAndRun(final String url) {
        new Thread(() -> {
            try {
                HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
                c.setInstanceFollowRedirects(true);
                c.connect();
                final String cdName = c.getHeaderField("Content-Disposition");
                InputStream in = c.getInputStream();
                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                byte[] buf = new byte[16384];
                int n;
                while ((n = in.read(buf)) > 0 && bos.size() < 8 * 1024 * 1024) bos.write(buf, 0, n);
                in.close();
                final byte[] data = bos.toByteArray();
                final String name = guessDownloadName(url, cdName);
                ui.post(() -> {
                    String cur = web.getUrl();
                    if (cur != null && cur.startsWith(EMU_URL)) {
                        injectGame(name, data);
                    } else {
                        pendingGame = data;
                        pendingName = name;
                        web.loadUrl(EMU_URL + "?autorun=1");   // otevri emulator bez auto POWER BASIC; soubor se vlozi po nacteni
                    }
                });
            } catch (Exception ex) {
                ui.post(() -> {
                    try {
                        if (web.getUrl() == null || !web.getUrl().startsWith(EMU_URL)) web.loadUrl(EMU_URL);
                        final String msg = ex.getMessage() == null ? "neznamá chyba" : ex.getMessage();
                        web.postDelayed(() -> web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote("NET HRY: download selhal - " + msg) + ")", null), 500);
                    } catch (Exception ignored) {}
                });
            }
        }).start();
    }

    private String jsQuote(String text) {
        if (text == null) text = "";
        return "'" + text.replace("\\", "\\\\").replace("'", "\\'").replace("\n", " ").replace("\r", " ") + "'";
    }

    private void injectGame(String name, byte[] data) {
        web.evaluateJavascript("AHRECV_BEGIN(" + jsQuote(name) + ")", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHRECV_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHRECV_END()", null);
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

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
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
                        else injectGame(name, bytes);
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
            web.goBack();
        }
        else super.onBackPressed();
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopNativeInPlaceHard("activityPause");
        if (web != null) web.onPause();
    }

    @Override
    protected void onDestroy() {
        stopNativeInPlaceHard("activityDestroy");
        super.onDestroy();
    }

    @Override
    protected void onResume() { super.onResume(); if (web != null) web.onResume(); }
}
