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
    private ValueCallback<Uri[]> pendingChooser;
    private byte[] pendingGame;
    private String pendingName;
    private String pendingBridgeKind;
    private final Handler ui = new Handler(Looper.getMainLooper());

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
    public class AHNative {
        @JavascriptInterface
        public void openSegaCppProof() {
            ui.post(() -> {
                try {
                    Intent i = new Intent(MainActivity.this, NativeSegaProofActivity.class);
                    startActivity(i);
                } catch (Throwable t) {
                    try {
                        web.evaluateJavascript("AHJAVA_ERROR(" + jsQuote("SEGA C++ TEST: nelze otevrit native aktivitu - " + t.getMessage()) + ")", null);
                    } catch (Throwable ignored) {}
                }
            });
        }
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
        setContentView(web);
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
        if (web != null && web.canGoBack()) web.goBack();
        else super.onBackPressed();
    }

    @Override
    protected void onPause() { super.onPause(); if (web != null) web.onPause(); }

    @Override
    protected void onResume() { super.onResume(); if (web != null) web.onResume(); }
}
