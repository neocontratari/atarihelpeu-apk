package eu.atarihelp.emu10;

import java.util.Map;

/**
 * B358: ZMENY STAVU ONLINE DO LOGU APPKY (test: test_overeni/b358/jvm, B360: test_overeni/b360/jvm).
 * Rene poslal 23. 11. log z testu online Segy: telefon 1 hru zalozil 4x a kazdy pokus po par vterinach skoncil -
 * ale PROC, v logu nebylo (stav z C++ videla jen stranka). Ted ho Java sleduje sama (MainActivity.napOnlineHlidej)
 * a kazdou zmenu zapise: pripojovani, cislo hry, druhy hrac, hra bezi (odezva, zpozdeni), konec s duvodem z C++.
 *
 * B360 (Rene 23. 11. vecer: "telefony se sekaji ... kazdy jina chyba"): za hry navic kazdych 10 s PRUBEH (kolikrat
 * a jak dlouho emulator cekal na stisky druheho telefonu, zpozdeni ovladani, odezva), kazda zmena zpozdeni
 * (zadost / zvyseni / snizeni) a SYNCHRONIZACE po rozjeti (ve kterem otisku, co se lisilo, velikost stavu).
 * Vic radku najednou = oddelene '\n' (MainActivity je zapise kazdy zvlast).
 *
 * Vstup = radky klic=hodnota: krok (appka: STAHUJI / SPOUSTIM), start (kolikate spojeni od spusteni appky - novy
 * pokus se pozna, i kdyz skonci stejne jako minuly), faze, kod, druhy, chyba, konec, odezva, zpozdeni, overeno,
 * snimek, hraje, hrac1 a B360 zpozdeni_ted, cekani, cekani_ms, cekani_max_ms, zadosti, zvyseni, snizeni,
 * synchronizaci, synchronizovano, rozjeti_snimek, rozjeti_casti, stav_kb (NativeSegaCoreBridge.onlineStav).
 */
final class NapOnlineHlidac {
    static final long PRUBEH_KAZDYCH_MS = 10000;

    private String krok = "", start = "", faze = "NIC", kod = "", druhy = "", hraje = "0", konec = "";
    private boolean prvni = true;
    private long zadosti = 0, zvyseni = 0, snizeni = 0, synchronizaci = 0, synchronizovano = 0;
    private long prubehPosledniMs = 0;

    /** Radky stavu -> radek / radky do logu, kdyz se neco zmenilo; jinak null. Prvni volani si stav jen zapamatuje. */
    String zmena(String radky) {
        return zmena(radky, System.nanoTime() / 1000000L);
    }

    String zmena(String radky, long tedMs) {
        Map<String, String> m = NapOnlineHra.stav(radky);
        String nKrok = hodnota(m, "krok"), nStart = hodnota(m, "start"), nFaze = hodnota(m, "faze"), nKod = hodnota(m, "kod");
        String nDruhy = NapOnlineHra.jmeno(hodnota(m, "druhy")), nHraje = hodnota(m, "hraje");
        if (nFaze.isEmpty()) nFaze = "NIC";
        if (nHraje.isEmpty()) nHraje = "0";
        String nKonec = hodnota(m, "konec");
        if (nKonec.isEmpty()) nKonec = hodnota(m, "chyba");
        boolean novyPokus = !nStart.equals(start);
        boolean zmena = !nKrok.equals(krok) || novyPokus || !nFaze.equals(faze) || !nKod.equals(kod)
                || !nHraje.equals(hraje) || ("HRAJE".equals(nFaze) && !nDruhy.equals(druhy)) || ("KONEC".equals(nFaze) && !nKonec.equals(konec));
        boolean poprve = prvni;
        boolean hralo = "HRAJE".equals(faze) && "1".equals(hraje);
        prvni = false;
        krok = nKrok; start = nStart; faze = nFaze; kod = nKod; druhy = nDruhy; hraje = nHraje; konec = nKonec;
        // B360: citace prubehu (novy pokus = od nuly)
        long nZadosti = cislo(m, "zadosti"), nZvyseni = cislo(m, "zvyseni"), nSnizeni = cislo(m, "snizeni");
        long nSync = cislo(m, "synchronizaci"), nSynchronizovano = cislo(m, "synchronizovano");
        if (novyPokus || poprve) { zadosti = zvyseni = snizeni = synchronizaci = synchronizovano = 0; }
        StringBuilder ven = new StringBuilder();
        if (!poprve && zmena) {
            String r = radekZmeny(m, nKrok, nStart, nFaze, nKod, nDruhy, nHraje, nKonec);
            if (r != null) pridej(ven, r);
        }
        boolean hraje1 = "HRAJE".equals(nFaze) && "1".equals(nHraje);
        if (!poprve && hraje1) {
            boolean hrac1 = "1".equals(hodnota(m, "hrac1"));
            if (nZadosti > zadosti)
                pridej(ven, "B360 ONLINE cekal jsem na stisky druheho telefonu (nejdel " + hodnota(m, "cekani_max_ms")
                        + " ms) - zadam ho o vetsi zpozdeni (zadost c. " + nZadosti + ")");
            if (nZvyseni > zvyseni)
                pridej(ven, "B360 ONLINE zpozdeni ovladani ZVYSENO na " + hodnota(m, "zpozdeni_ted")
                        + " snimku - druhy telefon cekal na moje stisky (sit kolisa)");
            if (nSnizeni > snizeni)
                pridej(ven, "B360 ONLINE zpozdeni ovladani snizeno na " + hodnota(m, "zpozdeni_ted") + " snimku (sit je klidna)");
            if (nSync > synchronizaci) {
                String casti = hodnota(m, "rozjeti_casti");
                pridej(ven, "B360 ONLINE SYNCHRONIZACE c. " + nSync + ": hra se rozjela"
                        + (hodnota(m, "rozjeti_snimek").isEmpty() || "0".equals(hodnota(m, "rozjeti_snimek")) ? "" : " (otisk ve snimku " + hodnota(m, "rozjeti_snimek") + ")")
                        + (casti.isEmpty() ? "" : ", lisi se: " + casti)
                        + (hrac1 ? " - posilam druhemu telefonu cely stav hry" : " - nahravam stav hry od hrace 1"));
            }
            if (nSynchronizovano > synchronizovano)
                pridej(ven, "B360 ONLINE synchronizace c. " + nSynchronizovano + " hotova (stav hry " + hodnota(m, "stav_kb") + " KB) - hraje se dal");
            if (!hralo) prubehPosledniMs = tedMs;
            else if (tedMs - prubehPosledniMs >= PRUBEH_KAZDYCH_MS) {
                prubehPosledniMs = tedMs;
                pridej(ven, "B360 ONLINE PRUBEH snimek " + hodnota(m, "snimek") + ": " + prubeh(m) + ", odezva " + hodnota(m, "odezva") + " ms");
            }
        }
        if (nZadosti > zadosti) zadosti = nZadosti;
        if (nZvyseni > zvyseni) zvyseni = nZvyseni;
        if (nSnizeni > snizeni) snizeni = nSnizeni;
        if (nSync > synchronizaci) synchronizaci = nSync;
        if (nSynchronizovano > synchronizovano) synchronizovano = nSynchronizovano;
        return ven.length() == 0 ? null : ven.toString();
    }

