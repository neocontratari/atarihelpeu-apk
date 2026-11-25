package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B363: CO JE NOVEHO po B363 - NapNovinky nad SKUTECNYM assets/novinky/novinky.js.
 *  1) nejnovejsi zaznam = B363 = versionCode z build.gradle (zasada Reneho - kazdy build ma svuj zaznam), 5 vet
 *  2) telefon s B362 (videno 405): okno jen B363; s B361 (404): B363 + B362; s B360 (403): B363 + B362 a odkaz
 *     do Napovedy; anglicky totez
 *  3) okno rika, kde co je - presne jako appka: ONLINE na pristroji Atari, ZALOZIT HRU / PRIPOJIT SE a OBA NA
 *     JOYSTICKU 1 v okne ONLINE (MainActivity), setrne kresleni v C++ (Atari i Sega)
 * Pouziti: java eu.atarihelp.emu10.TestB363Novinky <koren repozitare> VYSTUP.txt
 */
public class TestB363Novinky {
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
        out.println("B363 - co je noveho: online Atari, setrne kresleni (NapNovinky nad skutecnym novinky.js)");
        String js = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/novinky/novinky.js")), StandardCharsets.UTF_8);
        String gradle = new String(Files.readAllBytes(Paths.get(koren, "app/build.gradle")), StandardCharsets.UTF_8);
        String main = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/java/eu/atarihelp/emu10/MainActivity.java")), StandardCharsets.UTF_8);
        String pristroj = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/cpp/atari/nap_atari_device.h")), StandardCharsets.UTF_8);
        String atari = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/cpp/atari/nap_atari_native.cpp")), StandardCharsets.UTF_8);
        String sega = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/cpp/nap_sega_native_proof.cpp")), StandardCharsets.UTF_8);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;
        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(!vse.isEmpty() && vse.get(0).verze == verze && "B363".equals(vse.get(0).build) && vse.get(0).cs.size() == 5 && vse.get(0).en.size() == 5,
             "1) nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze) + " (build.gradle " + verze + "), 5 vet cesky i anglicky");

        List<NapNovinky.Zaznam> z362 = NapNovinky.nove(js, 405, verze), z361 = NapNovinky.nove(js, 404, verze), z360 = NapNovinky.nove(js, 403, verze);
        String t362 = NapNovinky.textOkna(z362, false), e362 = NapNovinky.textOkna(z362, true);
        String t361 = NapNovinky.textOkna(z361, false), t360 = NapNovinky.textOkna(z360, false), e360 = NapNovinky.textOkna(z360, true);
        over("B363".equals(builds(z362)) && t362.startsWith("B363 · 25. 11. 2026\n• Online Atari 130XE: na přístroji klepni na ONLINE → ZALOŽIT HRU")
                && !t362.contains("Nápověda") && "CO JE NOVÉHO · B363".equals(NapNovinky.nadpis(z362, false)),
             "2) telefon s B362: okno jen B363 (" + t362.split("\n").length + " radku), bez odkazu, nadpis 'CO JE NOVÉHO · B363'");
        over("B363,B362".equals(builds(z361)) && t361.contains("\n\nB362 · 24. 11. 2026\n• Online Sega: když stisky druhého telefonu přijdou pozdě") && !t361.contains("Nápověda"),
             "   telefon s B361 (nove " + builds(z361) + "): okno B363 + B362, bez odkazu");
        over("B363,B362,B361".equals(builds(z360)) && !t360.contains("B361 ·") && t360.endsWith("\n\nStarší novinky: Nápověda → CO JE NOVÉHO."),
             "   telefon s B360 (nove " + builds(z360) + "): okno B363 + B362 a odkaz do Napovedy, B361 ne");
        over(e362.startsWith("B363 · 25. 11. 2026\n• Online Atari 130XE: on the device tap ONLINE → CREATE A GAME")
                && e360.endsWith("\n\nOlder news: Help → WHAT'S NEW."),
             "   anglicky: B363 a u telefonu s B360 'Older news: Help → WHAT'S NEW.'");
        // nazvy v okne = nazvy v appce
        over(t362.contains("ONLINE → ZALOŽIT HRU") && t362.contains("PŘIPOJIT SE") && t362.contains("OBA NA JOYSTICKU 1")
                && pristroj.contains("{\"\",\"ONLINE\",0x2fd36b,false}")
                && main.contains("tx(\"ZALOŽIT HRU\", \"CREATE A GAME\")") && main.contains("tx(\"PŘIPOJIT SE\", \"JOIN\")")
                && main.contains("tx(\"OBA NA JOYSTICKU 1 – hry, kde se hráči střídají\", \"BOTH ON JOYSTICK 1 – games where the players take turns\")")
                && e362.contains("ONLINE → CREATE A GAME") && e362.contains("BOTH ON JOYSTICK 1"),
             "3) okno rika presne, co v appce je: ONLINE na pristroji, ZALOZIT HRU / PRIPOJIT SE, OBA NA JOYSTICKU 1 (CZ i EN)");
        over(t362.contains("šetrné kreslení") && t362.contains("(Atari i Sega)") && atari.contains("SETRNE KRESLENI zapnuto")
                && sega.contains("if (online == 1 && g_online_setrne.load()) divisor = std::max(divisor, 2);"),
             "   setrne kresleni je v obou jadrech: Atari (kazdy 2. snimek na displej) i Sega (delitel obrazu 2)");
        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
