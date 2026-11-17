package eu.atarihelp.emu10;

import android.content.pm.ActivityInfo;

import java.io.FileOutputStream;

/**
 * B342: OBRAZOVKA NA VYSKU / NA SIRKU (JVM, skutecna trida appky NapObrazovka; konstanty ActivityInfo z android-34.jar).
 *  S1) skupina podle adresy stranky: Atari C++, Sega (i jeji pomocne stranky), PS1, vse ostatni = hlavni menu
 *      (OPTIONS, napoveda, seznamy her, web) - stejne hranice jako isAtariCppOwnerUrl / isSegaNativeOwnerUrl / isPs1OwnerUrl,
 *  S2) ulozena hodnota -> volba (nezname = AUTO), klice v SharedPreferences, jmena skupin tam i zpet,
 *  S3) telefon: AUTO = UNSPECIFIED (jako dosud), VYSKA = PORTRAIT, SIRKA = SENSOR_LANDSCAPE (na kteroukoli stranu),
 *  S4) stisk tlacitka = opak toho, co je na obrazovce (i z AUTO),
 *  S5) TV: pruh jen pro VYSKA (AUTO a SIRKA = cela obrazovka), telefon nikdy pruh,
 *  S6) pruh 9:16 - sirka pro bezne televize (sude cislo, na stred, nikdy sirsi nez obrazovka),
 *  S7) texty tlacitka a popisu CZ / EN (TV popis rika "pruh").
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b342_java.txt
 */
public class TestB342 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    public static void main(String[] a) throws Exception {
        String hl = "B342 - obrazovka na vysku / na sirku (NapObrazovka)\n";
        out.append(hl).append('\n');
        System.out.println(hl);

        // S1
        final String A = "file:///android_asset/";
        final Object[][] url = {
                {A + "emu_atari_cpp/index.html", NapObrazovka.ATARI},
                {A + "EMU_ATARI_CPP/index.html?x=1", NapObrazovka.ATARI},
                {A + "emu_sega/index.html", NapObrazovka.SEGA},
                {A + "emu_sega/sega_poweroff.html", NapObrazovka.SEGA},
                {A + "emu_ps1/index.html#hra", NapObrazovka.PS1},
                {A + "index.html", NapObrazovka.MENU},
                {A + "napoveda/index.html", NapObrazovka.MENU},
                {A + "hry/odemknout.html?co=sega", NapObrazovka.MENU},
                {A + "hry/jazyk.html", NapObrazovka.MENU},
                {A + "emu_sega_eu/index.html", NapObrazovka.MENU},      // stary alias - Sega tam nebezi (isSegaNativeOwnerUrl taky ne)
                {"https://atarihelp.eu/", NapObrazovka.MENU},
                {"about:blank", NapObrazovka.MENU},
                {null, NapObrazovka.MENU},
        };
        boolean s1 = true;
        StringBuilder spatne = new StringBuilder();
        for (Object[] u : url) {
            int sk = NapObrazovka.skupina((String) u[0]);
            if (sk != (Integer) u[1]) { s1 = false; spatne.append(' ').append(u[0]).append("->").append(sk); }
        }
        over(s1, "S1) skupina podle stranky: Atari / Sega (i pomocne stranky) / PS1, ostatni = hlavni menu (" + url.length + " adres)" + spatne);

        // S2
        over(NapObrazovka.stav(0) == NapObrazovka.AUTO && NapObrazovka.stav(1) == NapObrazovka.VYSKA && NapObrazovka.stav(2) == NapObrazovka.SIRKA
                        && NapObrazovka.stav(-1) == NapObrazovka.AUTO && NapObrazovka.stav(7) == NapObrazovka.AUTO,
                "S2a) ulozena hodnota -> volba (nezname cislo = AUTO, jako dosud)");
        boolean s2 = true;
        java.util.HashSet<String> klice = new java.util.HashSet<>();
        for (int i = 0; i < NapObrazovka.JMENA.length; i++) {
            s2 &= NapObrazovka.skupina(NapObrazovka.jmeno(i), -1) == i;
            klice.add(NapObrazovka.klic(i));
        }
        over(s2 && klice.size() == 4 && NapObrazovka.klic(NapObrazovka.SEGA).equals("obrazovka_sega")
                        && NapObrazovka.skupina("nesmysl", NapObrazovka.MENU) == NapObrazovka.MENU && NapObrazovka.jmeno(9).equals("menu"),
                "S2b) 4 skupiny, kazda vlastni klic (obrazovka_menu / _atari / _sega / _ps1), jmena tam i zpet");

