package eu.atarihelp.emu10;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * B331: BLUETOOTH OVLADAC PRIMO V APPCE - cista logika a texty (bez Androidu, testuje se v JVM).
 *
 * Rene: "jestli pujde, aby si appka sama nasla ovladac BT, abych nemusel otravne chodit do menu
 * Androidu a nastavovat" + "aby si appka zapamatovala sparovany ovladac - ale osetrit to tak, aby kdyz
 * sedim ve vlaku, byla nadale moznost vyberu". OPTIONS -> HERNI OVLADAC -> PRIPOJIT OVLADAC (BLUETOOTH):
 * hledani, seznam, klepnutim sparovat, seznam zapamatovanych (pripojen / vypnuty). Parovani si pamatuje
 * Android - zapnuty ovladac se pripoji sam; bez nej (vlak) jsou dotykova tlacitka (rezim AUTOMATICKY).
 */
final class NapBluetoothTexty {

    private NapBluetoothTexty() {}

    // faze (stav) hledani / parovani - posila se strance OPTIONS v JSON
    static final String F_NIC = "NIC", F_NENI_BT = "NENI_BT", F_POVOLENI = "POVOLENI", F_ODMITNUTO = "ODMITNUTO",
            F_ZAPINANI = "ZAPINANI", F_BT_VYPNUTE = "BT_VYPNUTE", F_POLOHA_VYPNUTA = "POLOHA_VYPNUTA",
            F_HLEDAM = "HLEDAM", F_HOTOVO = "HOTOVO", F_PARUJI = "PARUJI", F_SPAROVANO = "SPAROVANO",
            F_PRIPOJENO = "PRIPOJENO", F_NEPOVEDLO = "NEPOVEDLO", F_CHYBA = "CHYBA";

    // ===== B332: ZAPAMATOVANY OVLADAC SE PRIPOJI SAM (Rene: "aby ovladac, ktery uz byl jednou sparovany
    // z appky, fungoval automaticky"; noname ovladac se "nekdy ani pres cisty Android nechce pripojit") =====
    // Spojeni zacina OVLADAC (po zapnuti / HOME vola telefon, se kterym je sparovany) a Android ho prijme sam.
    // Pripojit ovladac ze strany telefonu umi od Androidu 9 jen systemove Nastaveni (BluetoothHidHost.connect
    // je skryte API, od Androidu 12 navic systemove opravneni) - appka na Androidu 7 a 8 to zkusi, jinak
    // hlida a radi: PRIPOJIT u zapamatovaneho ovladace = 30 s cekani s odpocitavanim, co se deje
    // (ozval se, ale nepripojil / neozval se) a co s tim. Hlidac v appce pozna "ozval se, ale za 10 s se
    // nepripojil" i bez klepnuti (zprava + log). Do logu jen zapamatovane ovladace a konec adresy.
    static final String F_PRIPOJUJI = "PRIPOJUJI", F_OZVAL_SE = "OZVAL_SE", F_NEOZVAL = "NEOZVAL";
    static final long LIMIT_PRIPOJENI_MS = 30000L, LIMIT_OZVAL_MS = 10000L;
    /** Stav HID (BluetoothProfile.STATE_*): 0 odpojen, 1 pripojuje se, 2 pripojen, 3 odpojuje se; -1 = nevi. */
    static final int HID_NEVI = -1, HID_ODPOJEN = 0, HID_PRIPOJUJE = 1, HID_PRIPOJEN = 2, HID_ODPOJUJE = 3;

    /** Smi aplikace pripojit ovladac sama (skryte BluetoothHidHost.connect)? Jen Android 7 a 8 - od 9 ho Android aplikacim nepusti. */
    static boolean appkaSmiPripojit(int sdk) {
        return sdk < 28;
    }

    /**
     * Cekani po PRIPOJIT: od = kdy klepnuto, aclOd = kdy se ovladac ozval (spojeni Bluetooth, 0 = ne),
     * hid = stav ovladace v Androidu, vstup = Android ho uz posila jako herni ovladac.
     */
    static String hlidani(long ted, long od, long aclOd, int hid, boolean vstup) {
        if (vstup || hid == HID_PRIPOJEN) return F_PRIPOJENO;
        if (aclOd > 0 && ted - aclOd >= LIMIT_OZVAL_MS && hid != HID_PRIPOJUJE) return F_OZVAL_SE;
        if (ted - od >= LIMIT_PRIPOJENI_MS) return aclOd > 0 ? F_OZVAL_SE : F_NEOZVAL;
        return F_PRIPOJUJI;
    }

