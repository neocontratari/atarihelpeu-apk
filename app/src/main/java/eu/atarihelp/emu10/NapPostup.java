package eu.atarihelp.emu10;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * BUILD2SA40: POSTUP HRÁČE. B322: logika je v NapPostupJadro (bez Androidu, testuje se
 * v JVM), tady je jen obal pro appku nad SharedPreferences "nap_postup".
 *
 * Aplikace je herní. Kdo neudělá jedno, nedostane se k dalšímu - nebo zaplatí:
 *
 *   ATARI      od začátku otevřené
 *   SEGA       otevře se po 1500 řádcích programu v Atari (B322, dřív 50) - nebo po platbě
 *   PS1        otevře se po 120 hodinách hraní Segy (B322, dřív 30 minut) - nebo po platbě,
 *              ale až po Seze
 *
 * Platbu potvrzuje správce N&P na svém serveru; appka se to dozví z /api/v1/stav
 * a pamatuje si to (hraní bez sítě). Postup se zálohuje na serveru (preinstalace).
 *
 * PRO RENEHO JE TU ZADNI VRATKA: tlačítko ODEMKNOUT DALSI, které postupně
 * odemyká jednu úroveň po druhé, aby šlo testovat bez odehrání celé cesty.
 * A ZAMKNOUT VSE, které to vrátí na začátek, aby se dal vyzkoušet pohled
 * nového hráče. B312: obě jsou v OPTIONS schované pod servisním kódem
 * (NapServis) a Java je bez otevřeného servisu odmítne.
 *
 * B312: zámky platí všude - i pro hry z webu a přímý skok na stránku
 * emulátoru (MainActivity.konzoleZamcena, NapPravidla).
 *
 * Čas se počítá jen když Sega opravdu běží - ne když je aplikace
 * na pozadí nebo v jiném emulátoru.
 */
public final class NapPostup {

    private NapPostup() {}

    private static final String PREF = "nap_postup";

    /** Kolik radku kodu otevre Segu. */
    public static final int SEGA_POTREBA_RADKU = NapPostupJadro.SEGA_POTREBA_RADKU;
    /** Kratsi program se nepocita - jinak by stacilo bouchat jeden radek. */
    public static final int NEJMENE_RADKU = NapPostupJadro.NEJMENE_RADKU;
    /** Kolik hry na Seze otevře PlayStation. */
    public static final long PS1_POTREBA_MS = NapPostupJadro.PS1_POTREBA_MS;

    private static final Object ZAMEK = new Object();

    private static NapPostupJadro j(Context c) {
        final SharedPreferences sp = c.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        return new NapPostupJadro(new NapPostupJadro.Pamet() {
            @Override public boolean ctiB(String k, boolean v) { return sp.getBoolean(k, v); }
            @Override public int ctiI(String k, int v) { return sp.getInt(k, v); }
            @Override public long ctiL(String k, long v) { return sp.getLong(k, v); }
            @Override public void zapisB(String k, boolean v) { sp.edit().putBoolean(k, v).apply(); }
            @Override public void zapisI(String k, int v) { sp.edit().putInt(k, v).apply(); }
            @Override public void zapisL(String k, long v) { sp.edit().putLong(k, v).apply(); }
        });
    }

    // ------------------------------------------------------------------
    //  Stav
    // ------------------------------------------------------------------
    public static boolean segaOdemcena(Context c) {
        try { return j(c).segaOdemcena(); } catch (Throwable t) { return true; }
    }
    public static boolean ps1Odemcena(Context c) {
        try { return j(c).ps1Odemcena(); } catch (Throwable t) { return true; }
    }
    public static boolean segaKoupena(Context c) {
        try { return j(c).segaKoupena(); } catch (Throwable t) { return false; }
    }
    public static boolean ps1Koupena(Context c) {
        try { return j(c).ps1Koupena(); } catch (Throwable t) { return false; }
    }
    public static long segaOdehranoMs(Context c) {
        try { return j(c).segaOdehranoMs(); } catch (Throwable t) { return 0L; }
    }

