package eu.atarihelp.emu10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * BUILD2SA27: STAHOVANI SE SOUHLASEM UZIVATELE.
 *
 * Aplikace si NIC nestahuje sama od sebe. Pri prvnim spusteni se zepta,
 * rekne CO, ODKUD a KOLIK to ma, a stahuje az po odklepnuti. Kdo rekne
 * "ted ne", muze si to pustit kdykoli pozdeji z nabidky OPTIONS.
 *
 * Co se stahuje (obojí z Reneho vlastnich stranek):
 *   Sonic the Hedgehog  -> Download/AtariHelp/emu/sega
 *   BIOS pro PS1        -> Download/AtariHelp/emu/ps1
 *
 * K cemu to je:
 *   1) uzivatel ma rovnou co hrat, i bez site
 *   2) intro muze na dve tri vteriny spustit skutecne jadro Segy
 *      i PS1 - obrazovka a zvuk pak VZNIKNOU vypoctem z jeho souboru,
 *      stejne jako kdyz si hru pusti. Nic se neprehrava z aplikace.
 *
 * Kdyz se nic nestahne, intro to pozna a proste tu cast preskoci.
 */
public final class NapStahovaniSeSouhlasem {

    private NapStahovaniSeSouhlasem() {}

    public static final String SONIC_URL =
            "https://atarihelp.eu/wp-content/uploads/2026/07/Sonic-The-Hedgehog-USA-Europe.zip";
    public static final String BIOS_URL =
            "https://atarihelp.eu/wp-content/uploads/2026/07/PS1-BIOS_.zip";

    private static final String PREF = "nap_stahovani";
    private static final String KLIC_ZEPTANO = "zeptano";
    // BUILD2SA39: po stazeni si ZAPAMATUJEME PRESNE CESTY.
    // Na novejsim Androidu muze listFiles() v Download/AtariHelp vratit
    // prazdno, i kdyz tam soubory jsou - a aplikace se pak ptala porad
    // dokola. Primy pristup na znamou cestu projde.
    private static final String KLIC_SONIC = "cesta_sonic";
    private static final String KLIC_BIOS  = "cesta_bios";

    private static void zapamatujCestu(Context c, String klic, String cesta) {
        try {
            c.getSharedPreferences(PREF, Context.MODE_PRIVATE)
             .edit().putString(klic, cesta).apply();
        } catch (Throwable ignored) {}
    }
    private static File zapamatovana(Context c, String klic) {
        try {
            String p = c.getSharedPreferences(PREF, Context.MODE_PRIVATE)
                        .getString(klic, null);
            if (p == null) return null;
            File f = new File(p);
            return (f.isFile() && f.canRead() && f.length() > 1024) ? f : null;
        } catch (Throwable t) { return null; }
    }

    public interface Hotovo { void hotovo(String zprava); }
    /** Prubeh stahovani - aby uzivatel videl, co se deje. */
    public interface Prubeh { void krok(String co, long staženo, long celkem, long bajtuZaSekundu); }

    // ------------------------------------------------------------------
    //  B337: TELEVIZE (Rene: "Nejdou na TV TCL stahnout ty prvni dva soubory na zacatku")
    //  - pred stazenim povoleni uloziste (Android 9 a starsi - bez nej se do Download nezapise nic),
    //  - slozka podle appky v okamziku stazeni (bez povoleni slozka appky - MainActivity),
    //  - HTTPS na atarihelp.eu s doplnenim certifikatu jako u her (AtarihelpTls, B298 / B299),
    //  - kdyz atarihelp.eu nejde (sit vede jinam, ochrana WEDOS, certifikat): zaloha ze serveru N&P,
    //  - v okne "Nestazeno" srozumitelny duvod u kazdeho souboru (technicky zapis zustava dole).
    // ------------------------------------------------------------------
    /** Log appky (MainActivity nastavi pri startu). */
    interface Log { void log(String s); }
    static volatile Log log;
    private static void loguj(String s) {
        Log l = log;
        if (l != null) try { l.log(s); } catch (Throwable ignored) {}
    }

    /** Kam ukladat - zjisti se az po povoleni (bez povoleni slozka appky). */
    public interface Koren { File koren(); }
    /** Pred stazenim: povoleni uloziste (okno Androidu), pak 'pak' - povoleno nebo ne. */
    public interface Povoleni { void zajisti(Runnable pak); }

    /** Soubor ze zalohy (server N&P): obsah (ZIP nebo primo soubor) a jeho jmeno. */
    public static final class Zdroj {
        final byte[] data;
        final String jmeno;
        final String odkud;
        public Zdroj(byte[] data, String jmeno, String odkud) { this.data = data; this.jmeno = jmeno == null ? "" : jmeno; this.odkud = odkud; }
    }
    /**
     * Zaloha, kdyz atarihelp.eu nejde. co = "sonic" / "bios". Vrati soubor, null = na zaloze neni.
     * Chyba (server nedostupny ...) = vyjimka - jeji jmeno jde do vysledku.
     */
    public interface Zaloha { Zdroj stahni(String co) throws Exception; }

