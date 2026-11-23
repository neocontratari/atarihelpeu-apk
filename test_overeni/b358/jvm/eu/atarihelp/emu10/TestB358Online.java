package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

/**
 * B358: ONLINE JEDNODUSE - Java bez Androidu.
 *  1) NapOnlineHra.klicNazvu: stejne vzorky jako stranka Segy (test_b358_stranka.py, bod 14)
 *  2) NapOnlineHra.najdiVKnihovne: hra hrace 1 z jeho telefonu -> soubor v HRY tohoto telefonu (stejny nazev),
 *     cela cesta jako v appce: nazev souboru hrace 1 -> nazevHry -> engine (Hodnota: trim, bez < > " \) -> seznam her
 *  3) NapOnlineHra.popisSeznamu: seznam her z enginu do logu
 *  4) NapOnlineHlidac: kazda zmena stavu online do logu - pripojovani, cislo hry, druhy hrac, hra bezi, konec s duvodem;
 *     novy pokus se stejnym koncem (Reneho log 23. 11.: 4 pokusy, duvod nikde), zadny spam pri zmene odezvy
 * Pouziti: java eu.atarihelp.emu10.TestB358Online VYSTUP.txt
 */
public class TestB358Online {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    /** Engine (NapServer.cs NapOnline.Hodnota): trim, bez ridicich znaku a < > " \, nejvys max znaku. */
    static String engine(String v, int max) {
        StringBuilder b = new StringBuilder();
        for (char c : v.trim().toCharArray()) if (!Character.isISOControl(c) && c != '<' && c != '>' && c != '"' && c != '\\') b.append(c);
        String s = b.toString();
        return s.length() > max ? s.substring(0, max) : s;
    }

