package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;

/**
 * B363: ONLINE ATARI v Jave (bez Androidu) - NapOnlineHra (okno ONLINE na pristroji Atari) a NapOnlineHlidac (log).
 * Rene 25. 11.: "uz pustit do online atari" + "proc treba na slabsi telefony nepridat optimalizaci?".
 *  1) zalozeni hry Atari: hra z C++ (onlineHra "nazev\npripraveno\nproc"), cesta na engine (system=atari, otisk
 *     40 nul - engine chce 40 hex znaku, hra jde ve stavu), Atari vypnute = nejde
 *  2) seznam her na enginu: jen Atari (Sega / PS1 ne), cislo hry 4 cislice, jmeno hrace bez znaku, ktere engine
 *     zahodi, "čeká 2 min" / "waiting 45 s", spatny JSON = prazdny seznam
 *  3) okno za hry (atariPopis): cislo hry, posilam / prijimam hru, s kym, odezva, zpozdeni, joysticky, setrne
 *     kresleni, konec s duvodem - cesky i anglicky
 *  4) hlidac: Atari radky "(Atari 130XE)", SETRNE KRESLENI u Segy do logu (u Atari ho pise C++ - ne dvakrat),
 *     PRUBEH rekne "setrne kresleni"; Sega bez klice "system" beze zmeny (stare radky B358 / B360)
 * Pouziti: java eu.atarihelp.emu10.TestB363Online VYSTUP.txt
 */
public class TestB363Online {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    static String n(String x) { return x == null ? "(nic)" : x.replace("\n", " || "); }

    /** stav hry jako NativeAtariCoreBridge.onlineStav B363 (system=atari) */
    static String atari(String faze, String hraje, String navic) {
        return "krok=\nsystem=atari\nfaze=" + faze + "\nkod=4821\ndruhy=SM-A125F\nchyba=\nodezva=12\nzpozdeni=2\novereno=10\ncekani=1\nsnimek=600"
                + "\naktivni=1\nhraje=" + hraje + "\nhrac1=1\nkonec=\nstart=3\nstart_stav=4\nsetrne=0\nsetrne_c=0\nsetrne_proc=\nporty=J1J2"
                + "\nzpozdeni_ted=3\ncekani_ms=35\ncekani_max_ms=35\ndlouhych=1\nzadosti=0\nzvyseni=0\nsnizeni=0\nsync=0\nsynchronizaci=0"
                + "\nsynchronizovano=0\nrozjeti=0\nrozjeti_snimek=0\nrozjeti_casti=\nstav_kb=11\nticho_ms=12\nstoji_ms=0"
                + "\nodhad=45\nodhad_sedelo=40\noprav=5\noprava_max=6\noprava_max_ms=31\nopravy_ms=90\nprepocteno=20\nodhad_max=8"
                + "\nchyb_opravy=0\nnavrat_ze=0\nnavrat_na=0\nposledni_zprava=VSTUP\njeho_do=601\ndruhy_info_c=0\ndruhy_info_snimek=0\ndruhy_info=" + navic;
    }

    /** stav hry jako NativeSegaCoreBridge.onlineStav B363 (bez klice system, navic setrne) */
    static String sega(String setrne, String snimek) {
        return "krok=\nfaze=HRAJE\nkod=5649\ndruhy=SM-G950F\nchyba=\nodezva=9\nzpozdeni=2\novereno=10\ncekani=1\nsnimek=" + snimek
                + "\naktivni=1\nhraje=1\nhrac1=0\nkonec=\nstart=1"
                + "\nzpozdeni_ted=2\ncekani_ms=35\ncekani_max_ms=35\ndlouhych=1\nzadosti=0\nzvyseni=0\nsnizeni=0\nsync=0\nsynchronizaci=0"
                + "\nsynchronizovano=0\nrozjeti=0\nrozjeti_snimek=0\nrozjeti_casti=\nstav_kb=0\nticho_ms=12\nstoji_ms=0"
                + "\nodhad=45\nodhad_sedelo=40\noprav=5\noprava_max=6\noprava_max_ms=31\nopravy_ms=90\nprepocteno=20\nodhad_max=8"
                + "\nchyb_opravy=0\nnavrat_ze=0\nnavrat_na=0\nposledni_zprava=VSTUP\njeho_do=1201\ndruhy_info_c=0\ndruhy_info_snimek=0\ndruhy_info="
                + "\nsetrne=" + setrne + "\nsetrne_c=" + ("1".equals(setrne) ? "1" : "0") + "\nsetrne_proc=jadro 10.4 ms na snimek (z 16.7 ms)";
    }

