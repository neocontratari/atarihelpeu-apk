package eu.atarihelp.nap;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import java.util.Locale;

/**
 * BUILD2GF - ExternalUrlHelper
 *
 * CIL:
 * - odkazy na YouTube / Facebook otevrit jako normalni WWW stranky
 * - nesmi spadnout do Atari emulator NET/XEX loaderu
 *
 * DULEZITE:
 * Tuto tridu je nutne zavolat z MainActivity WebViewClientu PRED kodem,
 * ktery stahuje web do emulatoru jako XEX/ZIP/BAS.
 */
public final class ExternalUrlHelper {
    private static final String TAG = "AtariHelpExternalUrl";

    private ExternalUrlHelper() {}

    public static boolean isExternalBrowserUrl(String rawUrl) {
        if (rawUrl == null) return false;
        String url = rawUrl.trim();
        if (url.length() == 0) return false;

        Uri uri;
        try {
            uri = Uri.parse(url);
        } catch (Throwable t) {
            return false;
        }

        String scheme = uri.getScheme();
        if (scheme == null) return false;
        scheme = scheme.toLowerCase(Locale.US);

        if (!scheme.equals("http") && !scheme.equals("https")) return false;

        String host = uri.getHost();
        if (host == null) return false;
        host = host.toLowerCase(Locale.US);

        // Tohle jsou ciste WWW odkazy z N&P UI. Ty nikdy nesmi jit do emulator loaderu.
        return host.equals("youtube.com")
                || host.equals("www.youtube.com")
                || host.equals("m.youtube.com")
                || host.endsWith(".youtube.com")
                || host.equals("youtu.be")
                || host.equals("facebook.com")
                || host.equals("www.facebook.com")
                || host.equals("m.facebook.com")
                || host.endsWith(".facebook.com");
    }

    public static boolean openExternalIfNeeded(Activity activity, String rawUrl) {
        if (activity == null || !isExternalBrowserUrl(rawUrl)) return false;

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(rawUrl));
            intent.addCategory(Intent.CATEGORY_BROWSABLE);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            activity.startActivity(intent);
            Log.i(TAG, "OPEN EXTERNAL WWW: " + rawUrl);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "No browser activity for: " + rawUrl, e);
            return true; // i kdyz neni browser, NEPOSILAT do emulatoru
        } catch (Throwable t) {
            Log.e(TAG, "External open failed: " + rawUrl, t);
            return true; // i pri chybe NEPOSILAT do emulatoru
        }
    }
}
