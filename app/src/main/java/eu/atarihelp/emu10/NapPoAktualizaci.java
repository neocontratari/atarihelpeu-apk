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

    @Override
    public void onReceive(Context c, Intent i) {
        if (c == null || i == null || !Intent.ACTION_MY_PACKAGE_REPLACED.equals(i.getAction())) return;
        if (!NapAktualizaceTexty.otevreSeSama(Build.VERSION.SDK_INT)) return;
        try {
            Intent o = new Intent(c, MainActivity.class);
            o.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            o.putExtra(EXTRA, true);
            c.startActivity(o);
        } catch (Throwable ignored) {}
    }
}