    /** Uz jsme se ptali? */
    public static boolean uzZeptano(Context c) {
        try {
            SharedPreferences p = c.getSharedPreferences(PREF, Context.MODE_PRIVATE);
            return p.getBoolean(KLIC_ZEPTANO, false);
        } catch (Throwable t) { return true; }
    }

    /**
     * MAME TO OPRAVDU NA DISKU?
     *
     * Ptat se jen na to, jestli uz jsem se ptal, NESTACI - uzivatel muze
     * slozku Download/AtariHelp/emu kdykoli smazat a aplikace by pak
     * tvrdohlave mlcela a intro by nemelo z ceho hrat.
     * Rozhoduje SOUBOR NA DISKU, ne vzpominka v nastaveni.
     */
    public static boolean maSonica(File korenSlozky) {
        return najdiSegaRom(korenSlozky) != null;
    }

    /** Kontrola i pres zapamatovanou cestu - nezavisi na vypisu adresare. */
    public static boolean maSonica(Context c, File korenSlozky) {
        if (zapamatovana(c, KLIC_SONIC) != null) return true;
        return najdiSegaRom(korenSlozky) != null;
    }
    public static boolean maBios(Context c, File korenSlozky) {
        if (zapamatovana(c, KLIC_BIOS) != null) return true;
        return maBios(korenSlozky);
    }
    public static boolean neceMChybi(Context c, File korenSlozky) {
        return !maSonica(c, korenSlozky) || !maBios(c, korenSlozky);
    }
    public static String stav(Context c, File korenSlozky) {
        File rs = zapamatovana(c, KLIC_SONIC);
        File rb = zapamatovana(c, KLIC_BIOS);
        return "sonic=" + (maSonica(c, korenSlozky)
                    ? (rs != null ? "zapamatovan" : "nalezen") : "CHYBI")
             + " bios=" + (maBios(c, korenSlozky)
                    ? (rb != null ? "zapamatovan" : "nalezen") : "CHYBI");
    }

    /** Po stazeni si ulozime, kam presne to slo. */
    public static void zapisCesty(Context c, File korenSlozky) {
        try {
            File rom = najdiSegaRom(korenSlozky);
            if (rom != null) zapamatujCestu(c, KLIC_SONIC, rom.getAbsolutePath());
            for (File k : koreny(korenSlozky)) {   // B337: i slozka appky
                File[] kde = { new File(k, "PS1_BIOS"), new File(k, "BIOS"), k };
                for (File d : kde) {
                    if (d == null || !d.isDirectory()) continue;
                    File[] deti = d.listFiles();
                    if (deti == null) continue;
                    for (File f : deti) {
                        if (f.isFile() && f.getName().toLowerCase(java.util.Locale.US).endsWith(".bin")
                                && f.length() == 524288) {
                            zapamatujCestu(c, KLIC_BIOS, f.getAbsolutePath());
                            return;
                        }
                    }
                }
            }
        } catch (Throwable ignored) {}
    }

    public static boolean maBios(File korenSlozky) {
        for (File k : koreny(korenSlozky)) if (maBiosV(k)) return true;
        return false;
    }

    private static boolean maBiosV(File korenSlozky) {
        // stejne slozky, jake prohledava ps1EnsureBios()
        File[] kde = {
            new File(korenSlozky, "PS1_BIOS"),
            new File(korenSlozky, "BIOS"),
            korenSlozky,
            new File(new File(korenSlozky, "emu"), "ps1")   // stara cesta z B137
        };
        for (File d : kde) {
            if (d == null || !d.isDirectory()) continue;
            File[] deti = d.listFiles();
            if (deti == null) continue;
            for (File f : deti) {
                if (!f.isFile()) continue;
                String m = f.getName().toLowerCase(java.util.Locale.US);
                // BIOS PS1 ma 512 kB - podle velikosti se pozna spolehlive
                if (m.endsWith(".bin") && f.length() == 524288) return true;
            }
        }
        return false;
    }

    /** Chybi neco? Pak se ma aplikace zeptat znovu, i kdyz uz se ptala. */
    public static boolean neceMChybi(File korenSlozky) {
        return !maSonica(korenSlozky) || !maBios(korenSlozky);
    }

    /** Kratky popis stavu do logu i pro uzivatele. */
    public static String stav(File korenSlozky) {
        File rom = najdiSegaRom(korenSlozky);
        return "sonic=" + (rom != null ? rom.getName() + "(" + rom.length() + "B)" : "CHYBI")
             + " bios=" + (maBios(korenSlozky) ? "je" : "CHYBI");
    }

    private static void zapamatuj(Context c) {
        try {
            c.getSharedPreferences(PREF, Context.MODE_PRIVATE)
             .edit().putBoolean(KLIC_ZEPTANO, true).apply();
        } catch (Throwable ignored) {}
    }

