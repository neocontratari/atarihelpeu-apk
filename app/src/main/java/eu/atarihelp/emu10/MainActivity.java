package eu.atarihelp.emu10;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.database.Cursor;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
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
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLDecoder;

/**
 * AtariHelp.eu EMU-10 BUILD2AM
 * - file chooser (NAHRAJ XEX/ATR/ZIP)
 * - AHSAVE (ulozeni logu)
 * - DownloadListener: ZIP/XEX/ATR z webu se stahne a rovnou spusti v emulatoru
 * - BUILD2AG UI: NET HRY + XC12 WAV/MP3 real seek pres REW/F.FWD
 * - BUILD2AM HELP: vysvetlivky nejsou pres grafiku, napoveda je pod tlacitkem HELP
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

    public class AHSave {
        @JavascriptInterface
        public String save(String name, String text) {
            try {
                File dir = getExternalFilesDir(null);
                if (dir == null) dir = getFilesDir();
                File f = new File(dir, name.replaceAll("[^A-Za-z0-9._-]", "_"));
                FileOutputStream out = new FileOutputStream(f);
                out.write(text.getBytes("UTF-8"));
                out.close();
                return f.getAbsolutePath();
            } catch (Exception e) {
                return "CHYBA: " + e.getMessage();
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

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
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
                return handleMaybeGameUrl(url);
            }
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest request) {
                if (request != null && request.getUrl() != null) {
                    return handleMaybeGameUrl(request.getUrl().toString());
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
            if (isGameUrl(url, contentDisposition, mimetype)) {
                downloadAndRun(url);
            } else {
                try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url))); }
                catch (Exception ignored) {}
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
            startActivityForResult(Intent.createChooser(i, "Vyber MP3 hudbu z Downloads"), PICK_BRIDGE);
        } else if ("audio".equals(kind)) {
            i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"audio/*", "audio/wav", "audio/x-wav", "audio/mpeg", "application/octet-stream"});
            startActivityForResult(Intent.createChooser(i, "Vyber WAV / MP3 / CAS z mobilu"), PICK_BRIDGE);
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
        String all = ((url == null ? "" : url) + " "
                + (contentDisposition == null ? "" : contentDisposition) + " "
                + (mimetype == null ? "" : mimetype)).toLowerCase();
        return all.contains(".zip") || all.contains(".xex") || all.contains(".atr")
                || all.contains(".com") || all.contains(".exe");
    }

    private boolean handleMaybeGameUrl(String url) {
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
            if (res == RESULT_OK && data != null && data.getData() != null) {
                try {
                    Uri uri = data.getData();
                    String name = getDisplayName(uri);
                    int max = ("audio".equals(pendingBridgeKind) || "mp3".equals(pendingBridgeKind)) ? 64 * 1024 * 1024 : ("text".equals(pendingBridgeKind) ? 2 * 1024 * 1024 : 16 * 1024 * 1024);
                    byte[] bytes = readUriBytes(uri, max);
                    if ("audio".equals(pendingBridgeKind) || "mp3".equals(pendingBridgeKind)) injectAudio(name, bytes);
                    else if ("text".equals(pendingBridgeKind)) injectText(name, bytes);
                    else injectGame(name, bytes);
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
