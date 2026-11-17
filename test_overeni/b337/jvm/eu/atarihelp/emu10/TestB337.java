package eu.atarihelp.emu10;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * B337: STAZENI SONICA A BIOSU NA TELEVIZI (JVM, skutecna trida NapStahovaniSeSouhlasem z appky).
 *  S1) zaloha N&P: ktery soubor ze seznamu her serveru je Sonic (ne dily 2 / 3 / & Knuckles) a BIOS (prednost PS1),
 *  S2) zaloha: ZIP ze serveru se rozbali jako z atarihelp.eu ("sonic=OK(1,N&P)"), na serveru neni ->
 *      "sonic=<chyba atarihelp.eu>+NP:neni", server nedostupny -> "+NP:server-nedostupny",
 *  S3) ulozeni: BIOS ZIP (jen .bin 512 kB), primo scphNNNN.bin, spatna velikost, webova stranka misto ZIPu,
 *      slozka, kam nejde zapsat,
 *  S4) srozumitelny duvod v okne "Nestazeno" (povoleni uloziste, mistni server, certifikat, DNS, spojeni, HTTP,
 *      stranka misto souboru) CZ i EN, slozka v okne podle toho, kam se ukladalo,
 *  S5) Sonic a BIOS se najdou i ve slozce appky (dalsiKoren - Android 9 bez povoleni uloziste).
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b337_java.txt
 */