    /** Sekund do konce cekani po PRIPOJIT. */
    static int zbyva(long ted, long od) {
        return (int) Math.max(0L, (od + LIMIT_PRIPOJENI_MS - ted + 999L) / 1000L);
    }

    /** Stav ovladace v Androidu do logu. */
    static String hidLog(int hid) {
        switch (hid) {
            case HID_ODPOJEN: return "odpojen";
            case HID_PRIPOJUJE: return "pripojuje se";
            case HID_PRIPOJEN: return "PRIPOJEN";
            case HID_ODPOJUJE: return "odpojuje se";
            default: return "?";
        }
    }

    /** Zprava, kdyz se zapamatovany ovladac ozval, ale za 10 s se nepripojil (hlidac, bez klepnuti). */
    static String zpravaOzvalSe(boolean en, String jmeno) {
        final String jm = jmeno == null || jmeno.isEmpty() ? (en ? "The controller" : "Ovladač") : jmeno;
        return en ? "🎮 “" + jm + "” responded but didn't connect – turn it off and on again (details: OPTIONS → GAME CONTROLLER)"
                  : "🎮 „" + jm + "“ se ozval, ale nepřipojil se – vypni ho a zapni znovu (víc: OPTIONS → HERNÍ OVLADAČ)";
    }

    /** Jedno nalezene / zapamatovane zarizeni. trida = BluetoothClass.getDeviceClass() (-1 = neznama). */
    static final class Nalez {
        final String adresa;
        String jmeno;
        int trida, sila;            // sila = RSSI v dBm (Integer.MIN_VALUE = neznama)
        boolean sparovany, pripojeny;
        int hid = HID_NEVI;         // B332: stav v HID profilu Androidu (zapamatovane ovladace)
        Nalez(String adresa, String jmeno, int trida, int sila, boolean sparovany) {
            this.adresa = adresa; this.jmeno = jmeno; this.trida = trida; this.sila = sila; this.sparovany = sparovany;
        }
        boolean ovladac() { return jeOvladac(trida, jmeno); }
    }

    /** Slova v nazvu, podle kterych se pozna herni ovladac (kdyz trida Bluetooth chybi nebo je spatne). */
    private static final String[] SLOVA = {"controller", "gamepad", "game pad", "joystick", "joy-con", "joycon",
            "xbox", "dualshock", "dualsense", "8bitdo", "gamesir", "ipega", "battletron", "retroid", "nimbus",
            "stadia", "moga", "flydigi", "gulikit", "mocute", "nacon", "kishi", "backbone", "powera", "zikway"};

    /** Je to herni ovladac? Trida Bluetooth: periferie (0x0500) a typ joystick (0x04) nebo gamepad (0x08); jinak nazev. */
    static boolean jeOvladac(int trida, String jmeno) {
        if (trida >= 0 && (trida & 0x1F00) == 0x0500) {
            final int typ = trida & 0x3C;
            if (typ == 0x04 || typ == 0x08) return true;
        }
        return jmenoOvladace(jmeno);
    }

    static boolean jmenoOvladace(String jmeno) {
        if (jmeno == null) return false;
        final String j = jmeno.toLowerCase(Locale.ROOT);
        for (String s : SLOVA) if (j.contains(s)) return true;
        return false;
    }

    /** Povoleni, ktera Android chce pro hledani a parovani (od Androidu 12 "Zarizeni v okoli", 6-11 poloha). */
    static String[] povoleni(int sdk) {
        if (sdk >= 31) return new String[]{"android.permission.BLUETOOTH_SCAN", "android.permission.BLUETOOTH_CONNECT"};
        if (sdk >= 23) return new String[]{"android.permission.ACCESS_FINE_LOCATION", "android.permission.ACCESS_COARSE_LOCATION"};
        return new String[0];
    }

    /** Android 10 a 11 bez zapnute Polohy Bluetooth nehleda (vysledky neprijdou). */
    static boolean potrebujePolohuZapnutou(int sdk) {
        return sdk == 29 || sdk == 30;
    }

    /** Do logu jen konec adresy ("..:A1:B2") - cela adresa zustava v appce. */
    static String kratkaAdresa(String mac) {
        if (mac == null || mac.length() < 5) return "?";
        return ".." + mac.substring(mac.length() - 5);
    }

    /** Jmeno do seznamu: bez jmena -> "Bez jména (..:A1:B2)". */
    static String jmenoDoSeznamu(boolean en, String jmeno, String adresa) {
        if (jmeno != null && !jmeno.trim().isEmpty()) return jmeno.trim();
        return (en ? "No name (" : "Bez jména (") + kratkaAdresa(adresa) + ")";
    }

