package eu.atarihelp.emu10;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

/**
 * B312: JVM test ciste Javy z appky (bez Androidu):
 *  - NapServis: PBKDF2-HMAC-SHA256 proti Pythonu (hashlib), normalizace kodu, kodSedi,
 *    pocitadlo spatnych pokusu (pauza), platnost servisu 10 min; se skutecnym kodem
 *    z promenne NAP_SERVIS_KOD i otisk zabudovany v appce (kod se nikam nevypisuje)
 *  - NapPravidla: ktere stranky jsou SEGA / PS1, zamceno, duvod (stejna slova jako menu),
 *    hudba v seznamu MP3 prehravace, stranky se ZPET
 *
 *   javac -d OUT app/.../NapServis.java app/.../NapPravidla.java TestB312Java.java
 *   java -cp OUT eu.atarihelp.emu10.TestB312Java vektory.txt
 */
public class TestB312Java {
    static int kontrol = 0, chyb = 0;

    static void over(boolean ok, String co) {
        kontrol++;
        if (!ok) chyb++;
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
    }

    static String hex(byte[] b) {
        StringBuilder sb = new StringBuilder();
        for (byte x : b) sb.append(String.format("%02x", x & 255));
        return sb.toString();
    }

    public static void main(String[] a) throws Exception {
        // ---- 1) PBKDF2 proti Pythonu ----
        List<String> radky = Files.readAllLines(Paths.get(a[0]));
        int vektoru = 0, sedi = 0;
        String ukazkaKod = null;
        byte[] ukazkaSul = null, ukazkaOtisk = null;
        int ukazkaIt = 0;
        for (String r : radky) {
            String[] p = r.trim().split(" ");
            if (p.length == 5 && p[0].equals("UKAZKA")) {
                ukazkaKod = p[1];
                ukazkaSul = NapServis.hex(p[2]);
                ukazkaIt = Integer.parseInt(p[3]);
                ukazkaOtisk = NapServis.hex(p[4]);
                continue;
            }
            if (p.length != 5) continue;
            vektoru++;
            byte[] o = NapServis.pbkdf2(NapServis.hex(p[0]), NapServis.hex(p[1]), Integer.parseInt(p[2]), Integer.parseInt(p[3]));
            if (hex(o).equals(p[4])) sedi++;
            else System.out.println("   nesedi: " + r + " -> " + hex(o));
        }
        over(vektoru >= 40 && sedi == vektoru, "1) PBKDF2-HMAC-SHA256 = Python hashlib: " + sedi + "/" + vektoru
                + " vektoru (i RFC: password/salt 1, 2, 4096 iteraci, 40 B = 2 bloky, nulove bajty)");

        // ---- 2) normalizace a kodSedi s ukazkovym kodem (NE skutecny kod) ----
        over("ABCDEFGHJK".equals(NapServis.normalizuj(" abcde-FGHJK ")) && "".equals(NapServis.normalizuj(null)),
                "2) normalizace: mala pismena, pomlcky a mezery nevadi");
        boolean s1 = NapServis.kodSedi(ukazkaKod, ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean s2 = NapServis.kodSedi("testk-od234", ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean s3 = NapServis.kodSedi("TESTK OD 234", ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean n1 = NapServis.kodSedi("TESTKOD235", ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean n2 = NapServis.kodSedi("", ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean n3 = NapServis.kodSedi(null, ukazkaSul, ukazkaOtisk, ukazkaIt);
        boolean n4 = NapServis.kodSedi("TESTKOD2345", ukazkaSul, ukazkaOtisk, ukazkaIt);
        over(s1 && s2 && s3 && !n1 && !n2 && !n3 && !n4,
                "2) ukazkovy kod sedi (i malymi pismeny, s pomlckou / mezerami), jiny / prazdny / delsi ne");

        // ---- 3) otisk zabudovany v appce: spatne kody ne ----
        boolean zadnySpatny = true;
        String[] spatne = {"", "A", "ABCDEFGHJK", "ODEMKNOUT", "RENE", "NAPVISION", "1234567890", "ZAMKNOUTVSE"};
        for (String s : spatne) if (NapServis.kodSedi(s)) zadnySpatny = false;
        over(zadnySpatny, "3) otisk v appce: " + spatne.length + " spatnych kodu neprojde");
        long t0 = System.nanoTime();
        NapServis.kodSedi("XXXXXXXXXX");
        long ms = (System.nanoTime() - t0) / 1000000L;
        System.out.println("   jedno overeni (" + NapServis.ITERACI + " opakovani) na PC: " + ms + " ms");

        // ---- 4) pokusy, pauza, platnost servisu ----
        String kod = System.getenv("NAP_SERVIS_KOD");
        long T = 1_000_000_000L;
        NapServis.vynulujProTest();
        String r1 = NapServis.zkus("SPATNY1234", T), r2 = NapServis.zkus("SPATNY1234", T + 1000), r3 = NapServis.zkus("SPATNY1234", T + 2000);
        String r4 = NapServis.zkus("SPATNY1234", T + 30000);
        over("SPATNE".equals(r1) && "SPATNE".equals(r2) && "POCKEJ:60".equals(r3) && "POCKEJ:32".equals(r4)
                && !NapServis.otevreno(T + 30000),
                "4) 3 spatne pokusy -> minuta pauza (" + r1 + ", " + r2 + ", " + r3 + "; za 28 s " + r4 + "), servis zavreny");
        if (kod != null && !kod.isEmpty()) {
            String r5 = NapServis.zkus(kod, T + 40000);                       // jeste v pauze
            String r6 = NapServis.zkus(kod, T + 62001);                       // po pauze
            boolean o1 = NapServis.otevreno(T + 62001 + 9 * 60000L), o2 = NapServis.otevreno(T + 62001 + 10 * 60000L + 1);
            long z = NapServis.zbyvaS(T + 62001);
            over(r5.startsWith("POCKEJ:") && "OK".equals(r6) && o1 && !o2 && z == 600,
                    "4) skutecny servisni kod (z NAP_SERVIS_KOD, nevypisuje se): v pauze " + r5 + ", pak OK; servis 10 min ("
                    + z + " s), po 10 min zavreny");
            boolean varianty = NapServis.kodSedi(kod.toLowerCase()) && NapServis.kodSedi(kod.substring(0, 5) + "-" + kod.substring(5));
            over(varianty, "4) skutecny kod i malymi pismeny a s pomlckou uprostred");
            NapServis.vynulujProTest();
            boolean jeden = true;
            for (int i = 0; i < kod.length(); i++) {          // kod s jednim zmenenym znakem neprojde
                char c = kod.charAt(i);
                String jiny = kod.substring(0, i) + (c == 'A' ? 'B' : 'A') + kod.substring(i + 1);
                if (NapServis.kodSedi(jiny)) jeden = false;
            }
            over(jeden, "4) skutecny kod s jednim jinym znakem (" + kod.length() + " variant) neprojde");
        } else {
            System.out.println("   (NAP_SERVIS_KOD neni nastaveny - skutecny kod se netestuje)");
        }
        NapServis.vynulujProTest();
        over(!NapServis.otevreno(T) && NapServis.zbyvaS(T) == 0, "4) po startu appky je servis zavreny");

        // ---- 5) NapPravidla: zamky ----
        String[][] stranky = {
            {"file:///android_asset/emu_sega/index.html", "SEGA"},
            {"file:///android_asset/emu_sega/genesis_runner.html", "SEGA"},
            {"file:///android_asset/emu_sega_eu/index.html", "SEGA"},
            {"file:///android_asset/emu_sega_usa_stable/index.html", "SEGA"},
            {"FILE:///android_asset/EMU_SEGA/index.html?x=1", "SEGA"},
            {"file:///android_asset/emu_ps1/index.html", "PS1"},
            {"file:///android_asset/emu_ps1/", "PS1"},
            {"file:///android_asset/index.html", null},
            {"file:///android_asset/emu_atari_cpp/index.html", null},
            {"file:///android_asset/player/index.html", null},
            {"https://atarihelp.eu/?page_id=1003", null},
            {"https://evil.example/file:///android_asset/emu_sega/", null},
            {null, null}};
        boolean vse = true;
        for (String[] s : stranky) {
            String k = NapPravidla.konzoleStranky(s[0]);
            if (k == null ? s[1] != null : !k.equals(s[1])) { vse = false; System.out.println("   spatne: " + s[0] + " -> " + k); }
        }
        over(vse, "5) stranky emulatoru: emu_sega (i vetve _eu / _usa_stable) = SEGA, emu_ps1 = PS1, ostatni nic ("
                + stranky.length + " adres)");
        over(NapPravidla.zamceno("SEGA", false, false) && !NapPravidla.zamceno("SEGA", true, false)
                && NapPravidla.zamceno("PS1", true, false) && !NapPravidla.zamceno("PS1", true, true)
                && !NapPravidla.zamceno(null, false, false),
                "5) zamceno: SEGA podle Segy, PS1 podle PS1, jina stranka nikdy");
        String d1 = NapPravidla.duvodZamku("SEGA", false, 45, 30), d2 = NapPravidla.duvodZamku("PS1", true, 0, 12),
               d3 = NapPravidla.duvodZamku("PS1", false, 50, 30), d4 = NapPravidla.duvodZamku("SEGA", false, 1, 30),
               d5 = NapPravidla.duvodZamku("SEGA", false, 3, 30);
        System.out.println("   " + d1.replace('\n', '|') + "\n   " + d2.replace('\n', '|') + "\n   " + d3.replace('\n', '|'));
        over(d1.startsWith("SEGA JE ZAMČENÁ\nNapiš v Atari ještě 45 řádků programu")
                && d2.equals("PS1 JE ZAMČENÁ\nHraj na Seze ještě 12 min.")
                && d3.equals("PS1 JE ZAMČENÁ\nNejdřív odemkni Segu, pak 30 minut hraní na Seze.")
                && d4.contains("ještě 1 řádek ") && d5.contains("ještě 3 řádky "),
                "5) duvod zamku = stejna slova jako zamcena karta v menu (i 1 radek / 3 radky / 45 radku)");

        // ---- 6) MP3 prehravac: hudba v seznamu, ZPET ----
        String[] ano = {"a.mp3", "B.WAV", "NP_VISION_mixtape_20261020_120000.m4a", "NP_VISION_mix_20261020_120000.webm",
                        "x.aac", "x.ogg", "x.oga", "x.opus", "x.flac"};
        String[] ne = {"a.txt", "a.mp4", "a.xex", "a.m4a.part", "", null, "mp3"};
        boolean au = true;
        for (String s : ano) if (!NapPravidla.jeAudioJmeno(s)) { au = false; System.out.println("   melo byt: " + s); }
        for (String s : ne) if (NapPravidla.jeAudioJmeno(s)) { au = false; System.out.println("   nemelo byt: " + s); }
        over(au, "6) seznam hudby: MP3 / WAV / M4A / AAC / OGG / OPUS / FLAC / WEBM ano, jine ne");
        over(NapPravidla.jeStrankaSeZpet("file:///android_asset/player/index.html")
                && NapPravidla.jeStrankaSeZpet("file:///android_asset/dj/index.html")
                && !NapPravidla.jeStrankaSeZpet("file:///android_asset/index.html")
                && !NapPravidla.jeStrankaSeZpet("file:///android_asset/emu_sega/index.html")
                && !NapPravidla.jeStrankaSeZpet(null),
                "6) ZPET si berou samy jen MP3 prehravac a DZUBOX");

        System.out.println("VYSLEDEK: " + kontrol + " kontrol, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
