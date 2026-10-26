package eu.atarihelp.emu10;

// B314: JVM test - NapPravidla cesky / anglicky (bez Androidu):
//   - duvod zamku Segy / PS1 pod kartou v menu a hlaska u hry z webu, cesky beze zmeny, anglicky
//   - karta HELP: anglicky napoveda/en.html (i s #kapitolou), ostatni cile beze zmeny
//   - "texty" = vypis vet pro porovnani se stejnymi vetami v assets/index.html (kontrola_zamku_texty.py)
//   javac --release 17 -d OUT NapPravidla.java TestB314Java.java && java -cp OUT eu.atarihelp.emu10.TestB314Java [texty]
public class TestB314Java {
    static int ok = 0, chyb = 0;

    static void over(boolean b, String co) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + co);
    }

    public static void main(String[] a) {
        if (a.length > 0 && a[0].equals("texty")) {
            // radky "jazyk|konzole|segaOtevrena|radku|minut|text" pro porovnani s JavaScriptem menu
            for (String jaz : new String[] {"cs", "en"}) {
                boolean en = jaz.equals("en");
                for (int r : new int[] {0, 1, 2, 4, 5, 45})
                    System.out.println(jaz + "|SEGA|false|" + r + "|0|" + NapPravidla.duvodKarty(NapPravidla.SEGA, false, r, 0, en));
                for (int m : new int[] {1, 30})
                    System.out.println(jaz + "|PS1|true|0|" + m + "|" + NapPravidla.duvodKarty(NapPravidla.PS1, true, 0, m, en));
                System.out.println(jaz + "|PS1|false|0|30|" + NapPravidla.duvodKarty(NapPravidla.PS1, false, 0, 30, en));
            }
            return;
        }
        String s45 = NapPravidla.duvodKarty(NapPravidla.SEGA, false, 45, 0, false);
        over(s45.equals("Napiš v Atari ještě 45 řádků programu (řádky s číslem, počítají se při RUN nebo LIST)."), "cesky Sega 45: " + s45);
        String e45 = NapPravidla.duvodKarty(NapPravidla.SEGA, false, 45, 0, true);
        over(e45.equals("Type 45 more program lines on the Atari (numbered lines – they count on RUN or LIST)."), "anglicky Sega 45: " + e45);
        String e1 = NapPravidla.duvodKarty(NapPravidla.SEGA, false, 1, 0, true);
        over(e1.equals("Type 1 more program line on the Atari (numbered lines – they count on RUN or LIST)."), "anglicky Sega 1 (jednotne cislo): " + e1);
        over(NapPravidla.duvodKarty(NapPravidla.PS1, true, 0, 12, true).equals("Play the Sega for 12 more min.")
                && NapPravidla.duvodKarty(NapPravidla.PS1, true, 0, 12, false).equals("Hraj na Seze ještě 12 min."),
             "PS1 po odemceni Segy: Play the Sega for 12 more min. / Hraj na Seze ještě 12 min.");
        over(NapPravidla.duvodKarty(NapPravidla.PS1, false, 0, 30, true).equals("Unlock the Sega first, then play it for 30 minutes."),
             "PS1 bez Segy anglicky");
        String z4 = NapPravidla.duvodZamku(NapPravidla.SEGA, false, 37, 0);
        String z5 = NapPravidla.duvodZamku(NapPravidla.SEGA, false, 37, 0, false);
        over(z4.equals(z5) && z4.startsWith("SEGA JE ZAMČENÁ\nNapiš v Atari ještě 37 řádků"), "hlaska u hry z webu cesky beze zmeny (stara metoda = cesky)");
        String ze = NapPravidla.duvodZamku(NapPravidla.SEGA, false, 37, 0, true);
        over(ze.equals("SEGA IS LOCKED\nType 37 more program lines on the Atari (numbered lines – they count on RUN or LIST)."),
             "hlaska u hry z webu anglicky: " + ze.replace("\n", " / "));
        over(NapPravidla.duvodZamku(NapPravidla.PS1, true, 0, 3, true).equals("PS1 IS LOCKED\nPlay the Sega for 3 more min.")
                && NapPravidla.duvodZamku("NIC", true, 0, 3, true).isEmpty(), "PS1 IS LOCKED, neznama konzole = prazdne");
        over("napoveda/en.html".equals(NapPravidla.cilPodleJazyka("napoveda/index.html", true))
                && "napoveda/en.html#tv".equals(NapPravidla.cilPodleJazyka("napoveda/index.html#tv", true))
                && "napoveda/index.html".equals(NapPravidla.cilPodleJazyka("napoveda/index.html", false)),
             "karta HELP anglicky -> napoveda/en.html (i #kapitola), cesky beze zmeny");
        over("emu_sega/index.html".equals(NapPravidla.cilPodleJazyka("emu_sega/index.html", true))
                && "index.html".equals(NapPravidla.cilPodleJazyka("index.html", true))
                && NapPravidla.cilPodleJazyka(null, true) == null
                && "https://atarihelp.eu/".equals(NapPravidla.cilPodleJazyka("https://atarihelp.eu/", true)),
             "ostatni cile beze zmeny (stranky se prekladaji samy), null nespadne");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
