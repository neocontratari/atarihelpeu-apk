// B332 v JVM na PC - skutecna trida z appky NapBluetoothTexty (cista logika, bez Androidu):
//  A) cekani po PRIPOJIT u zapamatovaneho ovladace: pripojeny / ozval se, ale nepripojil / neozval se,
//     odpocitavani, kdy smi appka pripojit sama (jen Android 7 a 8)
//  B) texty pro stranku OPTIONS cesky / anglicky a zprava hlidace "ozval se, ale nepripojil"
//  C) JSON zapamatovanych ovladacu: adresa (tlacitko PRIPOJIT) a stav v Androidu
// Spusteni: viz ../JAK_SPUSTIT.txt
package eu.atarihelp.emu10;

import java.util.ArrayList;
import java.util.List;

public class TestB332 {
    static int ok = 0, chyb = 0;

    static void kontrola(String co, boolean v, String detail) {
        System.out.println((v ? "OK     " : "CHYBA  ") + co + (v || detail.isEmpty() ? "" : "  [" + detail + "]"));
        if (v) ok++; else chyb++;
    }

    static final String P = NapBluetoothTexty.F_PRIPOJUJI, OK = NapBluetoothTexty.F_PRIPOJENO,
            OZ = NapBluetoothTexty.F_OZVAL_SE, NE = NapBluetoothTexty.F_NEOZVAL;
    static final int NEVI = NapBluetoothTexty.HID_NEVI, ODP = NapBluetoothTexty.HID_ODPOJEN,
            PRIPOJUJE = NapBluetoothTexty.HID_PRIPOJUJE, PRIPOJEN = NapBluetoothTexty.HID_PRIPOJEN;

    static String h(long ted, long aclOd, int hid, boolean vstup) {
        return NapBluetoothTexty.hlidani(ted, 1000L, aclOd, hid, vstup);   // klepnuto v case 1000 ms
    }

