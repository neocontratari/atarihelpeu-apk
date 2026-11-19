package eu.atarihelp.emu10;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.nio.file.Files;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * B356: Sonic a BIOS, kdyz do Download/AtariHelp zapsat nejde (Xiaomi brachy, Android 16, EACCES u souboru, ktery
 * tam zustal po drivejsi instalaci appky). Skutecny NapStahovaniSeSouhlasem (ulozSoubor, ulozZeZipu, prelozVysledek,
 * najdiSegaRom). "Cizi" soubor tu hraje ADRESAR stejneho jmena - zapis do nej skonci chybou jako EACCES
 * (test bezi jako root, prava souboru by nic nezastavila).
 */
public class TestB356Uloziste {

    static int ok = 0, chyb = 0;
    static void over(boolean podminka, String co) {
        if (podminka) { ok++; System.out.println("OK    " + co); }
        else { chyb++; System.out.println("CHYBA " + co); }
    }

    static byte[] zip(String jmeno, byte[] obsah) throws Exception {
        ByteArrayOutputStream bo = new ByteArrayOutputStream();
        try (ZipOutputStream z = new ZipOutputStream(bo)) {
            z.putNextEntry(new ZipEntry("cti_me.txt"));
            z.write("readme".getBytes("UTF-8"));
            z.closeEntry();
            z.putNextEntry(new ZipEntry("slozka/" + jmeno));
            z.write(obsah);
            z.closeEntry();
        }
        return bo.toByteArray();
    }

