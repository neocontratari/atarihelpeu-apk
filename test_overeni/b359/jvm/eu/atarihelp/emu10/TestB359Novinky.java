package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B359: CO JE NOVEHO - NapNovinky (ciste Java) nad SKUTECNYM assets/novinky/novinky.js.
 * Rene 23. 11. 2026: "v kazde aktualizaci musi byt videt co je noveho - to je zasada".
 *  1) soubor se precte cely, novejsi prvni, nejnovejsi = versionCode z app/build.gradle
 *  2) co telefon jeste nevidel: z B357 (nic si nepamatuje) -> B359 + B358; z B358 -> B359; aktualni -> nic
 *  3) okno cesky a anglicky (nadpis, verze s datem, odrazky), chybejici preklad -> druhy jazyk
 *  4) poskozeny soubor appku neshodi (prazdny seznam), poradi podle verze
 * Pouziti: java eu.atarihelp.emu10.TestB359Novinky &lt;koren repozitare&gt; VYSTUP.txt
 */
public class TestB359Novinky {
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
        out.println("B359 - co je noveho: NapNovinky nad skutecnym assets/novinky/novinky.js");
        String js = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/novinky/novinky.js")), StandardCharsets.UTF_8);
        String gradle = new String(Files.readAllBytes(Paths.get(koren, "app/build.gradle")), StandardCharsets.UTF_8);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;

        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(vse.size() >= 4 && vse.get(0).verze == verze && "B359".equals(vse.get(0).build) && !vse.get(0).cs.isEmpty() && !vse.get(0).en.isEmpty(),
             "1) novinky.js: " + vse.size() + " zaznamu (" + builds(vse) + "), nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze)
             + " (build.gradle " + verze + ")");

        List<NapNovinky.Zaznam> zB357 = NapNovinky.nove(js, NapNovinky.VIDENO_PRED_B359, verze);
        List<NapNovinky.Zaznam> zB358 = NapNovinky.nove(js, 401, verze);
        List<NapNovinky.Zaznam> nic = NapNovinky.nove(js, verze, verze);
        over("B359,B358".equals(builds(zB357)) && "B359".equals(builds(zB358)) && nic.isEmpty() && "B359,B358,B357,B356".equals(builds(NapNovinky.nove(js, 0, verze))),
             "2) telefon z B357 (nic si nepamatuje) -> " + builds(zB357) + "; z B358 -> " + builds(zB358) + "; aktualni -> nic");

        String cs = NapNovinky.text(zB357, false), en = NapNovinky.text(zB357, true);
        over("CO JE NOVÉHO · B359".equals(NapNovinky.nadpis(zB357, false)) && "WHAT'S NEW · B359".equals(NapNovinky.nadpis(zB357, true))
             && cs.startsWith("B359 · 23. 11. 2026\n• Co je nového: po každé aktualizaci appka jednou ukáže, co se změnilo.")
             && cs.contains("\n\nB358 · 23. 11. 2026\n• Sega online je jednodušší: velké tlačítko 🌐 ONLINE")
             && en.startsWith("B359 · 23. 11. 2026\n• What's new: after every update the app shows once what has changed.")
             && en.contains("\n\nB358 · 23. 11. 2026\n• Sega online is simpler:") && !en.contains("jednodušší"),
             "3) okno cesky \"" + NapNovinky.nadpis(zB357, false) + "\" (" + cs.split("\n").length + " radku), anglicky \"" + NapNovinky.nadpis(zB357, true) + "\"");

        String bezEn = "window.NAP_NOVINKY = [{\"verze\": 5, \"build\": \"B1\", \"datum\": \"1. 1.\", \"cs\": [\"jen cesky\"], \"en\": []}];";
        over(NapNovinky.text(NapNovinky.vse(bezEn), true).equals("B1 · 1. 1.\n• jen cesky"), "   chybi anglicky preklad -> cesky text (radsi nez prazdne okno)");

        String[] spatne = {null, "", "window.NAP_NOVINKY = [ {\"verze\": 1, \"cs\": [\"x\"", "NAP_NOVINKY = 5;", "[ a ] /* bez znacky */",
                           "window.NAP_NOVINKY = [{\"verze\": \"402\", \"cs\": [\"x\"], \"en\": [\"x\"]}, {\"build\": \"B2\", \"cs\": [], \"en\": []}];"};
        int prazdne = 0;
        for (String s : spatne) {
            try { if (NapNovinky.vse(s).isEmpty()) prazdne++; } catch (Throwable t) { /* spadlo = chyba */ }
        }
        over(prazdne == spatne.length, "4) poskozeny / prazdny soubor appku neshodi: " + prazdne + "/" + spatne.length + " -> prazdny seznam (okno se neukaze)");
        String neserazene = "window.NAP_NOVINKY = [{\"verze\": 7, \"build\": \"B7\", \"datum\": \"\", \"cs\": [\"a\"], \"en\": [\"a\"]},"
                + " {\"verze\": 9, \"build\": \"B9\", \"datum\": \"\", \"cs\": [\"b\"], \"en\": [\"b\"]}, {\"verze\": 8, \"build\": \"B8\", \"datum\": \"\", \"cs\": [\"c\"], \"en\": [\"c\"]}];";
        over("B9,B8,B7".equals(builds(NapNovinky.vse(neserazene))) && "B9,B8".equals(builds(NapNovinky.nove(neserazene, 7, 9))),
             "   poradi podle verze (novejsi prvni), i kdyz je v souboru jinak: " + builds(NapNovinky.vse(neserazene)));

        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
