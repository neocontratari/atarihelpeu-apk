package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

/**
 * B360: PRUBEH ONLINE HRY DO LOGU - NapOnlineHlidac (Java bez Androidu).
 * Rene 23. 11. vecer: "telefony se sekaji ... posilam dva logy, kazdy jina chyba". V logu bylo jen kdy hra zacala
 * a skoncila - ne kolikrat a jak dlouho telefon cekal na stisky druheho, ani proc pokus 2 skoncil ("online skoncilo").
 *  1) za hry kazdych 10 s radek PRUBEH (cekani, zpozdeni, synchronizace, odezva) - ne casteji
 *  2) zadost o vetsi zpozdeni / zvyseni / snizeni zpozdeni - kazde jednou, s hodnotou
 *  3) synchronizace po rozjeti: hrac 1 "posilam stav", hrac 2 "nahravam stav", co se lisilo, hotovo + velikost
 *  4) vic udalosti v jednom kroku = vic radku (oddelene \n), novy pokus = citace od nuly
 *  5) KONEC s prubehem; skutecny text z C++ (ticho) projde cely; stare radky B358 beze zmeny
 *  6) aktivni(): Wi-Fi bez usporneho rezimu jen behem online (pripojuje / ceka / hraje / stahuje)
 * Pouziti: java eu.atarihelp.emu10.TestB360Hlidac VYSTUP.txt
 */
public class TestB360Hlidac {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    static String hraje(String start, String snimek, String navic) {
        return "krok=\nfaze=HRAJE\nkod=5649\ndruhy=SM-A125F\nchyba=\nodezva=9\nzpozdeni=3\novereno=10\ncekani=2\nsnimek=" + snimek
                + "\naktivni=1\nhraje=1\nhrac1=1\nkonec=\nstart=" + start
                + "\nzpozdeni_ted=3\ncekani_ms=60\ncekani_max_ms=40\ndlouhych=1\nzadosti=0\nzvyseni=0\nsnizeni=0\nsync=0\nsynchronizaci=0"
                + "\nsynchronizovano=0\nrozjeti=0\nrozjeti_snimek=0\nrozjeti_casti=\nstav_kb=0\nticho_ms=20\nstoji_ms=0" + navic;
    }

    /** prida / prepise radky klic=hodnota (posledni vyhrava - NapOnlineHra.stav cte radky poporade) */
    static String s(String zaklad, String... kv) {
        StringBuilder b = new StringBuilder(zaklad);
        for (String x : kv) b.append('\n').append(x);
        return b.toString();
    }

    static String n(String x) { return x == null ? "(nic)" : x.replace("\n", " || "); }

