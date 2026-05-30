package eu.atarihelp.mobile;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.WebChromeClient.FileChooserParams;
import android.webkit.ValueCallback;
import android.net.Uri;
import android.content.Intent;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.os.Build;
import android.webkit.JavascriptInterface;
import android.provider.MediaStore;
import android.content.ContentValues;
import android.os.Environment;
import android.widget.Toast;
import java.io.OutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class MainActivity extends Activity {
    private WebView webView;
    private ValueCallback<Uri[]> filePathCallback;
    private static final int FILE_CHOOSER_REQUEST = 901;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        webView = new WebView(this);
        setContentView(webView);

        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setMediaPlaybackRequiresUserGesture(false);

        webView.addJavascriptInterface(new LogBridge(), "AtariLogBridge");

        webView.setWebViewClient(new WebViewClient());
        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> filePathCallback, FileChooserParams fileChooserParams) {
                if (MainActivity.this.filePathCallback != null) {
                    MainActivity.this.filePathCallback.onReceiveValue(null);
                }
                MainActivity.this.filePathCallback = filePathCallback;
                Intent intent;
                try {
                    intent = fileChooserParams.createIntent();
                } catch (Exception e) {
                    intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("application/octet-stream");
                }
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                try {
                    startActivityForResult(intent, FILE_CHOOSER_REQUEST);
                } catch (Exception e) {
                    Intent fallback = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    fallback.addCategory(Intent.CATEGORY_OPENABLE);
                    fallback.setType("*/*");
                    startActivityForResult(fallback, FILE_CHOOSER_REQUEST);
                }
                return true;
            }
        });
        webView.loadUrl("file:///android_asset/index.html");

        if (Build.VERSION.SDK_INT >= 30) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.statusBars());
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == FILE_CHOOSER_REQUEST) {
            Uri[] results = null;
            if (resultCode == RESULT_OK && data != null) {
                Uri uri = data.getData();
                if (uri != null) {
                    results = new Uri[]{uri};
                }
            }
            if (filePathCallback != null) {
                filePathCallback.onReceiveValue(results);
                filePathCallback = null;
            }
        }
    }


    public class LogBridge {
        @JavascriptInterface
        public String saveLog(String text, String suggestedName) {
            String safeName = sanitizeFileName(suggestedName);
            if (safeName.length() == 0) {
                safeName = "atarihelp-log-" + new SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(new Date()) + ".txt";
            }
            if (!safeName.toLowerCase(Locale.US).endsWith(".txt")) {
                safeName += ".txt";
            }
            if (text == null) text = "";
            try {
                if (Build.VERSION.SDK_INT >= 29) {
                    ContentValues values = new ContentValues();
                    values.put(MediaStore.Downloads.DISPLAY_NAME, safeName);
                    values.put(MediaStore.Downloads.MIME_TYPE, "text/plain");
                    values.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/AtariHelpLogs");
                    Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
                    if (uri == null) throw new Exception("MediaStore uri je null");
                    OutputStream os = getContentResolver().openOutputStream(uri);
                    if (os == null) throw new Exception("Nelze otevřít OutputStream");
                    os.write(text.getBytes("UTF-8"));
                    os.flush();
                    os.close();
                    final String msg = "Log uložen: Download/AtariHelpLogs/" + safeName;
                    runOnUiThread(() -> Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show());
                    return msg;
                } else {
                    File dir = new File(getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), "AtariHelpLogs");
                    if (!dir.exists()) dir.mkdirs();
                    File out = new File(dir, safeName);
                    FileOutputStream fos = new FileOutputStream(out);
                    fos.write(text.getBytes("UTF-8"));
                    fos.flush();
                    fos.close();
                    final String msg = "Log uložen: " + out.getAbsolutePath();
                    runOnUiThread(() -> Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show());
                    return msg;
                }
            } catch (Exception e) {
                final String msg = "Log se nepodařilo uložit: " + e.getMessage();
                runOnUiThread(() -> Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show());
                return msg;
            }
        }

        private String sanitizeFileName(String name) {
            if (name == null) return "";
            return name.replaceAll("[^A-Za-z0-9._-]", "_");
        }
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }
}
