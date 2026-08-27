package eu.atarihelp.emu10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.os.Build;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * BUILD2SA42: AKTUALIZACE PRIMO ZE SITE.
 *
 * Rene chtel, aby se pri aktualizaci na telefonu NEOBJEVIL zadny soubor.
 * Jde to: APK se necha protect ROVNOU DO INSTALACNI RELACE pres
 * PackageInstaller. Na disk se nic neuklada - ani ZIP, ani APK.
 *
 *    sit  ->  ZipInputStream  ->  PackageInstaller  ->  systemove okno
 *
 * Na webu staci ZIP, ktery uz tam Rene ma - rozbaluje se za letu v pameti.
 *
 * Male upozorneni, ktere se obejit NEDA a je to tak spravne:
 *   - systemove okno "Nainstalovat" uzivatel uvidi vzdycky
 *   - poprve se telefon zepta na povoleni instalovat z tohoto zdroje
 *
 * Kontrola verze:
 *   https://atarihelp.eu/wp-content/uploads/2026/08/emu10_verze.txt
 * obsahuje jeden radek  "cislo|odkaz_na_zip".  Ma 69 bajtu, takze se
 * nemusi tahat celych 48 MB jen kvuli zjisteni, jestli je neco noveho.
 */
public final class NapAktualizace {

    private NapAktualizace() {}

    public static final String VERZE_URL =
            "https://atarihelp.eu/wp-content/uploads/2026/08/emu10_verze.txt";
    /** Kdyby soubor s verzi chybel, pouzije se tenhle odkaz. */
    public static final String ZALOZNI_ZIP =
            "https://atarihelp.eu/wp-content/uploads/2026/08/app-debug-43.zip";

    public interface Hlaska { void rekni(String zprava); }

    private static final class Novinka { int verze; String zip; }

    // ------------------------------------------------------------------
    //  Kontrola, jestli je venku novejsi
    // ------------------------------------------------------------------
    private static Novinka zjistiNovinku() {
        HttpURLConnection c = null;
        try {
            c = (HttpURLConnection) new URL(VERZE_URL).openConnection();
            c.setConnectTimeout(15000);
            c.setReadTimeout(15000);
            c.setRequestProperty("User-Agent", "AtariHelpEMU10");
            if (c.getResponseCode() != 200) return null;
            InputStream in = c.getInputStream();
            byte[] buf = new byte[512];
            int n = 0, k;
            while (n < buf.length && (k = in.read(buf, n, buf.length - n)) > 0) n += k;
            in.close();
            String radek = new String(buf, 0, n, "UTF-8").trim();
            int svisla = radek.indexOf('|');
            if (svisla <= 0) return null;
            Novinka v = new Novinka();
            v.verze = Integer.parseInt(radek.substring(0, svisla).trim());
            v.zip = radek.substring(svisla + 1).trim();
            return v;
        } catch (Throwable t) {
            return null;
        } finally {
            if (c != null) try { c.disconnect(); } catch (Throwable ignored) {}
        }
    }

    private static int mojeVerze(Context c) {
        try {
            return c.getPackageManager().getPackageInfo(c.getPackageName(), 0).versionCode;
        } catch (Throwable t) { return 0; }
    }

    /**
     * Podiva se, jestli je venku novejsi verze, a kdyz ano, zepta se.
     * Kdyz soubor s verzi chybi nebo neni sit, MLCI - nic se nedeje.
     */
    public static void zkontrolujTise(final Activity a, final Hlaska log) {
        new Thread(() -> {
            Novinka v = zjistiNovinku();
            if (v == null) { if (log != null) log.rekni("AKTUALIZACE nezjistena"); return; }
            final int moje = mojeVerze(a);
            if (log != null) log.rekni("AKTUALIZACE na webu=" + v.verze + " moje=" + moje);
            if (v.verze <= moje) return;
            a.runOnUiThread(() -> zeptejSe(a, v.verze, moje, v.zip, log));
        }, "nap-aktualizace").start();
    }

    /** Rucni spusteni z nabidky OPTIONS - stahne i kdyz je verze stejna. */
    public static void spustRucne(final Activity a, final Hlaska log) {
        new Thread(() -> {
            Novinka v = zjistiNovinku();
            final String zip = (v != null) ? v.zip : ZALOZNI_ZIP;
            final int nova = (v != null) ? v.verze : 0;
            a.runOnUiThread(() -> zeptejSe(a, nova, mojeVerze(a), zip, log));
        }, "nap-aktualizace-rucne").start();
    }

    private static void zeptejSe(final Activity a, int nova, int moje,
                                 final String zip, final Hlaska log) {
        try {
            String text = "Na atarihelp.eu je verze " + nova
                    + ", ty mas " + moje + ".\n\n"
                    + "Stahne se a rovnou nainstaluje - do telefonu se pritom"
                    + " NEULOZI zadny soubor.\n\n"
                    + "Telefon se jeste zepta, jestli instalaci povolis.";
            new AlertDialog.Builder(a)
                    .setTitle("Aktualizace")
                    .setMessage(text)
                    .setPositiveButton("AKTUALIZOVAT", (d, w) -> nainstaluj(a, zip, log))
                    .setNegativeButton("TED NE", null)
                    .show();
        } catch (Throwable t) {
            if (log != null) log.rekni("AKTUALIZACE dialog: " + t.getClass().getSimpleName());
        }
    }

