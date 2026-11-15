// B329 v JVM na PC - skutecne tridy z appky (javac proti android.jar, ciste casti bez Androidu):
//  A) NapOvladac.klavesaSwitch: "Pro Controller" se starou tabulkou Androidu 9 (Generic.kl) -> kody podle
//     mista jako tabulka Androidu 10+ (Vendor_057e_Product_2009.kl); Reneho hledani START z logu B328
//  B) NapOvladac.switchStaraTabulka: kdy se preklada (jen 057e:2009 s tlacitkem C) a kdy ne
//  C) NapOvladac.jmenoKlavesy (log)
//  D) NapAktualizaceTexty.oznameni + texty okna aktualizace (co delat po instalaci)
// Spusteni: viz ../JAK_SPUSTIT.txt
package eu.atarihelp.emu10;

public class TestB329 {
    static int ok = 0, chyb = 0;

    static void kontrola(String co, boolean v, String detail) {
        System.out.println((v ? "OK     " : "CHYBA  ") + co + (v || detail.isEmpty() ? "" : "  [" + detail + "]"));
        if (v) ok++; else chyb++;
    }

    // KeyEvent.KEYCODE_* (pevna cisla API Androidu)
    static final int A = 96, B = 97, C = 98, X = 99, Y = 100, Z = 101, L1 = 102, R1 = 103, L2 = 104, R2 = 105,
            L3 = 106, R3 = 107, START = 108, SELECT = 109, MODE = 110, DPAD_UP = 19, DPAD_CENTER = 23, BUTTON_1 = 188, BUTTON_3 = 190;

