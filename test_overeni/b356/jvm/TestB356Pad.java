package eu.atarihelp.emu10;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** B356: NapPad - texty a zapis padu (Java, WebView, konec predchoziho behu). */
public class TestB356Pad {

    static int ok = 0, chyb = 0;
    static void over(boolean podminka, String co) {
        if (podminka) { ok++; System.out.println("OK    " + co); }
        else { chyb++; System.out.println("CHYBA " + co); }
    }

    static void hloubka(int n) { if (n == 0) throw new IllegalStateException("test padu", new java.io.IOException("pricina")); hloubka(n - 1); }

    public static void main(String[] a) throws Exception {
        System.out.println("== 1) duvod konce od Androidu 11+ (ApplicationExitInfo.REASON_*)");
        over(NapPad.duvodKonce(4).contains("PAD JAVY") && NapPad.duvodKonce(5).contains("NATIVNI PAD") && NapPad.duvodKonce(6).contains("ANR")
                && NapPad.duvodKonce(3).contains("malo pameti") && NapPad.duvodKonce(10).contains("uzivatel"), "4 Java, 5 C++, 6 ANR, 3 pamet, 10 uzivatel");
        over(NapPad.dulezityKonec(4) && NapPad.dulezityKonec(5) && NapPad.dulezityKonec(6) && NapPad.dulezityKonec(3)
                && !NapPad.dulezityKonec(10) && !NapPad.dulezityKonec(16) && !NapPad.dulezityKonec(1), "<<< jen pady / zabiti, ne aktualizace a uzivatel");
        over(NapPad.duvodKonce(99).contains("99"), "neznamy kod: " + NapPad.duvodKonce(99));

        System.out.println("== 2) pad Javy - radek do logu");
        String s = null;
        try { hloubka(20); } catch (IllegalStateException e) { s = NapPad.popisPadu("nap-ps1-remote-download", e, 5); }
        over(s.startsWith("B356 PAD JAVA ve vlakne \"nap-ps1-remote-download\": java.lang.IllegalStateException: test padu"), s.split("\n")[0]);
        over(s.split("\n      at ").length == 1 + 5 + 1 && s.contains("pricina: java.io.IOException: pricina"), "5 radku zasobniku + pricina s 1 radkem");

        System.out.println("== 3) znacka behu -> veta o predchozim behu");
        over(NapPad.predchoziBeh("bezi|2026-11-19 21:49:09.229|EMU10-B356 (399)").startsWith("B356 PREDCHOZI BEH SKONCIL NAHLE"), "bezi = skoncila nahle (v popredi)");
        over(NapPad.predchoziBeh("pauza|2026-11-19 21:03:40.000|EMU10-B356 (399)") == null && NapPad.predchoziBeh("konec|x|y") == null
                && NapPad.predchoziBeh("") == null && NapPad.predchoziBeh(null) == null, "pauza / konec / nic = normalne, zadna veta");

        System.out.println("== 4) okamzity zapis (pripsat / prepsat)");
        File f = File.createTempFile("b356pad", ".txt");
        NapPad.zapisHned(f, "radek 1\n", false);
        NapPad.zapisHned(f, "radek 2\n", true);
        over("radek 1\nradek 2\n".equals(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)), "pripsat");
        NapPad.zapisHned(f, "jen tohle\n", false);
        over("jen tohle\n".equals(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)), "prepsat");
        NapPad.zapisHned(new File(f, "nejde/sem"), "x", true);   // chyba se jen ignoruje (v padu nesmi nic dalsiho spadnout)
        NapPad.zapisHned(null, "x", true);
        over(true, "neplatna cesta / null -> zadna vyjimka");

        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
