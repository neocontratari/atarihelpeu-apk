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
        if (SEGA.equals(konzole)) {
            int r = Math.max(0, zbyvaRadku);
            return "SEGA JE ZAMČENÁ\nNapiš v Atari ještě " + r + " " + radkuSlovo(r)
                    + " programu (řádky s číslem, počítají se při RUN nebo LIST).";
        }
        if (PS1.equals(konzole)) {
            return "PS1 JE ZAMČENÁ\n" + (segaOtevrena
                    ? "Hraj na Seze ještě " + Math.max(0, zbyvaMinut) + " min."
                    : "Nejdřív odemkni Segu, pak 30 minut hraní na Seze.");
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
}
