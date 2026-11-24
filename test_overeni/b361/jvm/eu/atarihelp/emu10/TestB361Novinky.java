package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B361: CO JE NOVEHO po B361 - NapNovinky nad SKUTECNYM assets/novinky/novinky.js.
 *  1) nejnovejsi zaznam = B361 = versionCode z build.gradle (zasada Reneho - kazdy build ma svuj zaznam), 4 vety
 *  2) telefon s B360 (videno 403): okno jen B361 (POKEY stereo); s B359 (videno 402): B361 + B360, bez odkazu (nic se
 *     nevynechalo); s B358 (videno 401): B361 + B360 a odkaz do Napovedy (B359 uz se do okna nevejde, nejvys 2 verze);
 *     anglicky totez
 *  3) okno rika, kde se stereo zapina - presne nazev polozky z menu appky (POKEY STEREO (2 ČIPY) / (2 CHIPS))
 * Pouziti: java eu.atarihelp.emu10.TestB361Novinky <koren repozitare> VYSTUP.txt
 */
public class TestB361Novinky {
    static int ok = 0, chyb = 0;
    static PrintWriter out;

    static void over(boolean podminka, String co) {
        if (podminka) ok++; else chyb++;
        String r = (podminka ? "OK     " : "CHYBA  ") + co;
        System.out.println(r);
        out.println(r);
    }

    static String builds(List<NapNovinky.Zaznam> z) {
        StringBuilder b = new StringBuilder();
        for (NapNovinky.Zaznam x : z) b.append(b.length() > 0 ? "," : "").append(x.build);
        return b.toString();
    }

    public static void main(String[] a) throws Exception {
        String koren = a[0];
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[1]), StandardCharsets.UTF_8));
        out.println("B361 - co je noveho: POKEY stereo (NapNovinky nad skutecnym novinky.js)");
        String js = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/novinky/novinky.js")), StandardCharsets.UTF_8);
        String gradle = new String(Files.readAllBytes(Paths.get(koren, "app/build.gradle")), StandardCharsets.UTF_8);
        String main = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/java/eu/atarihelp/emu10/MainActivity.java")), StandardCharsets.UTF_8);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;
        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(!vse.isEmpty() && vse.get(0).verze == verze && "B361".equals(vse.get(0).build) && vse.get(0).cs.size() == 4 && vse.get(0).en.size() == 4,
             "1) nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze) + " (build.gradle " + verze + "), 4 vety cesky i anglicky");

        List<NapNovinky.Zaznam> z360 = NapNovinky.nove(js, 403, verze), z359 = NapNovinky.nove(js, 402, verze), z358 = NapNovinky.nove(js, 401, verze);
        String t360 = NapNovinky.textOkna(z360, false), e360 = NapNovinky.textOkna(z360, true);
        String t359 = NapNovinky.textOkna(z359, false), t358 = NapNovinky.textOkna(z358, false), e358 = NapNovinky.textOkna(z358, true);
        over("B361".equals(builds(z360)) && t360.startsWith("B361 · 24. 11. 2026\n• Atari 130XE má druhý zvukový čip POKEY") && !t360.contains("Nápověda")
                && "CO JE NOVÉHO · B361".equals(NapNovinky.nadpis(z360, false)),
             "2) telefon s B360: okno jen B361 (" + t360.split("\n").length + " radku), bez odkazu, nadpis 'CO JE NOVÉHO · B361'");
        over("B361,B360".equals(builds(z359)) && t359.startsWith("B361 · ") && t359.contains("\n\nB360 · 23. 11. 2026\n• Online Sega:") && !t359.contains("Nápověda"),
             "   telefon s B359 (nove " + builds(z359) + "): okno B361 + B360, bez odkazu");
        over("B361,B360,B359".equals(builds(z358)) && t358.contains("\n\nB360 · 23. 11. 2026\n") && !t358.contains("B359 ·")
                && t358.endsWith("\n\nStarší novinky: Nápověda → CO JE NOVÉHO."),
             "   telefon s B358 (nove " + builds(z358) + "): okno B361 + B360 a odkaz do Napovedy, B359 ne");
        over(e360.startsWith("B361 · 24. 11. 2026\n• The Atari 130XE has a second POKEY sound chip") && e358.endsWith("\n\nOlder news: Help → WHAT'S NEW.") && !e358.contains("Nápověda"),
             "   anglicky: B361 a u telefonu s B358 'Older news: Help → WHAT'S NEW.'");
        // nazev polozky v okne = nazev v menu appky (MainActivity.atariPokeyStereoPolozka, i \\uXXXX by se tu neshodovalo)
        over(t360.contains("POKEY STEREO (2 ČIPY)") && main.contains("tx(\"POKEY STEREO (2 ČIPY): ZAPNUTO ✓\"")
                && e360.contains("POKEY STEREO (2 CHIPS)") && main.contains("\"POKEY STEREO (2 CHIPS): ON ✓\")"),
             "3) okno rika, kde se stereo zapina: 'POKEY STEREO (2 ČIPY)' / '(2 CHIPS)' - presne jako polozka v menu appky");
        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
