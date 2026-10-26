package eu.atarihelp.emu10;

import java.util.Locale;

/**
 * B312: ciste rozhodovani bez Androidu - testuje se na PC v JVM (test_overeni/b312).
 *
 *  - zamky SEGA / PS1 (postup hrace, NapPostup) plati VSUDE: i pro hry spoustene z webu
 *    atarihelp.eu (odkaz, stazeni, ahgame://, AHNET.runGameUrl, AHPS1.ps1DownloadAndBoot)
 *    a pro primy skok na stranku emulatoru. Drive hlidalo jen menu.
 *  - ktere soubory ukazuje MP3 prehravac v seznamu hudby v mobilu
 *  - ktere stranky si tlacitko ZPET vezmou samy (MP3 prehravac, DZUBOX)
 */
final class NapPravidla {

    private NapPravidla() {}

    static final String SEGA = "SEGA";
    static final String PS1 = "PS1";

    private static final String ASSET = "file:///android_asset/";

    /**
     * Kterou zamykanou konzoli stranka spousti: SEGA (emu_sega a jeji vetve emu_sega_eu,
     * emu_sega_usa_stable - stare aliasy sega/, genesis/, atari/... presmeruji do emu_sega),
     * PS1 (emu_ps1), jinak null.
     */
    static String konzoleStranky(String url) {
        if (url == null) return null;
        String u = url.trim().toLowerCase(Locale.US);
        if (!u.startsWith(ASSET)) return null;
        String p = u.substring(ASSET.length());
        if (p.startsWith("emu_sega")) return SEGA;
        if (p.startsWith("emu_ps1")) return PS1;
        return null;
    }

    static boolean zamceno(String konzole, boolean segaOtevrena, boolean ps1Otevrena) {
        if (SEGA.equals(konzole)) return !segaOtevrena;
        if (PS1.equals(konzole)) return !ps1Otevrena;
        return false;
    }

    static String radkuSlovo(int n) {
        if (n == 1) return "řádek";
        if (n >= 2 && n <= 4) return "řádky";
        return "řádků";
    }

    /** Proc je zamceno - stejna slova jako u zamcene karty v hlavnim menu. */
    static String duvodZamku(String konzole, boolean segaOtevrena, int zbyvaRadku, int zbyvaMinut) {
        return duvodZamku(konzole, segaOtevrena, zbyvaRadku, zbyvaMinut, false);
    }

    /** B314: totez cesky / anglicky (hlaska, kdyz se zamcena konzole spousti z webu). */
    static String duvodZamku(String konzole, boolean segaOtevrena, int zbyvaRadku, int zbyvaMinut, boolean en) {
        String d = duvodKarty(konzole, segaOtevrena, zbyvaRadku, zbyvaMinut, en);
        if (SEGA.equals(konzole)) return (en ? "SEGA IS LOCKED\n" : "SEGA JE ZAMČENÁ\n") + d;
        if (PS1.equals(konzole)) return (en ? "PS1 IS LOCKED\n" : "PS1 JE ZAMČENÁ\n") + d;
        return "";
    }

    /** B314: duvod pod zamcenou kartou v hlavnim menu (bez nadpisu), cesky / anglicky. */
    static String duvodKarty(String konzole, boolean segaOtevrena, int zbyvaRadku, int zbyvaMinut, boolean en) {
        if (SEGA.equals(konzole)) {
            int r = Math.max(0, zbyvaRadku);
            return en ? "Type " + r + " more program " + (r == 1 ? "line" : "lines")
                            + " on the Atari (numbered lines – they count on RUN or LIST)."
                      : "Napiš v Atari ještě " + r + " " + radkuSlovo(r)
                            + " programu (řádky s číslem, počítají se při RUN nebo LIST).";
        }
        if (PS1.equals(konzole)) {
            int m = Math.max(0, zbyvaMinut);
            if (segaOtevrena) return en ? "Play the Sega for " + m + " more min." : "Hraj na Seze ještě " + m + " min.";
            return en ? "Unlock the Sega first, then play it for 30 minutes."
                      : "Nejdřív odemkni Segu, pak 30 minut hraní na Seze.";
        }
        return "";
    }

    /** Soubor s hudbou pro MP3 prehravac (i nahravky RECORD a DZUBOXU: M4A, WebM). */
    static boolean jeAudioJmeno(String name) {
        if (name == null) return false;
        String n = name.toLowerCase(Locale.US);
        return n.endsWith(".mp3") || n.endsWith(".wav") || n.endsWith(".m4a") || n.endsWith(".aac")
                || n.endsWith(".ogg") || n.endsWith(".oga") || n.endsWith(".opus") || n.endsWith(".flac")
                || n.endsWith(".webm");
    }

    /** Stranky, ktere si tlacitko ZPET muzou vzit samy (window.napZpet). */
    static boolean jeStrankaSeZpet(String url) {
        if (url == null) return false;
        return url.startsWith(ASSET + "player/") || url.startsWith(ASSET + "dj/");
    }

    private static final String NAVOD_CZ = "napoveda/index.html";
    private static final String NAVOD_EN = "napoveda/en.html";

    /**
     * B314: cil karty menu podle jazyka appky. Karta HELP vede na cesky navod
     * (napoveda/index.html, i s #kapitolou); kdyz appka mluvi anglicky, rovnou na anglicky
     * (napoveda/en.html) - cesky by se tam stejne presmeroval, takhle bez mezikroku.
     * Ostatni cile beze zmeny (stranky se prekladaji samy - nap_jazyk.js).
     */
    static String cilPodleJazyka(String cil, boolean en) {
        if (!en || cil == null) return cil;
        if (cil.startsWith(NAVOD_CZ)) return NAVOD_EN + cil.substring(NAVOD_CZ.length());
        return cil;
    }
}
