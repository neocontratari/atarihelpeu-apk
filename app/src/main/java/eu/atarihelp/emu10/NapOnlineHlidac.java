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
 * B362 (Rene 24. 11.: "furt je ten online spatny ... testuju na mistni LAN" - a log druheho telefonu chybel): PRUBEH
 * navic s odhady a opravami (odhad stisku druheho), kazdych 10 s DRUHY TELEFON (jeho cisla prisla po siti - log
 * jednoho telefonu ukaze oba), TICHO (od druheho nic uz 2 / 5 / 10 s - co prislo naposledy), navrat hrace 2 pri
 * synchronizaci a oprava bez ulozeneho stavu. infoTelefonu(): text o telefonu pro druhy telefon (jen ASCII).
 *
 * Vstup = radky klic=hodnota: krok (appka: STAHUJI / SPOUSTIM), start (kolikate spojeni od spusteni appky - novy
 * pokus se pozna, i kdyz skonci stejne jako minuly), faze, kod, druhy, chyba, konec, odezva, zpozdeni, overeno,
 * snimek, hraje, hrac1 a B360 zpozdeni_ted, cekani, cekani_ms, cekani_max_ms, zadosti, zvyseni, snizeni,
 * synchronizaci, synchronizovano, rozjeti_snimek, rozjeti_casti, stav_kb (NativeSegaCoreBridge.onlineStav).
 * B362: odhad, odhad_sedelo, oprav, oprava_max, oprava_max_ms, opravy_ms, prepocteno, odhad_max, chyb_opravy,
 * navrat_ze, navrat_na, posledni_zprava, jeho_do, ticho_ms, druhy_info_c, druhy_info_snimek, druhy_info.
 */
final class NapOnlineHlidac {
    static final long PRUBEH_KAZDYCH_MS = 10000;

    private String krok = "", start = "", faze = "NIC", kod = "", druhy = "", hraje = "0", konec = "";
    private boolean prvni = true;
    private long zadosti = 0, zvyseni = 0, snizeni = 0, synchronizaci = 0, synchronizovano = 0;
    private long prubehPosledniMs = 0;
    // B362: statistika od druheho telefonu, ticho, navrat pri synchronizaci, oprava bez stavu
    private long druhyInfo = 0, chybOpravy = 0, navratZe = 0;
    private int tichoHlaseno = 0;   // kolik prahu ticha (2 / 5 / 10 s) uz je v logu
    static final long[] TICHO_PRAHY_MS = {2000, 5000, 10000};

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
        if (novyPokus || poprve) { zadosti = zvyseni = snizeni = synchronizaci = synchronizovano = 0; druhyInfo = chybOpravy = navratZe = 0; tichoHlaseno = 0; }
        long nDruhyInfo = cislo(m, "druhy_info_c"), nChybOpravy = cislo(m, "chyb_opravy"), nNavratZe = cislo(m, "navrat_ze");
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
            // B362: hrac 2 byl s odhadem uz za snimkem synchronizace - stav od hrace 1 ho vratil
            if (nNavratZe > 0 && nNavratZe != navratZe)
                pridej(ven, "B362 ONLINE synchronizace: byl jsem s odhadem uz na snimku " + nNavratZe + " - stav od hrace 1 me vratil na snimek "
                        + hodnota(m, "navrat_na"));
            if (nChybOpravy > chybOpravy)
                pridej(ven, "B362 ONLINE oprava odhadu nenasla ulozeny stav (" + nChybOpravy + "x) - rozdil poznaji otisky, synchronizace ho spravi");
            // B362: cisla druheho telefonu (prisla po siti) - log jednoho telefonu ukaze oba
            if (nDruhyInfo > druhyInfo && !hodnota(m, "druhy_info").isEmpty())
                pridej(ven, "B362 ONLINE DRUHY TELEFON (ja snimek " + hodnota(m, "druhy_info_snimek") + "): " + jedenRadek(hodnota(m, "druhy_info")));
            // B362: od druheho nic uz 2 / 5 / 10 s - kdy presne to zacalo a co prislo naposledy
            long ticho = cislo(m, "ticho_ms");
            if (ticho < 1000) tichoHlaseno = 0;
            else if (tichoHlaseno < TICHO_PRAHY_MS.length && ticho >= TICHO_PRAHY_MS[tichoHlaseno]) {
                while (tichoHlaseno < TICHO_PRAHY_MS.length && ticho >= TICHO_PRAHY_MS[tichoHlaseno]) tichoHlaseno++;
                long jehoDo = cislo(m, "jeho_do");
                pridej(ven, "B362 ONLINE TICHO: od druheho telefonu nic neprislo uz " + (ticho / 1000) + " s (posledni zprava: "
                        + (hodnota(m, "posledni_zprava").isEmpty() ? "-" : hodnota(m, "posledni_zprava"))
                        + (jehoDo > 0 ? ", jeho stisky do snimku " + (jehoDo - 1) : "") + ", ja snimek " + hodnota(m, "snimek")
                        + ", stojim " + hodnota(m, "stoji_ms") + " ms)");
            }
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
        if (nDruhyInfo > druhyInfo) druhyInfo = nDruhyInfo;
        if (nChybOpravy > chybOpravy) chybOpravy = nChybOpravy;
        if (nNavratZe > 0) navratZe = nNavratZe;
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

