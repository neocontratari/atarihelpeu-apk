package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

/**
 * B364: OPTIONS -> AKTUALIZOVAT APLIKACI uz nechce stahovat, kdyz appka nejnovejsi verzi ma. Rene 25. 11.: "apka pri
 * kontrole aktualizace v seting chce stahovat i kdyz ma nejnovejsi aktualizaci" (spustRucne do B363 vzdy volalo
 * zeptejSe = okno se stazenim, i pri stejne verzi).
 *  1) rucneCo: server ma vyssi verzi = nabidnout stazeni; stejnou nebo nizsi = "mas nejnovejsi", nic nestahovat;
 *     verze nezjistena (bez spojeni, 0) = "nevim"
 *  2) rucneText cesky i anglicky (stejna / novejsi u tebe / nevim) - cisla verzi v textu
 *  3) zdrojovy kod: spustRucne vola zeptejSe JEN pro RUCNE_NABIDNOUT, jinak okno MÁŠ NEJNOVĚJŠÍ VERZI s OK a
 *     PŘESTO STÁHNOUT ZNOVU (oprava zaseknute instalace B324 dal jde) - nazvy jako v napovede CZ / EN
 * NapAktualizace pouziva Android (okna, instalace) - preklad i beh s android.jar (stuby; volaji se jen ciste metody).
 * Pouziti: java -cp OUT:android-34.jar eu.atarihelp.emu10.TestB364Aktualizace <koren repozitare> VYSTUP.txt
 */
public class TestB364Aktualizace {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    public static void main(String[] a) throws Exception {
        String koren = a[0];
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[1]), StandardCharsets.UTF_8));
        out.println("B364 - AKTUALIZOVAT APLIKACI v OPTIONS: kdyz je appka nejnovejsi, nic nestahuje (NapAktualizace)");
        final int N = NapAktualizace.RUCNE_NABIDNOUT, J = NapAktualizace.RUCNE_NEJNOVEJSI, V = NapAktualizace.RUCNE_NEVIM;

        // ---------------------------------------------------------------- 1) co ukazat
        over(NapAktualizace.rucneCo(true, 408, 407) == N && NapAktualizace.rucneCo(true, 500, 407) == N,
                "1) server 408 / 500, moje 407 -> nabidnout stazeni (okno AKTUALIZOVAT / TED NE jako drive)");
        over(NapAktualizace.rucneCo(true, 407, 407) == J && NapAktualizace.rucneCo(true, 406, 407) == J,
                "   server 407 = moje 407 (Reneho pripad) a server 406 < moje 407 -> 'mas nejnovejsi', NIC SE NESTAHUJE");
        over(NapAktualizace.rucneCo(false, 0, 407) == V && NapAktualizace.rucneCo(true, 0, 407) == V && NapAktualizace.rucneCo(true, -1, 407) == V,
                "   verzi na serveru nezjistim (bez spojeni / 0) -> 'nevim', nic nestahovat bez ptani");

        // ---------------------------------------------------------------- 2) texty
        String cs = NapAktualizace.rucneText(false, J, 407, 407), en = NapAktualizace.rucneText(true, J, 407, 407);
        String csN = NapAktualizace.rucneText(false, J, 406, 407), enN = NapAktualizace.rucneText(true, J, 406, 407);
        String csV = NapAktualizace.rucneText(false, V, 0, 407), enV = NapAktualizace.rucneText(true, V, 0, 407);
        over("Máš nejnovější verzi 407 – na serveru N&P je stejná. Není co stahovat.".equals(cs)
                        && "You already have the latest version 407 – the N&P server has the same one. There is nothing to download.".equals(en),
                "2) stejna verze: '" + cs + "' / '" + en + "'");
        over(csN.equals("Máš verzi 407, novější než je na serveru N&P (406). Není co stahovat.")
                        && enN.equals("You have version 407, newer than the N&P server (406). There is nothing to download."),
                "   u tebe novejsi: '" + csN + "'");
        over(csV.startsWith("Nepodařilo se zjistit, jakou verzi má server N&P") && enV.startsWith("I could not find out which version the N&P server has"),
                "   nevim: '" + csV + "'");

        // ---------------------------------------------------------------- 3) zdrojovy kod a napoveda
        String src = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java")), StandardCharsets.UTF_8);
        String nap = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/napoveda/index.html")), StandardCharsets.UTF_8);
        String napEn = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/napoveda/en.html")), StandardCharsets.UTF_8);
        int sr = src.indexOf("public static void spustRucne("), konec = sr < 0 ? -1 : src.indexOf("\n    }\n", sr);
        String rucne = sr < 0 || konec < 0 ? "" : src.substring(sr, konec);
        over(rucne.contains("if (co == RUCNE_NABIDNOUT) zeptejSe(a, nova, nazev, moje, zip, log);")
                        && rucne.contains("else rucneBezStazeni(a, co, nova, moje, zip, log);")
                        && rucne.split("zeptejSe\\(", -1).length == 2,
                "3) spustRucne vola zeptejSe (okno se stazenim) jen pro RUCNE_NABIDNOUT, jinak rucneBezStazeni");
        over(src.contains("NapJazyk.t(a, \"MÁŠ NEJNOVĚJŠÍ VERZI\", \"YOU HAVE THE LATEST VERSION\")")
                        && src.contains("NapJazyk.t(a, \"PŘESTO STÁHNOUT ZNOVU\", \"DOWNLOAD AGAIN ANYWAY\")")
                        && src.contains("nainstaluj(a, zip, log);")
                        && nap.contains("<span class=\"t\">MÁŠ NEJNOVĚJŠÍ VERZI</span>") && nap.contains("<span class=\"t\">PŘESTO STÁHNOUT ZNOVU</span>")
                        && napEn.contains("<span class=\"t\">YOU HAVE THE LATEST VERSION</span>") && napEn.contains("<span class=\"t\">DOWNLOAD AGAIN ANYWAY</span>"),
                "   okno MÁŠ NEJNOVĚJŠÍ VERZI s PŘESTO STÁHNOUT ZNOVU (zaseknuta instalace B324 jde opravit dal) - napoveda CZ / EN ma stejne nazvy");

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