    /**
     * Zepta se a po odklepnuti stahne. Otazka rika presne co, odkud
     * a kolik - zadne "povolit vse" mimochodem.
     */
    public static void zeptejSeAStahni(final Activity a, final File korenSlozky,
                                       final Hotovo pak) {
        zeptejSeAStahni(a, () -> korenSlozky, null, null, pak);
    }

    /** B337: s povolenim uloziste pred stazenim, slozkou az po nem a se zalohou (server N&P). */
    public static void zeptejSeAStahni(final Activity a, final Koren koren, final Povoleni povoleni,
                                       final Zaloha zaloha, final Hotovo pak) {
        try {
            // B314: cesky / anglicky (NapJazyk)
            String text = NapJazyk.t(a,
                "Stahnout ze stranek atarihelp.eu?\n\n" +
                "  \u2022 Sonic the Hedgehog  (asi 0,5 MB)\n" +
                "  \u2022 BIOS pro PlayStation  (asi 1 MB)\n\n" +
                "Ulozi se do slozky Download/AtariHelp a zustanou v telefonu -" +
                " budes je moci pouzit i bez internetu.\n\n" +
                "Bez nich aplikace funguje dal, jen se v uvodnim filmu preskoci" +
                " cast se Segou a PlayStation.",
                "Download from atarihelp.eu?\n\n" +
                "  \u2022 Sonic the Hedgehog  (about 0.5 MB)\n" +
                "  \u2022 BIOS for PlayStation  (about 1 MB)\n\n" +
                "They are saved to the Download/AtariHelp folder and stay on the phone -" +
                " you can use them without the internet too.\n\n" +
                "The app works without them, the intro film just skips" +
                " the Sega and PlayStation part.");
            new AlertDialog.Builder(a)
                .setTitle(NapJazyk.t(a, "Stazeni souboru", "File download"))
                .setMessage(text)
                .setCancelable(true)
                .setPositiveButton(NapJazyk.t(a, "STAHNOUT", "DOWNLOAD"), (d, w) -> {
                    zapamatuj(a);
                    final Runnable stahni = () -> {
                        File k;
                        try { k = koren.koren(); } catch (Throwable t) { k = null; }
                        if (k == null) { if (pak != null) pak.hotovo("sonic=slozka-nejde bios=slozka-nejde"); return; }
                        stahniSPrubehem(a, k, zaloha, pak);
                    };
                    if (povoleni != null) povoleni.zajisti(stahni); else stahni.run();
                })
                .setNegativeButton(NapJazyk.t(a, "TED NE", "NOT NOW"), (d, w) -> {
                    zapamatuj(a);
                    if (pak != null) pak.hotovo("ODMITNUTO");
                })
                .setOnCancelListener(d -> {          // i kdyz dialog zrusi jinak
                    zapamatuj(a);
                    if (pak != null) pak.hotovo("ZRUSENO");
                })
                .show();
        } catch (Throwable t) {
            if (pak != null) pak.hotovo("DIALOG_CHYBA " + t.getClass().getSimpleName());
        }
    }

    /**
     * Stahne a UKAZUJE, co se deje - nazev souboru, kolik uz je, jak
     * rychle. Na konci rekne, co se stahlo, a teprve po odklepnuti
     * pusti dal.
     */
    public static void stahniSPrubehem(final Activity a, final File korenSlozky,
                                       final Hotovo pak) {
        stahniSPrubehem(a, korenSlozky, null, pak);
    }

    public static void stahniSPrubehem(final Activity a, final File korenSlozky, final Zaloha zaloha,
                                       final Hotovo pak) {
        final android.widget.LinearLayout box = new android.widget.LinearLayout(a);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);
        int p = (int) (16 * a.getResources().getDisplayMetrics().density);
        box.setPadding(p * 2, p, p * 2, p);

        final android.widget.TextView popisek = new android.widget.TextView(a);
        popisek.setText(NapJazyk.t(a, "Pripojuji se k atarihelp.eu...", "Connecting to atarihelp.eu..."));
        popisek.setTextSize(15f);
        box.addView(popisek);

