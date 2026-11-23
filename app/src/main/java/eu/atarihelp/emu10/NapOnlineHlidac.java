package eu.atarihelp.emu10;

import java.util.Map;

/**
 * B358: ZMENY STAVU ONLINE DO LOGU APPKY (test: test_overeni/b358/jvm).
 * Rene poslal 23. 11. log z testu online Segy: telefon 1 hru zalozil 4x a kazdy pokus po par vterinach skoncil -
 * ale PROC, v logu nebylo (stav z C++ videla jen stranka). Ted ho Java sleduje sama (MainActivity.napOnlineHlidej)
 * a kazdou zmenu zapise: pripojovani, cislo hry, druhy hrac, hra bezi (odezva, zpozdeni), konec s duvodem z C++.
 *
 * Vstup = radky klic=hodnota: krok (appka: STAHUJI / SPOUSTIM), start (kolikate spojeni od spusteni appky - novy
 * pokus se pozna, i kdyz skonci stejne jako minuly), faze, kod, druhy, chyba, konec, odezva, zpozdeni, overeno,
 * snimek, hraje (NativeSegaCoreBridge.onlineStav).
 */
final class NapOnlineHlidac {
    private String krok = "", start = "", faze = "NIC", kod = "", druhy = "", hraje = "0", konec = "";
    private boolean prvni = true;

    /** Radky stavu -> radek do logu, kdyz se neco zmenilo; jinak null. Prvni volani si stav jen zapamatuje. */
    String zmena(String radky) {
        Map<String, String> m = NapOnlineHra.stav(radky);
        String nKrok = hodnota(m, "krok"), nStart = hodnota(m, "start"), nFaze = hodnota(m, "faze"), nKod = hodnota(m, "kod");
        String nDruhy = NapOnlineHra.jmeno(hodnota(m, "druhy")), nHraje = hodnota(m, "hraje");
        if (nFaze.isEmpty()) nFaze = "NIC";
        if (nHraje.isEmpty()) nHraje = "0";
        String nKonec = hodnota(m, "konec");
        if (nKonec.isEmpty()) nKonec = hodnota(m, "chyba");
        boolean zmena = !nKrok.equals(krok) || !nStart.equals(start) || !nFaze.equals(faze) || !nKod.equals(kod)
                || !nHraje.equals(hraje) || ("HRAJE".equals(nFaze) && !nDruhy.equals(druhy)) || ("KONEC".equals(nFaze) && !nKonec.equals(konec));
        boolean poprve = prvni;
        prvni = false;
        krok = nKrok; start = nStart; faze = nFaze; kod = nKod; druhy = nDruhy; hraje = nHraje; konec = nKonec;
        if (poprve || !zmena) return null;
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
                        + " | overeno " + hodnota(m, "overeno") + " otisku, snimek " + hodnota(m, "snimek");
            default:
                return "B358 ONLINE nic";
        }
    }

    /** Konec sledovani: online skoncilo (nebo nebezi) a appka nic nedela. */
    boolean hotovo() {
        return ("KONEC".equals(faze) || "NIC".equals(faze)) && krok.isEmpty();
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
