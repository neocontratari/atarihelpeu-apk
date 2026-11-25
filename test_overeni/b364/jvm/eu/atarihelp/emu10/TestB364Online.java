package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;

/**
 * B364: ZALOZIT HRU s hrou z naseho serveru N&P (NapOnlineHra bez Androidu). Rene 25. 11.: "kdyz nekdo zaklada hru
 * jako novacek - at nevybira ze sveho mobilu ale at najede na nas engine nase hry" a "je docela slozite pochopit
 * v atari jak nastavit pro dva".
 *  1) hry na serveru (/api/v1/hry): jen Atari (Sega / PS1 ne), id "atari-" + 16 hex, bez BIOSu, serazene podle
 *     nazvu bez diakritiky (Čáp mezi C a D), prazdny nazev = jmeno souboru, spatny JSON = prazdny seznam
 *  2) onlineHra z C++ ma 4. radek gen (kolikaty stroj); stara 3radkova odpoved dal funguje (gen "")
 *  3) hra ze serveru se spustila: gen se zmenil a Atari je zapnute (vypnute / stejny stroj = jeste ne)
 *  4) bezi v Atari ta hra? jmeno souboru jako ze serveru: '_' = mezera, diakritika z C++ jako "??" (pcAscii po
 *     bajtech) = stejne, jina hra ne, ZIP / RAR = cokoli krome ATARI BASICu, dlouhe jmeno useknute v C++ na 120 B
 *  5) cislo zalozene hry z radku hlidace (okno HRA ZALOZENA) - jen "ceka na hrace 2 - cislo hry NNNN"
 * Pouziti: java eu.atarihelp.emu10.TestB364Online VYSTUP.txt
 */
public class TestB364Online {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    static String nazvy(List<String[]> h) {
        StringBuilder b = new StringBuilder();
        for (String[] x : h) b.append(b.length() > 0 ? " | " : "").append(x[1]);
        return b.toString();
    }

    /** jako C++ nap::pcAscii(jm, 120, true): bajty UTF-8 mimo ASCII = '?', nejvys 120 bajtu */
    static String pcAscii(String s) {
        StringBuilder b = new StringBuilder();
        for (byte x : s.getBytes(StandardCharsets.UTF_8)) {
            if (b.length() >= 120) break;
            int c = x & 0xFF;
            b.append((c >= 0x21 && c <= 0x7E) || c == ' ' ? (char) c : '?');
        }
        return b.toString();
    }