        final android.widget.ProgressBar pruh = new android.widget.ProgressBar(
                a, null, android.R.attr.progressBarStyleHorizontal);
        pruh.setMax(100);
        pruh.setIndeterminate(true);
        android.widget.LinearLayout.LayoutParams lp =
                new android.widget.LinearLayout.LayoutParams(
                        android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                        android.widget.LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = p;
        box.addView(pruh, lp);

        final android.widget.TextView cisla = new android.widget.TextView(a);
        cisla.setTextSize(13f);
        cisla.setText(" ");
        box.addView(cisla);

        final AlertDialog dlg = new AlertDialog.Builder(a)
                .setTitle(NapJazyk.t(a, "Stahuji", "Downloading"))
                .setView(box)
                .setCancelable(false)
                .create();
        dlg.show();

        stahniNaPozadi(a, korenSlozky, zaloha, (co, kolik, celkem, rychlost) -> {
            // prubeh - volano z UI vlakna
            popisek.setText(co);
            if (celkem > 0) {
                pruh.setIndeterminate(false);
                pruh.setProgress((int) (kolik * 100 / celkem));
                cisla.setText(kb(kolik) + NapJazyk.t(a, " z ", " of ") + kb(celkem)
                        + "   " + kb(rychlost) + "/s");
            } else {
                cisla.setText(kb(kolik) + "   " + kb(rychlost) + "/s");
            }
        }, zprava -> {
            try { dlg.dismiss(); } catch (Throwable ignored) {}
            String hezky = prelozVysledek(zprava, NapJazyk.en(a), AtarihelpTls.mistniServer(10 * 60 * 1000L), popisSlozky(a, korenSlozky));
            new AlertDialog.Builder(a)
                    .setTitle(zprava.contains("OK") ? NapJazyk.t(a, "Stazeno", "Downloaded") : NapJazyk.t(a, "Nestazeno", "Not downloaded"))
                    .setMessage(hezky)
                    .setCancelable(false)
                    .setPositiveButton(NapJazyk.t(a, "POKRACOVAT", "CONTINUE"), (d2, w2) -> {
                        if (pak != null) pak.hotovo(zprava);
                    })
                    .show();
        });
    }

    /** B337: kam se uklada - Download/AtariHelp, nebo (bez povoleni uloziste) slozka appky. */
    static String popisSlozky(Context c, File koren) {
        String p = koren == null ? "" : koren.getAbsolutePath();
        int i = p.indexOf("/Android/data/");
        if (i >= 0) return p.substring(i + 1);
        i = p.indexOf("/Download/");
        return i >= 0 ? p.substring(i + 1) : (p.isEmpty() ? "Download/AtariHelp" : p);
    }

    private static String kb(long b) {
        if (b >= 1024 * 1024) return String.format(java.util.Locale.US, "%.1f MB", b / 1048576.0);
        if (b >= 1024)        return String.format(java.util.Locale.US, "%.0f kB", b / 1024.0);
        return b + " B";
    }

    /** Vysledek jedne polozky ze zpravy ("sonic=OK(1) bios=HTTP503" -> pro "bios" "HTTP503"). */
    static String polozka(String z, String co) {
        if (z == null) return "";
        int i = z.indexOf(co + "=");
        if (i < 0) return "";
        int k = z.indexOf(' ', i);
        return z.substring(i + co.length() + 1, k < 0 ? z.length() : k);
    }

    /**
     * B337: PROC se soubor nestahl - veta pro cloveka (ne "SSLHandshakeException").
     * v = vysledek polozky (jedenPokus / zaloha), mistni = popis mistniho serveru (AtarihelpTls.mistniServer) nebo null.
     */
    static String duvod(String v, String mistni, boolean en) {
        if (v == null || v.isEmpty() || v.startsWith("OK")) return "";
        final String s = v.toLowerCase(java.util.Locale.US);
        if (s.contains("slozka-nejde") || s.contains("eacces") || s.contains("filenotfound") || s.contains("zapis-nejde"))
            return en ? "the app may not save files (storage permission) - allow it: Settings -> Apps -> AtariHelp -> Permissions -> Storage"
                      : "appka nesm\u00ed ukl\u00e1dat soubory (povolen\u00ed \u00dalo\u017ei\u0161t\u011b) \u2013 povol ho: Nastaven\u00ed -> Aplikace -> AtariHelp -> Opr\u00e1vn\u011bn\u00ed -> \u00dalo\u017ei\u0161t\u011b";
        if (mistni != null && (s.contains("ssl") || s.contains("cert")))
            return en ? "on this Wi-Fi atarihelp.eu leads to a LOCAL server (" + mistni + "), not the real website"
                      : "na t\u00e9to Wi-Fi vede atarihelp.eu na M\u00cdSTN\u00cd server (" + mistni + "), ne na skute\u010dn\u00fd web";
        if (s.contains("ssl") || s.contains("cert"))
            return en ? "the secure connection to atarihelp.eu failed (certificate) - check the date and time on the device"
                      : "zabezpe\u010den\u00e9 spojen\u00ed s atarihelp.eu se nepovedlo (certifik\u00e1t) \u2013 zkontroluj datum a \u010das v za\u0159\u00edzen\u00ed";
        if (s.contains("unknownhost"))
            return en ? "atarihelp.eu was not found (no internet or DNS)" : "atarihelp.eu se nena\u0161el (internet nebo DNS)";
        if (s.contains("timeout") || s.contains("connectexception") || s.contains("noroutetohost") || s.contains("socketexception"))
            return en ? "the connection to atarihelp.eu failed or timed out" : "spojen\u00ed s atarihelp.eu se nepovedlo nebo vypr\u0161elo";
        if (s.startsWith("http"))
            return en ? "atarihelp.eu refused the download (" + v.replace("HTTP", "HTTP ") + ")"
                      : "atarihelp.eu sta\u017een\u00ed odm\u00edtl (" + v.replace("HTTP", "HTTP ") + ")";
        if (s.contains("nic-v-zipu") || s.contains("prazdne") || s.contains("zipexception"))
            return en ? "instead of the file a web page came back (website protection)" : "m\u00edsto souboru p\u0159i\u0161la webov\u00e1 str\u00e1nka (ochrana webu)";
        return en ? "error " + v : "chyba " + v;
    }

