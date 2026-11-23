package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B360: CO JE NOVEHO po B360 - NapNovinky nad SKUTECNYM assets/novinky/novinky.js.
 *  1) nejnovejsi zaznam = B360 = versionCode z build.gradle (zasada Reneho - kazdy build ma svuj zaznam)
 *  2) telefon s B359 (videno 402): okno jen B360; telefon s B358 (nic si nepamatuje): B360 + B359 a odkaz do Napovedy
 *     (B358 uz videl - okno nema s kazdym buildem rust), anglicky totez
 * Pouziti: java eu.atarihelp.emu10.TestB360Novinky <koren repozitare> VYSTUP.txt
 */
public class TestB360Novinky {
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
        out.println("B360 - co je noveho: okno nejvys 2 verze (NapNovinky nad skutecnym novinky.js)");
        String js = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/novinky/novinky.js")), StandardCharsets.UTF_8);
        String gradle = new String(Files.readAllBytes(Paths.get(koren, "app/build.gradle")), StandardCharsets.UTF_8);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;
        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(!vse.isEmpty() && vse.get(0).verze == verze && "B360".equals(vse.get(0).build) && vse.get(0).cs.size() == 5 && vse.get(0).en.size() == 5,
             "1) nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze) + " (build.gradle " + verze + "), 5 vet cesky i anglicky");

        List<NapNovinky.Zaznam> z359 = NapNovinky.nove(js, 402, verze), z358 = NapNovinky.nove(js, NapNovinky.VIDENO_PRED_B359, verze);
        String t359 = NapNovinky.textOkna(z359, false), t358 = NapNovinky.textOkna(z358, false), e358 = NapNovinky.textOkna(z358, true);
        over("B360".equals(builds(z359)) && t359.startsWith("B360 · 23. 11. 2026\n• Online Sega: když se hra na telefonech rozjede") && !t359.contains("Nápověda"),
             "2) telefon s B359: okno jen B360 (" + t359.split("\n").length + " radku), bez odkazu");
        over("B360,B359,B358".equals(builds(z358)) && t358.contains("\n\nB359 · 23. 11. 2026\n• Co je nového:") && !t358.contains("B358 ·")
                && t358.endsWith("\n\nStarší novinky: Nápověda → CO JE NOVÉHO.") && "CO JE NOVÉHO · B360".equals(NapNovinky.nadpis(z358, false)),
             "   telefon s B358 (nic si nepamatuje, nove " + builds(z358) + "): okno B360 + B359 a odkaz do Napovedy, B358 ne");
        over(e358.startsWith("B360 · 23. 11. 2026\n• Sega online: when the game drifts apart") && e358.endsWith("\n\nOlder news: Help → WHAT'S NEW.")
                && !e358.contains("Nápověda"),
             "   anglicky: B360 + B359, odkaz 'Older news: Help → WHAT'S NEW.'");
        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
