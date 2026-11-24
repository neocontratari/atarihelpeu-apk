package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

/**
 * B362: ONLINE S ODHADEM DO LOGU - NapOnlineHlidac (Java bez Androidu).
 * Rene 24. 11.: "furt je ten online spatny Partaku. A to ho testuju na mistni LAN" - a v logu S8 bylo jen, ze S8 cekal
 * na A12, ne PROC (log A12 nebyl). Ted:
 *  1) PRUBEH navic s odhady a opravami (jen kdyz je C++ posila - stary radek B360 beze zmeny)
 *  2) DRUHY TELEFON: kazda nova statistika od druheho telefonu = jeden radek (jen ASCII, jeden radek)
 *  3) TICHO: od druheho nic uz 2 / 5 / 10 s - kazdy prah jednou, co prislo naposledy; po obnoveni znovu od zacatku
 *  4) navrat hrace 2 pri synchronizaci, oprava bez ulozeneho stavu - jednou
 *  5) KONEC s odhady; infoTelefonu(): text o telefonu pro druhy telefon (ASCII, nejvys 200 znaku, pasmo 2.4 / 5 / 6 GHz)
 * Pouziti: java eu.atarihelp.emu10.TestB362Hlidac VYSTUP.txt
 */
public class TestB362Hlidac {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    /** stav hry jako NativeSegaCoreBridge.onlineStav B362 */
    static String hraje(String snimek, String navic) {
        return "krok=\nfaze=HRAJE\nkod=5649\ndruhy=SM-A125F\nchyba=\nodezva=9\nzpozdeni=2\novereno=10\ncekani=1\nsnimek=" + snimek
                + "\naktivni=1\nhraje=1\nhrac1=1\nkonec=\nstart=1"
                + "\nzpozdeni_ted=2\ncekani_ms=35\ncekani_max_ms=35\ndlouhych=1\nzadosti=0\nzvyseni=0\nsnizeni=0\nsync=0\nsynchronizaci=0"
                + "\nsynchronizovano=0\nrozjeti=0\nrozjeti_snimek=0\nrozjeti_casti=\nstav_kb=0\nticho_ms=12\nstoji_ms=0"
                + "\nodhad=45\nodhad_sedelo=40\noprav=5\noprava_max=6\noprava_max_ms=31\nopravy_ms=90\nprepocteno=20\nodhad_max=8"
                + "\nchyb_opravy=0\nnavrat_ze=0\nnavrat_na=0\nposledni_zprava=VSTUP\njeho_do=1201\ndruhy_info_c=0\ndruhy_info_snimek=0\ndruhy_info=" + navic;
    }

    static String s(String zaklad, String... kv) {
        StringBuilder b = new StringBuilder(zaklad);
        for (String x : kv) b.append('\n').append(x);
        return b.toString();
    }

    static String n(String x) { return x == null ? "(nic)" : x.replace("\n", " || "); }

    static int pocet(String x, String co) {
        if (x == null) return 0;
        int n = 0;
        for (int i = x.indexOf(co); i >= 0; i = x.indexOf(co, i + 1)) n++;
        return n;
    }