        // S3
        over(NapObrazovka.orientace(NapObrazovka.AUTO) == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
                        && NapObrazovka.orientace(NapObrazovka.VYSKA) == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
                        && NapObrazovka.orientace(NapObrazovka.SIRKA) == ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE,
                "S3) telefon: AUTO = UNSPECIFIED (" + ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED + "), VYSKA = PORTRAIT ("
                        + ActivityInfo.SCREEN_ORIENTATION_PORTRAIT + "), SIRKA = SENSOR_LANDSCAPE (" + ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE + ")");

        // S4
        over(NapObrazovka.prepni(true) == NapObrazovka.VYSKA && NapObrazovka.prepni(false) == NapObrazovka.SIRKA,
                "S4) stisk = opak toho, co je na obrazovce (na sirku -> VYSKA, na vysku -> SIRKA; plati i z AUTO)");

        // S5
        over(NapObrazovka.pruh(NapObrazovka.VYSKA, true) && !NapObrazovka.pruh(NapObrazovka.AUTO, true) && !NapObrazovka.pruh(NapObrazovka.SIRKA, true)
                        && !NapObrazovka.pruh(NapObrazovka.VYSKA, false) && !NapObrazovka.pruh(NapObrazovka.SIRKA, false),
                "S5) TV: pruh jen pri NA VYSKU (AUTO = cela obrazovka jako dosud); telefon pruh nikdy");
        // TV: dvakrat stisk = zpet (pruh -> cela -> pruh)
        boolean tvKolo = true;
        boolean pruh = false;
        for (int i = 0; i < 4; i++) {
            int novy = NapObrazovka.prepni(!pruh);
            boolean p2 = NapObrazovka.pruh(novy, true);
            tvKolo &= p2 != pruh;
            pruh = p2;
        }
        over(tvKolo, "S5b) TV: kazdy stisk prepne cela obrazovka <-> pruh (4x)");

        // S6
        final int[][] obr = {{1920, 1080, 608}, {1280, 720, 406}, {3840, 2160, 1216}, {1366, 768, 432}, {960, 540, 304}};
        boolean s6 = true;
        StringBuilder det = new StringBuilder();
        for (int[] o : obr) {
            int w = NapObrazovka.sirkaPruhu(o[0], o[1]);
            det.append(' ').append(o[0]).append('x').append(o[1]).append("->").append(w);
            s6 &= w == o[2] && (w & 1) == 0 && w <= o[0] && Math.abs((double) w / o[1] - 9.0 / 16) < 0.004;
        }
        over(s6, "S6a) pruh 9:16 pro TV (sude cislo):" + det);
        over(NapObrazovka.sirkaPruhu(500, 1000) == 500 && NapObrazovka.sirkaPruhu(0, 0) == 0 && NapObrazovka.sirkaPruhu(1, 1080) == 1
                        && NapObrazovka.sirkaPruhu(1920, 1) == 2,
                "S6b) nikdy sirsi nez obrazovka (uzka / nulova plocha neprekroci sirku)");

        // S7
        over(NapObrazovka.tlacitko(true, false).equals("OBRAZOVKA NA VÝŠKU") && NapObrazovka.tlacitko(false, false).equals("OBRAZOVKA NA ŠÍŘKU")
                        && NapObrazovka.tlacitko(true, true).equals("SCREEN TO PORTRAIT") && NapObrazovka.tlacitko(false, true).equals("SCREEN TO LANDSCAPE"),
                "S7a) polozka menu konzole na TV: co stisk udela (CZ / EN)");
        boolean s7 = NapObrazovka.popis(NapObrazovka.VYSKA, true, false).contains("pruh")
                && NapObrazovka.popis(NapObrazovka.VYSKA, true, true).contains("strip")
                && NapObrazovka.popis(NapObrazovka.AUTO, false, false).contains("podle telefonu")
                && NapObrazovka.popis(NapObrazovka.AUTO, true, false).contains("celá obrazovka")
                && NapObrazovka.nazev(NapObrazovka.MENU, false).equals("HLAVNÍ MENU") && NapObrazovka.nazev(NapObrazovka.MENU, true).equals("MAIN MENU")
                && NapObrazovka.nazev(NapObrazovka.PS1, true).equals("PS1");
        over(s7, "S7b) popis volby (OPTIONS, potvrzeni): TV na vysku = pruh, AUTO na telefonu = podle telefonu, na TV = cela obrazovka");

        String v = "VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA";
        out.append(v).append('\n');
        System.out.println(v);
        try (FileOutputStream f = new FileOutputStream("VYSTUP_test_b342_java.txt")) { f.write(out.toString().getBytes("UTF-8")); }
        if (chyb > 0) System.exit(1);
    }
}