    /** Z technicke zpravy udela vetu, ktera dava smysl. B337: u nestazeneho souboru i proc. */
    static String prelozVysledek(String z, boolean en, String mistni, String slozka) {
        StringBuilder v = new StringBuilder();
        boolean sonicOk = z.contains("sonic=OK");
        boolean biosOk  = z.contains("bios=OK");
        final String ds = sonicOk ? "" : duvod(polozka(z, "sonic").split("\\+NP:")[0], mistni, en);
        final String db = biosOk ? "" : duvod(polozka(z, "bios").split("\\+NP:")[0], mistni, en);
        final boolean np = z.contains("N&P");
        if (en) {   // B314
            v.append(sonicOk ? "\u2713 Sonic the Hedgehog - saved\n" : "\u2717 Sonic the Hedgehog - not downloaded" + (ds.isEmpty() ? "" : ": " + ds) + "\n");
            v.append(biosOk  ? "\u2713 BIOS for PlayStation - saved\n" : "\u2717 BIOS for PlayStation - not downloaded" + (db.isEmpty() ? "" : ": " + db) + "\n");
            if (np) v.append("\n(from the N&P server - atarihelp.eu did not work)\n");
            v.append("\nFolder: ").append(slozka).append("\n");
            if (!sonicOk || !biosOk) {
                v.append("\nWhat did not download can be tried again in");
                v.append(" OPTIONS -> DOWNLOAD GAMES AND BIOS.");
                v.append("\n\nDetails: ").append(z);
            }
            return v.toString();
        }
        v.append(sonicOk ? "\u2713 Sonic the Hedgehog - ulozen\n"
                         : "\u2717 Sonic the Hedgehog - nestazen" + (ds.isEmpty() ? "" : ": " + ds) + "\n");
        v.append(biosOk  ? "\u2713 BIOS pro PlayStation - ulozen\n"
                         : "\u2717 BIOS pro PlayStation - nestazen" + (db.isEmpty() ? "" : ": " + db) + "\n");
        if (np) v.append("\n(ze serveru N&P - atarihelp.eu neslo)\n");
        v.append("\nSlozka: ").append(slozka).append("\n");
        if (!sonicOk || !biosOk) {
            v.append("\nCo se nestahlo, jde zkusit znovu v nabidce");
            v.append(" OPTIONS -> STAHNOUT HRY A BIOS.");
            v.append("\n\nPodrobnosti: ").append(z);
        }
        return v.toString();
    }

    /** Stazeni bez ptani - pouziva se z nabidky OPTIONS, kde si o to rekl sam. */
    public static void stahniNaPozadi(final Activity a, final File korenSlozky,
                                      final Hotovo pak) {
        stahniNaPozadi(a, korenSlozky, null, pak);
    }

    public static void stahniNaPozadi(final Activity a, final File korenSlozky,
                                      final Prubeh prubeh, final Hotovo pak) {
        stahniNaPozadi(a, korenSlozky, null, prubeh, pak);
    }

    public static void stahniNaPozadi(final Activity a, final File korenSlozky, final Zaloha zaloha,
                                      final Prubeh prubeh, final Hotovo pak) {
        new Thread(() -> {
            StringBuilder z = new StringBuilder();
            try {
                File segaDir = new File(new File(korenSlozky, "emu"), "sega");
                // BIOS MUSI JIT TAM, KDE HO PS1 HLEDA.
                // ps1EnsureBios() prohledava presne tyhle tri slozky:
                //   Download/AtariHelp
                //   Download/AtariHelp/BIOS
                //   Download/AtariHelp/PS1_BIOS
                // Do emu/ps1 se nikdy nepodiva - kdyz jsem ho tam ukladal,
                // PS1 ho nenaslo a stahovalo si vlastni znovu ze site.
                File ps1Dir  = new File(korenSlozky, "PS1_BIOS");
                final String nazevBios = NapJazyk.t(a, "BIOS pro PlayStation", "BIOS for PlayStation");
                String s = jednoStazeni(a, SONIC_URL, segaDir, "sonic", PRIPONY_SONIC, "Sonic the Hedgehog", prubeh);
                if (!s.contains("OK") && zaloha != null) s = zeZalohy(a, zaloha, "sonic", segaDir, PRIPONY_SONIC, "Sonic the Hedgehog", prubeh, s);
                z.append(s).append(' ');
                String b = jednoStazeni(a, BIOS_URL, ps1Dir, "bios", PRIPONY_BIOS, nazevBios, prubeh);
                if (!b.contains("OK") && zaloha != null) b = zeZalohy(a, zaloha, "bios", ps1Dir, PRIPONY_BIOS, nazevBios, prubeh, b);
                z.append(b);
            } catch (Throwable t) {
                z.append("CHYBA ").append(t.getClass().getSimpleName());
            }
            zapisCesty(a, korenSlozky);
            final String zprava = z.toString();
            loguj("B337 STAHOVANI SOUBORU (Sonic, BIOS) do " + korenSlozky + ": " + zprava
                    + (AtarihelpTls.mistniServer(10 * 60 * 1000L) != null ? " | atarihelp.eu na teto siti = " + AtarihelpTls.mistniServer(10 * 60 * 1000L) : ""));
            if (pak != null) a.runOnUiThread(() -> pak.hotovo(zprava));
        }, "nap-stahovani").start();
    }

