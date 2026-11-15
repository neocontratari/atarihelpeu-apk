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
 * B321: AKTUALIZACE ZE SERVERU N&P (Reneho PC). Sparovany telefon se pta serveru
 * (/api/v1/stav: apk_verze = versionCode APK na serveru) a novou verzi stahuje podepsane
 * z /api/v1/apk - primo, bez ZIPu. Web atarihelp.eu zustava jen jako zaloha.
 *
 * B324: Rene - "po odsouhlaseni v apce to skace z apky ven pro dalsi potvrzeni ... pak se to
 * nesmyslne vraci zpet do apky a stahuje to znova". Vysledek instalace (PendingIntent na
 * MainActivity) spoustel KAZDYM zavolanim NOVOU kopii MainActivity (launchMode standard): ta se
 * zeptala serveru, videla novejsi verzi (stara jeste nebyla nainstalovana) a nabidla ji znovu.
 * Ted: FLAG_ACTIVITY_SINGLE_TOP | CLEAR_TOP (vysledek dostane bezici MainActivity.onNewIntent),
 * behem instalace se nic znovu nenabizi (instalaceBezi) a Android 12+ (API 31) dalsi aktualizaci
 * nainstaluje bez potvrzovani (USER_ACTION_NOT_REQUIRED - appka uz je "installer of record").
 *
 * Kontrola verze (zaloha):
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

    /** B321: server N&P (nastavi MainActivity). Sparovany telefon bere aktualizace odtud. */
    public static volatile NapServerKlient server;

    private static final class Novinka { int verze; String zip; String nazev = ""; }

    /** B324: od kdy bezi stazeni / instalace - mezitim se aktualizace znovu nenabizi (10 minut). */
    private static volatile long instalaceOd = 0L;

    static boolean instalaceBezi() {
        final long t = instalaceOd;
        return t != 0L && System.currentTimeMillis() - t < 10L * 60L * 1000L;
    }

    /** B324: instalace skoncila (hotovo, zruseno, chyba) - zase se smi nabizet. */
    static void instalaceSkoncila() { instalaceOd = 0L; }

    private static boolean zeServeru() {
        NapServerKlient s = server;
        return s != null && s.sparovano();
    }

    // ------------------------------------------------------------------
    //  Kontrola, jestli je venku novejsi
    // ------------------------------------------------------------------
    private static Novinka zjistiNovinku() {
        if (zeServeru()) {   // B321: verze APK na serveru N&P (zip = null -> stahuje se z /api/v1/apk)
            NapServerKlient.Odpoved o = server.get("/api/v1/stav");
            if (!o.ok()) return null;
            Novinka v = new Novinka();
            v.verze = (int) NapJson.cislo(o.json, "apk_verze", 0);
            v.nazev = NapJson.text(o.json, "apk_nazev");
            v.zip = null;
            return v.verze > 0 ? v : null;
        }
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
            if (log != null) log.rekni("AKTUALIZACE " + (v.zip == null ? "na serveru N&P=" : "na webu=") + v.verze + " moje=" + moje);
            if (v.verze <= moje) return;
            a.runOnUiThread(() -> zeptejSe(a, v.verze, v.nazev, moje, v.zip, log));
        }, "nap-aktualizace").start();
    }

    /** Rucni spusteni z nabidky OPTIONS - stahne i kdyz je verze stejna. */
    public static void spustRucne(final Activity a, final Hlaska log) {
        instalaceSkoncila();   // B324: rucne = uzivatel to chce znovu (i po zaseknute instalaci)
        new Thread(() -> {
            Novinka v = zjistiNovinku();
            final boolean srv = zeServeru();
            final String zip = srv ? null : (v != null) ? v.zip : ZALOZNI_ZIP;
            final int nova = (v != null) ? v.verze : 0;
            final String nazev = (v != null) ? v.nazev : "";
            a.runOnUiThread(() -> zeptejSe(a, nova, nazev, mojeVerze(a), zip, log));
        }, "nap-aktualizace-rucne").start();
    }

    /** B321: server N&P ma novejsi appku - nabidnout (bez povoleni instalace nejdriv nastaveni). */
    public static void nabidniZeServeru(final Activity a, int nova, String nazev, int moje, final Hlaska log) {
        if (log != null) log.rekni("AKTUALIZACE na serveru N&P " + nazev + " (" + nova + "), moje " + moje);
        if (instalaceBezi()) { if (log != null) log.rekni("AKTUALIZACE uz se instaluje - znovu nenabizim (B324)"); return; }
        if (!smiInstalovat(a)) {
            try {
                new AlertDialog.Builder(a)
                        .setTitle(NapJazyk.t(a, "Nová verze appky", "New app version"))
                        .setMessage(NapJazyk.t(a, "Na serveru N&P je verze " + nova + ", ty máš " + moje
                                + ".\n\nTelefon zatím nedovoluje instalovat z této aplikace. Otevřu nastavení, kde se to zapíná,"
                                + " potom zvol v OPTIONS AKTUALIZOVAT.",
                                "The N&P server has version " + nova + ", you have " + moje
                                + ".\n\nThe phone does not allow installing from this app yet. I will open the settings where you can"
                                + " turn it on, then choose UPDATE in OPTIONS."))
                        .setPositiveButton(NapJazyk.t(a, "OTEVŘÍT", "OPEN"), (d, w) -> otevriPovoleni(a))
                        .setNegativeButton(NapJazyk.t(a, "TEĎ NE", "NOT NOW"), null).show();
            } catch (Throwable ignored) { }
            return;
        }
        zeptejSe(a, nova, nazev, moje, null, log);
    }

    private static void zeptejSe(final Activity a, int nova, String nazev, int moje,
                                 final String zip, final Hlaska log) {
        if (instalaceBezi()) { if (log != null) log.rekni("AKTUALIZACE uz se instaluje - druhy dotaz ne (B324)"); return; }
        try {
            // B314: cesky / anglicky (NapJazyk); B321: ze serveru N&P (zip == null)
            String odkud = zip == null ? NapJazyk.t(a, "Na serveru N&P", "The N&P server has")
                                       : NapJazyk.t(a, "Na atarihelp.eu", "atarihelp.eu has");
            String jmeno = nazev != null && nazev.length() > 0 ? " (" + nazev + ")" : "";
            String text = zip == null ? NapJazyk.t(a, odkud + " je verze " + nova + jmeno + ", ty máš " + moje + ".\n\n"
                    + "Stáhne se a rovnou nainstaluje – do telefonu se přitom NEULOŽÍ žádný soubor"
                    + " a uložené hry zůstanou.\n\nTelefon se ještě zeptá, jestli instalaci povolíš.",
                    odkud + " version " + nova + jmeno + ", you have " + moje + ".\n\n"
                    + "It downloads and installs right away – NO file is saved on the phone and your saves stay."
                    + "\n\nThe phone will still ask whether you allow the installation.")
                    : NapJazyk.t(a, "Na atarihelp.eu je verze " + nova
                    + ", ty mas " + moje + ".\n\n"
                    + "Stahne se a rovnou nainstaluje - do telefonu se pritom"
                    + " NEULOZI zadny soubor.\n\n"
                    + "Telefon se jeste zepta, jestli instalaci povolis.",
                    "atarihelp.eu has version " + nova
                    + ", you have " + moje + ".\n\n"
                    + "It downloads and installs right away - NO file is"
                    + " saved on the phone.\n\n"
                    + "The phone will still ask whether you allow the installation.");
            new AlertDialog.Builder(a)
                    .setTitle(NapJazyk.t(a, "Aktualizace", "Update"))
                    .setMessage(text)
                    .setPositiveButton(NapJazyk.t(a, "AKTUALIZOVAT", "UPDATE"), (d, w) -> nainstaluj(a, zip, log))
                    .setNegativeButton(NapJazyk.t(a, "TED NE", "NOT NOW"), null)
                    .show();
        } catch (Throwable t) {
            if (log != null) log.rekni("AKTUALIZACE dialog: " + t.getClass().getSimpleName());
        }
    }

    // ------------------------------------------------------------------
    //  Stazeni a instalace - bez souboru na disku
    // ------------------------------------------------------------------
    private static void nainstaluj(final Activity a, final String zipUrl, final Hlaska log) {
        if (instalaceBezi()) { if (log != null) log.rekni("AKTUALIZACE uz bezi - druhe stazeni ne (B324)"); return; }
        instalaceOd = System.currentTimeMillis();
        final AlertDialog dlg;
        final android.widget.TextView popis = new android.widget.TextView(a);
        final android.widget.ProgressBar pruh = new android.widget.ProgressBar(
                a, null, android.R.attr.progressBarStyleHorizontal);
        try {
            android.widget.LinearLayout box = new android.widget.LinearLayout(a);
            box.setOrientation(android.widget.LinearLayout.VERTICAL);
            int p = (int) (16 * a.getResources().getDisplayMetrics().density);
            box.setPadding(p * 2, p, p * 2, p);
            popis.setText(NapJazyk.t(a, "Pripojuji se...", "Connecting..."));
            popis.setTextSize(15f);
            box.addView(popis);
            pruh.setIndeterminate(true);
            box.addView(pruh);
            dlg = new AlertDialog.Builder(a).setTitle(NapJazyk.t(a, "Aktualizace", "Update"))
                    .setView(box).setCancelable(false).create();
            dlg.show();
        } catch (Throwable t) {
            instalaceSkoncila();
            if (log != null) log.rekni("AKTUALIZACE okno: " + t.getClass().getSimpleName());
            return;
        }

        new Thread(() -> {
            String vysledek;
            HttpURLConnection c = null;
            PackageInstaller.Session relace = null;
            try {
                if (Build.VERSION.SDK_INT < 21) throw new Exception("stary Android");
                final boolean primoApk = zipUrl == null;   // B321: ze serveru N&P rovnou APK (podepsany dotaz)
                if (primoApk) {
                    if (server == null || !server.sparovano()) throw new Exception(NapJazyk.t(a, "telefon není spárovaný se serverem N&P", "the phone is not paired with the N&P server"));
                    c = server.otevri("/api/v1/apk", "GET");
                    c.setReadTimeout(90000);
                } else {
                    c = (HttpURLConnection) new URL(zipUrl).openConnection();
                    c.setConnectTimeout(30000);
                    c.setReadTimeout(90000);
                    c.setRequestProperty("User-Agent", "AtariHelpEMU10");
                    c.setInstanceFollowRedirects(true);
                }
                int kod = c.getResponseCode();
                if (kod != 200) {
                    if (primoApk) {
                        java.io.InputStream e = c.getErrorStream();
                        server.zpracujChybuStahovani(kod, e == null ? "" : new String(NapServerKlient.precti(e, 65536), "UTF-8"));
                    }
                    throw new Exception("HTTP " + kod);
                }
                final long celkem = c.getContentLength();

                PackageInstaller pi = a.getPackageManager().getPackageInstaller();
                PackageInstaller.SessionParams sp = new PackageInstaller.SessionParams(
                        PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                // B324: Android 12+ - kdyz appku naposledy instalovala ona sama (aktualizace odsud),
                // dalsi aktualizace jde bez potvrzovaciho okna Androidu. Jinak se Android zepta jako drive.
                if (Build.VERSION.SDK_INT >= 31) {
                    try { sp.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED); } catch (Throwable ignored) {}
                }
                int id = pi.createSession(sp);
                relace = pi.openSession(id);

                InputStream sit = c.getInputStream();
                byte[] buf = new byte[65536];
                long staženo = 0;
                long zacatek = System.currentTimeMillis();
                boolean naslo = false;
                if (primoApk) {   // B321: APK rovnou do instalacni relace
                    naslo = true;
                    OutputStream ven = relace.openWrite("emu10", 0, celkem > 0 ? celkem : -1);
                    int n;
                    while ((n = sit.read(buf)) > 0) {
                        ven.write(buf, 0, n);
                        staženo += n;
                        final long mam = staženo;
                        a.runOnUiThread(() -> {
                            popis.setText(NapJazyk.t(a, "Instaluji přímo ze serveru N&P", "Installing straight from the N&P server"));
                            pruh.setIndeterminate(celkem <= 0);
                            if (celkem > 0) pruh.setProgress((int) (mam * 100 / celkem));
                        });
                    }
                    relace.fsync(ven);
                    ven.close();
                    sit.close();
                    if (celkem > 0 && staženo != celkem) throw new Exception(NapJazyk.t(a, "stahování se přerušilo", "the download was interrupted"));
                }
                ZipInputStream zi = primoApk ? null : new ZipInputStream(sit);
                ZipEntry e;
                while (zi != null && (e = zi.getNextEntry()) != null) {
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
                            popis.setText(NapJazyk.t(a, "Instaluji primo ze site", "Installing straight from the network"));
                            pruh.setIndeterminate(celkem <= 0);
                            if (celkem > 0) pruh.setProgress((int) (mam * 100 / celkem));
                        });
                    }
                    relace.fsync(ven);
                    ven.close();
                    break;
                }
                if (zi != null) zi.close();
                if (!naslo) throw new Exception("v ZIPu neni APK");

                Intent i = new Intent(a, MainActivity.class);
                i.setAction("eu.atarihelp.emu10.INSTALACE");
                // B324: vysledek dostane BEZICI MainActivity (onNewIntent) - bez tohohle Android
                // pokazde vytvoril novou kopii appky, ta se zeptala serveru a nabidla aktualizaci znovu
                i.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
                // B321: vysledek (a hlavne potvrzeni instalace) zpracuje MainActivity.napInstalace
                android.app.PendingIntent pending = android.app.PendingIntent.getActivity(
                        a, 0, i,
                        (Build.VERSION.SDK_INT >= 31 ? android.app.PendingIntent.FLAG_MUTABLE : 0)
                                | android.app.PendingIntent.FLAG_UPDATE_CURRENT);
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
            if (!v.startsWith("OK")) instalaceSkoncila();
            a.runOnUiThread(() -> {
                try { dlg.dismiss(); } catch (Throwable ignored) {}
                if (log != null) log.rekni("AKTUALIZACE " + v);
                if (v.startsWith("OK")) {
                    try {
                        android.widget.Toast.makeText(a, NapJazyk.t(a, "Instaluje se nová verze – appka se sama zavře a otevře znovu.",
                                "Installing the new version – the app will close and open again by itself."), android.widget.Toast.LENGTH_LONG).show();
                    } catch (Throwable ignored) {}
                }
                if (!v.startsWith("OK")) {
                    try {
                        new AlertDialog.Builder(a).setTitle(NapJazyk.t(a, "Aktualizace nevysla", "The update failed"))
                                .setMessage(v + NapJazyk.t(a, "\n\nZkus to znovu z nabidky OPTIONS.", "\n\nTry again from the OPTIONS menu."))
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
