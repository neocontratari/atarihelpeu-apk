package eu.atarihelp.emu10;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.Locale;

/**
 * B314: JAZYK APPKY - ČEŠTINA / ANGLIČTINA.
 *
 * René: "šlo by dát výběr jazyka - volbu Cze a Ang? Myslím tím, že by po volbě
 * na ang byla apka v angličtině? a naopak?"
 *
 * Volba je v OPTIONS (JAZYK · LANGUAGE). Dokud ji nikdo nezmění, platí jazyk
 * telefonu: čeština nebo slovenština = česky, cokoli jiného = anglicky.
 * Uloženo v SharedPreferences "nap_jazyk" (záloha Androidu je od B313 vypnutá,
 * takže nová instalace začne zase podle telefonu).
 *
 * Kdo jazyk používá:
 *   Java       NapJazyk.t(c, "česky", "English") - dialogy, hlášky, stránka TV / PC
 *   stránky    most AHJAZYK.kod() + assets/spolecne/nap_jazyk.js a slovníky assets/jazyk/
 *   C++        menu (karty.txt má anglické sloupce), Atari (hlášky na přístroji), intro
 */
public final class NapJazyk {

    private NapJazyk() {}

    public static final String CS = "cs";
    public static final String EN = "en";

    private static final String PREF = "nap_jazyk";
    private static final String K_JAZYK = "jazyk";

    private static volatile String kod = null;      // po prvnim cteni z pameti

    private static SharedPreferences p(Context c) {
        return c.getApplicationContext().getSharedPreferences(PREF, Context.MODE_PRIVATE);
    }

    /** Jazyk telefonu: cs / sk = čeština, jinak angličtina. */
    public static String podleTelefonu() {
        try {
            String j = Locale.getDefault().getLanguage();
            if (j != null) {
                j = j.toLowerCase(Locale.ROOT);
                if (j.equals("cs") || j.equals("sk")) return CS;
            }
        } catch (Throwable ignored) {}
        return EN;
    }

    /** Uložená volba, nebo null (ještě nikdo nevolil). */
    public static String ulozeny(Context c) {
        try {
            String k = p(c).getString(K_JAZYK, null);
            if (CS.equals(k) || EN.equals(k)) return k;
        } catch (Throwable ignored) {}
        return null;
    }

    /** "cs" nebo "en". */
    public static String kod(Context c) {
        String k = kod;
        if (k != null) return k;
        k = c == null ? null : ulozeny(c);
        if (k == null) k = podleTelefonu();
        kod = k;
        return k;
    }

    public static boolean en(Context c) {
        return EN.equals(kod(c));
    }

    /** Uloží volbu. true = jazyk se změnil. */
    public static boolean nastav(Context c, String novy) {
        if (!CS.equals(novy) && !EN.equals(novy)) return false;
        final String byl = kod(c);
        try { p(c).edit().putString(K_JAZYK, novy).apply(); } catch (Throwable ignored) {}
        kod = novy;
        return !novy.equals(byl);
    }

    /** Text podle jazyka: t(c, "ZAVŘÍT", "CLOSE"). */
    public static String t(Context c, String cs, String en) {
        return en(c) ? en : cs;
    }

    /** Pro log: "en (volba v OPTIONS)" / "cs (podle telefonu cs-CZ)". */
    public static String popis(Context c) {
        String u = c == null ? null : ulozeny(c);
        if (u != null) return u + " (volba v OPTIONS)";
        String tel = "?";
        try { tel = Locale.getDefault().toLanguageTag(); } catch (Throwable ignored) {}
        return kod(c) + " (podle telefonu " + tel + ")";
    }
}