    static final String PRIPONY_SONIC = ".gen,.bin,.md,.smd";
    static final String PRIPONY_BIOS = ".bin";

    /**
     * B337: atarihelp.eu nejde -> soubor ze zalohy (server N&P). predtim = vysledek z atarihelp.eu
     * (zustane ve zprave: "sonic=SSLHandshakeException+NP:neni").
     */
    static String zeZalohy(final Activity a, Zaloha zaloha, String popis, File kam, String pripony,
                                   final String hezkyNazev, final Prubeh prubeh, String predtim) {
        final String drive = predtim.startsWith(popis + "=") ? predtim.substring(popis.length() + 1) : predtim;
        if (prubeh != null) a.runOnUiThread(() -> prubeh.krok(hezkyNazev + NapJazyk.t(a, " - server N&P", " - N&P server"), 0, 0, 0));
        try {
            Zdroj zd = zaloha.stahni(popis);
            if (zd == null || zd.data == null || zd.data.length == 0) return popis + "=" + drive + "+NP:neni";
            String v = ulozSoubor(a, zd.data, zd.jmeno, kam, popis, pripony);
            if (v.contains("OK")) return v.replace(")", ",N&P)");
            return popis + "=" + drive + "+NP:" + v.substring(v.indexOf('=') + 1);
        } catch (Throwable t) {
            // zaloha hlasi kratky duvod ve zprave ("nesparovano", "server-nedostupny", "hra-404"), jinak jmeno chyby
            final String m = t.getMessage();
            return popis + "=" + drive + "+NP:" + (m != null && m.matches("[a-z0-9-]{1,40}") ? m : t.getClass().getSimpleName());
        }
    }

    /**
     * B337: soubor ze zalohy ulozit - ZIP rozbalit (jako z atarihelp.eu), jinak primo (napr. scph1001.bin).
     * @return "sonic=OK(1)" / "bios=nic-v-zipu" ...
     */
    static String ulozSoubor(Context c, byte[] data, String jmeno, File kam, String popis, String pripony) {
        try {
            if (!kam.isDirectory() && !kam.mkdirs()) return popis + "=slozka-nejde";
            final boolean zip = data.length > 4 && data[0] == 'P' && data[1] == 'K' && data[2] == 3 && data[3] == 4;
            if (zip) return ulozZeZipu(c, data, kam, popis, pripony);
            String jm = new File(jmeno == null ? "" : jmeno).getName();
            String male = jm.toLowerCase(java.util.Locale.US);
            if (jm.isEmpty() || jm.startsWith(".") || !maPriponu(male, pripony)) return popis + "=jiny-soubor";
            if ("bios".equals(popis) && data.length != 524288) return popis + "=jiny-soubor";
            File ven = new File(kam, jm);
            FileOutputStream fo = new FileOutputStream(ven);
            try { fo.write(data); } finally { fo.close(); }
            zapamatujPolozku(c, popis, ven);
            return popis + "=OK(1)";
        } catch (Throwable t) {
            return popis + "=" + jmenoChyby(t);
        }
    }

    private static boolean maPriponu(String male, String pripony) {
        for (String p : pripony.split(",")) if (male.endsWith(p.trim())) return true;
        return false;
    }

    private static void zapamatujPolozku(Context c, String popis, File ven) {
        // BUILD2SB47: cesta se zapamatuje HNED - zadne pozdejsi hadani pres listFiles()
        if ("bios".equals(popis) && ven.length() == 524288) zapamatujCestu(c, KLIC_BIOS, ven.getAbsolutePath());
        else if ("sonic".equals(popis)) zapamatujCestu(c, KLIC_SONIC, ven.getAbsolutePath());
    }

    /** B337: jmeno chyby do zpravy - u zapisu bez povoleni i EACCES (FileNotFoundException nic nerika). */
    static String jmenoChyby(Throwable t) {
        String m = t.getMessage();
        return t.getClass().getSimpleName() + (m != null && m.contains("EACCES") ? "(EACCES)" : "");
    }