    /** Text pro stranku OPTIONS podle faze. jmeno = ovladac (parovani), zbyva = s do konce hledani. */
    static String text(boolean en, String faze, int sdk, String jmeno, int nalezeno, int ovladacu, int zbyva, String chyba) {
        final String jm = jmeno == null || jmeno.isEmpty() ? (en ? "the controller" : "ovladač") : jmeno;
        switch (faze) {
            case F_NENI_BT:
                return en ? "This phone has no Bluetooth." : "Tenhle telefon nemá Bluetooth.";
            case F_POVOLENI:
                if (sdk >= 31) return en ? "Android will now ask for the “Nearby devices” permission – without it the app can't find the controller."
                                         : "Android se teď zeptá na povolení „Zařízení v okolí“ – bez něj appka ovladač nenajde.";
                return en ? "Android will now ask for the location permission – Android requires it to search for Bluetooth. The app does not track your location."
                          : "Android se teď zeptá na povolení k poloze – chce ho kvůli hledání Bluetooth. Appka polohu nezjišťuje.";
            case F_ODMITNUTO:
                return en ? "Without the permission the app can't find the controller. You can pair it in BLUETOOTH SETTINGS."
                          : "Bez povolení appka ovladač nenajde. Spárovat ho můžeš v NASTAVENÍ BLUETOOTH.";
            case F_ZAPINANI:
                return en ? "Android asks whether to turn Bluetooth on – tap ALLOW." : "Android se ptá, jestli zapnout Bluetooth – klepni na POVOLIT.";
            case F_BT_VYPNUTE:
                return en ? "Bluetooth is off – the controller can't be found without it." : "Bluetooth je vypnuté – bez něj ovladač nenajdu.";
            case F_POLOHA_VYPNUTA:
                return en ? "Turn Location on for a moment – Android 10 and 11 don't search for Bluetooth without it."
                          : "Zapni na chvíli Polohu – Android 10 a 11 bez ní Bluetooth nehledá.";
            case F_HLEDAM:
                return (en ? "Searching for controllers nearby… (" + zbyva + " s)\nPut the controller into pairing mode – usually hold HOME, on some also SHARE or the pairing button."
                           : "Hledám ovladače v okolí… (" + zbyva + " s)\nDej ovladač do párování – většinou podržet HOME, u některých i SHARE nebo tlačítko párování.")
                        + (ovladacu > 0 ? (en ? "\nFound – tap the controller in the list." : "\nNalezeno – klepni na ovladač v seznamu.") : "");
            case F_HOTOVO:
                if (ovladacu > 0) return en ? "Search finished – tap the controller in the list." : "Hledání skončilo – klepni na ovladač v seznamu.";
                return en ? "No controller found (" + nalezeno + " other devices). Is it in pairing mode? Tap CONNECT CONTROLLER again."
                          : "Žádný ovladač jsem nenašel (jiných zařízení " + nalezeno + "). Je v párování? Klepni znovu na PŘIPOJIT OVLADAČ.";
            case F_PARUJI:
                return en ? "Pairing with “" + jm + "”… If the phone asks, tap PAIR." : "Páruji s „" + jm + "“… Když se telefon zeptá, klepni na SPÁROVAT.";
            case F_SPAROVANO:
                return en ? "“" + jm + "” is paired ✔ It connects by itself – if not, press HOME on it."
                          : "„" + jm + "“ je spárovaný ✔ Připojí se sám – kdyby ne, zmáčkni na něm HOME.";
            case F_PRIPOJENO:
                return en ? "“" + jm + "” is connected ✔ You can play." : "„" + jm + "“ je připojený ✔ Můžeš hrát.";
            case F_PRIPOJUJI: {   // B332: PRIPOJIT u zapamatovaneho ovladace
                final boolean bez = jmeno == null || jmeno.isEmpty();
                if (appkaSmiPripojit(sdk))
                    return en ? "Connecting " + (bez ? "the controller" : "“" + jm + "”") + "… If it's off, turn it on (HOME). (" + zbyva + " s)"
                              : "Připojuji " + (bez ? "ovladač" : "„" + jm + "“") + "… Když je vypnutý, zapni ho (HOME). (" + zbyva + " s)";
                return en ? "Turn " + (bez ? "the controller" : "“" + jm + "”") + " on (HOME button) – Android connects it by itself. Waiting… (" + zbyva + " s)"
                          : "Zapni ovladač" + (bez ? "" : " „" + jm + "“") + " (tlačítko HOME / domeček) – Android ho připojí sám. Čekám… (" + zbyva + " s)";
            }
            case F_OZVAL_SE:
                return en ? "“" + jm + "” responded but didn't connect as a controller. Turn it off and on again. If that doesn't help: "
                            + "BLUETOOTH SETTINGS → “" + jm + "” → unpair (forget) and pair it again with CONNECT CONTROLLER (BLUETOOTH)."
                          : "„" + jm + "“ se ozval, ale jako ovladač se nepřipojil. Vypni ho a zapni znovu. Když to nepomůže: "
                            + "NASTAVENÍ BLUETOOTH → „" + jm + "“ → zrušit párování (odebrat) a spáruj ho znovu přes PŘIPOJIT OVLADAČ (BLUETOOTH).";
            case F_NEOZVAL:
                return en ? "“" + jm + "” didn't respond. Is it on and charged? A controller with several modes (Switch / Android / PC) "
                            + "must be turned on in the same mode as when it was paired. Otherwise pair it again (CONNECT CONTROLLER)."
                          : "„" + jm + "“ se neozval. Je zapnutý a nabitý? Ovladač s víc režimy (Switch / Android / PC) zapni "
                            + "ve stejném režimu jako při párování. Jinak ho spáruj znovu (PŘIPOJIT OVLADAČ).";
            case F_NEPOVEDLO:
                return en ? "Pairing with “" + jm + "” failed. Put the controller into pairing mode again and tap it once more."
                          : "Spárování s „" + jm + "“ se nepovedlo. Dej ovladač znovu do párování a klepni na něj znovu.";
            case F_CHYBA:
                return (en ? "Search can't start" : "Hledání nejde spustit") + (chyba == null || chyba.isEmpty() ? "" : " (" + chyba + ")")
                        + (en ? ". Try BLUETOOTH SETTINGS." : ". Zkus NASTAVENÍ BLUETOOTH.");
            default:
                return "";
        }
    }

