package eu.atarihelp.emu10;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.file.Files;
import java.util.Random;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * B339: ULOZISTE A PS1 HRY V TELEVIZI (JVM, skutecna trida appky NapPravidla).
 *  U1) velikost pro cloveka CZ (desetinna carka) / EN (tecka): kB, MB, GB, neznama "?",
 *  U2) kolik mista chce PS1: stazeni 2x + 200 MB (nezname 1,6 GB), rozbaleni ZIPu = soucet + 200 MB,
 *      volno 0 (nejde zjistit) stahovani nebrani - jako drive,
 *  U3) hlaska MALO MISTA: CZ / EN, TV (ZPET -> HRY V TELEVIZI) / telefon (CD ISO -> kos), bez "v telefonu"
 *      a bez nepravdy o automatickem uklidu; cela se vejde do pruhu na strance PS1 (110 znaku vc. PS1_REMOTE_FAIL),
 *  U4) jmeno hry v knihovne: katalog N&P, soubor bez pripony, CUE slozene appkou -> nejvetsi soubor,
 *  U5) OPTIONS -> ULOZISTE: veta CZ / EN (TV / zarizeni), varovani pod 1 GB, nejde zjistit,
 *  Z)  skutecny ZIP (java.util.zip): .bin v ZIPu zmenseny na polovinu - kontrola pred stazenim (2x ZIP)
 *      projde, ale na rozbaleni by misto nestacilo; B339 kontrola podle velikosti v ZIPu (ZipFile.getSize,
 *      stejne jako appka) to pozna PRED rozbalenim.
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b339_java.txt
 */
public class TestB339 {
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