    public static void main(String[] a) throws Exception {
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[0]), StandardCharsets.UTF_8));
        out.println("B360 - prubeh online hry do logu: NapOnlineHlidac");

        // ---------------------------------------------------------------- 1) PRUBEH kazdych 10 s
        NapOnlineHlidac h = new NapOnlineHlidac();
        h.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 0);
        String zac = h.zmena(hraje("1", "10", ""), 1000);
        String r5 = h.zmena(hraje("1", "300", ""), 6000);
        String r10 = h.zmena(hraje("1", "600", ""), 11000);
        String r15 = h.zmena(hraje("1", "900", ""), 16000);
        String r20 = h.zmena(hraje("1", "1200", ""), 21000);
        over(zac != null && zac.startsWith("B358 ONLINE HRAJE s SM-A125F - odezva 9 ms, zpozdeni 3 snimku") && r5 == null
                && "B360 ONLINE PRUBEH snimek 600: cekalo se 2x (60 ms, nejdel 40 ms), zpozdeni 3, synchronizaci 0, odezva 9 ms".equals(r10)
                && r15 == null && r20 != null && r20.startsWith("B360 ONLINE PRUBEH snimek 1200"),
                "1) za hry PRUBEH kazdych 10 s (ne casteji): '" + n(r10) + "', po 5 s " + n(r5) + ", po 15 s " + n(r15));

        // ---------------------------------------------------------------- 2) zpozdeni
        String z1 = h.zmena(s(hraje("1", "1250", ""), "zadosti=1", "cekani_max_ms=97"), 22000);
        String z2 = h.zmena(s(hraje("1", "1300", ""), "zadosti=1", "zvyseni=1", "zpozdeni_ted=7"), 23000);
        String z3 = h.zmena(s(hraje("1", "1350", ""), "zadosti=1", "zvyseni=1", "zpozdeni_ted=7"), 24000);
        String z4 = h.zmena(s(hraje("1", "3200", ""), "zadosti=1", "zvyseni=1", "snizeni=1", "zpozdeni_ted=6"), 31500);
        over("B360 ONLINE cekal jsem na stisky druheho telefonu (nejdel 97 ms) - zadam ho o vetsi zpozdeni (zadost c. 1)".equals(z1)
                && "B360 ONLINE zpozdeni ovladani ZVYSENO na 7 snimku - druhy telefon cekal na moje stisky (sit kolisa)".equals(z2)
                && z3 == null && z4 != null && z4.startsWith("B360 ONLINE zpozdeni ovladani snizeno na 6 snimku (sit je klidna)"),
                "2) zadost / zvyseni / snizeni zpozdeni kazde jednou: '" + n(z1) + "' | '" + n(z2) + "' | '" + n(z4) + "'");
        over(z4 != null && z4.contains("\nB360 ONLINE PRUBEH snimek 3200: cekalo se 2x (60 ms, nejdel 40 ms), zpozdeni 3 -> 6, synchronizaci 0"),
                "   snizeni + PRUBEH v jednom kroku = dva radky (oddelene \\n), PRUBEH ukazuje zpozdeni od startu: 3 -> 6");

        // ---------------------------------------------------------------- 3) synchronizace (hrac 1 / hrac 2)
        NapOnlineHlidac h1 = new NapOnlineHlidac();
        h1.zmena(hraje("1", "100", ""), 0);
        String s1 = h1.zmena(s(hraje("1", "305", ""), "synchronizaci=1", "rozjeti=1", "rozjeti_snimek=300", "rozjeti_casti=RAM 68000, VRAM", "sync=1"), 500);
        String s2 = h1.zmena(s(hraje("1", "320", ""), "synchronizaci=1", "synchronizovano=1", "stav_kb=85", "rozjeti_snimek=300"), 900);
        NapOnlineHlidac h2 = new NapOnlineHlidac();
        h2.zmena(s(hraje("1", "100", ""), "hrac1=0"), 0);
        String s3 = h2.zmena(s(hraje("1", "305", ""), "hrac1=0", "synchronizaci=1", "sync=1"), 500);
        String s4 = h2.zmena(s(hraje("1", "320", ""), "hrac1=0", "synchronizaci=1", "synchronizovano=1", "stav_kb=85"), 900);
        over("B360 ONLINE SYNCHRONIZACE c. 1: hra se rozjela (otisk ve snimku 300), lisi se: RAM 68000, VRAM - posilam druhemu telefonu cely stav hry".equals(s1)
                && "B360 ONLINE synchronizace c. 1 hotova (stav hry 85 KB) - hraje se dal".equals(s2),
                "3) hrac 1: '" + n(s1) + "' | '" + n(s2) + "'");
        over("B360 ONLINE SYNCHRONIZACE c. 1: hra se rozjela - nahravam stav hry od hrace 1".equals(s3) && "B360 ONLINE synchronizace c. 1 hotova (stav hry 85 KB) - hraje se dal".equals(s4),
                "   hrac 2: '" + n(s3) + "' | '" + n(s4) + "'");

        // ---------------------------------------------------------------- 4) novy pokus = citace od nuly
        NapOnlineHlidac h3 = new NapOnlineHlidac();
        h3.zmena(s(hraje("1", "100", ""), "synchronizaci=2", "zvyseni=3"), 0);
        h3.zmena("krok=\nfaze=KONEC\nkonec=druhy hrac hru ukoncil\nstart=1\nhraje=0", 100);
        h3.zmena("krok=\nfaze=PRIPOJUJE\nstart=2\nhraje=0", 200);
        String p1 = h3.zmena(hraje("2", "10", ""), 300);
        String p2 = h3.zmena(s(hraje("2", "400", ""), "synchronizaci=1"), 400);
        over(p1 != null && !p1.contains("SYNCHRONIZACE") && !p1.contains("ZVYSENO") && p2 != null && p2.startsWith("B360 ONLINE SYNCHRONIZACE c. 1"),
                "4) novy pokus (start 2): citace od nuly - '" + n(p1) + "' | '" + n(p2) + "'");

        // ---------------------------------------------------------------- 5) KONEC s prubehem, text z C++ cely, B358 beze zmeny
        NapOnlineHlidac h4 = new NapOnlineHlidac();
        h4.zmena(hraje("1", "100", ""), 0);
        String ticho = "od druheho hrace dlouho nic neprislo - spadla mu sit, nebo se telefon zasekl";
        String k1 = h4.zmena(s(hraje("1", "7991", ""), "faze=KONEC", "hraje=0", "konec=" + ticho, "overeno=134", "cekani=40", "cekani_ms=980",
                "cekani_max_ms=16000", "zpozdeni_ted=5", "synchronizaci=0"), 1000);
        over(("B358 ONLINE KONEC (pokus 1): " + ticho + " | overeno 134 otisku, snimek 7991 | cekalo se 40x (980 ms, nejdel 16000 ms), zpozdeni 3 -> 5, synchronizaci 0").equals(k1),
                "5) KONEC: '" + n(k1) + "'");
        NapOnlineHlidac h5 = new NapOnlineHlidac();
        h5.zmena("krok=\nfaze=HRAJE\nhraje=1\ndruhy=x\nstart=1", 0);
        String k2 = h5.zmena("krok=\nfaze=KONEC\nhraje=0\nkonec=hra se rozjela\novereno=13\nsnimek=783\nstart=1", 10);
        over("B358 ONLINE KONEC (pokus 1): hra se rozjela | overeno 13 otisku, snimek 783".equals(k2),
                "   stary stav bez polozek B360 (appka B358): radek KONEC presne jako v B358 '" + n(k2) + "'");

        // ---------------------------------------------------------------- 6) aktivni() - Wi-Fi zamek
        NapOnlineHlidac h6 = new NapOnlineHlidac();
        boolean[] ak = new boolean[6];
        h6.zmena("krok=\nfaze=NIC\nstart=0", 0); ak[0] = h6.aktivni();
        h6.zmena("krok=STAHUJI\nfaze=NIC\nstart=0", 1); ak[1] = h6.aktivni();
        h6.zmena("krok=\nfaze=PRIPOJUJE\nstart=1", 2); ak[2] = h6.aktivni();
        h6.zmena("krok=\nfaze=CEKA_NA_HRACE\nkod=1234\nstart=1", 3); ak[3] = h6.aktivni();
        h6.zmena(hraje("1", "5", ""), 4); ak[4] = h6.aktivni();
        h6.zmena("krok=\nfaze=KONEC\nkonec=x\nstart=1", 5); ak[5] = h6.aktivni();
        over(!ak[0] && ak[1] && ak[2] && ak[3] && ak[4] && !ak[5],
                "6) Wi-Fi bez usporneho rezimu: NIC ne, stahuje ano, pripojuje ano, ceka ano, hraje ano, KONEC ne");

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