    public static void main(String[] a) throws Exception {
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[0]), StandardCharsets.UTF_8));
        out.println("B358 - online jednoduse: Java bez Androidu (NapOnlineHra, NapOnlineHlidac)");

        // ---------------------------------------------------------------- 1) klic nazvu
        String[][] klice = {
            {"Golden Axe II  World", "golden axe ii world"},
            {"Golden_Axe_II__World_", "golden axe ii world"},
            {"Golden Axe II (World)", "golden axe ii world"},
            {"  Streets of Rage 2 ", "streets of rage 2"},
            {"Žluťoučký kůň", "lu ou k k"},
            {"_lu_ou_k__k__", "lu ou k k"},
            {"Sonic The Hedgehog (USA, Europe)", "sonic the hedgehog usa europe"},
            {"Sonic-The.Hedgehog\u3000(USA,_Europe)", "sonic the hedgehog usa europe"},
        };
        int shod = 0;
        StringBuilder sp = new StringBuilder();
        for (String[] k : klice) {
            String r = NapOnlineHra.klicNazvu(k[0]);
            if (r.equals(k[1])) shod++; else sp.append(" [").append(k[0]).append(" -> ").append(r).append("]");
        }
        over(shod == klice.length, "1) klicNazvu: " + shod + "/" + klice.length + " vzorku jako stranka Segy" + sp);
        over(NapOnlineHra.klicNazvu(null).isEmpty() && NapOnlineHra.klicNazvu("  __ ()").isEmpty(), "   prazdny nazev -> prazdny klic");
        // safeFileName appky (HRY): [^A-Za-z0-9._-] -> '_' - klic puvodniho jmena = klic jmena v HRY
        String[] puvodni = {"Golden Axe II (World).gen", "Sonic The Hedgehog (USA, Europe).gen", "Žluťoučký kůň & Co. – 2.bin", "Streets of Rage 2 (USA).md"};
        int stejne = 0;
        for (String p : puvodni) {
            String vHry = p.replaceAll("[^A-Za-z0-9._-]", "_");
            if (NapOnlineHra.klicNazvu(NapOnlineHra.nazevHry(p)).equals(NapOnlineHra.klicNazvu(NapOnlineHra.nazevHry(vHry)))) stejne++;
        }
        over(stejne == puvodni.length, "   puvodni jmeno souboru (CARTRIDGE) a jmeno v HRY (safeFileName) = stejny klic: " + stejne + "/" + puvodni.length);

        // ---------------------------------------------------------------- 2) hra z HRY
        List<String> hry = Arrays.asList("Aladdin__USA_.gen", "Golden_Axe_II__World_.gen", "Golden_Axe_II__World_.gen.state", "Sonic_The_Hedgehog__USA__Europe_.gen");
        String odHrace1 = engine(NapOnlineHra.nazevHry("Golden_Axe_II__World_.gen"), 80);   // telefon hrace 1 -> engine -> seznam her
        over("Golden Axe II  World".equals(odHrace1) && "Golden_Axe_II__World_.gen".equals(NapOnlineHra.najdiVKnihovne(hry, odHrace1)),
             "2) hra hrace 1 \"" + odHrace1 + "\" (jeho soubor Golden_Axe_II__World_.gen) -> v HRY " + NapOnlineHra.najdiVKnihovne(hry, odHrace1));
        over("Sonic_The_Hedgehog__USA__Europe_.gen".equals(NapOnlineHra.najdiVKnihovne(hry, engine(NapOnlineHra.nazevHry("Sonic The Hedgehog (USA, Europe).gen"), 80))),
             "   hrac 1 z CARTRIDGE (\"Sonic The Hedgehog (USA, Europe)\") -> v HRY Sonic_The_Hedgehog__USA__Europe_.gen");
        over(NapOnlineHra.najdiVKnihovne(hry, "Streets of Rage 2") == null && NapOnlineHra.najdiVKnihovne(hry, "") == null
             && NapOnlineHra.najdiVKnihovne(null, "Aladdin  USA") == null
             && NapOnlineHra.najdiVKnihovne(Arrays.asList("Golden_Axe_II__World_.gen.state"), "Golden Axe II  World.gen") == null,
             "   hru nema / prazdny nazev / bez HRY / ulozena pozice (.state) neni hra -> null");

        // ---------------------------------------------------------------- 3) seznam do logu
        String json = "{\"ok\":true,\"mistnosti\":[{\"kod\":\"1234\",\"system\":\"sega\",\"hra\":\"Golden Axe \\\"II\\\"\",\"hra_id\":\"\",\"otisk\":\"aa\","
                + "\"nastaveni\":\"16\",\"hrac\":\"René\",\"ceka_s\":40},{\"kod\":\"5555\",\"system\":\"ps1\",\"hra\":\"x\",\"hra_id\":\"\",\"otisk\":\"bb\","
                + "\"nastaveni\":\"0\",\"hrac\":\"y\",\"ceka_s\":1}],\"server_cas\":1795000000}";
        over("2 (sega 1234, ps1 5555)".equals(NapOnlineHra.popisSeznamu(json)) && "0".equals(NapOnlineHra.popisSeznamu("{\"ok\":true,\"mistnosti\":[],\"server_cas\":1}"))
             && "?".equals(NapOnlineHra.popisSeznamu(null)),
             "3) seznam her do logu: \"" + NapOnlineHra.popisSeznamu(json) + "\", prazdny \"0\"");

        // ---------------------------------------------------------------- 4) hlidac stavu
        NapOnlineHlidac h = new NapOnlineHlidac();
        String z0 = h.zmena("krok=\nfaze=KONEC\nchyba=server neodpovedel (timeout)\nstart=3");
        String z0b = h.zmena("krok=\nfaze=KONEC\nchyba=server neodpovedel (timeout)\nstart=3");
        over(z0 == null && z0b == null && h.hotovo(), "4) hlidac: stav pred pokusem si jen zapamatuje (zadny stary KONEC do logu), stejny stav = nic");
        String z1 = h.zmena("krok=\nfaze=PRIPOJUJE\nstart=4");
        String z2 = h.zmena("krok=\nfaze=CEKA_NA_HRACE\nkod=\nstart=4");
        String z3 = h.zmena("krok=\nfaze=CEKA_NA_HRACE\nkod=1234\nstart=4");
        String z3b = h.zmena("krok=\nfaze=CEKA_NA_HRACE\nkod=1234\nstart=4\nsnimek=999");
        over("B358 ONLINE pripojuji se k enginu N&P (pokus 4)".equals(z1) && "B358 ONLINE spojeno s enginem, zakladam hru".equals(z2)
             && "B358 ONLINE ceka na hrace 2 - cislo hry 1234".equals(z3) && z3b == null && !h.hotovo(),
             "   " + z1 + " | " + z2 + " | " + z3);
        String z4 = h.zmena("faze=HRAJE\nkod=1234\ndruhy=bracha\nhraje=0\nstart=4");
        String z5 = h.zmena("faze=HRAJE\nkod=1234\ndruhy=bracha\nhraje=1\nodezva=23\nzpozdeni=3\nstart=4");
        String z5b = h.zmena("faze=HRAJE\nkod=1234\ndruhy=bracha\nhraje=1\nodezva=31\nzpozdeni=3\nstart=4");
        over("B358 ONLINE domlouva se s hracem bracha".equals(z4) && "B358 ONLINE HRAJE s bracha - odezva 23 ms, zpozdeni 3 snimku".equals(z5) && z5b == null,
             "   " + z4 + " | " + z5 + " | zmena odezvy = nic");
        String z6 = h.zmena("faze=KONEC\nkod=1234\ndruhy=bracha\nchyba=druhy hrac odesel\nkonec=druhy hrac odesel\novereno=12\nsnimek=720\nstart=4");
        over("B358 ONLINE KONEC (pokus 4): druhy hrac odesel | overeno 12 otisku, snimek 720".equals(z6) && h.hotovo(), "   " + z6);
        String z7 = h.zmena("faze=KONEC\nchyba=druhy hrac odesel\nkonec=druhy hrac odesel\novereno=0\nsnimek=0\nstart=5");
        over("B358 ONLINE KONEC (pokus 5): druhy hrac odesel | overeno 0 otisku, snimek 0".equals(z7),
             "   novy pokus skoncil hned a stejne (pripojeni nestihl hlidac uvidet) -> pozna se podle cisla pokusu: " + z7);
        String z8 = h.zmena("krok=STAHUJI\nfaze=KONEC\nchyba=druhy hrac odesel\nstart=5");
        boolean hot8 = h.hotovo();
        String z9 = h.zmena("krok=SPOUSTIM\nfaze=KONEC\nchyba=druhy hrac odesel\nstart=5");
        over("B358 ONLINE stahuji hru hrace 1 ze serveru N&P".equals(z8) && !hot8 && "B358 ONLINE spoustim stejnou hru z HRY".equals(z9),
             "   kroky appky pred spojenim: " + z8 + " | " + z9 + " (hlidani bezi dal)");
        StringBuilder dlouhy = new StringBuilder("engine odmitl online (HTTP 400): {\"ok\":false,\"zprava\":\"radek1\nradek2\"}");
        while (dlouhy.length() < 600) dlouhy.append('x');
        String z10 = h.zmena("krok=\nfaze=KONEC\nchyba=" + dlouhy.toString().replace("\n", "\\n") + "\nstart=6");
        String z11 = h.zmena("krok=\nfaze=KONEC\nchyba=ignoruje se\nkonec=nahrana jina hra - online skoncilo\novereno=0\nsnimek=0\nstart=7");
        over(z10 != null && z10.startsWith("B358 ONLINE KONEC (pokus 6): engine odmitl online (HTTP 400): {") && z10.indexOf('\n') < 0 && z10.contains("...")
             && z10.length() < 520 && "B358 ONLINE KONEC (pokus 7): nahrana jina hra - online skoncilo | overeno 0 otisku, snimek 0".equals(z11),
             "   duvod s odpovedi enginu jednim radkem a zkraceny (" + (z10 == null ? 0 : z10.length()) + " znaku); 'konec' ma prednost pred 'chyba'");
        String z12 = h.zmena("krok=\nfaze=HRAJE\ndruhy=Zlý\u0007<hráč>\nhraje=1\nodezva=5\nzpozdeni=2\nstart=8");
        over("B358 ONLINE HRAJE s Zlýhráč - odezva 5 ms, zpozdeni 2 snimku".equals(z12), "   jmeno druheho hrace bez ridicich znaku a < > do logu: " + z12);

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