    /** Poradi v seznamu: ovladace nahoru, pak silnejsi signal, pak jmeno. */
    static List<Nalez> serad(List<Nalez> v) {
        final ArrayList<Nalez> r = new ArrayList<>(v);
        Collections.sort(r, (x, y) -> {
            if (x.ovladac() != y.ovladac()) return x.ovladac() ? -1 : 1;
            if (x.sila != y.sila) return Integer.compare(y.sila, x.sila);
            return String.valueOf(x.jmeno).compareToIgnoreCase(String.valueOf(y.jmeno));
        });
        return r;
    }

    /** JSON pro stranku: {"f":faze,"t":text,"z":[nalezene],"m":[zapamatovane ovladace]}. */
    static String json(boolean en, String faze, String text, List<Nalez> nalezene, List<Nalez> zapamatovane) {
        final StringBuilder b = new StringBuilder(256);
        b.append("{\"f\":").append(js(faze)).append(",\"t\":").append(js(text)).append(",\"z\":[");
        boolean prvni = true;
        for (Nalez n : serad(nalezene)) {
            if (!prvni) b.append(',');
            prvni = false;
            b.append("{\"a\":").append(js(n.adresa)).append(",\"j\":").append(js(jmenoDoSeznamu(en, n.jmeno, n.adresa)))
             .append(",\"o\":").append(n.ovladac() ? 1 : 0).append(",\"s\":").append(n.sparovany ? 1 : 0)
             .append(",\"p\":").append(n.pripojeny ? 1 : 0).append('}');
        }
        b.append("],\"m\":[");
        prvni = true;
        for (Nalez n : zapamatovane) {
            if (!prvni) b.append(',');
            prvni = false;
            // B332: + adresa (tlacitko PRIPOJIT) a stav v Androidu (h: 1 = pripojuje se)
            b.append("{\"j\":").append(js(jmenoDoSeznamu(en, n.jmeno, n.adresa))).append(",\"p\":").append(n.pripojeny ? 1 : 0)
             .append(",\"a\":").append(js(n.adresa)).append(",\"h\":").append(n.hid).append('}');
        }
        return b.append("]}").toString();
    }

    /** Retezec do JSON (uvozovky, zpetne lomitko, ridici znaky; ostatni Unicode beze zmeny). */
    static String js(String s) {
        if (s == null) return "\"\"";
        final StringBuilder b = new StringBuilder(s.length() + 2).append('"');
        for (int i = 0; i < s.length(); i++) {
            final char c = s.charAt(i);
            switch (c) {
                case '"': b.append("\\\""); break;
                case '\\': b.append("\\\\"); break;
                case '\n': b.append("\\n"); break;
                case '\r': b.append("\\r"); break;
                case '\t': b.append("\\t"); break;
                default:
                    if (c < 0x20 || c == 0x2028 || c == 0x2029) b.append(String.format("\\u%04x", (int) c));
                    else b.append(c);
            }
        }
        return b.append('"').toString();
    }
}