    public static void main(String[] a) {
        System.out.println("B329 - Pro Controller podle mista, kazde tlacitko do logu, oznameni nove verze (skutecne tridy z appky)");

        // ================= A) preklad podle mista =================
        // Generic.kl (Android 9): 0x130 A, 0x131 B, 0x132 C, 0x133 X, 0x134 Y, 0x135 Z, 0x136 L1, 0x137 R1,
        // 0x138 L2, 0x139 R2, 0x13a SELECT, 0x13b START, 0x13c MODE, 0x13d THUMBL.
        // Vendor_057e_Product_2009.kl (Android 10+): 0x130 A, 0x131 B, 0x132 X, 0x133 Y, 0x134 L1, 0x135 R1,
        // 0x136 L2, 0x137 R2, 0x138 SELECT, 0x139 START, 0x13a THUMBL, 0x13b THUMBR, 0x13c HOME, 0x13d MODE.
        int[][] tab = {
                {A, A, 0x130}, {B, B, 0x131}, {C, X, 0x132}, {X, Y, 0x133}, {Y, L1, 0x134}, {Z, R1, 0x135},
                {L1, L2, 0x136}, {R1, R2, 0x137}, {L2, SELECT, 0x138}, {R2, START, 0x139}, {SELECT, L3, 0x13a},
                {START, R3, 0x13b}, {MODE, MODE, 0x13c}, {L3, MODE, 0x13d}};
        String[] mista = {"B dole", "A vpravo", "Y vlevo", "X nahore", "L", "R", "ZL", "ZR", "minus", "plus",
                "stisk leve packy", "stisk prave packy", "HOME", "snimek"};
        StringBuilder spatne = new StringBuilder();
        for (int i = 0; i < tab.length; i++) {
            final int ven = NapOvladac.klavesaSwitch(tab[i][0]);
            if (ven != tab[i][1]) spatne.append(String.format("%s (0x%x): %s -> %s, cekano %s; ", mista[i], tab[i][2],
                    NapOvladac.jmenoKlavesy(tab[i][0]), NapOvladac.jmenoKlavesy(ven), NapOvladac.jmenoKlavesy(tab[i][1])));
        }
        kontrola("A1 vsech 14 tlacitek Pro Controlleru: kod z tabulky Androidu 9 -> kod podle mista (jako tabulka Androidu 10+, HOME zustava MODE)",
                spatne.length() == 0, spatne.toString());
        kontrola("A2 plus = START (Rene: \"jedine, co nefunguje, je tlacitko START\" - chodilo jako R2), minus = SELECT",
                NapOvladac.klavesaSwitch(R2) == START && NapOvladac.klavesaSwitch(L2) == SELECT, "");
        kontrola("A3 ctverec (vlevo) uz neni KRIZEK (tlacitko C), trojuhelnik (nahore) uz neni CTVEREC, L/R uz nejsou trojuhelnik/kolecko",
                NapOvladac.klavesaSwitch(C) == X && NapOvladac.klavesaSwitch(X) == Y && NapOvladac.klavesaSwitch(Y) == L1
                        && NapOvladac.klavesaSwitch(Z) == R1, "");
        boolean beze = true;
        for (int k : new int[]{A, B, MODE, R3, DPAD_UP, 20, 21, 22, DPAD_CENTER, BUTTON_1, 4, 66})
            beze &= NapOvladac.klavesaSwitch(k) == k;
        kontrola("A4 krizek, A, B, HOME a ostatni klavesy beze zmeny", beze, "");
        // kazde standardni tlacitko po prekladu prave jednou (HOME i snimek = MODE)
        int[] pocty = new int[300];
        for (int[] r : tab) pocty[NapOvladac.klavesaSwitch(r[0])]++;
        boolean jednou = true;
        for (int k : new int[]{A, B, X, Y, L1, R1, L2, R2, SELECT, START, L3, R3}) jednou &= pocty[k] == 1;
        kontrola("A5 po prekladu ma kazde tlacitko PlayStation (krizek ... R3) prave jedno misto, nic neni dvakrat", jednou && pocty[MODE] == 2, "");
        // Reneho log B328 (PS1, hledani START): do hry slo SELECT, SELECT, R2, L3, L3, L3 = kody MODE/SELECT, R2, THUMBL
        kontrola("A6 Reneho hledani START z logu: R2 (plus) je ted START; HOME a snimek (drive SELECT a L3) jsou MODE",
                NapOvladac.klavesaSwitch(R2) == START && NapOvladac.klavesaSwitch(MODE) == MODE && NapOvladac.klavesaSwitch(L3) == MODE, "");

        // ================= B) kdy prekladat =================
        kontrola("B1 Pro Controller 057e:2009 s tlacitkem C (stara tabulka Androidu 9, Reneho Battletron) -> prekladat",
                NapOvladac.switchStaraTabulka(0x057e, 0x2009, true), "");
        kontrola("B2 Pro Controller bez tlacitka C (Android 10+ ma vlastni tabulku) -> NEprekladat (jinak by se tlacitka posunula podruhe)",
                !NapOvladac.switchStaraTabulka(0x057e, 0x2009, false), "");
        kontrola("B3 jine ovladace i s tlacitkem C (Joy-Con, DualShock 4, Xbox, 8BitDo, Retroid) -> NEprekladat",
                !NapOvladac.switchStaraTabulka(0x057e, 0x2006, true) && !NapOvladac.switchStaraTabulka(0x057e, 0x2007, true)
                        && !NapOvladac.switchStaraTabulka(0x054c, 0x09cc, true) && !NapOvladac.switchStaraTabulka(0x045e, 0x02e0, true)
                        && !NapOvladac.switchStaraTabulka(0x2dc8, 0x6001, true) && !NapOvladac.switchStaraTabulka(0, 0, true), "");

        // ================= C) jmena do logu =================
        kontrola("C1 jmena klaves do logu (R2, START, C, L3, BUTTON_3, kod 999)",
                "R2".equals(NapOvladac.jmenoKlavesy(R2)) && "START".equals(NapOvladac.jmenoKlavesy(START)) && "C".equals(NapOvladac.jmenoKlavesy(C))
                        && "L3".equals(NapOvladac.jmenoKlavesy(L3)) && "BUTTON_3".equals(NapOvladac.jmenoKlavesy(BUTTON_3))
                        && "kod 999".equals(NapOvladac.jmenoKlavesy(999)), "");

        // ================= D) oznameni nove verze =================
        final int NIC = NapAktualizaceTexty.OZN_NIC, UK = NapAktualizaceTexty.OZN_UKAZAT, SM = NapAktualizaceTexty.OZN_SMAZAT;
        kontrola("D1 nic neceka -> nic", NapAktualizaceTexty.oznameni(0, 372, 0) == NIC, "");
        kontrola("D2 ceka pro tuhle verzi -> ukazat (i po 4 ukazanich bez OK)",
                NapAktualizaceTexty.oznameni(372, 372, 0) == UK && NapAktualizaceTexty.oznameni(372, 372, 4) == UK, "");
        kontrola("D3 po 5 ukazanich bez OK, nebo pro jinou verzi -> smazat (zadne nekonecne okno)",
                NapAktualizaceTexty.oznameni(372, 372, 5) == SM && NapAktualizaceTexty.oznameni(371, 372, 0) == SM
                        && NapAktualizaceTexty.oznameni(373, 372, 0) == SM, "");
        String p9 = NapAktualizaceTexty.poPredani(false, 28, 29_465_190L), p12 = NapAktualizaceTexty.poPredani(false, 31, 29_465_190L);
        kontrola("D4 okno aktualizace, Android 9 (S8): nic nezavirat, appka se sama znovu otevre, pak OK v okne Aktualizace hotova",
                p9.contains("nic nezavírej") && p9.contains("sama se znovu otevře") && p9.contains("„Aktualizace hotová“") && p9.contains("klepni na OK"), p9);
        kontrola("D5 okno aktualizace, Android 12 (syn): nic nevypinat, otevrit ikonou, pak OK",
                p12.contains("nic nevypínej") && p12.contains("otevři ji znovu ikonou") && p12.contains("nová verze nainstalovaná")
                        && p12.contains("„Aktualizace hotová“") && p12.contains("klepni na OK") && !p12.contains("sama se znovu otevře"), p12);
        String e9 = NapAktualizaceTexty.poPredani(true, 28, 1048576L), e12 = NapAktualizaceTexty.poPredani(true, 31, 1048576L);
        kontrola("D6 anglicky: don't close anything / don't force-stop it, Update done, tap OK",
                e9.contains("don't close anything") && e9.contains("“Update done”") && e9.contains("tap OK")
                        && e12.contains("don't force-stop it") && e12.contains("open it again from its icon") && e12.contains("tap OK"), e9 + " | " + e12);
        String c9 = NapAktualizaceTexty.potvrd(false, 28), c12 = NapAktualizaceTexty.potvrd(false, 31);
        kontrola("D7 kdyz se Android pta: INSTALOVAT, nic nezavirat, pak OK v okne Aktualizace hotova (Android 10+ bez slibu, ze se otevre sama)",
                c9.startsWith("Telefon se ptá, jestli instalovat – klepni na INSTALOVAT.") && c9.contains("nic nezavírej")
                        && c9.contains("„Aktualizace hotová“ na OK") && c12.contains("nic nevypínej") && !c12.contains("sama"), c9 + " | " + c12);

        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