    /**
     * B360: "cekalo se 3x (120 ms, nejdel 70 ms), zpozdeni 2 -> 4, synchronizaci 0"; B362 navic (kdyz je C++ posila):
     * ", odhad 40 snimku (sedelo 35), oprav 5 (nejdel 6 snimku / 31 ms), limit odhadu 8"
     */
    private static String prubeh(Map<String, String> m) {
        String zac = hodnota(m, "zpozdeni"), ted = hodnota(m, "zpozdeni_ted");
        String r = "cekalo se " + hodnota(m, "cekani") + "x (" + hodnota(m, "cekani_ms") + " ms, nejdel " + hodnota(m, "cekani_max_ms") + " ms)"
                + ", zpozdeni " + (zac.equals(ted) || ted.isEmpty() ? zac : zac + " -> " + ted)
                + ", synchronizaci " + (hodnota(m, "synchronizaci").isEmpty() ? "0" : hodnota(m, "synchronizaci"));
        if (m.containsKey("odhad"))
            r += ", odhad " + hodnota(m, "odhad") + " snimku (sedelo " + hodnota(m, "odhad_sedelo") + "), oprav " + hodnota(m, "oprav")
                    + " (nejdel " + hodnota(m, "oprava_max") + " snimku / " + hodnota(m, "oprava_max_ms") + " ms), limit odhadu " + hodnota(m, "odhad_max");
        return r;
    }

    /**
     * B362: text o tomto telefonu pro druhy telefon (jde mu ve STATISTIKA, jen ASCII, nejvys 200 znaku):
     * "SM-A125F Android 12 (API 31), Wi-Fi -61 dBm 2.4 GHz 72 Mb/s, LOW_LATENCY, baterie 31.2 C, sporic vyp, teplo NONE".
     * rssi / frekvence / rychlost 0 = nevim (Wi-Fi vypnuta, bez opravneni); rezim "" = zamek Wi-Fi drzen neni.
     */
    static String infoTelefonu(String model, String android, int api, int rssiDbm, int frekvenceMhz, int rychlostMbs,
                               String rezimWifi, String baterieC, boolean sporic, String teplo) {
        StringBuilder b = new StringBuilder();
        b.append(model == null || model.isEmpty() ? "telefon" : model).append(" Android ").append(android == null ? "?" : android)
                .append(" (API ").append(api).append(")");
        if (rssiDbm < 0 || frekvenceMhz > 0) {
            b.append(", Wi-Fi");
            if (rssiDbm < 0) b.append(' ').append(rssiDbm).append(" dBm");
            if (frekvenceMhz > 0) b.append(' ').append(frekvenceMhz >= 5900 ? "6" : frekvenceMhz >= 4900 ? "5" : "2.4").append(" GHz");
            if (rychlostMbs > 0) b.append(' ').append(rychlostMbs).append(" Mb/s");
        } else b.append(", Wi-Fi ?");
        b.append(", ").append(rezimWifi == null || rezimWifi.isEmpty() ? "Wi-Fi zamek ne" : rezimWifi);
        if (baterieC != null && !baterieC.isEmpty() && !"na".equals(baterieC) && !"err".equals(baterieC)) b.append(", baterie ").append(baterieC).append(" C");
        b.append(", sporic ").append(sporic ? "ZAP" : "vyp");
        if (teplo != null && !teplo.isEmpty()) b.append(", teplo ").append(teplo);
        StringBuilder a = new StringBuilder();
        for (int i = 0; i < b.length() && a.length() < 200; i++) {
            char c = b.charAt(i);
            a.append(c >= 32 && c < 127 ? c : '?');
        }
        return a.toString();
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
