package eu.atarihelp.emu10;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * AtariHelp.eu EMU-10 BUILD2W
 * - file chooser (NAHRAJ XEX/ATR/ZIP)
 * - AHSAVE (ulozeni logu)
 * - DownloadListener: ZIP/XEX/ATR z webu se stahne a rovnou spusti v emulatoru
 * - BUILD2W UI: REC otevira AtariHelp hry, PLAY v HTML vybira lokalni WAV/MP3, PAUSE TBXL
 */
public class MainActivity extends Activity {
    private static final int PICK_FILE = 1;
    private static final String EMU_URL = "file:///android_asset/emu/index.html";
    private WebView web;
    private ValueCallback<Uri[]> pendingChooser;
    private byte[] pendingGame;
    private String pendingName;
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

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        web = new WebView(this);
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setAllowFileAccess(true);
        s.setMediaPlaybackRequiresUserGesture(false);
        web.addJavascriptInterface(new AHSave(), "AHSAVE");
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
            public void onPageFinished(WebView v, String url) {
                if (pendingGame != null && url.startsWith(EMU_URL)) {
                    injectGame(pendingName, pendingGame);
                    pendingGame = null;
                }
            }
        });
        web.setDownloadListener((url, userAgent, contentDisposition, mimetype, contentLength) -> {
            String low = url.toLowerCase();
            if (low.contains(".zip") || low.contains(".xex") || low.contains(".atr")
                    || low.contains(".com") || low.contains(".exe")) {
                downloadAndRun(url);
            } else {
                try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url))); }
                catch (Exception ignored) {}
            }
        });
        web.loadUrl("file:///android_asset/index.html");
        setContentView(web);
    }

    private void downloadAndRun(final String url) {
        new Thread(() -> {
            try {
                HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
                c.setInstanceFollowRedirects(true);
                c.connect();
                InputStream in = c.getInputStream();
                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                byte[] buf = new byte[16384];
                int n;
                while ((n = in.read(buf)) > 0 && bos.size() < 8 * 1024 * 1024) bos.write(buf, 0, n);
                in.close();
                final byte[] data = bos.toByteArray();
                String path = Uri.parse(url).getLastPathSegment();
                final String name = (path == null || path.isEmpty()) ? "stazeno.zip" : path;
                ui.post(() -> {
                    String cur = web.getUrl();
                    if (cur != null && cur.startsWith(EMU_URL)) {
                        injectGame(name, data);
                    } else {
                        pendingGame = data;
                        pendingName = name;
                        web.loadUrl(EMU_URL);   // otevri emulator, soubor se vlozi po nacteni
                    }
                });
            } catch (Exception ignored) { }
        }).start();
    }

    private void injectGame(String name, byte[] data) {
        String safe = name.replace("'", "_").replace("\\", "_");
        web.evaluateJavascript("AHRECV_BEGIN('" + safe + "')", null);
        String b64 = Base64.encodeToString(data, Base64.NO_WRAP);
        for (int i = 0; i < b64.length(); i += 262144) {
            String part = b64.substring(i, Math.min(i + 262144, b64.length()));
            web.evaluateJavascript("AHRECV_PART('" + part + "')", null);
        }
        web.evaluateJavascript("AHRECV_END()", null);
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (req == PICK_FILE && pendingChooser != null) {
            Uri[] out = (res == RESULT_OK && data != null && data.getData() != null)
                    ? new Uri[]{ data.getData() } : null;
            pendingChooser.onReceiveValue(out);
            pendingChooser = null;
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
