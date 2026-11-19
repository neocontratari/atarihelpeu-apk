package eu.atarihelp.emu10;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

/**
 * B355: NapOvladacMapa (cista trida) - TLACITKA OVLADACE v appce. Katalogy bere ze SKUTECNEHO vystupu C++
 * (test_b355_mapa zapise katalog_<jadro>_<cs|en>.txt), aby parser Javy sedel na to, co posle knihovna Atari.
 * Preklad a spusteni: viz ../JAK_SPUSTIT.txt krok 5                       -> VYSTUP_TestB355Mapa.txt
 */
public class TestB355Mapa {
    static int ok = 0, chyb = 0;
    static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); }

    public static void main(String[] a) throws Exception {
        final String kat = a.length > 0 ? a[0] : ".";
        System.out.println("B355 - NapOvladacMapa: radky podle mista, mapa jako text, katalog z C++, MENU, zmena radku");
        // 1) klavesa -> radek (kod uz podle mista)
        final int[] kody = {96, 97, 99, 100, 102, 103, 104, 105, 106, 107, 108, 109, 110};
        boolean radky = true;
        for (int i = 0; i < kody.length; i++) radky &= NapOvladacMapa.radekKlavesy(kody[i]) == i;
        radky &= NapOvladacMapa.radekKlavesy(19) == -1 && NapOvladacMapa.radekKlavesy(23) == -1 && NapOvladacMapa.radekKlavesy(98) == -1
                && NapOvladacMapa.radekKlavesy(101) == -1;
        over(radky, "1) BUTTON_A B X Y L1 R1 L2 R2 L3 R3 START SELECT MODE = radky 0-12; krizek, OK, C, Z = zadny radek");
        // 2) katalogy ze skutecneho C++
        final String[] ocekVychozi = {
                "A=FIRE;B=FIRE;X=K33;Y=K12;L1=FIRE;R1=FIRE;L2=OPTION;R2=OPTION;L3=0;R3=0;START=START;SELECT=SELECT;MODE=MENU",
                "A=B;B=C;X=A;Y=Y;L1=A;R1=C;L2=X;R2=Z;L3=0;R3=0;START=START;SELECT=0;MODE=MENU",
                "A=KRIZEK;B=KOLECKO;X=CTVEREC;Y=TROJUHELNIK;L1=L1;R1=R1;L2=L2;R2=R2;L3=L3;R3=R3;START=START;SELECT=SELECT;MODE=MENU"};
        final int[] cilu = {65, 14, 18};
        final NapOvladacMapa.Katalog[] k = new NapOvladacMapa.Katalog[3];
        boolean katOk = true;
        final StringBuilder pop = new StringBuilder();
        for (int j = 0; j < 3; j++) {
            for (String jaz : new String[]{"cs", "en"}) {
                final String text = new String(Files.readAllBytes(Paths.get(kat, "katalog_" + j + "_" + jaz + ".txt")), StandardCharsets.UTF_8);
                final NapOvladacMapa.Katalog x = NapOvladacMapa.Katalog.z(text);
                final boolean v = x.ok() && x.vychozi.equals(ocekVychozi[j]) && x.cilToken.size() == cilu[j] && x.cilToken.contains("MENU")
                        && x.cilToken.contains("0") && !x.radekJmeno[0].equals("A") && x.radekJmeno[12].startsWith("HOME");
                katOk &= v;
                pop.append(' ').append(j).append(jaz).append('=').append(x.cilToken.size()).append(v ? "" : "!");
                if (jaz.equals("cs")) k[j] = x;
            }
        }
        System.out.println("katalogy (jadro jazyk = cilu):" + pop + " | Atari A: " + k[0].radekJmeno[0] + " | cil K33: " + k[0].jmenoCile("K33"));
        over(katOk && k[0].jmenoCile("K33").equals("klávesa MEZERA") && k[2].jmenoCile("KRIZEK").equals("✕ křížek"),
                "2) katalog z C++ (Atari 65 cilu, Sega 14, PS1 18; CZ i EN): 13 radku, vychozi mapa = C++, jmena (klávesa MEZERA, ✕ křížek)");
        // 3) mapa, MENU, zmena radku
        final NapOvladacMapa m = new NapOvladacMapa();
        boolean menu = true;
        for (int j = 0; j < 3; j++) menu &= NapOvladacMapa.MENU.equals(m.cil(j, NapOvladacMapa.R_MODE, k[j].vychozi)) && m.mapa(j).isEmpty();
        over(menu, "3a) nova appka: mapy prazdne (= vychozi C++), HOME = MENU ve vsech trech jadrech");
        final int v0 = m.verze(0);
        final boolean z1 = m.nastavRadek(0, 0, "K10", k[0].vychozi);
        final String po1 = m.mapa(0);
        final boolean z2 = m.nastavRadek(0, 0, "K10", k[0].vychozi);
        final int v1 = m.verze(0);
        final boolean z3 = m.nastavRadek(0, 0, "FIRE", k[0].vychozi);
        final String po3 = m.mapa(0);
        System.out.println("A -> K10: " + po1 + " | zpet FIRE: \"" + po3 + "\" | verze " + v0 + " -> " + v1 + " -> " + m.verze(0));
        over(z1 && po1.startsWith("A=K10;B=FIRE;X=K33;") && po1.endsWith("MODE=MENU") && !z2 && v1 == v0 + 1 && z3 && po3.isEmpty() && m.verze(0) == v0 + 2,
                "3b) zmena radku: cela mapa (13 radku), stejna volba nic nezmeni (verze stoji), navrat na vychozi = \"\" (jadro dostane znovu)");
        final boolean s1 = m.nastav(1, "Y=X;;Q=1;A=;B=MENU");
        final String[] r = NapOvladacMapa.rozloz(m.mapa(1), k[1].vychozi);
        over(s1 && r[3].equals("X") && r[0].equals("B") && r[1].equals("MENU") && r.length == 13
                        && NapOvladacMapa.MENU.equals(m.cil(1, 1, k[1].vychozi)) && m.cil(2, 99, k[2].vychozi).equals("0"),
                "3c) ulozena mapa s chybami (prazdne, neznamy radek Q, A bez cile): co nejde, zustane vychozi; B = MENU (otevre dialog)");
        // 4) jmena do logu
        final String l1 = NapOvladacMapa.doLogu("klávesa MEZERA"), l2 = NapOvladacMapa.doLogu("✕ křížek"),
                l3 = NapOvladacMapa.doLogu("MENU – tlačítka ovladače"), l4 = NapOvladacMapa.doLogu("SELECT (−, SHARE)");
        System.out.println("do logu: " + l1 + " | " + l2 + " | " + l3 + " | " + l4);
        over(l1.equals("klavesa MEZERA") && l2.equals("krizek") && l3.equals("MENU - tlacitka ovladace") && l4.equals("SELECT (-, SHARE)"),
                "4) jmena do logu bez diakritiky a symbolu (klavesa MEZERA, krizek, MENU - tlacitka ovladace)");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
