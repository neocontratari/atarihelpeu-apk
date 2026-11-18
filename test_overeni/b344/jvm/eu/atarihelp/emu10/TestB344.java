// B344 v JVM na PC - skutecna trida z appky NapBluetoothTexty (cista logika, bez Androidu):
//  A) klepnuti na zarizeni v seznamu hledani: parovat / uz pripojeny / sparovany ovladac SPAROVAT ZNOVU /
//     sparovane jine zarizeni (reproduktor) appka nerusi
//  B) po zruseni stareho sparovani (BOND_NONE) se paruje znovu jen ten ovladac, kteremu appka sparovani rusila
//  C) texty: "neozval se" / "ozval se, ale nepripojil" uz neposilaji do slepe ulicky (drive "spáruj znovu
//     (PŘIPOJIT OVLADAČ)" -> klepnuti -> "už je spárovaný – stačí ho zapnout"), nove faze ZNOVU a ZNOVU_RUCNE
// Spusteni: viz ../JAK_SPUSTIT.txt
package eu.atarihelp.emu10;

public class TestB344 {
    static int ok = 0, chyb = 0;

    static void kontrola(String co, boolean v, String detail) {
        System.out.println((v ? "OK     " : "CHYBA  ") + co + (v || detail.isEmpty() ? "" : "  [" + detail + "]"));
        if (v) ok++; else chyb++;
    }

    static boolean bezCestiny(String s) {
        // „ = cesky zacatek uvozovek („“); “ ” jsou i anglicke, ty nevadi
        for (char c : "ěščřžýáíéůúťďňĚŠČŘŽÝÁÍÉŮÚ„".toCharArray()) if (s.indexOf(c) >= 0) return false;
        return true;
    }