    // ------------------------------------------------------------------
    //  Stazeni a instalace - bez souboru na disku
    // ------------------------------------------------------------------
    private static void nainstaluj(final Activity a, final String zipUrl, final Hlaska log) {
        final AlertDialog dlg;
        final android.widget.TextView popis = new android.widget.TextView(a);
        final android.widget.ProgressBar pruh = new android.widget.ProgressBar(
                a, null, android.R.attr.progressBarStyleHorizontal);
        try {
            android.widget.LinearLayout box = new android.widget.LinearLayout(a);
            box.setOrientation(android.widget.LinearLayout.VERTICAL);
            int p = (int) (16 * a.getResources().getDisplayMetrics().density);
            box.setPadding(p * 2, p, p * 2, p);
            popis.setText("Pripojuji se...");
            popis.setTextSize(15f);
            box.addView(popis);
            pruh.setIndeterminate(true);
            box.addView(pruh);
            dlg = new AlertDialog.Builder(a).setTitle("Aktualizace")
                    .setView(box).setCancelable(false).create();
            dlg.show();
        } catch (Throwable t) {
            if (log != null) log.rekni("AKTUALIZACE okno: " + t.getClass().getSimpleName());
            return;
        }

        new Thread(() -> {
            String vysledek;
            HttpURLConnection c = null;
            PackageInstaller.Session relace = null;
            try {
                if (Build.VERSION.SDK_INT < 21) throw new Exception("stary Android");
                c = (HttpURLConnection) new URL(zipUrl).openConnection();
                c.setConnectTimeout(30000);
                c.setReadTimeout(90000);
                c.setRequestProperty("User-Agent", "AtariHelpEMU10");
                c.setInstanceFollowRedirects(true);
                int kod = c.getResponseCode();
                if (kod != 200) throw new Exception("HTTP " + kod);
                final long celkem = c.getContentLength();

                PackageInstaller pi = a.getPackageManager().getPackageInstaller();
                PackageInstaller.SessionParams sp = new PackageInstaller.SessionParams(
                        PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                int id = pi.createSession(sp);
                relace = pi.openSession(id);

                InputStream sit = c.getInputStream();
                ZipInputStream zi = new ZipInputStream(sit);
                ZipEntry e;
                boolean naslo = false;
                byte[] buf = new byte[65536];
                long staženo = 0;
                long zacatek = System.currentTimeMillis();
                while ((e = zi.getNextEntry()) != null) {
                    if (e.isDirectory()) continue;
                    if (!e.getName().toLowerCase(java.util.Locale.US).endsWith(".apk")) continue;
                    naslo = true;
                    OutputStream ven = relace.openWrite("emu10", 0, -1);
                    int n;
                    while ((n = zi.read(buf)) > 0) {
                        ven.write(buf, 0, n);
                        staženo += n;
                        long ted = System.currentTimeMillis();
                        final long mam = staženo;
                        final long rychlost = mam * 1000 / Math.max(1, ted - zacatek);
                        a.runOnUiThread(() -> {
                            popis.setText("Instaluji primo ze site");
                            pruh.setIndeterminate(celkem <= 0);
                            if (celkem > 0) pruh.setProgress((int) (mam * 100 / celkem));
                        });
                    }
                    relace.fsync(ven);
                    ven.close();
                    break;
                }
                zi.close();
                if (!naslo) throw new Exception("v ZIPu neni APK");

                Intent i = new Intent(a, MainActivity.class);
                i.setAction("eu.atarihelp.emu10.INSTALACE");
                android.app.PendingIntent pending = android.app.PendingIntent.getActivity(
                        a, 0, i,
                        Build.VERSION.SDK_INT >= 31
                                ? android.app.PendingIntent.FLAG_MUTABLE
                                : 0);
                relace.commit(pending.getIntentSender());
                relace = null;               // relaci uz drzi system
                vysledek = "OK (" + (staženo / 1024) + " kB, na disku nic)";
            } catch (Throwable t) {
                vysledek = "CHYBA " + t.getClass().getSimpleName()
                         + (t.getMessage() != null ? ": " + t.getMessage() : "");
            } finally {
                if (relace != null) try { relace.abandon(); } catch (Throwable ignored) {}
                if (c != null) try { c.disconnect(); } catch (Throwable ignored) {}
            }
            final String v = vysledek;
            a.runOnUiThread(() -> {
                try { dlg.dismiss(); } catch (Throwable ignored) {}
                if (log != null) log.rekni("AKTUALIZACE " + v);
                if (!v.startsWith("OK")) {
                    try {
                        new AlertDialog.Builder(a).setTitle("Aktualizace nevysla")
                                .setMessage(v + "\n\nZkus to znovu z nabidky OPTIONS.")
                                .setPositiveButton("OK", null).show();
                    } catch (Throwable ignored) {}
                }
            });
        }, "nap-instalace").start();
    }

    /** Umi telefon vubec instalovat z teto aplikace? */
    public static boolean smiInstalovat(Context c) {
        try {
            if (Build.VERSION.SDK_INT < 26) return true;
            return c.getPackageManager().canRequestPackageInstalls();
        } catch (Throwable t) { return false; }
    }

    /** Otevre systemove nastaveni, kde se to povoluje. */
    public static void otevriPovoleni(Activity a) {
        try {
            if (Build.VERSION.SDK_INT < 26) return;
            Intent i = new Intent(android.provider.Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES);
            i.setData(android.net.Uri.parse("package:" + a.getPackageName()));
            a.startActivity(i);
        } catch (Throwable ignored) {}
    }
}
