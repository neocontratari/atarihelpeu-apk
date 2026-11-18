package eu.atarihelp.emu10;

/**
 * B345: MISTO NA PS1 HRU BEZ ZIPU (JVM, skutecna trida appky NapPravidla).
 *  M1) hra bez ZIPu (.chd .pbp .bin .iso) chce velikost + 200 MB, ZIP 2x velikost + 200 MB, velikost neznama 1,6 GB
 *      (obe), puvodni ps1PotrebaMista(delka) = ZIP (nic, co ji vola postaru, nepovoli min),
 *  M2) priklad: volno 2,4 GB a hra 1,2 GB - bez ZIPu se ted vejde (drive MALO MISTA), v ZIPu porad ne
 *      (ZIP i rozbalena hra najednou); hranice presne na bajt,
 *  M3) OPTIONS -> ULOZISTE: veta CZ / EN rika "velikost + 200 MB, hra v ZIPu 2x velikost + 200 MB",
 *  M4) hlaska MALO MISTA (B339) se nezmenila,
 *  M5) hezke jmeno hry (NapPravidla.ps1JmenoHry - B345 ho bere stranka PS1 pri stahovani a nahravani):
 *      katalog N&P ma prednost, jinak soubor bez pripony; stitek ulozenych pozic "nap_ps1-...__hra.cue"
 *      appka uz na obrazovku neposila (MainActivity.ps1JmenoHryZeSouboru bere jmeno souboru, ne stitek).
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b345_java.txt
 */
public class TestB345 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();
    static final long MB = 1024L * 1024, GB = 1024L * MB;

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    static String v(long b) { return NapPravidla.velikost(b, false); }

    public static void main(String[] a) throws Exception {
        // ---- M1 ----
        long bez = NapPravidla.ps1PotrebaMista(500 * MB, false), zip = NapPravidla.ps1PotrebaMista(500 * MB, true);
        boolean m1 = bez == 700 * MB && zip == 1200 * MB
                && NapPravidla.ps1PotrebaMista(-1, false) == 1600 * MB && NapPravidla.ps1PotrebaMista(0, false) == 1600 * MB
                && NapPravidla.ps1PotrebaMista(-1, true) == 1600 * MB
                && NapPravidla.ps1PotrebaMista(500 * MB) == zip && NapPravidla.PS1_REZERVA == 200 * MB;
        over(m1, "M1) hra 500 MB: bez ZIPu " + v(bez) + ", ZIP " + v(zip) + ", velikost neznama "
                + v(NapPravidla.ps1PotrebaMista(-1, false)) + " (obe), stara ps1PotrebaMista(delka) = ZIP");

        // ---- M2 ----
        long volno = (long) (2.4 * GB), hra = (long) (1.2 * GB);
        long drive = hra * 2 + NapPravidla.PS1_REZERVA;           // do B345 vzdy 2x
        boolean m2 = !NapPravidla.dostMista(volno, drive)
                && NapPravidla.dostMista(volno, NapPravidla.ps1PotrebaMista(hra, false))
                && !NapPravidla.dostMista(volno, NapPravidla.ps1PotrebaMista(hra, true))
                && NapPravidla.dostMista(hra + 200 * MB, NapPravidla.ps1PotrebaMista(hra, false))
                && !NapPravidla.dostMista(hra + 200 * MB - 1, NapPravidla.ps1PotrebaMista(hra, false));
        over(m2, "M2) volno " + v(volno) + ", hra " + v(hra) + ": drive chtela " + v(drive) + " = MALO MISTA, ted bez ZIPu "
                + v(NapPravidla.ps1PotrebaMista(hra, false)) + " = vejde se, v ZIPu " + v(NapPravidla.ps1PotrebaMista(hra, true))
                + " = porad MALO MISTA; hranice velikost + 200 MB presne na bajt");

        // ---- M3 ----
        String cz = NapPravidla.ulozistePopis((long) (2.1 * GB), (long) (4.5 * GB), 0, 0, true, false);
        String en = NapPravidla.ulozistePopis((long) (2.1 * GB), (long) (4.5 * GB), 1, 650 * MB, false, true);
        over(cz.equals("Volno 2,1 GB z 4,5 GB · PS1 hry v televizi: 0 · stažení PS1 hry chce volno asi její velikost + 200 MB (hra v ZIPu 2× velikost + 200 MB)")
                && en.equals("2.1 GB free of 4.5 GB · PS1 games on this device: 1 (650 MB) · downloading a PS1 game needs about its size + 200 MB free (a game in a ZIP 2× its size + 200 MB)"),
                "M3) OPTIONS -> ULOZISTE: \"" + cz + "\" / \"" + en + "\"");

        // ---- M4 ----
        String mm = NapPravidla.ps1MaloMista(1536 * MB, 999 * MB, true, false);
        over(mm.equals("MÁLO MÍSTA: potřeba ~1,5 GB, volno 999 MB – smaž starou PS1 hru: ZPĚT → HRY V TELEVIZI"),
                "M4) MALO MISTA beze zmeny: \"" + mm + "\"");

        // ---- M5 ----
        String j1 = NapPravidla.ps1JmenoHry("Nickelodeon SpongeBob SquarePants - SuperSponge (USA)", "Nickelodeon_SpongeBob_SquarePants_-_SuperSponge__USA_.cue", null);
        String j2 = NapPravidla.ps1JmenoHry(null, "Final_Fantasy_VII_disc1.zip", null);
        String j3 = NapPravidla.ps1JmenoHry(null, "Crash.bin", null);
        String j4 = NapPravidla.ps1JmenoHry(null, "_ps1_synth.cue", "Wipeout XL (Track 01).bin");
        over(j1.equals("Nickelodeon SpongeBob SquarePants - SuperSponge (USA)") && j2.equals("Final_Fantasy_VII_disc1")
                && j3.equals("Crash") && j4.equals("Wipeout XL (Track 01)") && !j1.contains("nap_ps1-"),
                "M5) jmeno na obrazovku: katalog \"" + j1 + "\", soubor \"" + j2 + "\", vyber souboru \"" + j3
                + "\", CUE slozene appkou \"" + j4 + "\"");

        String souhrn = "VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA";
        out.append(souhrn).append('\n');
        System.out.println(souhrn);
        if (a.length > 0) java.nio.file.Files.write(java.nio.file.Paths.get(a[0]), out.toString().getBytes("UTF-8"));
        System.exit(chyb == 0 ? 0 : 1);
    }
}