public class TestB337 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    static Map<String, Object> hra(String id, String system, String soubor) {
        Map<String, Object> m = new HashMap<>();
        m.put("id", id);
        m.put("system", system);
        m.put("soubor", soubor);
        m.put("nazev", soubor);
        return m;
    }

    static byte[] zip(String[] jmena, byte[][] data) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        try (ZipOutputStream z = new ZipOutputStream(b)) {
            for (int i = 0; i < jmena.length; i++) {
                z.putNextEntry(new ZipEntry(jmena[i]));
                z.write(data[i]);
                z.closeEntry();
            }
        }
        return b.toByteArray();
    }

    public static void main(String[] a) throws Exception {
        // ---------------- S1 ----------------
        List<Object> hry = new ArrayList<>();
        hry.add(hra("sega-1", "sega", "Sonic The Hedgehog 2 (World).zip"));
        hry.add(hra("sega-2", "sega", "Sonic & Knuckles (World).md"));
        hry.add(hra("sega-3", "sega", "Sonic-The-Hedgehog-USA-Europe.zip"));
        hry.add(hra("sega-4", "sega", "Streets of Rage 2.zip"));
        hry.add(hra("atari-5", "atari", "sonic.xex"));
        hry.add(hra("ps1-6", "ps1", "Crash Bandicoot.chd"));
        hry.add(hra("ps1-7", "ps1", "PS1-BIOS_.zip"));
        hry.add(hra("atari-8", "atari", "bios_xl.rom"));
        String s1 = NapStahovaniSeSouhlasem.vyberZeSeznamu(hry, "sonic");
        String b1 = NapStahovaniSeSouhlasem.vyberZeSeznamu(hry, "bios");
        List<Object> jen2 = new ArrayList<>();
        jen2.add(hra("sega-9", "sega", "Sonic The Hedgehog 2.md"));
        jen2.add(hra("atari-10", "atari", "sonic.xex"));
        String s2 = NapStahovaniSeSouhlasem.vyberZeSeznamu(jen2, "sonic");
        List<Object> scph = new ArrayList<>();
        scph.add(hra("ps1-11", "ps1", "scph1001.bin"));
        scph.add(hra("ps1-12", "ps1", "Tekken 3.chd"));
        String b2 = NapStahovaniSeSouhlasem.vyberZeSeznamu(scph, "bios");
        List<Object> nic = new ArrayList<>();
        nic.add(hra("atari-13", "atari", "sonic.xex"));
        nic.add(hra("ps1-14", "ps1", "Tekken 3.chd"));
        over("sega-3".equals(s1) && "ps1-7".equals(b1) && "sega-9".equals(s2) && "ps1-11".equals(b2)
                        && NapStahovaniSeSouhlasem.vyberZeSeznamu(nic, "sonic") == null && NapStahovaniSeSouhlasem.vyberZeSeznamu(nic, "bios") == null
                        && NapStahovaniSeSouhlasem.vyberZeSeznamu(null, "sonic") == null,
                "S1) zaloha N&P: Sonic = " + s1 + " (ne dil 2 / & Knuckles / Atari), BIOS = " + b1 + " (PS1 pred Atari), jen dil 2 -> "
                        + s2 + ", scph1001.bin -> " + b2 + ", nic / null -> null");

        // ---------------- S2 ----------------
        File tmp = Files.createTempDirectory("b337").toFile();
        byte[] bios = new byte[524288];
        bios[0] = 0x13;
        byte[] biosZip = zip(new String[]{"PS1 BIOS/scph1001.bin", "precti.txt"}, new byte[][]{bios, "info".getBytes()});
        byte[] sonic = new byte[524288 / 2];
        byte[] sonicZip = zip(new String[]{"Sonic The Hedgehog (USA, Europe).md"}, new byte[][]{sonic});
        File sega = new File(tmp, "emu/sega"), ps1 = new File(tmp, "PS1_BIOS");
        String z1 = NapStahovaniSeSouhlasem.zeZalohy(null, co -> new NapStahovaniSeSouhlasem.Zdroj("sonic".equals(co) ? sonicZip : biosZip,
                "x.zip", "N&P"), "sonic", sega, NapStahovaniSeSouhlasem.PRIPONY_SONIC, "Sonic", null, "sonic=SSLHandshakeException");
        String z2 = NapStahovaniSeSouhlasem.zeZalohy(null, co -> null, "bios", ps1, NapStahovaniSeSouhlasem.PRIPONY_BIOS, "BIOS", null,
                "bios=SSLHandshakeException");
        String z3 = NapStahovaniSeSouhlasem.zeZalohy(null, co -> { throw new IOException("server-nedostupny"); }, "bios", ps1,
                NapStahovaniSeSouhlasem.PRIPONY_BIOS, "BIOS", null, "bios=HTTP503");
        String z4 = NapStahovaniSeSouhlasem.zeZalohy(null, co -> new NapStahovaniSeSouhlasem.Zdroj(biosZip, "PS1-BIOS_.zip", "N&P"), "bios", ps1,
                NapStahovaniSeSouhlasem.PRIPONY_BIOS, "BIOS", null, "bios=UnknownHostException");
        over("sonic=OK(1,N&P)".equals(z1) && new File(sega, "Sonic The Hedgehog (USA, Europe).md").length() == sonic.length
                        && "bios=SSLHandshakeException+NP:neni".equals(z2) && "bios=HTTP503+NP:server-nedostupny".equals(z3)
                        && "bios=OK(1,N&P)".equals(z4) && new File(ps1, "scph1001.bin").length() == 524288 && !new File(ps1, "precti.txt").exists(),
                "S2) zaloha N&P: " + z1 + " | na serveru neni: " + z2 + " | server nedostupny: " + z3 + " | BIOS ZIP: " + z4
                        + " (jen .bin, bez textu)");

        // ---------------- S3 ----------------
        File t3 = new File(tmp, "t3");
        String u1 = NapStahovaniSeSouhlasem.ulozSoubor(null, bios, "scph5502.bin", t3, "bios", ".bin");
        String u2 = NapStahovaniSeSouhlasem.ulozSoubor(null, new byte[1000], "scph5502.bin", new File(tmp, "t3b"), "bios", ".bin");
        String u3 = NapStahovaniSeSouhlasem.ulozZeZipu(null, "<!doctype html><html><body>WEDOS ochrana</body></html>".getBytes(), new File(tmp, "t3c"), "sonic", NapStahovaniSeSouhlasem.PRIPONY_SONIC);
        File soubor = new File(tmp, "soubor");
        Files.write(soubor.toPath(), new byte[]{1});
        String u4 = NapStahovaniSeSouhlasem.ulozZeZipu(null, sonicZip, new File(soubor, "emu/sega"), "sonic", NapStahovaniSeSouhlasem.PRIPONY_SONIC);
        String u5 = NapStahovaniSeSouhlasem.ulozSoubor(null, "<html>".getBytes(), "index.html", new File(tmp, "t3d"), "sonic", NapStahovaniSeSouhlasem.PRIPONY_SONIC);
        over("bios=OK(1)".equals(u1) && new File(t3, "scph5502.bin").length() == 524288 && "bios=jiny-soubor".equals(u2)
                        && "sonic=nic-v-zipu".equals(u3) && "sonic=slozka-nejde".equals(u4) && "sonic=jiny-soubor".equals(u5),
                "S3) ulozeni: scph5502.bin " + u1 + ", 1000 B jako BIOS " + u2 + ", stranka misto ZIPu " + u3
                        + ", slozka nejde " + u4 + ", HTML " + u5);

        // ---------------- S4 ----------------
        String[][] pripady = {
                {"slozka-nejde", null, "Úložiště"},
                {"FileNotFoundException(EACCES)", null, "Úložiště"},
                {"SSLHandshakeException", "certifikát localhost od ospanel", "MÍSTNÍ server (certifikát localhost od ospanel)"},
                {"SSLHandshakeException", null, "certifikát"},
                {"UnknownHostException", null, "nenašel"},
                {"SocketTimeoutException", null, "vypršelo"},
                {"ConnectException", null, "nepovedlo"},
                {"HTTP503", null, "HTTP 503"},
                {"nic-v-zipu", null, "webová stránka"},
        };
        StringBuilder sb = new StringBuilder();
        boolean vse = true;
        for (String[] p : pripady) {
            String d = NapStahovaniSeSouhlasem.duvod(p[0], p[1], false);
            boolean ok = d.contains(p[2]);
            vse &= ok;
            sb.append(p[0]).append(ok ? " ok" : " -> '" + d + "'").append("; ");
        }
        String en = NapStahovaniSeSouhlasem.duvod("FileNotFoundException(EACCES)", null, true);
        String okDuvod = NapStahovaniSeSouhlasem.duvod("OK(1)", null, false);
        String okno = NapStahovaniSeSouhlasem.prelozVysledek("sonic=OK(1,N&P) bios=SSLHandshakeException+NP:neni", false, null,
                "Android/data/eu.atarihelp.emu10.test/files/AtariHelp");
        String oknoEn = NapStahovaniSeSouhlasem.prelozVysledek("sonic=slozka-nejde bios=OK(1)", true, null, "Download/AtariHelp");
        String slozkaApp = NapStahovaniSeSouhlasem.popisSlozky(null, new File("/storage/emulated/0/Android/data/eu.atarihelp.emu10.test/files/AtariHelp"));
        String slozkaDl = NapStahovaniSeSouhlasem.popisSlozky(null, new File("/storage/emulated/0/Download/AtariHelp"));
        vse &= en.contains("storage permission") && okDuvod.isEmpty()
                && okno.contains("✓ Sonic the Hedgehog - ulozen") && okno.contains("✗ BIOS pro PlayStation - nestazen: zabezpečené spojení")
                && okno.contains("(ze serveru N&P") && okno.contains("Slozka: Android/data/eu.atarihelp.emu10.test/files/AtariHelp")
                && okno.contains("Podrobnosti: sonic=OK(1,N&P) bios=SSLHandshakeException+NP:neni")
                && oknoEn.contains("✗ Sonic the Hedgehog - not downloaded: the app may not save files (storage permission)")
                && oknoEn.contains("Folder: Download/AtariHelp")
                && "Android/data/eu.atarihelp.emu10.test/files/AtariHelp".equals(slozkaApp) && "Download/AtariHelp".equals(slozkaDl)
                && "OK(1,N&P)".equals(NapStahovaniSeSouhlasem.polozka("sonic=OK(1,N&P) bios=X", "sonic"))
                && "X".equals(NapStahovaniSeSouhlasem.polozka("sonic=OK(1,N&P) bios=X", "bios"));
        over(vse, "S4) duvod v okne Nestazeno: " + sb + "EN ok, OK bez duvodu, okno: ["
                + okno.split("\n")[0] + " | " + okno.split("\n")[1] + "], slozka appky / Download");

        // ---------------- S5 ----------------
        File dl = new File(tmp, "Download/AtariHelp");
        dl.mkdirs();
        File app = new File(tmp, "Android/data/files/AtariHelp");
        new File(app, "emu/sega").mkdirs();
        new File(app, "PS1_BIOS").mkdirs();
        Files.write(new File(app, "emu/sega/Sonic.md").toPath(), sonic);
        Files.write(new File(app, "PS1_BIOS/scph1001.bin").toPath(), bios);
        NapStahovaniSeSouhlasem.dalsiKoren = null;
        boolean predtim = NapStahovaniSeSouhlasem.najdiSegaRom(dl) == null && !NapStahovaniSeSouhlasem.maBios(dl);
        NapStahovaniSeSouhlasem.dalsiKoren = app;
        File rom = NapStahovaniSeSouhlasem.najdiSegaRom(dl);
        boolean b = NapStahovaniSeSouhlasem.maBios(dl);
        boolean chybi = NapStahovaniSeSouhlasem.neceMChybi(dl);
        NapStahovaniSeSouhlasem.dalsiKoren = null;
        over(predtim && rom != null && rom.getName().equals("Sonic.md") && b && !chybi,
                "S5) slozka appky (Android 9 bez povoleni): bez dalsiKoren nic, s ni Sonic " + (rom == null ? "-" : rom.getName())
                        + ", BIOS " + (b ? "je" : "CHYBI") + ", neceMChybi=" + chybi);

        out.append("VYSLEDEK: ").append(testu).append(" testu, ").append(chyb).append(" CHYBA\n");
        System.out.println("VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA");
        String kam = a.length > 0 ? a[0] : "VYSTUP_test_b337_java.txt";
        try (FileOutputStream f = new FileOutputStream(kam)) { f.write(out.toString().getBytes("UTF-8")); }
        System.exit(chyb == 0 ? 0 : 1);
    }
}
