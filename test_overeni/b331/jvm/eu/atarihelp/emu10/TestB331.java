// B331 v JVM na PC - skutecna trida z appky NapBluetoothTexty (cista logika Bluetooth ovladace, bez Androidu):
//  A) co je herni ovladac (trida Bluetooth + jmeno) - skutecne ovladace i bezne veci v okoli
//  B) jaka povoleni chce ktera verze Androidu, kdy musi byt zapnuta Poloha
//  C) texty pro stranku OPTIONS cesky / anglicky (Android 9 vs 12, parovani, chyby)
//  D) JSON pro stranku: poradi (ovladace nahoru), uvozovky a zvlastni znaky ve jmenech, bez jmena, zapamatovane
// Spusteni: viz ../JAK_SPUSTIT.txt
package eu.atarihelp.emu10;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TestB331 {
    static int ok = 0, chyb = 0;

    static void kontrola(String co, boolean v, String detail) {
        System.out.println((v ? "OK     " : "CHYBA  ") + co + (v || detail.isEmpty() ? "" : "  [" + detail + "]"));
        if (v) ok++; else chyb++;
    }

    static NapBluetoothTexty.Nalez n(String a, String j, int trida, int sila, boolean sp) {
        return new NapBluetoothTexty.Nalez(a, j, trida, sila, sp);
    }

    public static void main(String[] x) {
        System.out.println("B331 - Bluetooth ovladac primo v appce (skutecna trida NapBluetoothTexty z appky)");

        // ================= A) je to herni ovladac? =================
        // trida Bluetooth (Class of Device): hlavni 0x0500 = periferie, typ 0x08 gamepad, 0x04 joystick
        final Object[][] ovladace = {
                {"Pro Controller", 0x2508},                 // Reneho Battletron (napodobenina Switch)
                {"Xbox Wireless Controller", 0x0508},
                {"Wireless Controller", 0x2508},            // DualShock 4
                {"DualSense Wireless Controller", 0x2508},
                {"8BitDo SN30 Pro", 0x0508},
                {"Gamesir-G4s", 0x2504},                    // joystick
                {"Joy-Con (L)", 0x2508},
                {null, 0x2508},                             // ovladac bez jmena (jmeno prijde pozdeji)
                {"XYZ-123", 0x0508},                        // nezname jmeno, ale trida gamepad
                {"BATTLETRON pad", -1},                     // bez tridy, pozna se podle jmena
                {"Retroid Pocket Controller", -1},
        };
        final StringBuilder spatne = new StringBuilder();
        for (Object[] o : ovladace) if (!NapBluetoothTexty.jeOvladac((Integer) o[1], (String) o[0])) spatne.append(o[0]).append("; ");
        kontrola("A1 herni ovladace se poznaji (trida gamepad / joystick nebo jmeno): Pro Controller, Xbox, DualShock 4, DualSense, 8BitDo, GameSir, Joy-Con, bez jmena, Battletron ...",
                spatne.length() == 0, spatne.toString());
        final Object[][] jine = {
                {"Galaxy Buds2", 0x0404},                   // sluchatka (audio)
                {"JBL Flip 5", 0x0414},                     // reproduktor
                {"[TV] Samsung 7 Series", 0x043C},          // televize
                {"Logitech K380", 0x0540},                  // klavesnice (periferie, ale ne gamepad)
                {"MX Master 3", 0x0580},                    // mys
                {"iPad", 0x011C},                           // tablet (pozor: "pad" nesmi stacit)
                {"Jana - iPhone", 0x020C},
                {"Galaxy Watch4", 0x0704},
                {null, -1},
                {"", 0x1F00},
        };
        spatne.setLength(0);
        for (Object[] o : jine) if (NapBluetoothTexty.jeOvladac((Integer) o[1], (String) o[0])) spatne.append(o[0]).append("; ");
        kontrola("A2 bezne veci v okoli NEJSOU ovladac: sluchatka, reproduktor, televize, klavesnice, mys, iPad, telefon, hodinky, bez jmena",
                spatne.length() == 0, spatne.toString());

        // ================= B) povoleni =================
        kontrola("B1 Android 12 a novejsi: povoleni Zarizeni v okoli (BLUETOOTH_SCAN + BLUETOOTH_CONNECT), zadna poloha",
                Arrays.equals(NapBluetoothTexty.povoleni(31), new String[]{"android.permission.BLUETOOTH_SCAN", "android.permission.BLUETOOTH_CONNECT"})
                        && Arrays.equals(NapBluetoothTexty.povoleni(34), NapBluetoothTexty.povoleni(31)), "");
        kontrola("B2 Android 6-11 (Reneho S8 = 9): poloha (kvuli hledani Bluetooth), Android 5 nic",
                Arrays.asList(NapBluetoothTexty.povoleni(28)).contains("android.permission.ACCESS_FINE_LOCATION")
                        && Arrays.asList(NapBluetoothTexty.povoleni(30)).contains("android.permission.ACCESS_FINE_LOCATION")
                        && NapBluetoothTexty.povoleni(22).length == 0, "");
        kontrola("B3 zapnuta Poloha je potreba jen na Androidu 10 a 11",
                !NapBluetoothTexty.potrebujePolohuZapnutou(28) && NapBluetoothTexty.potrebujePolohuZapnutou(29)
                        && NapBluetoothTexty.potrebujePolohuZapnutou(30) && !NapBluetoothTexty.potrebujePolohuZapnutou(31), "");

        // ================= C) texty =================
        final String p9 = NapBluetoothTexty.text(false, NapBluetoothTexty.F_POVOLENI, 28, "", 0, 0, 0, "");
        final String p12 = NapBluetoothTexty.text(false, NapBluetoothTexty.F_POVOLENI, 31, "", 0, 0, 0, "");
        kontrola("C1 povoleni: S8 (Android 9) vysvetli polohu a ze ji appka nezjistuje; Android 12 Zarizeni v okoli",
                p9.contains("povolení k poloze") && p9.contains("Appka polohu nezjišťuje") && p12.contains("„Zařízení v okolí“") && !p12.contains("poloze"), p9 + " | " + p12);
        final String h = NapBluetoothTexty.text(false, NapBluetoothTexty.F_HLEDAM, 28, "", 3, 0, 9, "");
        final String h1 = NapBluetoothTexty.text(false, NapBluetoothTexty.F_HLEDAM, 28, "", 3, 1, 5, "");
        kontrola("C2 hledani: odpocet, jak dat ovladac do parovani (HOME / SHARE), po nalezeni vyzva klepnout",
                h.startsWith("Hledám ovladače v okolí… (9 s)") && h.contains("podržet HOME") && !h.contains("klepni na ovladač")
                        && h1.contains("(5 s)") && h1.contains("Nalezeno – klepni na ovladač v seznamu."), h + " | " + h1);
        kontrola("C3 konec hledani bez ovladace: kolik jinych zarizeni a co dal",
                NapBluetoothTexty.text(false, NapBluetoothTexty.F_HOTOVO, 28, "", 7, 0, 0, "").equals(
                        "Žádný ovladač jsem nenašel (jiných zařízení 7). Je v párování? Klepni znovu na PŘIPOJIT OVLADAČ."), "");
        kontrola("C4 parovani -> sparovano -> pripojeno se jmenem ovladace",
                NapBluetoothTexty.text(false, NapBluetoothTexty.F_PARUJI, 28, "Pro Controller", 0, 1, 0, "").equals("Páruji s „Pro Controller“… Když se telefon zeptá, klepni na SPÁROVAT.")
                        && NapBluetoothTexty.text(false, NapBluetoothTexty.F_SPAROVANO, 28, "Pro Controller", 0, 1, 0, "").contains("Připojí se sám – kdyby ne, zmáčkni na něm HOME.")
                        && NapBluetoothTexty.text(false, NapBluetoothTexty.F_PRIPOJENO, 28, "Pro Controller", 0, 1, 0, "").equals("„Pro Controller“ je připojený ✔ Můžeš hrát."), "");
        kontrola("C5 anglicky: Nearby devices, location, pairing, connected",
                NapBluetoothTexty.text(true, NapBluetoothTexty.F_POVOLENI, 31, "", 0, 0, 0, "").contains("“Nearby devices”")
                        && NapBluetoothTexty.text(true, NapBluetoothTexty.F_POVOLENI, 28, "", 0, 0, 0, "").contains("does not track your location")
                        && NapBluetoothTexty.text(true, NapBluetoothTexty.F_PARUJI, 28, "Pro Controller", 0, 1, 0, "").equals("Pairing with “Pro Controller”… If the phone asks, tap PAIR.")
                        && NapBluetoothTexty.text(true, NapBluetoothTexty.F_PRIPOJENO, 28, "X", 0, 1, 0, "").equals("“X” is connected ✔ You can play."), "");
        boolean vse = true;
        for (String f : new String[]{NapBluetoothTexty.F_NENI_BT, NapBluetoothTexty.F_POVOLENI, NapBluetoothTexty.F_ODMITNUTO, NapBluetoothTexty.F_ZAPINANI,
                NapBluetoothTexty.F_BT_VYPNUTE, NapBluetoothTexty.F_POLOHA_VYPNUTA, NapBluetoothTexty.F_HLEDAM, NapBluetoothTexty.F_HOTOVO,
                NapBluetoothTexty.F_PARUJI, NapBluetoothTexty.F_SPAROVANO, NapBluetoothTexty.F_PRIPOJENO, NapBluetoothTexty.F_NEPOVEDLO, NapBluetoothTexty.F_CHYBA})
            for (boolean en : new boolean[]{false, true}) vse &= !NapBluetoothTexty.text(en, f, 28, "", 0, 0, 0, "").isEmpty();
        kontrola("C6 kazda faze ma text cesky i anglicky (NIC = prazdny)", vse && NapBluetoothTexty.text(false, NapBluetoothTexty.F_NIC, 28, "", 0, 0, 0, "").isEmpty(), "");
        kontrola("C7 chyba hledani a odmitnute povoleni odkazi na NASTAVENI BLUETOOTH",
                NapBluetoothTexty.text(false, NapBluetoothTexty.F_CHYBA, 28, "", 0, 0, 0, "startDiscovery").equals("Hledání nejde spustit (startDiscovery). Zkus NASTAVENÍ BLUETOOTH.")
                        && NapBluetoothTexty.text(false, NapBluetoothTexty.F_ODMITNUTO, 28, "", 0, 0, 0, "").contains("NASTAVENÍ BLUETOOTH"), "");

        // ================= D) JSON pro stranku =================
        final List<NapBluetoothTexty.Nalez> z = new ArrayList<>();
        z.add(n("11:22:33:44:55:01", "Jana \"A\" \\ iPhone\n", 0x020C, -40, false));
        z.add(n("11:22:33:44:55:02", "Pro Controller", 0x2508, -70, false));
        z.add(n("11:22:33:44:55:03", null, 0x0508, -60, false));
        z.add(n("11:22:33:44:55:04", "JBL Flip 5", 0x0414, -30, true));
        final List<NapBluetoothTexty.Nalez> m = new ArrayList<>();
        final NapBluetoothTexty.Nalez pz = n("AA:BB:CC:DD:EE:FF", "Pro Controller", 0x2508, Integer.MIN_VALUE, true);
        pz.pripojeny = true;
        m.add(pz);
        final String j = NapBluetoothTexty.json(false, NapBluetoothTexty.F_HOTOVO, "Text „s“ uvozovkami \"x\"", z, m);
        final String cekano = "{\"f\":\"HOTOVO\",\"t\":\"Text „s“ uvozovkami \\\"x\\\"\",\"z\":["
                + "{\"a\":\"11:22:33:44:55:03\",\"j\":\"Bez jména (..55:03)\",\"o\":1,\"s\":0,\"p\":0},"
                + "{\"a\":\"11:22:33:44:55:02\",\"j\":\"Pro Controller\",\"o\":1,\"s\":0,\"p\":0},"
                + "{\"a\":\"11:22:33:44:55:04\",\"j\":\"JBL Flip 5\",\"o\":0,\"s\":1,\"p\":0},"
                + "{\"a\":\"11:22:33:44:55:01\",\"j\":\"Jana \\\"A\\\" \\\\ iPhone\",\"o\":0,\"s\":0,\"p\":0}"
                + "],\"m\":[{\"j\":\"Pro Controller\",\"p\":1}]}";
        kontrola("D1 JSON: ovladace nahore (pak silnejsi signal), bez jmena -> \"Bez jména (..55:03)\", uvozovky a lomitka escapovane, zapamatovany pripojeny",
                j.equals(cekano), j);
        kontrola("D2 ridici znaky ve jmene (\\u0001, \\u2028) se escapuji, jinak JSON v JS spadne",
                NapBluetoothTexty.js("a\u0001b c").equals("\"a\\u0001b\\u2028c\"") && NapBluetoothTexty.js(null).equals("\"\""), "");
        kontrola("D3 do logu jen konec adresy (jmena a adresy cizich zarizeni v okoli se nezapisuji cele)",
                NapBluetoothTexty.kratkaAdresa("AA:BB:CC:DD:EE:FF").equals("..EE:FF") && NapBluetoothTexty.kratkaAdresa(null).equals("?")
                        && NapBluetoothTexty.jmenoDoSeznamu(true, " ", "AA:BB:CC:DD:EE:FF").equals("No name (..EE:FF)"), "");

        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
