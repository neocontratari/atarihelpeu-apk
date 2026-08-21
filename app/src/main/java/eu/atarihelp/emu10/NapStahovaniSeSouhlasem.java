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

    /** Uz jsme se ptali? Ptame se jen jednou, at to neotravuje. */
    public static boolean uzZeptano(Context c) {
        try {
            SharedPreferences p = c.getSharedPreferences(PREF, Context.MODE_PRIVATE);
            return p.getBoolean(KLIC_ZEPTANO, false);
        } catch (Throwable t) { return true; }
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
                    stahniNaPozadi(a, korenSlozky, pak);
                })
                .setNegativeButton("TED NE", (d, w) -> {
                    zapamatuj(a);
                    if (pak != null) pak.hotovo("ODMITNUTO");
                })
                .show();
        } catch (Throwable t) {
            if (pak != null) pak.hotovo("DIALOG_CHYBA " + t.getClass().getSimpleName());
        }
    }

    /** Stazeni bez ptani - pouziva se z nabidky OPTIONS, kde si o to rekl sam. */
    public static void stahniNaPozadi(final Activity a, final File korenSlozky,
                                      final Hotovo pak) {
        new Thread(() -> {
            StringBuilder z = new StringBuilder();
            try {
                File segaDir = new File(new File(korenSlozky, "emu"), "sega");
                File ps1Dir  = new File(new File(korenSlozky, "emu"), "ps1");
                z.append(jednoStazeni(SONIC_URL, segaDir, "sonic", ".gen,.bin,.md,.smd"));
                z.append(' ');
                z.append(jednoStazeni(BIOS_URL, ps1Dir, "bios", ".bin"));
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
    private static String jednoStazeni(String url, File kam, String popis, String pripony) {
        HttpURLConnection c = null;
        try {
            if (!kam.isDirectory() && !kam.mkdirs()) return popis + "=slozka-nejde";
            c = (HttpURLConnection) new URL(url).openConnection();
            c.setConnectTimeout(12000);
            c.setReadTimeout(20000);
            c.setRequestProperty("User-Agent", "Mozilla/5.0 (Linux; Android 9) AtariHelpEMU10");
            int kod = c.getResponseCode();
            if (kod != 200) return popis + "=HTTP" + kod;

            InputStream in = c.getInputStream();
            ByteArrayOutputStream bo = new ByteArrayOutputStream();
            byte[] buf = new byte[32768];
            int n;
            while ((n = in.read(buf)) > 0) {
                bo.write(buf, 0, n);
                if (bo.size() > 24 * 1024 * 1024) break;      // pojistka
            }
            in.close();
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
