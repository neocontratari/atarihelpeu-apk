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
    // B327: okno aktualizace zustava otevrene od stazeni az do instalace (Rene po B326: "clovek nevi,
    // jestli se provadi aktualizace ... trva to docela dlouho, nez to pozada telefon o povoleni").
    // MainActivity.napInstalace ho podle vysledku instalace prepise nebo zavre (vysledekInstalace).
    private static volatile AlertDialog okno;
    private static volatile android.widget.TextView oknoText;
    private static volatile android.widget.ProgressBar oknoPruh;
    /** B327: ZRUSIT behem stahovani (relace se zahodi, nic se neinstaluje). */
    private static volatile boolean zrusit = false;
    /** B327: stazene APK uz je predane Androidu (tlacitko okna je pak jen ZAVRIT). */
    private static volatile boolean predano = false;
    /** B327: spojeni, ze ktereho se stahuje - ZRUSIT ho hned prerusi (necekat na dalsi data ze site). */
    private static volatile HttpURLConnection spojeni;
    /** B327: SharedPreferences - verze pri minulem startu (poStartu). */
    private static final String PREF = "nap_aktualizace";

    private static void nainstaluj(final Activity a, final String zipUrl, final Hlaska log) {
        if (instalaceBezi()) { if (log != null) log.rekni("AKTUALIZACE uz bezi - druhe stazeni ne (B324)"); return; }
        instalaceOd = System.currentTimeMillis();
        zrusit = false;
        predano = false;
        final boolean en = NapJazyk.en(a);
        final AlertDialog dlg;
        final android.widget.TextView popis = new android.widget.TextView(a);
        final android.widget.ProgressBar pruh = new android.widget.ProgressBar(
                a, null, android.R.attr.progressBarStyleHorizontal);
        try {
            android.widget.LinearLayout box = new android.widget.LinearLayout(a);
            box.setOrientation(android.widget.LinearLayout.VERTICAL);
            int p = (int) (16 * a.getResources().getDisplayMetrics().density);
            box.setPadding(p * 2, p, p * 2, p);
            popis.setText(NapJazyk.t(a, "Připojuji se k serveru…", "Connecting to the server…"));
            popis.setTextSize(15f);
            box.addView(popis);
            pruh.setIndeterminate(true);
            pruh.setMax(1000);
            box.addView(pruh);
            // B327: ZRUSIT behem stahovani; po predani Androidu je to jen ZAVRIT (okno s navodem)
            dlg = new AlertDialog.Builder(a).setTitle(NapJazyk.t(a, "Aktualizace", "Update"))
                    .setView(box).setCancelable(false)
                    .setNegativeButton(NapJazyk.t(a, "ZRUŠIT", "CANCEL"), (d, w) -> {
                        okno = null;
                        if (predano) return;
                        zrusit = true;
                        final HttpURLConnection s0 = spojeni;
                        if (s0 != null) new Thread(() -> { try { s0.disconnect(); } catch (Throwable ignored) {} }, "nap-zrusit").start();
                    })
                    .create();
            dlg.show();
            okno = dlg;
            oknoText = popis;
            oknoPruh = pruh;
        } catch (Throwable t) {
            instalaceSkoncila();
            if (log != null) log.rekni("AKTUALIZACE okno: " + t.getClass().getSimpleName());
            return;
        }

        new Thread(() -> {
            String vysledek;
            HttpURLConnection c = null;
            PackageInstaller.Session relace = null;
            long staženo = 0;
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
                spojeni = c;
                if (zrusit) throw new java.io.InterruptedIOException("ZRUSENO");
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
                long zacatek = System.currentTimeMillis(), posledniUi = 0L;
                boolean naslo = false;
                // B327: zip -> APK uvnitr (web), primo -> APK ze serveru N&P; obe cesty stejne hlaseni
                ZipInputStream zi = primoApk ? null : new ZipInputStream(sit);
                InputStream zdroj = primoApk ? sit : null;
                if (!primoApk) {
                    ZipEntry e;
                    while ((e = zi.getNextEntry()) != null) {
                        if (e.isDirectory() || !e.getName().toLowerCase(java.util.Locale.US).endsWith(".apk")) continue;
                        zdroj = zi;
                        break;
                    }
                }
                if (zdroj != null) {
                    naslo = true;
                    OutputStream ven = relace.openWrite("emu10", 0, primoApk && celkem > 0 ? celkem : -1);
                    int n;
                    while ((n = zdroj.read(buf)) > 0) {
                        if (zrusit) throw new java.io.InterruptedIOException("ZRUSENO");
                        ven.write(buf, 0, n);
                        staženo += n;
                        long ted = System.currentTimeMillis();
                        if (ted - posledniUi >= 250L) {
                            posledniUi = ted;
                            final long zaS = ted - zacatek >= 1000L ? staženo * 1000L / (ted - zacatek) : 0L;
                            final String text = NapAktualizaceTexty.stahovani(en, primoApk, staženo, celkem, zaS);
                            final int dil = celkem > 0 ? (int) Math.min(1000L, staženo * 1000L / celkem) : -1;
                            a.runOnUiThread(() -> {
                                popis.setText(text);
                                if (dil >= 0) { pruh.setIndeterminate(false); pruh.setProgress(dil); }
                            });
                        }
                    }
                    if (zrusit) throw new java.io.InterruptedIOException("ZRUSENO");
                    relace.fsync(ven);
                    ven.close();
                    if (primoApk && celkem > 0 && staženo != celkem) throw new Exception(NapJazyk.t(a, "stahování se přerušilo", "the download was interrupted"));
                }
                if (zi != null) zi.close(); else sit.close();
                if (!naslo) throw new Exception("v ZIPu neni APK");
                a.runOnUiThread(() -> { popis.setText(NapAktualizaceTexty.predavam(en)); pruh.setIndeterminate(true); });

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
                predano = true;
                relace.commit(pending.getIntentSender());
                relace = null;               // relaci uz drzi system
                vysledek = "OK (" + (staženo / 1024) + " kB, na disku nic)";
            } catch (Throwable t) {
                // B327: ZRUSIT prerusi spojeni -> chyba cteni; to neni chyba, ale zruseni
                vysledek = zrusit ? "ZRUSENO (" + (staženo / 1024) + " kB)"
                         : "CHYBA " + t.getClass().getSimpleName() + (t.getMessage() != null ? ": " + t.getMessage() : "");
            } finally {
                spojeni = null;
                if (relace != null) try { relace.abandon(); } catch (Throwable ignored) {}
                if (c != null) try { c.disconnect(); } catch (Throwable ignored) {}
            }
            final String v = vysledek;
            final long hotovo = staženo;
            if (!v.startsWith("OK")) { instalaceSkoncila(); predano = false; }
            a.runOnUiThread(() -> {
                if (log != null) log.rekni("AKTUALIZACE " + v);
                if (v.startsWith("OK")) {
                    // B327: okno zustava - rika, co prijde (potvrdit INSTALOVAT, appka se zavre, ...).
                    // Drive tu byl Toast "appka se sama zavre a otevre znovu" - to Android nedela.
                    try {
                        popis.setText(NapAktualizaceTexty.poPredani(en, Build.VERSION.SDK_INT, hotovo));
                        pruh.setIndeterminate(true);
                        android.widget.Button b = dlg.getButton(AlertDialog.BUTTON_NEGATIVE);
                        if (b != null) b.setText(NapJazyk.t(a, "ZAVŘÍT", "CLOSE"));
                    } catch (Throwable ignored) {}
                    return;
                }
                okno = null;
                try { dlg.dismiss(); } catch (Throwable ignored) {}
                if (v.startsWith("ZRUSENO")) {
                    try {
                        android.widget.Toast.makeText(a, NapJazyk.t(a, "Aktualizace zrušena – nic se nenainstalovalo.",
                                "Update cancelled – nothing was installed."), android.widget.Toast.LENGTH_LONG).show();
                    } catch (Throwable ignored) {}
                    return;
                }
                try {
                    new AlertDialog.Builder(a).setTitle(NapJazyk.t(a, "Aktualizace nevyšla", "The update failed"))
                            .setMessage(v + NapJazyk.t(a, "\n\nZkus to znovu z nabídky OPTIONS.", "\n\nTry again from the OPTIONS menu."))
                            .setPositiveButton("OK", null).show();
                } catch (Throwable ignored) {}
            });
        }, "nap-instalace").start();
    }

    /**
     * B327: vysledek instalace (MainActivity.napInstalace) - okno aktualizace podle nej.
     * STATUS_PENDING_USER_ACTION = Android se pta -> text "klepni na INSTALOVAT", okno zustava;
     * cokoli jineho (hotovo, chyba, zruseno) = okno zavrit (hlaseni dela MainActivity).
     */
    static void vysledekInstalace(Activity a, int stav) {
        final AlertDialog d = okno;
        if (stav == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            final android.widget.TextView t = oknoText;
            if (t != null) try { t.setText(NapAktualizaceTexty.potvrd(NapJazyk.en(a), Build.VERSION.SDK_INT)); } catch (Throwable ignored) {}
            return;
        }
        okno = null;
        oknoText = null;
        oknoPruh = null;
        predano = false;
        if (d != null) try { d.dismiss(); } catch (Throwable ignored) {}
    }

    /**
     * B327: start appky - prvni start po aktualizaci? Toast "Nainstalovana nova verze ..." (Android 10+
     * appku po aktualizaci sam neotevre, at clovek vi, ze to proslo) a verze se ulozi. Vraci radek
     * do logu, nebo null.
     */
    public static String poStartu(final Activity a) {
        try {
            final android.content.pm.PackageInfo pi = a.getPackageManager().getPackageInfo(a.getPackageName(), 0);
            android.content.SharedPreferences sp = a.getSharedPreferences(PREF, Context.MODE_PRIVATE);
            final int byla = sp.getInt("verze", 0);
            if (byla != pi.versionCode) sp.edit().putInt("verze", pi.versionCode).apply();
            // B328: co udelal prijimac NapPoAktualizaci (stejny soubor nastaveni) - jednou do logu, pak smazat
            final long casPrijimace = sp.getLong(NapPoAktualizaci.K_CAS, 0L);
            final String vysledekPrijimace = sp.getString(NapPoAktualizaci.K_VYSLEDEK, null);
            if (casPrijimace > 0) sp.edit().remove(NapPoAktualizaci.K_CAS).remove(NapPoAktualizaci.K_VYSLEDEK).apply();
            if (!NapAktualizaceTexty.prvniStartPoAktualizaci(byla, pi.versionCode, pi.firstInstallTime,
                    pi.lastUpdateTime, System.currentTimeMillis())) {
                return casPrijimace > 0 ? "B328 PO AKTUALIZACI " + NapAktualizaceTexty.prijimacDoLogu(casPrijimace,
                        vysledekPrijimace, System.currentTimeMillis()) : null;
            }
            // B328: okno misto Toastu - Rene po B327: "myslim, ze se ta hlaska neukazala" (Toast je 3 s dole,
            // pres volbu jazyka nebo intro snadno zapadne). Az kdyz appka nabehne (1,5 s), s tlacitkem OK.
            final boolean en = NapJazyk.en(a);
            new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
                try {
                    if (a.isFinishing()) return;
                    new AlertDialog.Builder(a).setTitle(NapAktualizaceTexty.novaVerzeTitulek(en))
                            .setMessage(NapAktualizaceTexty.novaVerze(en, pi.versionName + " (" + pi.versionCode + ")"))
                            .setPositiveButton("OK", null).show();
                } catch (Throwable ignored) {}
            }, 1500L);
            return "B328 NOVA VERZE NAINSTALOVANA " + (byla > 0 ? String.valueOf(byla) : "(do B326 neukladano)")
                    + " -> " + pi.versionCode + " " + pi.versionName + " (okno) | "
                    + NapAktualizaceTexty.prijimacDoLogu(casPrijimace, vysledekPrijimace, System.currentTimeMillis());
        } catch (Throwable t) {
            return "B328 NOVA VERZE kontrola: " + t.getClass().getSimpleName();
        }
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
