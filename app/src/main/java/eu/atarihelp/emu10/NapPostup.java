package eu.atarihelp.emu10;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * BUILD2SA40: POSTUP HRAČE.
 *
 * Aplikace je herní. Kdo neudělá jedno, nedostane se k dalšímu:
 *
 *   ATARI      od začátku otevřené
 *   SEGA       otevře se, až hráč napíše ten 50řádkový kód v Atari
 *   PS1        otevře se, až hráč odehraje 30 minut Segy
 *
 * Nápověda ke kódu je v úvodním filmu - proto se tam píše celý listing.
 *
 * PRO RENEHO JE TU ZADNI VRATKA: v nabídce OPTIONS je tlačítko
 * ODEMKNOUT DALSI, které postupně odemyká jednu úroveň po druhé,
 * aby šlo testovat bez odehrání celé cesty. A ZAMKNOUT VSE, které
 * to vrátí na začátek, aby se dal vyzkoušet pohled nového hráče.
 *
 * Čas se počítá jen když Sega opravdu běží - ne když je aplikace
 * na pozadí nebo v jiném emulátoru.
 */
public final class NapPostup {

    private NapPostup() {}

    private static final String PREF = "nap_postup";
    private static final String K_SEGA   = "sega_odemcena";
    private static final String K_PS1    = "ps1_odemcena";
    private static final String K_MINUTY = "sega_odehrano_ms";
    private static final String K_RADKY  = "napsano_radku";

    /** Kolik radku kodu otevre Segu. */
    public static final int SEGA_POTREBA_RADKU = 50;
    /** Kratsi program se nepocita - jinak by stacilo bouchat jeden radek. */
    public static final int NEJMENE_RADKU = 5;

    /** Kolik hry na Seze otevře PlayStation. */
    public static final long PS1_POTREBA_MS = 30L * 60L * 1000L;

    private static SharedPreferences p(Context c) {
        return c.getSharedPreferences(PREF, Context.MODE_PRIVATE);
    }

    // ------------------------------------------------------------------
    //  Stav
    // ------------------------------------------------------------------
    public static boolean segaOdemcena(Context c) {
        try { return p(c).getBoolean(K_SEGA, false); } catch (Throwable t) { return true; }
    }
    public static boolean ps1Odemcena(Context c) {
        try { return p(c).getBoolean(K_PS1, false); } catch (Throwable t) { return true; }
    }
    public static long segaOdehranoMs(Context c) {
        try { return p(c).getLong(K_MINUTY, 0L); } catch (Throwable t) { return 0L; }
    }

    /** Kolik minut Segy ještě zbývá do otevření PS1. */
    public static int zbyvaMinutSegy(Context c) {
        long zbyva = PS1_POTREBA_MS - segaOdehranoMs(c);
        if (zbyva <= 0) return 0;
        return (int) ((zbyva + 59999L) / 60000L);
    }

    // ------------------------------------------------------------------
    //  Odemykání
    // ------------------------------------------------------------------
    public static int napsanoRadku(Context c) {
        try { return p(c).getInt(K_RADKY, 0); } catch (Throwable t) { return 0; }
    }
    public static int zbyvaRadku(Context c) {
        int z = SEGA_POTREBA_RADKU - napsanoRadku(c);
        return z < 0 ? 0 : z;
    }

    /**
     * Prictle davku napsanych radku. Vrací true, kdyz se prave otevrela Sega.
     *
     * Nemusi to byt jeden program - staci treba deset po peti radcich.
     * Davka kratsi nez pet radku se ale NEPOCITA, aby nestacilo psat
     * porad dokola jeden radek.
     */
    public static boolean pripocitejRadky(Context c, int kolik) {
        try {
            if (kolik < NEJMENE_RADKU) return false;
            int bylo = napsanoRadku(c);
            int ted = bylo + kolik;
            p(c).edit().putInt(K_RADKY, ted).apply();
            if (bylo < SEGA_POTREBA_RADKU && ted >= SEGA_POTREBA_RADKU) {
                otevriSegu(c);
                return true;
            }
        } catch (Throwable ignored) {}
        return false;
    }