    public static void main(String[] a) throws Exception {
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[0]), StandardCharsets.UTF_8));
        out.println("B364 - ZALOZIT HRU s hrou z naseho serveru N&P (NapOnlineHra bez Androidu)");

        // ---------------------------------------------------------------- 1) hry na serveru
        String json = "{\"ok\":true,\"server_cas\":1764000000,\"hry\":["
                + "{\"id\":\"atari-00000000000000a1\",\"system\":\"atari\",\"nazev\":\"Decathlon\",\"soubor\":\"Decathlon.xex\",\"velikost\":1,\"sha256\":\"\",\"zmeneno\":1},"
                + "{\"id\":\"sega-00000000000000b2\",\"system\":\"sega\",\"nazev\":\"Golden Axe II\",\"soubor\":\"Golden Axe II.zip\"},"
                + "{\"id\":\"atari-00000000000000c3\",\"system\":\"atari\",\"nazev\":\"Čáp a žába\",\"soubor\":\"Čáp a žába.atr\"},"
                + "{\"id\":\"atari-00000000000000d4\",\"system\":\"atari\",\"nazev\":\"\",\"soubor\":\"Archon_(1984).atr\"},"
                + "{\"id\":\"atari-00000000000000e5\",\"system\":\"atari\",\"nazev\":\"Atari BIOS\",\"soubor\":\"atari_bios.zip\"},"
                + "{\"id\":\"atari-NEPLATNE\",\"system\":\"atari\",\"nazev\":\"Spatne id\",\"soubor\":\"x.xex\"},"
                + "{\"id\":\"ps1-00000000000000f6\",\"system\":\"ps1\",\"nazev\":\"Crash\",\"soubor\":\"Crash.cue\"},"
                + "{\"id\":\"atari-0000000000000107\",\"system\":\"atari\",\"nazev\":\"barbarian\",\"soubor\":\"Barbarian.car\"},"
                + "{\"id\":\"sega-0000000000000108\",\"system\":\"atari\",\"nazev\":\"Cizi id\",\"soubor\":\"y.xex\"}]}";
        List<String[]> h = NapOnlineHra.serveroveHry(json, "atari");
        over(h.size() == 4 && "Archon (1984) | barbarian | Čáp a žába | Decathlon".equals(nazvy(h)),
                "1) hry Atari na serveru: " + h.size() + " (" + nazvy(h) + ") - Sega / PS1 ne, BIOS ne, spatne / cizi id ne, podle nazvu bez diakritiky");
        String[] arch = h.isEmpty() ? new String[3] : h.get(0);
        over("atari-00000000000000d4".equals(arch[0]) && "Archon (1984)".equals(arch[1]) && "Archon_(1984).atr".equals(arch[2]),
                "   prazdny nazev = jmeno souboru bez pripony ('" + arch[1] + "'), id a soubor pro stazeni /api/v1/hra?id=" + arch[0]);
        over(NapOnlineHra.serveroveHry("nesmysl", "atari").isEmpty() && NapOnlineHra.serveroveHry(null, "atari").isEmpty()
                        && NapOnlineHra.serveroveHry("{\"ok\":true,\"hry\":[]}", "atari").isEmpty() && NapOnlineHra.serveroveHry(json, null).isEmpty()
                        && NapOnlineHra.serveroveHry("{\"ok\":true,\"hry\":{\"x\":1}}", "atari").isEmpty(),
                "   spatny / prazdny JSON = zadna hra (zadna vyjimka) - okno pak rekne 'Na serveru N&P teď nejsou žádné hry pro Atari.'");

        // ---------------------------------------------------------------- 2) onlineHra se 4. radkem
        String[] h4 = NapOnlineHra.atariHra("Decathlon.xex\n1\n\n7");
        String[] h3 = NapOnlineHra.atariHra("Archon_(1984).atr\n1\n");
        over(h4 != null && h4.length == 4 && "7".equals(h4[3]) && "Decathlon.xex".equals(h4[0]) && "1".equals(h4[1])
                        && h3 != null && h3.length == 4 && "".equals(h3[3]),
                "2) onlineHra z C++: 'nazev / pripraveno / proc / gen' -> gen '" + (h4 == null ? "?" : h4[3]) + "'; stara odpoved bez gen dal funguje");

        // ---------------------------------------------------------------- 3) hra se spustila
        String[] vypnuto = NapOnlineHra.atariHra("ATARI BASIC\n0\nAtari je vypnute\n0");
        String[] basic = NapOnlineHra.atariHra("ATARI BASIC\n1\n\n3");
        String[] nahrava = NapOnlineHra.atariHra("ATARI BASIC\n0\nAtari je vypnute\n4");
        String[] dec = NapOnlineHra.atariHra("Decathlon.xex\n1\n\n4");
        over(NapOnlineHra.hraSpustena(basic, dec) && NapOnlineHra.hraSpustena(vypnuto, dec) && NapOnlineHra.hraSpustena(null, dec),
                "3) hra ze serveru se spustila: novy stroj (gen 3 -> 4) a Atari zapnute (i z vypnuteho Atari, i bez predchozi odpovedi)");
        over(!NapOnlineHra.hraSpustena(dec, dec) && !NapOnlineHra.hraSpustena(basic, basic) && !NapOnlineHra.hraSpustena(basic, nahrava)
                        && !NapOnlineHra.hraSpustena(basic, null) && !NapOnlineHra.hraSpustena(basic, h3),
                "   jeste ne: stejny stroj (porad tataz hra / BASIC), novy stroj jeste nebezi, zadna odpoved, C++ bez gen");

        // ---------------------------------------------------------------- 4) je to ta hra?
        String dlouhe = "Čtyři kluci z městečka a jejich neuvěřitelné dobrodružství na hradě Pernštejn – díl druhý, sběratelská edice (1989).xex";
        over(NapOnlineHra.jeTaHra("Decathlon.xex", "Decathlon.xex") && NapOnlineHra.jeTaHra("Archon_(1984).atr", "Archon (1984).atr")
                        && NapOnlineHra.jeTaHra(pcAscii("Čáp a žába.atr"), "Čáp a žába.atr") && NapOnlineHra.jeTaHra("BARBARIAN.CAR", "Barbarian.car"),
                "4) v Atari bezi ta hra: stejne jmeno, '_' = mezera, diakritika z C++ ('" + pcAscii("Čáp a žába.atr") + "') = 'Čáp a žába.atr', velka pismena");
        over(pcAscii(dlouhe).length() == 120 && NapOnlineHra.jeTaHra(pcAscii(dlouhe), dlouhe)
                        && !NapOnlineHra.jeTaHra(pcAscii("Čtyři kluci.xex"), dlouhe),
                "   dlouhe jmeno (" + dlouhe.length() + " znaku, z C++ useknute na 120 B bez pripony) = ta hra; kratke jine jmeno ne");
        over(!NapOnlineHra.jeTaHra("ATARI BASIC", "Decathlon.xex") && !NapOnlineHra.jeTaHra("Archon_(1984).atr", "Decathlon.xex")
                        && !NapOnlineHra.jeTaHra("", "Decathlon.xex") && !NapOnlineHra.jeTaHra(null, "Decathlon.xex") && !NapOnlineHra.jeTaHra("Decathlon.xex", null),
                "   jina hra / BASIC / nic = ne (online se nezalozi s necim jinym)");
        over(NapOnlineHra.jeTaHra("RIVER_RAID.XEX", "River Raid kolekce.zip") && NapOnlineHra.jeTaHra("hra.atr", "Sbirka.RAR")
                        && !NapOnlineHra.jeTaHra("ATARI BASIC", "River Raid kolekce.zip"),
                "   ZIP / RAR (C++ spusti soubor z archivu pod jeho jmenem): cokoli krome samotneho ATARI BASICu");

        // ---------------------------------------------------------------- 5) cislo hry pro okno HRA ZALOZENA
        over("4821".equals(NapOnlineHra.cisloZalozeneHry("B358 ONLINE ceka na hrace 2 - cislo hry 4821"))
                        && "".equals(NapOnlineHra.cisloZalozeneHry("B358 ONLINE spojeno s enginem, zakladam hru"))
                        && "".equals(NapOnlineHra.cisloZalozeneHry("B358 ONLINE ceka na hrace 2 - cislo hry 12"))
                        && "".equals(NapOnlineHra.cisloZalozeneHry("B358 ONLINE HRAJE (Atari 130XE) s SM-A125F - odezva 12 ms"))
                        && "".equals(NapOnlineHra.cisloZalozeneHry(null)),
                "5) okno HRA ZALOŽENA: cislo hry jen z radku 'ceka na hrace 2 - cislo hry 4821' (4 cislice; jine radky nic)");
        NapOnlineHlidac hl = new NapOnlineHlidac();
        hl.zmena("krok=\nsystem=atari\nfaze=PRIPOJUJE\nkod=\naktivni=1\nstart=1", 1000);
        String z = hl.zmena("krok=\nsystem=atari\nfaze=CEKA_NA_HRACE\nkod=4821\naktivni=1\nstart=1", 1400);
        String kod = "";
        if (z != null) for (String r : z.split("\n")) if (kod.isEmpty()) kod = NapOnlineHra.cisloZalozeneHry(r);
        over("4821".equals(kod), "   skutecny hlidac (NapOnlineHlidac) po zalozeni napise radek, ze ktereho okno cislo vezme: '"
                + (z == null ? "(nic)" : z.replace("\n", " || ")) + "'");

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