    public static void main(String[] a) throws Exception {
        File tmp = Files.createTempDirectory("b356u").toFile();
        File download = new File(tmp, "storage/emulated/0/Download/AtariHelp");
        File appka = new File(tmp, "storage/emulated/0/Android/data/eu.atarihelp.emu10.test/files/AtariHelp");
        download.mkdirs();
        appka.mkdirs();
        NapStahovaniSeSouhlasem.dalsiKoren = appka;
        final StringBuilder log = new StringBuilder();
        NapStahovaniSeSouhlasem.log = s -> log.append(s).append('\n');

        byte[] bios = new byte[524288];
        for (int i = 0; i < bios.length; i++) bios[i] = (byte) (i * 31);
        byte[] sonic = new byte[65536];
        for (int i = 0; i < sonic.length; i++) sonic[i] = (byte) (i * 7);

        System.out.println("== 1) nahradni slozka = stejna podslozka ve slozce appky");
        File segaDl = new File(new File(download, "emu"), "sega");
        File biosDl = new File(download, "PS1_BIOS");
        over(new File(new File(appka, "emu"), "sega").equals(NapStahovaniSeSouhlasem.nahradniSlozka(segaDl)), "Download/AtariHelp/emu/sega -> slozka appky/emu/sega");
        over(new File(appka, "PS1_BIOS").equals(NapStahovaniSeSouhlasem.nahradniSlozka(biosDl)), "Download/AtariHelp/PS1_BIOS -> slozka appky/PS1_BIOS");
        over(NapStahovaniSeSouhlasem.nahradniSlozka(new File(appka, "PS1_BIOS")) == null, "uz ve slozce appky -> zadna dalsi nahrada");

        System.out.println("== 2) normalne (zapis jde) - beze zmeny, do Download");
        File biosOk = new File(tmp, "normal/Download/AtariHelp/PS1_BIOS");
        String r = NapStahovaniSeSouhlasem.ulozSoubor(null, bios, "scph1001.bin", biosOk, "bios", NapStahovaniSeSouhlasem.PRIPONY_BIOS);
        over("bios=OK(1)".equals(r) && new File(biosOk, "scph1001.bin").length() == 524288, r + " -> Download/AtariHelp/PS1_BIOS/scph1001.bin");

        System.out.println("== 3) BIOS (scph5501.bin) v Download \"cizi\" -> slozka appky");
        new File(biosDl, "scph5501.bin").mkdirs();   // jako soubor po drivejsi instalaci: zapis skonci chybou
        r = NapStahovaniSeSouhlasem.ulozSoubor(null, bios, "scph5501.bin", biosDl, "bios", NapStahovaniSeSouhlasem.PRIPONY_BIOS);
        File v = new File(new File(appka, "PS1_BIOS"), "scph5501.bin");
        over("bios=OK(1,appka)".equals(r) && v.isFile() && v.length() == 524288, r + " -> " + v.getAbsolutePath().substring(tmp.getAbsolutePath().length()));
        over(log.toString().contains("B356 ULOZISTE: scph5501.bin do ") && log.toString().contains("ulozeno do slozky appky"), "log: " + log.toString().trim().split("\n")[0]);
        over(NapStahovaniSeSouhlasem.maBios(download), "maBios(Download/AtariHelp) najde BIOS ve slozce appky (dalsiKoren)");

        System.out.println("== 4) Sonic ze ZIPu (atarihelp.eu) v Download \"cizi\" -> slozka appky");
        new File(segaDl, "Sonic the Hedgehog.md").mkdirs();
        r = NapStahovaniSeSouhlasem.ulozZeZipu(null, zip("Sonic the Hedgehog.md", sonic), segaDl, "sonic", NapStahovaniSeSouhlasem.PRIPONY_SONIC);
        File s = new File(new File(new File(appka, "emu"), "sega"), "Sonic the Hedgehog.md");
        over("sonic=OK(1,appka)".equals(r) && s.isFile() && Files.readAllBytes(s.toPath()).length == sonic.length, r + " (readme.txt preskocen)");
        File rom = NapStahovaniSeSouhlasem.najdiSegaRom(download);
        over(rom != null && rom.equals(s), "najdiSegaRom(Download/AtariHelp) najde Sonica ve slozce appky");

        System.out.println("== 5) okno po stazeni: vysvetleni, kde soubory jsou");
        String z = "sonic=OK(1,appka) bios=OK(4,appka)";
        String cz = NapStahovaniSeSouhlasem.prelozVysledek(z, false, null, "Download/AtariHelp");
        String en = NapStahovaniSeSouhlasem.prelozVysledek(z, true, null, "Download/AtariHelp");
        over(cz.contains("✓ Sonic the Hedgehog - ulozen") && cz.contains("po drivejsi instalaci appky") && cz.contains("Appka si ho tam najde sama"), "CZ: " + cz.replace('\n', ' '));
        over(en.contains("earlier installation") && en.contains("finds it there by itself"), "EN: vysvetleni je");

        System.out.println("== 6) nejde ani Download, ani slozka appky -> spravna hlaska (ne 'povol Uloziste' na Androidu 11+)");
        File souborMistoSlozky = new File(tmp, "appka_je_soubor");
        Files.write(souborMistoSlozky.toPath(), new byte[] { 1 });
        NapStahovaniSeSouhlasem.dalsiKoren = souborMistoSlozky;   // slozka appky nejde vytvorit
        new File(biosDl, "scph7502.bin").mkdirs();
        r = NapStahovaniSeSouhlasem.ulozSoubor(null, bios, "scph7502.bin", biosDl, "bios", NapStahovaniSeSouhlasem.PRIPONY_BIOS);
        String hl = NapStahovaniSeSouhlasem.prelozVysledek("sonic=OK(1) " + r, false, null, "Download/AtariHelp");
        over(r.startsWith("bios=FileNotFoundException") && hl.contains("ani do Download/AtariHelp, ani do složky appky"), r + " -> " + hl.split("\n")[1]);
        over(!hl.contains("povol ho: Nastavení"), "stara veta 'appka nesmi ukladat ... povol ho' uz neni");
        File neniSlozka = new File(souborMistoSlozky, "PS1_BIOS");   // slozka nejde vytvorit (rodic je soubor), nahrada taky ne
        r = NapStahovaniSeSouhlasem.ulozSoubor(null, bios, "scph1001.bin", neniSlozka, "bios", NapStahovaniSeSouhlasem.PRIPONY_BIOS);
        over("bios=slozka-nejde".equals(r) && NapStahovaniSeSouhlasem.duvod("slozka-nejde", null, false).contains("ani do složky appky"),
                "slozka nejde vytvorit ani nahradni -> " + r + " (veta jako u EACCES)");
        NapStahovaniSeSouhlasem.dalsiKoren = appka;

        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
