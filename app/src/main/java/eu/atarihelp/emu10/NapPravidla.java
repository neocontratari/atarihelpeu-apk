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

    /** B322: zbyvajici cas Segy citelne - "118 h 47 min", "5 h", "47 min" (cesky i anglicky stejne). */
    static String casText(int minut) {
        int m = Math.max(0, minut), h = m / 60;
        if (h == 0) return m + " min";
        return h + " h" + (m % 60 > 0 ? " " + (m % 60) + " min" : "");
    }

    /**
     * B314: duvod pod zamcenou kartou v hlavnim menu (bez nadpisu), cesky / anglicky.
     * B322: Sega po 1500 radcich, PS1 po 120 hodinach Segy - nebo hned (tlacitko ODEMKNOUT
     * na karte otevre platbu, hry/odemknout.html). PS1 jde koupit az po Seze.
     */
    static String duvodKarty(String konzole, boolean segaOtevrena, int zbyvaRadku, int zbyvaMinut, boolean en) {
        if (SEGA.equals(konzole)) {
            int r = Math.max(0, zbyvaRadku);
            return en ? "Type " + r + " more program " + (r == 1 ? "line" : "lines") + " on the Atari – or unlock it now."
                      : "Napiš v Atari ještě " + r + " " + radkuSlovo(r) + " programu – nebo odemkni hned.";
        }
        if (PS1.equals(konzole)) {
            String c = casText(zbyvaMinut);
            if (segaOtevrena) return en ? "Play the Sega for " + c + " more – or unlock it now."
                                        : "Hraj na Seze ještě " + c + " – nebo odemkni hned.";
            return en ? "Unlock the Sega first, then play it for 120 hours."
                      : "Nejdřív odemkni Segu, pak 120 hodin hraní na Seze.";
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

    // ---------------------------------------------------------------- B338: Android TV, vykon
    /** B338: slabe zarizeni (TV TCL: jen 32 bitu, 4 x 1,1 GHz) - AUTO pak zapne LOW. maxMhz 0 = nezjisteno. */
    static boolean jeSlabeZarizeni(boolean bit64, int jader, int maxMhz) {
        return !bit64 || (maxMhz > 0 && maxMhz < 1500) || (jader > 0 && jader <= 2);
    }

    /**
     * B338: obraz Segy 320x224 na plose w x h na sirku - pomer stran, 94 % vysky (HUD a zivoty cele).
     * Vraci {x, y, sirka, vyska}.
     */
    static int[] segaVyrez(int w, int h) {
        final float scale = Math.min((float) w / 320f, ((float) h * 0.94f) / 224f);
        final int dw = Math.max(1, Math.round(320 * scale)), dh = Math.max(1, Math.round(224 * scale));
        return new int[]{Math.max(0, (w - dw) / 2), Math.max(0, (h - dh) / 2), dw, dh};
    }

    /** B338: JavaScript - klepnuti na tlacitko stranky konzole (i schovane na sirku - click() jde vzdy). */
    static String tvKlik(String id) {
        return "(function(){var b=document.getElementById('" + id + "');if(b&&!b.disabled){b.click();return 'ok';}return 'chybi';})()";
    }

    // ---------------- B339: uloziste, PS1 hry v televizi ----------------

    /** B339: rezerva, ktera v zarizeni po stazeni PS1 hry musi zustat volna (Android pri plnem disku zlobi). */
    static final long PS1_REZERVA = 200L * 1024 * 1024;

    /** B339: velikost pro cloveka - "2,1 GB", "650 MB", "12 kB" (cesky desetinna carka, anglicky tecka). */
    static String velikost(long b, boolean en) {
        if (b < 0) return "?";
        String s;
        if (b >= 1024L * 1024 * 1024) s = String.format(Locale.US, "%.1f GB", b / 1073741824.0);
        else if (b >= 1024L * 1024) s = String.format(Locale.US, "%.0f MB", b / 1048576.0);
        else s = String.format(Locale.US, "%.0f kB", b / 1024.0);
        return en ? s : s.replace('.', ',');
    }

    /**
     * B339 (puvodne BUILD2SA5AR): kolik volneho mista chce stazeni PS1 hry - stazeny soubor, rozbaleni ZIPu
     * (ZIP se po rozbaleni smaze) a rezerva. Velikost neznama = 1,6 GB.
     */
    static long ps1PotrebaMista(long delka) {
        return delka > 0 ? delka * 2 + PS1_REZERVA : 1600L * 1024 * 1024;
    }

    /** B339: misto na rozbaleni ZIPu (ZIP uz je na disku): soucet rozbalenych souboru + rezerva. */
    static long ps1PotrebaRozbaleni(long rozbaleno) {
        return Math.max(0L, rozbaleno) + PS1_REZERVA;
    }

    /** B339: volno 0 = nejde zjistit -> stahovani nebranit (jako drive). */
    static boolean dostMista(long volno, long potreba) {
        return volno <= 0 || volno >= potreba;
    }

    /** B339: hlaska MALO MISTA - bez "v telefonu" a bez nepravdy o automatickem uklidu (od SB14 knihovna her). */
    static String ps1MaloMista(long potreba, long volno, boolean tv, boolean en) {
        String p = velikost(potreba, en), v = velikost(volno, en);
        if (en) return "LOW STORAGE: needs ~" + p + ", free " + v + " – delete an old PS1 game: "
                + (tv ? "BACK → GAMES ON THIS TV" : "CD ISO → bin");
        return "MÁLO MÍSTA: potřeba ~" + p + ", volno " + v + " – smaž starou PS1 hru: "
                + (tv ? "ZPĚT → HRY V TELEVIZI" : "CD ISO → koš");
    }

    /**
     * B339: jmeno stazene PS1 hry v knihovne: nazev z katalogu serveru N&P, jinak jmeno souboru bez pripony;
     * u CUE slozeneho appkou (_ps1_synth.cue - ZIP s vice stopami) jmeno nejvetsiho souboru hry.
     */
    static String ps1JmenoHry(String katalog, String boot, String nejvetsi) {
        if (katalog != null && katalog.trim().length() > 0) return katalog.trim();
        String n = boot == null ? "" : boot.trim();
        if (n.toLowerCase(Locale.US).startsWith("_ps1_synth") && nejvetsi != null && nejvetsi.trim().length() > 0) n = nejvetsi.trim();
        int t = n.lastIndexOf('.');
        if (t > 0) n = n.substring(0, t);
        return n.length() > 0 ? n : "PS1";
    }

    /** B339: OPTIONS -> ULOZISTE: volno z celkem, PS1 hry v zarizeni, kolik chce stazeni PS1 hry. */
    static String ulozistePopis(long volno, long celkem, int ps1Her, long ps1Bajtu, boolean tv, boolean en) {
        if (volno <= 0 || celkem <= 0) return en ? "Storage cannot be read" : "Úložiště nejde zjistit";
        StringBuilder s = new StringBuilder();
        if (en) {
            s.append(velikost(volno, true)).append(" free of ").append(velikost(celkem, true))
             .append(" · PS1 games on this ").append(tv ? "TV" : "device").append(": ").append(ps1Her);
            if (ps1Her > 0) s.append(" (").append(velikost(ps1Bajtu, true)).append(")");
            s.append(" · downloading a PS1 game needs about 2× its size + 200 MB free");
            if (volno < 1024L * 1024 * 1024) s.append(" · LOW STORAGE – delete a PS1 game you no longer play");
        } else {
            s.append("Volno ").append(velikost(volno, false)).append(" z ").append(velikost(celkem, false))
             .append(" · PS1 hry v ").append(tv ? "televizi" : "zařízení").append(": ").append(ps1Her);
            if (ps1Her > 0) s.append(" (").append(velikost(ps1Bajtu, false)).append(")");
            s.append(" · stažení PS1 hry chce volno asi 2× její velikost + 200 MB");
            if (volno < 1024L * 1024 * 1024) s.append(" · MÁLO MÍSTA – smaž PS1 hru, kterou už nehraješ");
        }
        return s.toString();
    }
}