    private static String radekZmeny(Map<String, String> m, String nKrok, String nStart, String nFaze, String nKod,
                                     String nDruhy, String nHraje, String nKonec) {
        if ("STAHUJI".equals(nKrok)) return "B358 ONLINE stahuji hru hrace 1 ze serveru N&P";
        if ("SPOUSTIM".equals(nKrok)) return "B358 ONLINE spoustim stejnou hru z HRY";
        switch (nFaze) {
            case "PRIPOJUJE":
                return "B358 ONLINE pripojuji se k enginu N&P (pokus " + nStart + ")";
            case "CEKA_NA_HRACE":
                return nKod.isEmpty() ? "B358 ONLINE spojeno s enginem, zakladam hru" : "B358 ONLINE ceka na hrace 2 - cislo hry " + nKod;
            case "HRAJE":
                if (!"1".equals(nHraje)) return "B358 ONLINE domlouva se s hracem " + nDruhy;
                return "B358 ONLINE HRAJE s " + nDruhy + " - odezva " + hodnota(m, "odezva") + " ms, zpozdeni " + hodnota(m, "zpozdeni") + " snimku";
            case "KONEC":
                return "B358 ONLINE KONEC (pokus " + nStart + "): " + (nKonec.isEmpty() ? "bez duvodu" : jedenRadek(nKonec))
                        + " | overeno " + hodnota(m, "overeno") + " otisku, snimek " + hodnota(m, "snimek")
                        + (m.containsKey("cekani_ms") ? " | " + prubeh(m) : "");
            default:
                return "B358 ONLINE nic";
        }
    }

    /** B360: "cekalo se 3x (120 ms, nejdel 70 ms), zpozdeni 2 -> 4, synchronizaci 0" */
    private static String prubeh(Map<String, String> m) {
        String zac = hodnota(m, "zpozdeni"), ted = hodnota(m, "zpozdeni_ted");
        return "cekalo se " + hodnota(m, "cekani") + "x (" + hodnota(m, "cekani_ms") + " ms, nejdel " + hodnota(m, "cekani_max_ms") + " ms)"
                + ", zpozdeni " + (zac.equals(ted) || ted.isEmpty() ? zac : zac + " -> " + ted)
                + ", synchronizaci " + (hodnota(m, "synchronizaci").isEmpty() ? "0" : hodnota(m, "synchronizaci"));
    }

    /** Konec sledovani: online skoncilo (nebo nebezi) a appka nic nedela. */
    boolean hotovo() {
        return ("KONEC".equals(faze) || "NIC".equals(faze)) && krok.isEmpty();
    }

    /** B360: online prave bezi (pripojuje se, ceka na hrace, hraje) nebo appka stahuje / spousti hru - drzet Wi-Fi bez usporneho rezimu. */
    boolean aktivni() {
        return !krok.isEmpty() || "PRIPOJUJE".equals(faze) || "CEKA_NA_HRACE".equals(faze) || "HRAJE".equals(faze);
    }

    private static void pridej(StringBuilder b, String r) {
        if (b.length() > 0) b.append('\n');
        b.append(r);
    }

    private static long cislo(Map<String, String> m, String k) {
        try { return Long.parseLong(hodnota(m, k)); } catch (NumberFormatException e) { return 0; }
    }

    private static String hodnota(Map<String, String> m, String k) {
        String v = m.get(k);
        return v == null ? "" : v.trim();
    }

    /** Duvod konce z C++ muze obsahovat telo odpovedi enginu (JSON) - do logu jeden radek, nejvys 400 znaku. */
    private static String jedenRadek(String s) {
        String r = s.replace('\r', ' ').replace('\n', ' ');
        return r.length() > 400 ? r.substring(0, 400) + "..." : r;
    }
}