    public static void main(String[] a) throws Exception {
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[0]), StandardCharsets.UTF_8));
        out.println("B362 - online s odhadem do logu: NapOnlineHlidac");

        // ---------------------------------------------------------------- 1) PRUBEH s odhady
        NapOnlineHlidac h = new NapOnlineHlidac();
        h.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 0);
        String zac = h.zmena(hraje("10", ""), 1000);
        String r10 = h.zmena(hraje("600", ""), 11000);
        over(zac != null && zac.startsWith("B358 ONLINE HRAJE s SM-A125F")
                && ("B360 ONLINE PRUBEH snimek 600: cekalo se 1x (35 ms, nejdel 35 ms), zpozdeni 2, synchronizaci 0, odhad 45 snimku (sedelo 40), "
                   + "oprav 5 (nejdel 6 snimku / 31 ms), limit odhadu 8, odezva 9 ms").equals(r10),
             "1) PRUBEH s odhady a opravami: " + n(r10));

        // ---------------------------------------------------------------- 2) DRUHY TELEFON
        String info = "snimek 4210, cekal 3x (90 ms, nejdel 61 ms), odhad 120 snimku (sedelo 110), oprav 9 (nejdel 7 snimku / 80 ms, celkem 300 ms), "
                + "zpozdeni 3, odezva 14 ms, 57.9 snimku/s, jadro 11.8 ms (nejvys 24.0), odhad nejvys 5 | SM-A125F Android 12 (API 31), Wi-Fi -71 dBm 2.4 GHz 65 Mb/s";
        String d1 = h.zmena(s(hraje("610", info), "druhy_info_c=1", "druhy_info_snimek=612"), 11400);
        String d1b = h.zmena(s(hraje("620", info), "druhy_info_c=1", "druhy_info_snimek=612"), 11800);
        String d2 = h.zmena(s(hraje("1200", info + " 2"), "druhy_info_c=2", "druhy_info_snimek=1203"), 21000);
        over(("B362 ONLINE DRUHY TELEFON (ja snimek 612): " + info).equals(d1) && d1b == null && d2 != null
                && d2.contains("B362 ONLINE DRUHY TELEFON (ja snimek 1203): ") && pocet(d2, "DRUHY TELEFON") == 1,
             "2) statistika druheho telefonu: kazda nova jednou - " + n(d1));

        // ---------------------------------------------------------------- 3) TICHO
        NapOnlineHlidac t = new NapOnlineHlidac();
        t.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 0);
        t.zmena(hraje("10", ""), 500);
        String t1 = t.zmena(s(hraje("4290", ""), "ticho_ms=900"), 1000);
        String t2 = t.zmena(s(hraje("4293", ""), "ticho_ms=2100", "stoji_ms=1800", "jeho_do=4294"), 2000);
        String t2b = t.zmena(s(hraje("4293", ""), "ticho_ms=2500"), 2400);
        String t5 = t.zmena(s(hraje("4293", ""), "ticho_ms=5100"), 5000);
        String t10 = t.zmena(s(hraje("4293", ""), "ticho_ms=10200"), 10000);
        String t11 = t.zmena(s(hraje("4293", ""), "ticho_ms=11000"), 11000);
        String zpet = t.zmena(s(hraje("4300", ""), "ticho_ms=10"), 12000);
        String znovu = t.zmena(s(hraje("4400", ""), "ticho_ms=2300"), 14000);
        over(t1 == null && "B362 ONLINE TICHO: od druheho telefonu nic neprislo uz 2 s (posledni zprava: VSTUP, jeho stisky do snimku 4293, ja snimek 4293, stojim 1800 ms)".equals(t2)
                && t2b == null && t5 != null && t5.contains("uz 5 s") && t10 != null && t10.contains("uz 10 s") && (t11 == null || !t11.contains("TICHO")) && zpet == null
                && znovu != null && znovu.contains("uz 2 s"),
             "3) ticho 2 / 5 / 10 s - kazdy prah jednou, po obnoveni znovu: " + n(t2));

        // ---------------------------------------------------------------- 4) navrat pri synchronizaci, oprava bez stavu
        NapOnlineHlidac v = new NapOnlineHlidac();
        v.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 0);
        v.zmena(hraje("10", ""), 500);
        String nv = v.zmena(s(hraje("250", ""), "navrat_ze=249", "navrat_na=246"), 1000);
        String nv2 = v.zmena(s(hraje("260", ""), "navrat_ze=249", "navrat_na=246"), 1400);
        String ch = v.zmena(s(hraje("270", ""), "navrat_ze=249", "navrat_na=246", "chyb_opravy=1"), 1800);
        over("B362 ONLINE synchronizace: byl jsem s odhadem uz na snimku 249 - stav od hrace 1 me vratil na snimek 246".equals(nv) && nv2 == null
                && ch != null && ch.startsWith("B362 ONLINE oprava odhadu nenasla ulozeny stav (1x)"),
             "4) navrat hrace 2 a oprava bez stavu - jednou: " + n(nv) + " || " + n(ch));

        // ---------------------------------------------------------------- 5) KONEC s odhady, info o telefonu
        NapOnlineHlidac k = new NapOnlineHlidac();
        k.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 0);
        k.zmena(hraje("10", ""), 500);
        String konec = k.zmena(s(hraje("4293", ""), "faze=KONEC", "hraje=0",
                "konec=od druheho hrace dlouho nic neprislo - spadla mu sit, nebo se telefon zasekl"), 3000);
        over(konec != null && konec.startsWith("B358 ONLINE KONEC (pokus 1): od druheho hrace dlouho nic neprislo") && konec.contains("odhad 45 snimku (sedelo 40), oprav 5"),
             "5) KONEC s odhady: " + n(konec));
        String i1 = NapOnlineHlidac.infoTelefonu("SM-A125F", "12", 31, -61, 2437, 72, "LOW_LATENCY", "31.2", false, "NONE");
        String i2 = NapOnlineHlidac.infoTelefonu("SM-G950F", "9", 28, -48, 5180, 433, "HIGH_PERF", "na", true, "");
        String i3 = NapOnlineHlidac.infoTelefonu("Žluťoučký kůň \u0001", "14", 34, 0, 0, 0, "", "err", false, "");
        StringBuilder dlouhy = new StringBuilder();
        for (int i = 0; i < 300; i++) dlouhy.append('X');
        String i4 = NapOnlineHlidac.infoTelefonu(dlouhy.toString(), "14", 34, -80, 5955, 1200, "LOW_LATENCY", "40.0", false, "SEVERE");
        String i5 = NapOnlineHlidac.infoTelefonu("Pixel 8", "14", 34, -80, 5955, 1200, "LOW_LATENCY", "40.0", false, "SEVERE");
        boolean ascii = true;
        for (String x : new String[]{i1, i2, i3, i4}) for (char c : x.toCharArray()) if (c < 32 || c > 126) ascii = false;
        over("SM-A125F Android 12 (API 31), Wi-Fi -61 dBm 2.4 GHz 72 Mb/s, LOW_LATENCY, baterie 31.2 C, sporic vyp, teplo NONE".equals(i1)
                && "SM-G950F Android 9 (API 28), Wi-Fi -48 dBm 5 GHz 433 Mb/s, HIGH_PERF, sporic ZAP".equals(i2)
                && i3.startsWith("?lu?ou?k? k?? ? Android 14 (API 34), Wi-Fi ?, Wi-Fi zamek ne") && i4.length() == 200 && ascii
                && i5.contains("Wi-Fi -80 dBm 6 GHz 1200 Mb/s") && i5.endsWith("teplo SEVERE"),
             "   infoTelefonu: '" + i1 + "' / '" + i2 + "' / '" + i3 + "' / '" + i5 + "' (jen ASCII, nejvys 200 znaku)");

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