    /**
     * B337: zaloha ze serveru N&P - ktery soubor ze seznamu her (/api/v1/hry) to je. co = "sonic" / "bios".
     *  sonic: hra Segy se "sonic" v nazvu souboru, prednost "Sonic The Hedgehog" (ne dily 2, 3, & Knuckles ...),
     *  bios:  soubor s "bios" v nazvu (nebo scphNNNN.bin), prednost slozka PS1.
     * @return id hry na serveru nebo null
     */
    static String vyberZeSeznamu(java.util.List<?> hry, String co) {
        if (hry == null) return null;
        String nej = null;
        int nejSkore = Integer.MIN_VALUE;
        for (Object o : hry) {
            if (!(o instanceof java.util.Map)) continue;
            java.util.Map<?, ?> h = (java.util.Map<?, ?>) o;
            final String id = h.get("id") == null ? "" : String.valueOf(h.get("id"));
            final String sys = h.get("system") == null ? "" : String.valueOf(h.get("system"));
            final String soubor = h.get("soubor") == null ? "" : String.valueOf(h.get("soubor")).toLowerCase(java.util.Locale.US);
            if (id.isEmpty() || soubor.isEmpty()) continue;
            int s;
            if ("sonic".equals(co)) {
                int i = soubor.indexOf("sonic");
                if (!"sega".equals(sys) || i < 0) continue;
                s = 10;
                if (soubor.contains("hedgehog")) s += 5;
                String za = soubor.substring(i + 5).replaceAll("\\.[a-z0-9]+$", "");
                if (za.matches(".*(^|[^a-z0-9])(2|3|ii|iii)([^a-z0-9]|$).*") || za.contains("knuckles") || za.contains("&")
                        || za.contains("spinball") || za.contains("3d") || za.contains("chaos") || za.contains("blast")) s -= 8;
            } else if ("bios".equals(co)) {
                if (!soubor.contains("bios") && !soubor.matches("scph[0-9]+\\.bin")) continue;
                s = "ps1".equals(sys) ? 10 : 5;
                if (soubor.endsWith(".zip") || soubor.endsWith(".bin")) s += 1;
            } else {
                continue;
            }
            if (s > nejSkore) { nejSkore = s; nej = id; }
        }
        return nej;
    }

    /**
     * Stahne ZIP a rozbali z nej soubory s danymi priponami.
     * @return kratka zprava do logu
     */
    /**
     * Zkusi stahnout az TRIKRAT. Mobilni sit obcas prvni spojeni odmitne
     * nebo vyprsi - Rene to videl na vlastni oci, kdy se jeden ze dvou
     * souboru nestahl. Cekaci doby jsou proto delsi a mezi pokusy je
     * pauza, at se sit stihne vzpamatovat.
     */
    private static String jednoStazeni(final Activity a, String url, File kam,
                                       String popis, String pripony,
                                       final String hezkyNazev, final Prubeh prubeh) {
        String posledni = "?";
        for (int pokus = 1; pokus <= 3; pokus++) {
            if (pokus > 1) {
                if (prubeh != null) {
                    final int p2 = pokus;
                    a.runOnUiThread(() -> prubeh.krok(hezkyNazev + NapJazyk.t(a, " - pokus " + p2 + " ze 3", " - attempt " + p2 + " of 3"),
                            0, 0, 0));
                }
                try { Thread.sleep(1500L * (pokus - 1)); } catch (Throwable ignored) {}
            }
            posledni = jedenPokus(a, url, kam, popis, pripony, hezkyNazev, prubeh);
            if (posledni.contains("OK")) return posledni;
        }
        return posledni;
    }

