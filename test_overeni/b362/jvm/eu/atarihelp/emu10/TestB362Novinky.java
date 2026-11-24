package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B362: CO JE NOVEHO po B362 - NapNovinky nad SKUTECNYM assets/novinky/novinky.js.
 *  1) nejnovejsi zaznam = B362 = versionCode z build.gradle (zasada Reneho - kazdy build ma svuj zaznam), 5 vet
 *  2) telefon s B361 (videno 404): okno jen B362; s B360 (403): B362 + B361, bez odkazu; s B359 (402): B362 + B361
 *     a odkaz do Napovedy; anglicky totez
 *  3) okno rika, kde co je - presne jako appka: tlacitko ONLINE na pristroji Atari (C++ nap_atari_device.h),
 *     LOG / CHYBA v HELP (MainActivity.atariCppHelpDialog), 🌐 ONLINE na Seze
 * Pouziti: java eu.atarihelp.emu10.TestB362Novinky <koren repozitare> VYSTUP.txt
 */
public class TestB362Novinky {
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
        out.println("B362 - co je noveho: online s odhadem, Atari ONLINE (NapNovinky nad skutecnym novinky.js)");
        String js = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/novinky/novinky.js")), StandardCharsets.UTF_8);
        String gradle = new String(Files.readAllBytes(Paths.get(koren, "app/build.gradle")), StandardCharsets.UTF_8);
        String main = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/java/eu/atarihelp/emu10/MainActivity.java")), StandardCharsets.UTF_8);
        String pristroj = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/cpp/atari/nap_atari_device.h")), StandardCharsets.UTF_8);
        String sega = new String(Files.readAllBytes(Paths.get(koren, "app/src/main/assets/emu_sega/index.html")), StandardCharsets.UTF_8);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;
        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(!vse.isEmpty() && vse.get(0).verze == verze && "B362".equals(vse.get(0).build) && vse.get(0).cs.size() == 5 && vse.get(0).en.size() == 5,
             "1) nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze) + " (build.gradle " + verze + "), 5 vet cesky i anglicky");

        List<NapNovinky.Zaznam> z361 = NapNovinky.nove(js, 404, verze), z360 = NapNovinky.nove(js, 403, verze), z359 = NapNovinky.nove(js, 402, verze);
        String t361 = NapNovinky.textOkna(z361, false), e361 = NapNovinky.textOkna(z361, true);
        String t360 = NapNovinky.textOkna(z360, false), t359 = NapNovinky.textOkna(z359, false), e359 = NapNovinky.textOkna(z359, true);
        over("B362".equals(builds(z361)) && t361.startsWith("B362 · 24. 11. 2026\n• Online Sega: když stisky druhého telefonu přijdou pozdě, hra už nestojí")
                && !t361.contains("Nápověda") && "CO JE NOVÉHO · B362".equals(NapNovinky.nadpis(z361, false)),
             "2) telefon s B361: okno jen B362 (" + t361.split("\n").length + " radku), bez odkazu, nadpis 'CO JE NOVÉHO · B362'");
        over("B362,B361".equals(builds(z360)) && t360.contains("\n\nB361 · 24. 11. 2026\n• Atari 130XE má druhý zvukový čip POKEY") && !t360.contains("Nápověda"),
             "   telefon s B360 (nove " + builds(z360) + "): okno B362 + B361, bez odkazu");
        over("B362,B361,B360".equals(builds(z359)) && !t359.contains("B360 ·") && t359.endsWith("\n\nStarší novinky: Nápověda → CO JE NOVÉHO."),
             "   telefon s B359 (nove " + builds(z359) + "): okno B362 + B361 a odkaz do Napovedy, B360 ne");
        over(e361.startsWith("B362 · 24. 11. 2026\n• Online Sega: when the other phone's presses arrive late, the game no longer stops")
                && e359.endsWith("\n\nOlder news: Help → WHAT'S NEW."),
             "   anglicky: B362 a u telefonu s B359 'Older news: Help → WHAT'S NEW.'");
        // nazvy v okne = nazvy v appce
        over(t361.contains("místo tlačítka LOG / CHYBA je ONLINE") && pristroj.contains("{\"\",\"ONLINE\",0x2fd36b,false}") && !pristroj.contains("{\"LOG\",\"CHYBA\"")
                && t361.contains("najdeš v HELP – tlačítko LOG / CHYBA dole") && main.contains(".setNeutralButton(tx(\"LOG / CHYBA\", \"LOG / ERROR\"), (d, w) -> atariCppLogChyba())")
                && e361.contains("the LOG / ERROR button at the bottom") && sega.contains("ONLINE"),
             "3) okno rika presne, co v appce je: ONLINE na pristroji Atari (misto LOG / CHYBA), LOG / CHYBA dole v HELP (CZ i EN)");
        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