    public static void main(String[] x) {
        System.out.println("B344 - BT ovladac: SPAROVAT ZNOVU primo v appce (skutecna trida NapBluetoothTexty z appky)");

        // ================= A) klepnuti na zarizeni v seznamu =================
        kontrola("A1 nesparovany (ovladac i cokoli jineho) -> parovat (createBond)",
                NapBluetoothTexty.poKlepnuti(false, true, false).equals(NapBluetoothTexty.K_PARUJ)
                        && NapBluetoothTexty.poKlepnuti(false, false, false).equals(NapBluetoothTexty.K_PARUJ), "");
        kontrola("A2 sparovany a pripojeny jako ovladac -> nic nerusit (uz hraje)",
                NapBluetoothTexty.poKlepnuti(true, true, true).equals(NapBluetoothTexty.K_PRIPOJENY), "");
        kontrola("A3 log TV B343: \"Pro Controller\" sparovany, nepripojeny, nasel se v hledani -> SPAROVAT ZNOVU",
                NapBluetoothTexty.poKlepnuti(true, NapBluetoothTexty.jeOvladac(0x508, "Pro Controller"), false).equals(NapBluetoothTexty.K_ZNOVU), "");
        kontrola("A4 sparovany reproduktor (JBL Flip 5, trida 0x414) -> sparovani appka NERUSI",
                NapBluetoothTexty.poKlepnuti(true, NapBluetoothTexty.jeOvladac(0x414, "JBL Flip 5"), false).equals(NapBluetoothTexty.K_SPAROVANY), "");
        kontrola("A5 ovladac poznany jen podle jmena (trida neznama) -> taky znovu",
                NapBluetoothTexty.poKlepnuti(true, NapBluetoothTexty.jeOvladac(-1, "8BitDo Pro 2"), false).equals(NapBluetoothTexty.K_ZNOVU), "");

        // ================= B) po zruseni stareho sparovani =================
        final String adr = "AA:BB:CC:DD:EE:01";
        kontrola("B1 BOND_NONE (10) u ovladace, kteremu appka sparovani rusila -> parovat znovu",
                NapBluetoothTexty.znovuPoZruseni(adr, adr, NapBluetoothTexty.BOND_NONE), "");
        kontrola("B2 jine zarizeni / zadne ruseni / jiny stav (11 BONDING, 12 BONDED) -> nic",
                !NapBluetoothTexty.znovuPoZruseni(adr, "11:22:33:44:55:66", 10) && !NapBluetoothTexty.znovuPoZruseni(null, adr, 10)
                        && !NapBluetoothTexty.znovuPoZruseni(adr, adr, 11) && !NapBluetoothTexty.znovuPoZruseni(adr, adr, 12), "");
        kontrola("B3 limit na zruseni 8 s (pak rada NASTAVENI BLUETOOTH)", NapBluetoothTexty.LIMIT_ZRUSENI_MS == 8000L, "");

        // ================= C) texty =================
        final String ne = NapBluetoothTexty.text(false, NapBluetoothTexty.F_NEOZVAL, 28, "Pro Controller", 0, 0, 0, "");
        kontrola("C1 neozval se: proc (vraci se k poslednimu zarizeni, treba telefonu) a jak (parovani -> PŘIPOJIT OVLADAČ -> na nej)",
                ne.startsWith("„Pro Controller“ se neozval. Je zapnutý a nabitý? Ovladač se po zapnutí vrací jen k poslednímu zařízení")
                        && ne.contains("Dej ho do párování (podrž HOME nebo tlačítko párování / SYNC), klepni na PŘIPOJIT OVLADAČ (BLUETOOTH) a pak na něj v seznamu – appka ho spáruje znovu.")
                        && ne.contains("Switch / Android / PC") && !ne.contains("Jinak ho spáruj znovu"), ne);
        final String oz = NapBluetoothTexty.text(false, NapBluetoothTexty.F_OZVAL_SE, 28, "Pro Controller", 0, 0, 0, "");
        kontrola("C2 ozval se, ale nepripojil: vypnout a zapnout, jinak znovu sparovat v appce (ne Nastaveni Androidu)",
                oz.startsWith("„Pro Controller“ se ozval, ale jako ovladač se nepřipojil. Vypni ho a zapni znovu. Když to nepomůže: dej ho do párování")
                        && !oz.contains("NASTAVENÍ BLUETOOTH"), oz);
        final String zn = NapBluetoothTexty.text(false, NapBluetoothTexty.F_ZNOVU, 28, "Pro Controller", 0, 0, 0, "");
        final String ru = NapBluetoothTexty.text(false, NapBluetoothTexty.F_ZNOVU_RUCNE, 28, "Pro Controller", 0, 0, 0, "");
        kontrola("C3 faze ZNOVU: \"" + zn + "\"", zn.equals("„Pro Controller“ je spárovaný, ale nepřipojený – páruji ho znovu (staré spárování ruším)… Nech ho v párování."), zn);
        kontrola("C4 faze ZNOVU_RUCNE: rada NASTAVENI BLUETOOTH -> odebrat -> PŘIPOJIT OVLADAČ",
                ru.startsWith("Android nedovolil appce zrušit staré spárování „Pro Controller“. V NASTAVENÍ BLUETOOTH ho odeber (zapomenout)")
                        && ru.contains("PŘIPOJIT OVLADAČ (BLUETOOTH)"), ru);
        final String[] en = {
                NapBluetoothTexty.text(true, NapBluetoothTexty.F_NEOZVAL, 28, "Pro Controller", 0, 0, 0, ""),
                NapBluetoothTexty.text(true, NapBluetoothTexty.F_OZVAL_SE, 28, "Pro Controller", 0, 0, 0, ""),
                NapBluetoothTexty.text(true, NapBluetoothTexty.F_ZNOVU, 28, "Pro Controller", 0, 0, 0, ""),
                NapBluetoothTexty.text(true, NapBluetoothTexty.F_ZNOVU_RUCNE, 28, "Pro Controller", 0, 0, 0, "")};
        boolean bez = true;
        for (String s : en) bez &= bezCestiny(s);
        kontrola("C5 anglicky: neozval se (Put it into pairing mode ...), ozval se, ZNOVU, ZNOVU_RUCNE - bez ceskych znaku",
                bez && en[0].contains("returns only to the last device it played with (e.g. the phone). Put it into pairing mode")
                        && en[1].contains("If that doesn't help: put it into pairing mode") && en[2].startsWith("“Pro Controller” is paired but not connected")
                        && en[3].contains("BLUETOOTH SETTINGS"), String.join(" | ", en));
        final String bj = NapBluetoothTexty.text(false, NapBluetoothTexty.F_ZNOVU, 28, "", 0, 0, 0, "");
        kontrola("C6 bez jmena: \"" + bj + "\"", bj.startsWith("„ovladač“ je spárovaný"), bj);

        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