    private static String jedenPokus(final Activity a, String url, File kam,
                                     String popis, String pripony,
                                     final String hezkyNazev, final Prubeh prubeh) {
        HttpURLConnection c = null;
        try {
            if (!kam.isDirectory() && !kam.mkdirs()) return popis + "=slozka-nejde";
            c = (HttpURLConnection) new URL(url).openConnection();
            // B337: HTTPS jako u her z atarihelp.eu - chybejici mezilehly certifikat se dohleda (B298),
            // cizi certifikat (na Wi-Fi vede atarihelp.eu na mistni server) se pozna a hlasi (B299)
            AtarihelpTls.pouzij(c, log != null ? NapStahovaniSeSouhlasem::loguj : null);
            // Delsi cekani - mobilni sit a WordPress si obcas daji na cas.
            c.setConnectTimeout(30000);
            c.setReadTimeout(60000);
            c.setInstanceFollowRedirects(true);
            c.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 9) AtariHelpEMU10");
            int kod = c.getResponseCode();
            if (kod != 200) return popis + "=HTTP" + kod;

            final long celkem = c.getContentLength();
            InputStream in = c.getInputStream();
            ByteArrayOutputStream bo = new ByteArrayOutputStream();
            byte[] buf = new byte[32768];
            int n;
            final long zacatek = System.currentTimeMillis();
            long posledniHlaseni = 0;
            while ((n = in.read(buf)) > 0) {
                bo.write(buf, 0, n);
                if (bo.size() > 24 * 1024 * 1024) break;      // pojistka
                long ted = System.currentTimeMillis();
                if (prubeh != null && ted - posledniHlaseni > 120) {
                    posledniHlaseni = ted;
                    final long mam = bo.size();
                    long ubehlo = Math.max(1, ted - zacatek);
                    final long rychlost = mam * 1000 / ubehlo;
                    a.runOnUiThread(() -> prubeh.krok(hezkyNazev, mam, celkem, rychlost));
                }
            }
            in.close();
            if (prubeh != null) {
                final long mam = bo.size();
                long ubehlo = Math.max(1, System.currentTimeMillis() - zacatek);
                final long rychlost = mam * 1000 / ubehlo;
                a.runOnUiThread(() -> prubeh.krok(hezkyNazev + NapJazyk.t(a, " - rozbaluji", " - unpacking"),
                        mam, celkem, rychlost));
            }
            byte[] zip = bo.toByteArray();
            if (zip.length < 64) return popis + "=prazdne";
            return ulozZeZipu(a, zip, kam, popis, pripony);
        } catch (Throwable t) {
            return popis + "=" + jmenoChyby(t);
        } finally {
            if (c != null) try { c.disconnect(); } catch (Throwable ignored) {}
        }
    }

    /**
     * Rozbali ze ZIPu soubory s danymi priponami do 'kam' (B337: spolecne pro atarihelp.eu a zalohu N&P).
     * @return "sonic=OK(1)" / "bios=nic-v-zipu" / "sonic=FileNotFoundException(EACCES)" ...
     */
    static String ulozZeZipu(Context c, byte[] zip, File kam, String popis, String pripony) {
        try {
            if (!kam.isDirectory() && !kam.mkdirs()) return popis + "=slozka-nejde";
            byte[] buf = new byte[32768];
            int ulozeno = 0;
            ZipInputStream zi = new ZipInputStream(new java.io.ByteArrayInputStream(zip));
            try {
                ZipEntry e;
                while ((e = zi.getNextEntry()) != null) {
                    if (e.isDirectory()) continue;
                    String jm = new File(e.getName()).getName();       // bez cest ze ZIPu
                    if (jm.isEmpty() || jm.startsWith(".")) continue;
                    String male = jm.toLowerCase(java.util.Locale.US);
                    if (!maPriponu(male, pripony)) continue;
                    File ven = new File(kam, jm);
                    FileOutputStream fo = new FileOutputStream(ven);
                    try {
                        int m;
                        while ((m = zi.read(buf)) > 0) fo.write(buf, 0, m);
                    } finally { fo.close(); }
                    ulozeno++;
                    // BUILD2SB47: Rene - "neustale stahuje dva soubory pri
                    // spusteni apky i kdyz jsou v mobilu." Prislusna cesta
                    // (kam presne) se DOSUD zapamatovavala az DODATECNYM
                    // prohledanim adresare (zapisCesty->najdiSegaRom/
                    // listFiles) - a appka uz sama v komentarich uznava,
                    // ze listFiles() na novejsim Androidu (Scoped Storage)
                    // umi vratit prazdno, i kdyz soubory na disku jsou.
                    // Ted se cesta zapamatuje HNED, v okamziku, kdy jiste
                    // VIME, ze soubor existuje (prave jsme ho zapsali) -
                    // zadne pozdejsi hadani przez listFiles().
                    zapamatujPolozku(c, popis, ven);
                }
            } finally { try { zi.close(); } catch (Throwable ignored) {} }

            if (ulozeno == 0) return popis + "=nic-v-zipu";
            return popis + "=OK(" + ulozeno + ")";
        } catch (Throwable t) {
            return popis + "=" + jmenoChyby(t);
        }
    }

    /**
     * B337: druha slozka, kde muzou soubory byt - slozka appky (Android/data/.../files/AtariHelp), kam se
     * ukladalo bez povoleni uloziste (televize). Kdyz se povoleni da pozdeji, Sonic a BIOS se najdou i tam.
     */
    static volatile File dalsiKoren;

    private static File[] koreny(File korenSlozky) {
        File d = dalsiKoren;
        if (d == null || d.equals(korenSlozky)) return new File[] { korenSlozky };
        return new File[] { korenSlozky, d };
    }

    /** Najde stazenou ROM Segy, kdyz nejaka je. B337: i ve slozce appky (bez povoleni uloziste). */
    public static File najdiSegaRom(File korenSlozky) {
        for (File k : koreny(korenSlozky)) {
            File f = najdiSegaRomV(k);
            if (f != null) return f;
        }
        return null;
    }

    private static File najdiSegaRomV(File korenSlozky) {
        File[] kde = {
            new File(new File(korenSlozky, "emu"), "sega"),
            korenSlozky
        };
        for (File d : kde) {
            if (d == null || !d.isDirectory()) continue;
            File[] deti = d.listFiles();
            if (deti == null) continue;
            for (File f : deti) {
                String m = f.getName().toLowerCase(java.util.Locale.US);
                if (!f.isFile() || f.length() < 32768) continue;
                if (m.endsWith(".gen") || m.endsWith(".md")
                        || m.endsWith(".smd") || m.endsWith(".bin")) return f;
            }
        }
        return null;
    }
}
