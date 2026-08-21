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

    public interface Hotovo { void hotovo(String zprava); }
    /** Prubeh stahovani - aby uzivatel videl, co se deje. */
    public interface Prubeh { void krok(String co, long staženo, long celkem, long bajtuZaSekundu); }

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

    public static boolean maBios(File korenSlozky) {
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
        try {
            String text =
                "Stahnout ze stranek atarihelp.eu?\n\n" +
                "  \u2022 Sonic the Hedgehog  (asi 0,5 MB)\n" +
                "  \u2022 BIOS pro PlayStation  (asi 1 MB)\n\n" +
                "Ulozi se do slozky Download/AtariHelp a zustanou v telefonu -" +
                " budes je moci pouzit i bez internetu.\n\n" +
                "Bez nich aplikace funguje dal, jen se v uvodnim filmu preskoci" +
                " cast se Segou a PlayStation.";
            new AlertDialog.Builder(a)
                .setTitle("Stazeni souboru")
                .setMessage(text)
                .setCancelable(true)
                .setPositiveButton("STAHNOUT", (d, w) -> {
                    zapamatuj(a);
                    stahniSPrubehem(a, korenSlozky, pak);
                })
                .setNegativeButton("TED NE", (d, w) -> {
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
        final android.widget.LinearLayout box = new android.widget.LinearLayout(a);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);
        int p = (int) (16 * a.getResources().getDisplayMetrics().density);
        box.setPadding(p * 2, p, p * 2, p);

        final android.widget.TextView popisek = new android.widget.TextView(a);
        popisek.setText("Pripojuji se k atarihelp.eu...");
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
                .setTitle("Stahuji")
                .setView(box)
                .setCancelable(false)
                .create();
        dlg.show();

        stahniNaPozadi(a, korenSlozky, (co, kolik, celkem, rychlost) -> {
            // prubeh - volano z UI vlakna
            popisek.setText(co);
            if (celkem > 0) {
                pruh.setIndeterminate(false);
                pruh.setProgress((int) (kolik * 100 / celkem));
                cisla.setText(kb(kolik) + " z " + kb(celkem)
                        + "   " + kb(rychlost) + "/s");
            } else {
                cisla.setText(kb(kolik) + "   " + kb(rychlost) + "/s");
            }
        }, zprava -> {
            try { dlg.dismiss(); } catch (Throwable ignored) {}
            String hezky = prelozVysledek(zprava);
            new AlertDialog.Builder(a)
                    .setTitle(zprava.contains("OK") ? "Stazeno" : "Nestazeno")
                    .setMessage(hezky)
                    .setCancelable(false)
                    .setPositiveButton("POKRACOVAT", (d2, w2) -> {
                        if (pak != null) pak.hotovo(zprava);
                    })
                    .show();
        });
    }

    private static String kb(long b) {
        if (b >= 1024 * 1024) return String.format(java.util.Locale.US, "%.1f MB", b / 1048576.0);
        if (b >= 1024)        return String.format(java.util.Locale.US, "%.0f kB", b / 1024.0);
        return b + " B";
    }

    /** Z technicke zpravy udela vetu, ktera dava smysl. */
    private static String prelozVysledek(String z) {
        StringBuilder v = new StringBuilder();
        boolean sonicOk = z.contains("sonic=OK");
        boolean biosOk  = z.contains("bios=OK");
        v.append(sonicOk ? "\u2713 Sonic the Hedgehog - ulozen\n"
                         : "\u2717 Sonic the Hedgehog - nestazen\n");
        v.append(biosOk  ? "\u2713 BIOS pro PlayStation - ulozen\n"
                         : "\u2717 BIOS pro PlayStation - nestazen\n");
        v.append("\nSlozka: Download/AtariHelp/emu\n");
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
                z.append(jednoStazeni(a, SONIC_URL, segaDir, "sonic",
                        ".gen,.bin,.md,.smd", "Sonic the Hedgehog", prubeh));
                z.append(' ');
                z.append(jednoStazeni(a, BIOS_URL, ps1Dir, "bios",
                        ".bin", "BIOS pro PlayStation", prubeh));
            } catch (Throwable t) {
                z.append("CHYBA ").append(t.getClass().getSimpleName());
            }
            final String zprava = z.toString();
            if (pak != null) a.runOnUiThread(() -> pak.hotovo(zprava));
        }, "nap-stahovani").start();
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
                    a.runOnUiThread(() -> prubeh.krok(hezkyNazev + " - pokus " + p2 + " ze 3",
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
                a.runOnUiThread(() -> prubeh.krok(hezkyNazev + " - rozbaluji",
                        mam, celkem, rychlost));
            }
            byte[] zip = bo.toByteArray();
            if (zip.length < 64) return popis + "=prazdne";

            int ulozeno = 0;
            ZipInputStream zi = new ZipInputStream(new java.io.ByteArrayInputStream(zip));
            try {
                ZipEntry e;
                while ((e = zi.getNextEntry()) != null) {
                    if (e.isDirectory()) continue;
                    String jm = new File(e.getName()).getName();       // bez cest ze ZIPu
                    if (jm.isEmpty() || jm.startsWith(".")) continue;
                    String male = jm.toLowerCase(java.util.Locale.US);
                    boolean sedi = false;
                    for (String p : pripony.split(",")) {
                        if (male.endsWith(p.trim())) { sedi = true; break; }
                    }
                    if (!sedi) continue;
                    File ven = new File(kam, jm);
                    FileOutputStream fo = new FileOutputStream(ven);
                    try {
                        int m;
                        while ((m = zi.read(buf)) > 0) fo.write(buf, 0, m);
                    } finally { fo.close(); }
                    ulozeno++;
                }
            } finally { try { zi.close(); } catch (Throwable ignored) {} }

            if (ulozeno == 0) return popis + "=nic-v-zipu";
            return popis + "=OK(" + ulozeno + ")";
        } catch (Throwable t) {
            return popis + "=" + t.getClass().getSimpleName();
        } finally {
            if (c != null) try { c.disconnect(); } catch (Throwable ignored) {}
        }
    }

    /** Najde stazenou ROM Segy, kdyz nejaka je. */
    public static File najdiSegaRom(File korenSlozky) {
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