    /** Kolik minut Segy ještě zbývá do otevření PS1. */
    public static int zbyvaMinutSegy(Context c) {
        try { return j(c).zbyvaMinutSegy(); } catch (Throwable t) { return 0; }
    }

    // ------------------------------------------------------------------
    //  Odemykání
    // ------------------------------------------------------------------
    public static int napsanoRadku(Context c) {
        try { return j(c).napsanoRadku(); } catch (Throwable t) { return 0; }
    }
    public static int zbyvaRadku(Context c) {
        try { return j(c).zbyvaRadku(); } catch (Throwable t) { return 0; }
    }

    /**
     * Prictle davku napsanych radku. Vrací true, kdyz se prave otevrela Sega.
     *
     * Nemusi to byt jeden program - staci treba deset po peti radcich.
     * Davka kratsi nez pet radku se ale NEPOCITA, aby nestacilo psat
     * porad dokola jeden radek.
     */
    public static boolean pripocitejRadky(Context c, int kolik) {
        try { synchronized (ZAMEK) { return j(c).pripocitejRadky(kolik); } } catch (Throwable ignored) { return false; }
    }

    /**
     * B310: dávky napsaných řádků z C++ Atari ("12,7" - pořadí, jak je hráč
     * odeslal přes RUN / LIST). Řádky počítalo jen staré Atari v JavaScriptu
     * (smazané v B310), teď je počítá C++ (nap_atari_native.cpp) a tady se jen
     * každá dávka přičte stejně jako dřív. Do 'log' přidá řádky pro log appky.
     * Vrací true, když se právě otevřela Sega.
     */
    public static boolean pripocitejDavky(Context c, String davky, StringBuilder log) {
        boolean otevreno = false;
        if (davky == null || davky.isEmpty()) return false;
        for (String kus : davky.split(",")) {
            int kolik;
            try { kolik = Integer.parseInt(kus.trim()); } catch (Throwable t) { continue; }
            boolean ted = pripocitejRadky(c, kolik);
            otevreno |= ted;
            if (log != null) {
                log.append("B310 POSTUP_HRACE ATARI_RADKY +").append(kolik).append(" -> ").append(stav(c))
                   .append(ted ? "  *** SEGA PRAVE OTEVRENA ***"
                           : kolik < NEJMENE_RADKU ? " (davka pod " + NEJMENE_RADKU + " radku se nepocita)" : "")
                   .append('\n');
            }
        }
        return otevreno;
    }

    /** Přičte odehraný čas Segy a případně otevře PS1. Vrací true, když právě otevřel. */
    public static boolean pripocitejSegu(Context c, long ms) {
        try { synchronized (ZAMEK) { return j(c).pripocitejSegu(ms); } } catch (Throwable ignored) { return false; }
    }

    /** B322: záloha postupu ze serveru N&P (bere se vyšší). true = něco se tím otevřelo. */
    public static boolean slucPostup(Context c, long radku, long ms) {
        try { synchronized (ZAMEK) { return j(c).slucPostup(radku, ms); } } catch (Throwable ignored) { return false; }
    }

    /** B322: co je zaplacené podle serveru N&P. true = změna. */
    public static boolean nastavKoupeno(Context c, boolean sega, boolean ps1) {
        try { synchronized (ZAMEK) { return j(c).nastavKoupeno(sega, ps1); } } catch (Throwable ignored) { return false; }
    }

    // ------------------------------------------------------------------
    //  Zadní vrátka pro Reneho
    // ------------------------------------------------------------------
    /** Odemkne DALSI úroveň v pořadí. Vrací, co se právě otevřelo. */
    public static String odemkniDalsi(Context c) {
        try { synchronized (ZAMEK) { return j(c).odemkniDalsi(); } } catch (Throwable t) { return "CHYBA"; }
    }

    /** Vrátí vše na začátek - pohled nového hráče. */
    public static void zamkniVse(Context c) {
        try { synchronized (ZAMEK) { j(c).zamkniVse(); } } catch (Throwable ignored) {}
    }

    /** Krátký popis stavu do logu i pro uživatele. */
    public static String stav(Context c) {
        try { return j(c).stav(); } catch (Throwable t) { return "postup ?"; }
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
