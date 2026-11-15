package eu.atarihelp.emu10;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

/**
 * B327: APPKA SE PO VLASTNÍ AKTUALIZACI ZASE OTEVŘE (Android 9 a starší).
 *
 * René po B326 (S8, Android 9): "po té se aplikace jakoby minimalizovala ... já upky vypl a zapl".
 * Android appku při instalaci nové verze zavře a sám ji znovu neotevře (výsledek instalace,
 * který měl appku otevřít, Android s ukončením appky zahodí). Nová verze ale dostane od systému
 * zprávu MY_PACKAGE_REPLACED - tady ji otevře. Od Androidu 10 spouštění obrazovek z pozadí systém
 * nedovolí (bylo by zablokované), proto jen do Androidu 9; na novějším ji člověk otevře ikonou
 * a appka při startu ukáže "Nainstalována nová verze" (NapAktualizace.poStartu).
 */
public final class NapPoAktualizaci extends BroadcastReceiver {

    /** Příznak v intentu: appku otevřel tenhle přijímač (MainActivity to zapíše do logu). */
    public static final String EXTRA = "eu.atarihelp.emu10.PO_AKTUALIZACI";

    /** B328: co se tu stalo (MainActivity to pri dalsim startu zapise do logu) - Rene po B327 na S8
     *  nevedel jiste, jestli se appka otevrela sama. */
    static final String PREF = "nap_aktualizace", K_CAS = "prijimac_ms", K_VYSLEDEK = "prijimac";

    @Override
    public void onReceive(Context c, Intent i) {
        if (c == null || i == null || !Intent.ACTION_MY_PACKAGE_REPLACED.equals(i.getAction())) return;
        String vysledek;
        if (!NapAktualizaceTexty.otevreSeSama(Build.VERSION.SDK_INT)) {
            vysledek = "Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ") - appka se sama neotevira";
        } else {
            try {
                Intent o = new Intent(c, MainActivity.class);
                o.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                o.putExtra(EXTRA, true);
                c.startActivity(o);
                vysledek = "otevreni zadano (Android " + Build.VERSION.RELEASE + ")";
            } catch (Throwable t) {
                vysledek = "otevreni CHYBA " + t.getClass().getSimpleName() + (t.getMessage() != null ? ": " + t.getMessage() : "");
            }
        }
        try {
            c.getSharedPreferences(PREF, Context.MODE_PRIVATE).edit()
                    .putLong(K_CAS, System.currentTimeMillis()).putString(K_VYSLEDEK, vysledek).commit();
        } catch (Throwable ignored) {}
    }
}