    public static void main(String[] x) {
        System.out.println("B332 - zapamatovany Bluetooth ovladac: PRIPOJIT, hlidac (skutecna trida NapBluetoothTexty z appky)");

        // ================= A) cekani po PRIPOJIT =================
        kontrola("A1 hned po klepnuti (nic se nedeje) -> cekam", h(1500, 0, ODP, false).equals(P), h(1500, 0, ODP, false));
        kontrola("A2 Android ho posila jako herni ovladac -> pripojeny (i kdyz HID profil nevi)", h(5000, 0, NEVI, true).equals(OK), "");
        kontrola("A3 HID profil: pripojen -> pripojeny (i bez vstupniho zarizeni)", h(5000, 3000, PRIPOJEN, false).equals(OK), "");
        kontrola("A4 ozval se pred 9,9 s, jako ovladac nic -> porad cekam", h(12900, 3000, ODP, false).equals(P), "");
        kontrola("A5 ozval se pred 10 s a jako ovladac se nepripojil -> \"ozval se, ale nepripojil\"", h(13000, 3000, ODP, false).equals(OZ), "");
        kontrola("A6 ozval se pred 10 s, ale Android ho prave pripojuje -> porad cekam (ne hned spatna zprava)",
                h(13000, 3000, PRIPOJUJE, false).equals(P), "");
        kontrola("A7 30 s nic -> \"neozval se\"; 29,9 s -> porad cekam", h(31000, 0, ODP, false).equals(NE) && h(30900, 0, ODP, false).equals(P), "");
        kontrola("A8 30 s a ozval se pred 2 s (jeste nepripojen) -> \"ozval se, ale nepripojil\"", h(31000, 29000, PRIPOJUJE, false).equals(OZ), "");
        kontrola("A9 i po \"neozval se\" ho Android pozdeji pripoji -> pripojeny", h(90000, 0, PRIPOJEN, false).equals(OK), "");
        kontrola("A10 odpocitavani: 30 s po klepnuti, 25 s po 5 s, nikdy pod nulu",
                NapBluetoothTexty.zbyva(1000, 1000) == 30 && NapBluetoothTexty.zbyva(6000, 1000) == 25 && NapBluetoothTexty.zbyva(99000, 1000) == 0,
                NapBluetoothTexty.zbyva(6000, 1000) + "");
        kontrola("A11 sama pripojit smi appka jen na Androidu 7 a 8 (API 24-27), od Androidu 9 (S8) a 12 (A12) ne",
                NapBluetoothTexty.appkaSmiPripojit(24) && NapBluetoothTexty.appkaSmiPripojit(27) && !NapBluetoothTexty.appkaSmiPripojit(28)
                        && !NapBluetoothTexty.appkaSmiPripojit(31) && !NapBluetoothTexty.appkaSmiPripojit(34), "");
        kontrola("A12 stav v Androidu do logu", NapBluetoothTexty.hidLog(PRIPOJEN).equals("PRIPOJEN") && NapBluetoothTexty.hidLog(PRIPOJUJE).equals("pripojuje se")
                && NapBluetoothTexty.hidLog(ODP).equals("odpojen") && NapBluetoothTexty.hidLog(NEVI).equals("?"), "");

        // ================= B) texty =================
        final String t9 = NapBluetoothTexty.text(false, P, 28, "Pro Controller", 0, 0, 25, "");
        kontrola("B1 cekam (Android 9): " + t9, t9.equals("Zapni ovladač „Pro Controller“ (tlačítko HOME / domeček) – Android ho připojí sám. Čekám… (25 s)"), t9);
        final String t7 = NapBluetoothTexty.text(false, P, 26, "Pro Controller", 0, 0, 30, "");
        kontrola("B2 cekam (Android 8 - appka pripojuje): " + t7, t7.equals("Připojuji „Pro Controller“… Když je vypnutý, zapni ho (HOME). (30 s)"), t7);
        final String toz = NapBluetoothTexty.text(false, OZ, 31, "Pro Controller", 0, 0, 0, "");
        kontrola("B3 ozval se, ale nepripojil: vypnout a zapnout, jinak NASTAVENI BLUETOOTH -> zrusit parovani -> sparovat znovu",
                toz.startsWith("„Pro Controller“ se ozval, ale jako ovladač se nepřipojil. Vypni ho a zapni znovu.")
                        && toz.contains("NASTAVENÍ BLUETOOTH → „Pro Controller“ → zrušit párování") && toz.contains("PŘIPOJIT OVLADAČ (BLUETOOTH)"), toz);
        final String tne = NapBluetoothTexty.text(false, NE, 28, "Pro Controller", 0, 0, 0, "");
        kontrola("B4 neozval se: zapnuty a nabity? stejny rezim (Switch / Android / PC) jako pri parovani",
                tne.startsWith("„Pro Controller“ se neozval. Je zapnutý a nabitý?") && tne.contains("Switch / Android / PC"), tne);
        final String e9 = NapBluetoothTexty.text(true, P, 28, "Pro Controller", 0, 0, 12, "");
        final String eoz = NapBluetoothTexty.text(true, OZ, 28, "Pro Controller", 0, 0, 0, "");
        final String ene = NapBluetoothTexty.text(true, NE, 28, "Pro Controller", 0, 0, 0, "");
        kontrola("B5 anglicky: cekam / ozval se / neozval se", e9.equals("Turn “Pro Controller” on (HOME button) – Android connects it by itself. Waiting… (12 s)")
                && eoz.startsWith("“Pro Controller” responded but didn't connect as a controller.") && eoz.contains("BLUETOOTH SETTINGS")
                && ene.startsWith("“Pro Controller” didn't respond. Is it on and charged?"), e9 + " | " + eoz + " | " + ene);
        boolean bezCestiny = true;
        for (String s : new String[]{e9, eoz, ene, NapBluetoothTexty.text(true, P, 26, "X", 0, 0, 5, ""), NapBluetoothTexty.zpravaOzvalSe(true, "X")})
            for (char c : "ěščřžýáíéůúťďňĚŠČŘŽÝÁÍÉŮÚ".toCharArray()) if (s.indexOf(c) >= 0) bezCestiny = false;
        kontrola("B6 anglicke texty bez ceskych znaku", bezCestiny, "");
        final String zp = NapBluetoothTexty.zpravaOzvalSe(false, "Pro Controller"), zb = NapBluetoothTexty.zpravaOzvalSe(false, "");
        kontrola("B7 zprava hlidace: " + zp, zp.equals("🎮 „Pro Controller“ se ozval, ale nepřipojil se – vypni ho a zapni znovu (víc: OPTIONS → HERNÍ OVLADAČ)")
                && zb.startsWith("🎮 „Ovladač“"), zp);
        final String tb = NapBluetoothTexty.text(false, P, 28, "", 0, 0, 3, "");
        final String tbe = NapBluetoothTexty.text(true, P, 28, null, 0, 0, 3, "");
        kontrola("B8 ovladac bez jmena: \"" + tb + "\" / \"" + tbe + "\"",
                tb.equals("Zapni ovladač (tlačítko HOME / domeček) – Android ho připojí sám. Čekám… (3 s)")
                        && tbe.equals("Turn the controller on (HOME button) – Android connects it by itself. Waiting… (3 s)"), tb + " | " + tbe);

        // ================= C) JSON zapamatovanych =================
        final List<NapBluetoothTexty.Nalez> m = new ArrayList<>();
        final NapBluetoothTexty.Nalez a = new NapBluetoothTexty.Nalez("AA:BB:CC:DD:EE:FF", "Pro Controller", 0x2508, Integer.MIN_VALUE, true);
        a.hid = PRIPOJUJE;
        final NapBluetoothTexty.Nalez b = new NapBluetoothTexty.Nalez("11:22:33:44:55:66", "Xbox \"Elite\"", 0x0508, Integer.MIN_VALUE, true);
        b.pripojeny = true; b.hid = PRIPOJEN;
        final NapBluetoothTexty.Nalez c = new NapBluetoothTexty.Nalez("11:22:33:44:55:77", null, 0x0508, Integer.MIN_VALUE, true);
        m.add(a); m.add(b); m.add(c);
        final String j = NapBluetoothTexty.json(false, P, "t", new ArrayList<>(), m);
        final String cekano = "{\"f\":\"PRIPOJUJI\",\"t\":\"t\",\"z\":[],\"m\":["
                + "{\"j\":\"Pro Controller\",\"p\":0,\"a\":\"AA:BB:CC:DD:EE:FF\",\"h\":1},"
                + "{\"j\":\"Xbox \\\"Elite\\\"\",\"p\":1,\"a\":\"11:22:33:44:55:66\",\"h\":2},"
                + "{\"j\":\"Bez jména (..55:77)\",\"p\":0,\"a\":\"11:22:33:44:55:77\",\"h\":-1}]}";
        kontrola("C1 JSON zapamatovanych: jmeno, pripojeny, adresa (PRIPOJIT), stav v Androidu (1 = pripojuje se, -1 = nevi)", j.equals(cekano), j);

        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
