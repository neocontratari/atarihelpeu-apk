package eu.atarihelp.emu10;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B364: CO JE NOVEHO po B364 - NapNovinky nad SKUTECNYM assets/novinky/novinky.js.
 *  1) nejnovejsi zaznam = B364 = versionCode z build.gradle (zasada Reneho - kazdy build ma svuj zaznam), 5 vet
 *  2) telefon s B363 (videno 406): okno jen B364; s B362 (405): B364 + B363; s B361 (404): B364 + B363 a odkaz
 *     do Napovedy; anglicky totez
 *  3) okno rika, kde co je - presne jako appka: ZALOZIT HRU / HRA Z NASEHO SERVERU v okne ONLINE (MainActivity),
 *     OPTIONS -> AKTUALIZOVAT APLIKACI (NapAktualizace), EJECT na kazetaku; oprava CLOAD je v jadre (SKCTL)
 *     a stare nahravky opravuje nap_atari_tape.h
 * Pouziti: java eu.atarihelp.emu10.TestB364Novinky <koren repozitare> VYSTUP.txt
 */
public class TestB364Novinky {
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

    static String cti(String koren, String f) throws Exception {
        return new String(Files.readAllBytes(Paths.get(koren, f)), StandardCharsets.UTF_8);
    }

    public static void main(String[] a) throws Exception {
        String koren = a[0];
        out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(a[1]), StandardCharsets.UTF_8));
        out.println("B364 - co je noveho: CLOAD, hry ze serveru pro online, aktualizace (NapNovinky nad skutecnym novinky.js)");
        String js = cti(koren, "app/src/main/assets/novinky/novinky.js");
        String gradle = cti(koren, "app/build.gradle");
        String main = cti(koren, "app/src/main/java/eu/atarihelp/emu10/MainActivity.java");
        String akt = cti(koren, "app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java");
        String stroj = cti(koren, "app/src/main/cpp/atari/nap_atari_machine.h");
        String paska = cti(koren, "app/src/main/cpp/atari/nap_atari_tape.h");
        String pristroj = cti(koren, "app/src/main/cpp/atari/nap_atari_device.h");
        String menu = cti(koren, "app/src/main/assets/index.html");
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("versionCode\\s+(\\d+)").matcher(gradle);
        int verze = m.find() ? Integer.parseInt(m.group(1)) : -1;
        List<NapNovinky.Zaznam> vse = NapNovinky.vse(js);
        over(!vse.isEmpty() && vse.get(0).verze == verze && verze == 407 && "B364".equals(vse.get(0).build) && vse.get(0).cs.size() == 5 && vse.get(0).en.size() == 5,
             "1) nejnovejsi " + (vse.isEmpty() ? "?" : vse.get(0).build + " = versionCode " + vse.get(0).verze) + " (build.gradle " + verze + "), 5 vet cesky i anglicky");

        List<NapNovinky.Zaznam> z363 = NapNovinky.nove(js, 406, verze), z362 = NapNovinky.nove(js, 405, verze), z361 = NapNovinky.nove(js, 404, verze);
        String t363 = NapNovinky.textOkna(z363, false), e363 = NapNovinky.textOkna(z363, true);
        String t362 = NapNovinky.textOkna(z362, false), t361 = NapNovinky.textOkna(z361, false), e361 = NapNovinky.textOkna(z361, true);
        over("B364".equals(builds(z363)) && t363.startsWith("B364 · 25. 11. 2026\n• Kazeta: CLOAD už načte i program uložený přes CSAVE po resetu POKEY")
                && !t363.contains("Nápověda") && "CO JE NOVÉHO · B364".equals(NapNovinky.nadpis(z363, false)),
             "2) telefon s B363: okno jen B364 (" + t363.split("\n").length + " radku), bez odkazu, nadpis 'CO JE NOVÉHO · B364'");
        over("B364,B363".equals(builds(z362)) && t362.contains("\n\nB363 · 25. 11. 2026\n• Online Atari 130XE: na přístroji klepni na ONLINE") && !t362.contains("Nápověda"),
             "   telefon s B362 (nove " + builds(z362) + "): okno B364 + B363, bez odkazu");
        over("B364,B363,B362".equals(builds(z361)) && !t361.contains("B362 ·") && t361.endsWith("\n\nStarší novinky: Nápověda → CO JE NOVÉHO."),
             "   telefon s B361 (nove " + builds(z361) + "): okno B364 + B363 a odkaz do Napovedy, B362 ne");
        over(e363.startsWith("B364 · 25. 11. 2026\n• Tape: CLOAD now also loads a program saved with CSAVE after a POKEY reset")
                && e361.endsWith("\n\nOlder news: Help → WHAT'S NEW."),
             "   anglicky: B364 a u telefonu s B361 'Older news: Help → WHAT'S NEW.'");
        // nazvy v okne = nazvy v appce
        over(t363.contains("ZALOŽIT HRU teď nabídne hry z našeho serveru N&P") && e363.contains("CREATE A GAME now offers the games from our N&P server")
                && main.contains("tx(\"ZALOŽIT HRU\", \"CREATE A GAME\")") && main.contains("tx(\"HRA Z NAŠEHO SERVERU N&P\", \"A GAME FROM OUR N&P SERVER\")")
                && main.contains("tx(\"HRA ZALOŽENA – ČÍSLO \", \"GAME CREATED – NUMBER \")") && main.contains("tx(\"ONLINE – HRA PRO DVA\", \"ONLINE – A GAME FOR TWO\")")
                && pristroj.contains("{\"\",\"ONLINE\",0x2fd36b,false}"),
             "3) online: ZALOZIT HRU s hrami ze serveru, okno ONLINE krok za krokem a cislo hry (HRA ZALOZENA) - jako v MainActivity (CZ i EN)");
        over(t363.contains("OPTIONS → AKTUALIZOVAT APLIKACI") && e363.contains("OPTIONS → UPDATE THE APP")
                && menu.contains("<button id=\"stAktual\" type=\"button\">AKTUALIZOVAT APLIKACI</button>") && akt.contains("\"MÁŠ NEJNOVĚJŠÍ VERZI\""),
             "   aktualizace: OPTIONS → AKTUALIZOVAT APLIKACI (tlacitko v assets/index.html) rekne MÁŠ NEJNOVĚJŠÍ VERZI");
        over(t363.contains("POKE 53775,0") && t363.contains("přes EJECT") && e363.contains("with EJECT")
                && stroj.contains("serOutCounter = 0; serOutValid = false; serOutBusy = false;") && !stroj.contains("serOutState = false; serOutBusy = false;")
                && paska.contains("t.opravenoMezer++;"),
             "   kazeta: oprava v jadre (init rezim POKEY drzi posledni bit = klid MARK) a stare nahravky opravuje nacteni WAV (nap_atari_tape.h)");
        out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        out.close();
        System.exit(chyb == 0 ? 0 : 1);
    }
}