    public static void main(String[] a) throws Exception {
        // ---- U1 ----
        String c1 = NapPravidla.velikost(1536, false), c2 = NapPravidla.velikost(650 * MB, false),
               c3 = NapPravidla.velikost((long) (2.14 * GB), false), c4 = NapPravidla.velikost(8 * GB, false),
               e3 = NapPravidla.velikost((long) (2.14 * GB), true), n = NapPravidla.velikost(-1, true);
        over(c1.equals("2 kB") && c2.equals("650 MB") && c3.equals("2,1 GB") && c4.equals("8,0 GB") && e3.equals("2.1 GB") && n.equals("?"),
                "U1) velikost: 1536 B = " + c1 + ", 650 MB = " + c2 + ", 2,14 GB = " + c3 + " / EN " + e3 + ", 8 GB = " + c4 + ", nezname " + n);

        // ---- U2 ----
        boolean u2 = NapPravidla.ps1PotrebaMista(500 * MB) == 1200 * MB && NapPravidla.ps1PotrebaMista(-1) == 1600 * MB
                && NapPravidla.ps1PotrebaMista(0) == 1600 * MB && NapPravidla.ps1PotrebaRozbaleni(600 * MB) == 800 * MB
                && NapPravidla.ps1PotrebaRozbaleni(-5) == 200 * MB && NapPravidla.dostMista(0, 800 * MB)
                && !NapPravidla.dostMista(799 * MB, 800 * MB) && NapPravidla.dostMista(800 * MB, 800 * MB);
        over(u2, "U2) potreba: hra 500 MB -> " + NapPravidla.velikost(NapPravidla.ps1PotrebaMista(500 * MB), false)
                + ", velikost neznama -> " + NapPravidla.velikost(NapPravidla.ps1PotrebaMista(-1), false)
                + ", rozbaleni 600 MB -> " + NapPravidla.velikost(NapPravidla.ps1PotrebaRozbaleni(600 * MB), false)
                + ", volno 0 (nejde zjistit) nebrani, 799 MB < 800 MB brani");

        // ---- U3 ----
        String tvCz = NapPravidla.ps1MaloMista(1536 * MB, 999 * MB, true, false);
        String telCz = NapPravidla.ps1MaloMista(1536 * MB, 999 * MB, false, false);
        String tvEn = NapPravidla.ps1MaloMista(1536 * MB, 999 * MB, true, true);
        String telEn = NapPravidla.ps1MaloMista(1536 * MB, 999 * MB, false, true);
        int nejdelsi = 0;
        for (String s : new String[]{tvCz, telCz, tvEn, telEn}) nejdelsi = Math.max(nejdelsi, ("PS1_REMOTE_FAIL " + s).length());
        boolean bezStareho = true;
        for (String s : new String[]{tvCz, telCz, tvEn, telEn})
            if (s.contains("telefonu") || s.contains("uklizena") || s.contains("phone")) bezStareho = false;
        over(tvCz.equals("MÁLO MÍSTA: potřeba ~1,5 GB, volno 999 MB – smaž starou PS1 hru: ZPĚT → HRY V TELEVIZI")
                        && telCz.endsWith("CD ISO → koš") && tvEn.endsWith("BACK → GAMES ON THIS TV") && telEn.endsWith("CD ISO → bin")
                        && bezStareho && nejdelsi <= 110,
                "U3) MALO MISTA: TV \"" + tvCz + "\", telefon \"" + telCz + "\", EN \"" + tvEn + "\"; nejdelsi s PS1_REMOTE_FAIL "
                        + nejdelsi + " znaku (pruh na strance ukaze 110)");

        // ---- U4 ----
        String j1 = NapPravidla.ps1JmenoHry("Crash Bandicoot", "game.cue", null);
        String j2 = NapPravidla.ps1JmenoHry("", "Tekken 3 (Europe).cue", null);
        String j3 = NapPravidla.ps1JmenoHry(null, "_ps1_synth.cue", "Wipeout XL (Track 01).bin");
        String j4 = NapPravidla.ps1JmenoHry(null, "Doom.chd", null);
        String j5 = NapPravidla.ps1JmenoHry(null, "", null);
        over(j1.equals("Crash Bandicoot") && j2.equals("Tekken 3 (Europe)") && j3.equals("Wipeout XL (Track 01)") && j4.equals("Doom") && j5.equals("PS1"),
                "U4) jmeno hry: katalog N&P \"" + j1 + "\", soubor \"" + j2 + "\", CUE slozene appkou \"" + j3 + "\", .chd \"" + j4 + "\", nic \"" + j5 + "\"");

        // ---- U5 ----
        String p1 = NapPravidla.ulozistePopis((long) (2.1 * GB), (long) (4.5 * GB), 0, 0, true, false);
        String p2 = NapPravidla.ulozistePopis(800 * MB, (long) (4.5 * GB), 2, (long) (1.1 * GB), true, false);
        String p3 = NapPravidla.ulozistePopis((long) (2.1 * GB), (long) (4.5 * GB), 1, 650 * MB, false, true);
        String p4 = NapPravidla.ulozistePopis(0, 0, 0, 0, true, false);
        over(p1.equals("Volno 2,1 GB z 4,5 GB · PS1 hry v televizi: 0 · stažení PS1 hry chce volno asi 2× její velikost + 200 MB")
                        && p2.contains("PS1 hry v televizi: 2 (1,1 GB)") && p2.endsWith("MÁLO MÍSTA – smaž PS1 hru, kterou už nehraješ")
                        && p3.startsWith("2.1 GB free of 4.5 GB · PS1 games on this device: 1 (650 MB)") && !p3.contains("LOW STORAGE")
                        && p4.equals("Úložiště nejde zjistit"),
                "U5) OPTIONS ULOZISTE: \"" + p1 + "\" | pod 1 GB: \"" + p2 + "\" | EN: \"" + p3 + "\" | nejde: \"" + p4 + "\"");

        // ---- Z: skutecny ZIP ----
        File tmp = Files.createTempDirectory("b339zip").toFile();
        File zip = new File(tmp, "hra.zip");
        byte[] bin = new byte[4 * 1024 * 1024];
        Random r = new Random(339);
        for (int i = 0; i < bin.length; i++) bin[i] = (byte) (r.nextInt(4) == 0 ? r.nextInt(256) : 0);   // jako PS1 .bin: hodne nul
        try (ZipOutputStream zo = new ZipOutputStream(new FileOutputStream(zip))) {
            zo.putNextEntry(new ZipEntry("Hra (Europe)/Hra (Europe).bin"));
            zo.write(bin);
            zo.closeEntry();
            zo.putNextEntry(new ZipEntry("Hra (Europe)/Hra (Europe).cue"));
            zo.write("FILE \"Hra (Europe).bin\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n".getBytes("ISO-8859-1"));
            zo.closeEntry();
        }
        long zipB = zip.length(), rozbaleno = 0;
        try (ZipFile zf = new ZipFile(zip)) {
            java.util.Enumeration<? extends ZipEntry> en = zf.entries();
            while (en.hasMoreElements()) {
                ZipEntry ze = en.nextElement();
                String jm = ze.getName().toLowerCase();
                if (!ze.isDirectory() && (jm.endsWith(".bin") || jm.endsWith(".cue")) && ze.getSize() > 0) rozbaleno += ze.getSize();
            }
        }
        // zmensene na skutecnou velikost TV: ZIP ~ 300 MB (pomer z ZIPu nahore)
        double pomer = (double) rozbaleno / zipB;
        long zipTv = 300 * MB, binTv = (long) (zipTv * pomer);
        long volnoPred = NapPravidla.ps1PotrebaMista(zipTv);            // presne tolik, aby prosla kontrola pred stazenim
        boolean predProjde = NapPravidla.dostMista(volnoPred, NapPravidla.ps1PotrebaMista(zipTv));
        long volnoPoStazeni = volnoPred - zipTv;                         // ZIP lezi na disku
        boolean rozbaleniProjde = NapPravidla.dostMista(volnoPoStazeni, NapPravidla.ps1PotrebaRozbaleni(binTv));
        boolean opravduBySeVesel = volnoPoStazeni - binTv >= 0;
        over(pomer > 1.5 && rozbaleno == bin.length + 75 && predProjde && !rozbaleniProjde,
                "Z) ZIP " + zipB / 1024 + " kB -> rozbaleno " + rozbaleno / 1024 + " kB (ZipFile.getSize, pomer " + String.format("%.2f", pomer)
                        + "); na TV: ZIP 300 MB, volno " + NapPravidla.velikost(volnoPred, false) + " = kontrola pred stazenim projde, po stazeni volno "
                        + NapPravidla.velikost(volnoPoStazeni, false) + ", rozbalit " + NapPravidla.velikost(binTv, false)
                        + (opravduBySeVesel ? " (vesel by se, ale bez rezervy 200 MB)" : " (disk by dosel v pulce)")
                        + " -> B339 MALO MISTA pred rozbalenim");
        zip.delete();
        tmp.delete();

        out.append("VYSLEDEK: ").append(testu).append(" testu, ").append(chyb).append(" CHYBA\n");
        System.out.println("VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA");
        try (FileOutputStream f = new FileOutputStream(a.length > 0 ? a[0] : "VYSTUP_test_b339_java.txt")) { f.write(out.toString().getBytes("UTF-8")); }
        System.exit(chyb == 0 ? 0 : 1);
    }
}