    public static void main(String[] a) throws Exception {
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[0]), StandardCharsets.UTF_8));
        out.println("B363 - online Atari v Jave: NapOnlineHra (okno ONLINE) a NapOnlineHlidac (log)");

        // ---------------------------------------------------------------- 1) zalozeni hry Atari
        String[] h = NapOnlineHra.atariHra("Archon_(1984).atr\n1\n");
        String[] vyp = NapOnlineHra.atariHra("ATARI BASIC\n0\nAtari je vypnute");
        String cesta = NapOnlineHra.cestaZaloz("atari", h[0], "", NapOnlineHra.ATARI_OTISK, NapOnlineHra.ATARI_NASTAVENI, "SM-G950F");
        over(h != null && "Archon_(1984).atr".equals(h[0]) && "1".equals(h[1]) && vyp != null && "0".equals(vyp[1]) && "Atari je vypnute".equals(vyp[2])
                        && NapOnlineHra.atariHra(null) == null && NapOnlineHra.atariHra("x") == null,
                "1) hra z C++: '" + h[0] + "' pripravena, vypnute Atari = '" + vyp[2] + "', nesmysl = nic");
        over(cesta.equals("/api/v1/online/zaloz?system=atari&hra=Archon+%281984%29&hra_id=&otisk=0000000000000000000000000000000000000000&nastaveni=363&jmeno=SM-G950F")
                        && NapOnlineHra.ATARI_OTISK.matches("[0-9a-f]{40}"),
                "   cesta na engine: " + cesta + " (otisk 40 hex znaku jako chce engine JeOtisk, hra jde ve stavu)");

        // ---------------------------------------------------------------- 2) seznam her na enginu
        String json = "{\"ok\":true,\"mistnosti\":[{\"kod\":\"5649\",\"system\":\"sega\",\"hra\":\"Golden Axe II\",\"hra_id\":\"\",\"otisk\":\"ab\","
                + "\"nastaveni\":\"17\",\"hrac\":\"SM-G950F\",\"ceka_s\":12},"
                + "{\"kod\":\"4821\",\"system\":\"atari\",\"hra\":\"Archon (1984)\",\"hra_id\":\"\",\"otisk\":\"0000000000000000000000000000000000000000\","
                + "\"nastaveni\":\"363\",\"hrac\":\"Rene <S8>\",\"ceka_s\":135},"
                + "{\"kod\":\"12\",\"system\":\"atari\",\"hra\":\"spatne cislo\",\"hrac\":\"x\",\"ceka_s\":1},"
                + "{\"kod\":\"7702\",\"system\":\"atari\",\"hra\":\"\",\"hrac\":\"\",\"ceka_s\":45}],\"server_cas\":1764000000}";
        List<String[]> m = NapOnlineHra.mistnosti(json, "atari");
        String r0 = m.isEmpty() ? "" : NapOnlineHra.popisMistnosti(m.get(0), false), r1 = m.size() < 2 ? "" : NapOnlineHra.popisMistnosti(m.get(1), true);
        over(m.size() == 2 && "4821".equals(m.get(0)[0]) && "Rene S8".equals(m.get(0)[2]) && "7702".equals(m.get(1)[0]),
                "2) seznam: jen hry Atari (Sega ne), cislo hry musi mit 4 cislice (\"12\" pryc), jmeno bez < > (" + m.size() + " hry)");
        over("4821 – Archon (1984) (Rene S8, čeká 2 min)".equals(r0) && "7702 – Atari (?, waiting 45 s)".equals(r1),
                "   radky pro hrace: '" + r0 + "' / '" + r1 + "'");
        over(NapOnlineHra.mistnosti("nesmysl", "atari").isEmpty() && NapOnlineHra.mistnosti(null, "atari").isEmpty()
                        && NapOnlineHra.mistnosti("{\"ok\":true,\"mistnosti\":[]}", "atari").isEmpty(),
                "   spatny JSON / prazdny seznam = zadna hra (zadna vyjimka)");

        // ---------------------------------------------------------------- 3) okno za hry
        Map<String, String> ceka = NapOnlineHra.stav("system=atari\nfaze=CEKA_NA_HRACE\nkod=4821\naktivni=1");
        Map<String, String> posila = NapOnlineHra.stav("system=atari\nfaze=HRAJE\nkod=4821\ndruhy=SM-A125F\nhrac1=1\nstart_stav=2\naktivni=1");
        Map<String, String> prijima = NapOnlineHra.stav("system=atari\nfaze=HRAJE\nkod=4821\ndruhy=SM-G950F\nhrac1=0\nstart_stav=3\naktivni=1");
        Map<String, String> hra = NapOnlineHra.stav(atari("HRAJE", "1", "").replace("setrne=0", "setrne=1").replace("porty=J1J2", "porty=J1"));
        Map<String, String> konec = NapOnlineHra.stav("system=atari\nfaze=KONEC\nkonec=druhy hrac hru ukoncil\naktivni=0");
        String pc = NapOnlineHra.atariPopis(ceka, false), pe = NapOnlineHra.atariPopis(ceka, true);
        String hc = NapOnlineHra.atariPopis(hra, false), he = NapOnlineHra.atariPopis(hra, true);
        over(pc.contains("ČÍSLO HRY: 4821") && pc.contains("ONLINE → PŘIPOJIT SE") && pe.contains("GAME NUMBER: 4821") && pe.contains("ONLINE → JOIN"),
                "3) ceka na hrace 2: cislo hry a co ma udelat druhy (CZ: '" + n(pc).substring(0, Math.min(40, pc.length())) + "...')");
        over(NapOnlineHra.atariPopis(posila, false).startsWith("Posílám hráči SM-A125F celé Atari i s hrou")
                        && NapOnlineHra.atariPopis(prijima, false).startsWith("Přijímám hru od hráče SM-G950F")
                        && NapOnlineHra.atariPopis(prijima, true).startsWith("Receiving the game from SM-G950F"),
                "   start ze stavu: hrac 1 'Posílám ... celé Atari i s hrou', hrac 2 'Přijímám hru od hráče ...' (i EN)");
        over(hc.startsWith("Hraješ online s SM-A125F (jsi hráč 1)") && hc.contains("Odezva: 12 ms, zpoždění ovládání: 3 snímků")
                        && hc.contains("Oba hráči na joysticku 1.") && hc.contains("Šetrné kreslení: tenhle telefon ukazuje každý 2. snímek")
                        && he.contains("Both players on joystick 1.") && he.contains("Economy drawing"),
                "   hra bezi: s kym, odezva, zpozdeni, joysticky, setrne kresleni (CZ i EN)");
        over("Online skončilo: druhy hrac hru ukoncil".equals(NapOnlineHra.atariPopis(konec, false))
                        && "Online ended: druhy hrac hru ukoncil".equals(NapOnlineHra.atariPopis(konec, true)),
                "   konec s duvodem z C++: '" + NapOnlineHra.atariPopis(konec, false) + "'");

        // ---------------------------------------------------------------- 4) hlidac
        NapOnlineHlidac ha = new NapOnlineHlidac();
        ha.zmena(atari("HRAJE", "0", ""), 1000);
        String za = ha.zmena(atari("HRAJE", "1", ""), 1400);
        String zas = ha.zmena(atari("HRAJE", "1", "").replace("setrne=0", "setrne=1"), 1800);
        String zak = ha.zmena(atari("KONEC", "0", "").replace("konec=", "konec=druhy hrac hru ukoncil"), 2200);
        over(za != null && za.startsWith("B358 ONLINE HRAJE (Atari 130XE) s SM-A125F - odezva 12 ms") && zas == null
                        && zak != null && zak.startsWith("B358 ONLINE KONEC (Atari 130XE) (pokus 3): druhy hrac hru ukoncil"),
                "4) Atari v logu: '" + n(za) + "' / konec '" + n(zak).substring(0, Math.min(70, n(zak).length())) + "...', setrne kresleni u Atari nepise (C++ samo)");
        NapOnlineHlidac hs = new NapOnlineHlidac();
        hs.zmena(sega("0", "100"), 1000);
        String s1 = hs.zmena(sega("1", "200"), 1400);
        String s2 = hs.zmena(sega("1", "300"), 1800);
        String s3 = hs.zmena(sega("1", "900"), 13000);
        String s4 = hs.zmena(sega("0", "1000"), 13400);
        over(s1 != null && s1.equals("B363 ONLINE SETRNE KRESLENI zapnuto (telefon nestiha: jadro 10.4 ms na snimek (z 16.7 ms)) - na displej jde kazdy 2. snimek, hra sama bezi cela")
                        && s2 == null && s3 != null && s3.startsWith("B360 ONLINE PRUBEH snimek 900") && s3.contains(", setrne kresleni (obraz kazdy 2. snimek)")
                        && "B363 ONLINE SETRNE KRESLENI vypnuto - telefon zase stiha, obraz zase kazdy snimek".equals(s4),
                "   Sega: zapnuti / vypnuti setrneho kresleni jednou do logu, PRUBEH s 'setrne kresleni': " + n(s1).substring(0, 40) + "...");
        NapOnlineHlidac hx = new NapOnlineHlidac();
        String stara = "krok=\nfaze=HRAJE\nkod=5649\ndruhy=SM-A125F\nchyba=\nodezva=9\nzpozdeni=2\novereno=10\ncekani=1\nsnimek=100\naktivni=1\nhraje=0\nhrac1=1\nkonec=\nstart=1";
        hx.zmena(stara, 1000);
        String sx = hx.zmena(stara.replace("hraje=0", "hraje=1"), 1400);
        over("B358 ONLINE HRAJE s SM-A125F - odezva 9 ms, zpozdeni 2 snimku".equals(sx),
                "   Sega bez klice system: radek HRAJE beze zmeny ('" + n(sx) + "')");

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
