package eu.atarihelp.emu10;

import android.content.pm.ActivityInfo;

/**
 * B342: OBRAZOVKA NA VYSKU / NA SIRKU - Rene (TV, B340): "nech tu apku na vysku, pridelej tlacitko u kazde
 * emu i zakladni obrazovky - sirka vyska a je to vyresene - nepis dalsi nesmyslne kody jen kvuli TV".
 *
 * Jedna obecna funkce pro telefon i televizi (cisty Java, testuje se v JVM - test_overeni/b342):
 *  - kazda SKUPINA obrazovek si pamatuje svou volbu: hlavni menu (a vse ostatni - OPTIONS, napoveda, seznamy
 *    her), Atari 130XE, Sega, PS1. Tlacitko otoceni je v hlavnim menu (C++), v kazdem emulatoru a na TV
 *    v menu konzole (ZPET); v OPTIONS radek OBRAZOVKA.
 *  - TELEFON: skutecne otoceni displeje (setRequestedOrientation). Dokud uzivatel nic nestiskl (AUTO),
 *    appka se otaci jako dosud podle telefonu (zamek otaceni v telefonu plati).
 *  - TELEVIZE se otocit neumi: VYSKA = appka jako na telefonu v pruhu 9:16 uprostred obrazovky (po stranach
 *    cerno), SIRKA = cela obrazovka (jako dosud - vychozi).
 * Stisk tlacitka = opak toho, co je ted na obrazovce.
 */
final class NapObrazovka {
    private NapObrazovka() {}

    /** skupiny obrazovek (kazda si pamatuje svou volbu) */
    static final int MENU = 0, ATARI = 1, SEGA = 2, PS1 = 3;
    static final String[] JMENA = {"menu", "atari", "sega", "ps1"};

    /** volba: AUTO = jak dosud (telefon podle sebe, TV na sirku), VYSKA, SIRKA */
    static final int AUTO = 0, VYSKA = 1, SIRKA = 2;

    /** pruh na TV: sirka : vyska (jako telefon / skiny konzoli 941 x 1672) */
    static final int PRUH_SIRKA = 9, PRUH_VYSKA = 16;

    /** Skupina podle adresy stranky (stejne jako isSegaNativeOwnerUrl / isPs1OwnerUrl / isAtariCppOwnerUrl). */
    static int skupina(String url) {
        if (url == null) return MENU;
        final String u = url.toLowerCase(java.util.Locale.US);
        if (u.startsWith("file:///android_asset/emu_atari_cpp/")) return ATARI;
        if (u.startsWith("file:///android_asset/emu_sega/")) return SEGA;
        if (u.startsWith("file:///android_asset/emu_ps1/")) return PS1;
        return MENU;
    }

    static int skupina(String jmeno, int nahrada) {
        for (int i = 0; i < JMENA.length; i++) if (JMENA[i].equals(jmeno)) return i;
        return nahrada;
    }

    static String jmeno(int skupina) {
        return skupina >= 0 && skupina < JMENA.length ? JMENA[skupina] : JMENA[MENU];
    }

    /** klic v SharedPreferences "nap_obrazovka" */
    static String klic(int skupina) {
        return "obrazovka_" + jmeno(skupina);
    }

    /** Ulozena hodnota -> platna volba (neznama = AUTO). */
    static int stav(int ulozeno) {
        return ulozeno == VYSKA || ulozeno == SIRKA ? ulozeno : AUTO;
    }

    /** Je na TV pruh 9:16 uprostred? (jen volba VYSKA; AUTO na TV = cela obrazovka jako dosud) */
    static boolean pruh(int stav, boolean tv) {
        return tv && stav == VYSKA;
    }

    /** Telefon: pozadavek na otoceni displeje (AUTO = podle telefonu jako dosud). Na TV se nepouziva. */
    static int orientace(int stav) {
        switch (stav) {
            case VYSKA: return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
            case SIRKA: return ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;   // na kteroukoli stranu (hra)
            default:    return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        }
    }

    /** Stisk tlacitka: opak toho, co je ted na obrazovce (na sirku -> VYSKA, na vysku -> SIRKA). */
    static int prepni(boolean tedNaSirku) {
        return tedNaSirku ? VYSKA : SIRKA;
    }

    /** Sirka pruhu na TV (sude cislo, nejvys cela sirka) pro obrazovku W x H. */
    static int sirkaPruhu(int W, int H) {
        if (W <= 0 || H <= 0) return Math.max(W, 0);
        int w = (int) Math.round((double) H * PRUH_SIRKA / PRUH_VYSKA);
        w += w & 1;
        return Math.min(W, Math.max(2, w));
    }

    /** Text tlacitka / polozky menu: co stisk udela. */
    static String tlacitko(boolean tedNaSirku, boolean en) {
        if (tedNaSirku) return en ? "SCREEN TO PORTRAIT" : "OBRAZOVKA NA VÝŠKU";
        return en ? "SCREEN TO LANDSCAPE" : "OBRAZOVKA NA ŠÍŘKU";
    }

    /** Popis volby (OPTIONS, log). */
    static String popis(int stav, boolean tv, boolean en) {
        switch (stav) {
            case VYSKA:
                if (tv) return en ? "portrait - a phone-shaped strip in the middle of the TV" : "na výšku - pruh jako telefon uprostřed televize";
                return en ? "portrait" : "na výšku";
            case SIRKA:
                if (tv) return en ? "landscape - the whole TV screen" : "na šířku - celá obrazovka televize";
                return en ? "landscape" : "na šířku";
            default:
                if (tv) return en ? "the whole TV screen (default)" : "celá obrazovka televize (výchozí)";
                return en ? "follows the phone (auto-rotate)" : "podle telefonu (otáčení telefonu)";
        }
    }

    /** Nazev skupiny pro lidi. */
    static String nazev(int skupina, boolean en) {
        switch (skupina) {
            case ATARI: return "ATARI 130XE";
            case SEGA:  return "SEGA";
            case PS1:   return "PS1";
            default:    return en ? "MAIN MENU" : "HLAVNÍ MENU";
        }
    }
}