    /** Hráč zvládl kód v Atari. */
    public static void otevriSegu(Context c) {
        try { p(c).edit().putBoolean(K_SEGA, true).apply(); } catch (Throwable ignored) {}
    }

    /** Přičte odehraný čas Segy a případně otevře PS1. Vrací true, když právě otevřel. */
    public static boolean pripocitejSegu(Context c, long ms) {
        try {
            if (ms <= 0) return false;
            long bylo = segaOdehranoMs(c);
            long ted = bylo + ms;
            p(c).edit().putLong(K_MINUTY, ted).apply();
            if (bylo < PS1_POTREBA_MS && ted >= PS1_POTREBA_MS) {
                p(c).edit().putBoolean(K_PS1, true).apply();
                return true;
            }
        } catch (Throwable ignored) {}
        return false;
    }

    // ------------------------------------------------------------------
    //  Zadní vrátka pro Reneho
    // ------------------------------------------------------------------
    /**
     * Odemkne DALSI úroveň v pořadí. Vrací, co se právě otevřelo.
     * Postupně, ne všechno naráz - ať se dá vyzkoušet každý krok zvlášť.
     */
    public static String odemkniDalsi(Context c) {
        if (!segaOdemcena(c)) {
            otevriSegu(c);
            try { p(c).edit().putInt(K_RADKY, SEGA_POTREBA_RADKU).apply(); } catch (Throwable ignored) {}
            return "SEGA";
        }
        if (!ps1Odemcena(c)) {
            try {
                p(c).edit().putBoolean(K_PS1, true)
                           .putLong(K_MINUTY, PS1_POTREBA_MS).apply();
            } catch (Throwable ignored) {}
            return "PS1";
        }
        return "UZ JE VSE OTEVRENE";
    }

    /** Vrátí vše na začátek - pohled nového hráče. */
    public static void zamkniVse(Context c) {
        try {
            p(c).edit().putBoolean(K_SEGA, false)
                       .putBoolean(K_PS1, false)
                       .putInt(K_RADKY, 0)
                       .putLong(K_MINUTY, 0L).apply();
        } catch (Throwable ignored) {}
    }

    /** Krátký popis stavu do logu i pro uživatele. */
    public static String stav(Context c) {
        long ms = segaOdehranoMs(c);
        return "radku=" + napsanoRadku(c) + "/" + SEGA_POTREBA_RADKU
             + " sega=" + (segaOdemcena(c) ? "otevrena" : "ZAMCENA")
             + " ps1=" + (ps1Odemcena(c) ? "otevrena" : "ZAMCENA")
             + " odehrano=" + (ms / 60000L) + "min";
    }

    // ------------------------------------------------------------------
    //  Kontrola kódu z Atari
    // ------------------------------------------------------------------
    /**
     * Pozná, jestli hráč napsal ten správný program.
     *
     * Neporovnává se znak po znaku - to by bylo k vzteku. Hledá se, jestli
     * jsou v napsaném textu klíčové kusy: nahrání strojáku z DATA, volání
     * přes USR a aspoň polovina bajtů té rutiny.
     */
    public static boolean kodSedi(String napsano) {
        if (napsano == null) return false;
        String t = napsano.toUpperCase(java.util.Locale.US).replace(" ", "");
        if (t.indexOf("USR(1536)") < 0) return false;
        if (t.indexOf("POKE1536+I,A") < 0) return false;
        if (t.indexOf("GRAPHICS0") < 0) return false;
        // aspon polovina bajtu rutiny
        String[] kus = { "104,173,48,2", "133,203", "9,128", "192,29",
                         "141,0,2", "169,7", "32,92,228", "169,192",
                         "173,11,212", "41,240", "141,10,212", "141,24,208",
                         "76,98,228" };
        int sedi = 0;
        for (String k : kus) if (t.indexOf(k) >= 0) sedi++;
        return sedi >= kus.length / 2;
    }
}
