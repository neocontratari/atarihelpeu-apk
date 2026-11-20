package eu.atarihelp.emu10;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

/**
 * B357: ONLINE HRA - Java cast appky bez Androidu: NapOnlineHra (cesty dotazu na engine, jmeno hrace, otisk hry,
 * stav z C++) a NapServerKlient.hlavickyPro (podpis spojeni, ktere otevira C++). Podpis se overi presne tak,
 * jak ho overuje engine (NapServer v12.4, NapJadro.Overeni: HMAC-SHA256(token, zarizeni\ncas\nMETODA\nRawUrl),
 * hodiny +-600 s) - RawUrl = cesta i s ?dotazem tak, jak ji C++ posle v radku GET.
 *   javac --release 17 -cp OUT:$ANDROID_JAR -d T test_overeni/b357/jvm/eu/atarihelp/emu10/TestB357Online.java
 *   java -cp T:OUT:$ANDROID_JAR eu.atarihelp.emu10.TestB357Online      -> jvm/VYSTUP_TestB357Online.txt
 */
public class TestB357Online {
    static int ok = 0, chyb = 0;

    static void over(boolean b, String co) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + co);
    }

    static String hmac(String klic, String zprava) throws Exception {
        Mac m = Mac.getInstance("HmacSHA256");
        m.init(new SecretKeySpec(klic.getBytes(StandardCharsets.UTF_8), "HmacSHA256"));
        StringBuilder s = new StringBuilder();
        for (byte x : m.doFinal(zprava.getBytes(StandardCharsets.UTF_8))) s.append(String.format("%02x", x & 0xFF));
        return s.toString();
    }

    public static void main(String[] a) throws Exception {
        System.out.println("B357 - online hra: NapOnlineHra + NapServerKlient.hlavickyPro (bez Androidu)");
        // 1) jmeno hrace
        over(NapOnlineHra.jmeno("René").equals("René") && NapOnlineHra.jmeno("  ").equals("HRAC") && NapOnlineHra.jmeno(null).equals("HRAC"),
                "1) jmeno: 'René' zustane, prazdne = HRAC");
        over(NapOnlineHra.jmeno("a\nb\tc<d>\"e\\f").equals("abcdef"), "   ridici znaky a < > \" \\ pryc: " + NapOnlineHra.jmeno("a\nb\tc<d>\"e\\f"));
        String dlouhe = NapOnlineHra.jmeno("Xiaomi Redmi Note 13 Pro Plus 5G");
        over(dlouhe.length() <= 24 && dlouhe.startsWith("Xiaomi Redmi"), "   nejvys 24 znaku: '" + dlouhe + "'");
        String emo = NapOnlineHra.jmeno("Hráč 🎮");
        over(emo.equals("Hráč 🎮") && NapOnlineHra.jmeno("x\uD83C").equals("x"), "   emoji cele (par), pulka pryc");
        // 2) cesty
        String z = NapOnlineHra.cestaZaloz("sega", "Sonic_The_Hedgehog (World).md", "sega-0123456789abcdef",
                "0123456789abcdef0123456789abcdef01234567", "16", "René");
        over(z.equals("/api/v1/online/zaloz?system=sega&hra=Sonic+The+Hedgehog+%28World%29&hra_id=sega-0123456789abcdef"
                + "&otisk=0123456789abcdef0123456789abcdef01234567&nastaveni=16&jmeno=Ren%C3%A9"), "2) cesta hrace 1: " + z);
        String z2 = NapOnlineHra.cestaZaloz("sega", "/sdcard/hry/moje.bin", "nic", "0123456789abcdef0123456789abcdef01234567", "0", "");
        over(z2.contains("&hra=moje&") && z2.contains("&hra_id=&") && z2.endsWith("&jmeno=HRAC"), "   hra z telefonu: bez id, nazev bez cesty a pripony: " + z2);
        over(NapOnlineHra.cestaPripoj(" 0427 ", "bracha").equals("/api/v1/online/pripoj?kod=0427&jmeno=bracha")
                && NapOnlineHra.cestaPripoj("12345", "x").startsWith("/api/v1/online/pripoj?kod=&"), "   cesta hrace 2 (kod jen 4 cislice)");
        // 3) otisk hry z C++ a stejna hra
        String[] o = NapOnlineHra.otisk("891E5C3AB4C57347C9DCDC6A263580C860B8EF3E|16");
        over(o != null && o[0].equals("891e5c3ab4c57347c9dcdc6a263580c860b8ef3e") && o[1].equals("16"), "3) otisk z C++ (sha1|nastaveni)");
        over(NapOnlineHra.otisk("") == null && NapOnlineHra.otisk("abc|1") == null && NapOnlineHra.otisk("891e5c3ab4c57347c9dcdc6a263580c860b8ef3e|x") == null,
                "   zadna hra / nesmysl = null");
        String moje = "891e5c3ab4c57347c9dcdc6a263580c860b8ef3e|16";
        over(NapOnlineHra.stejnaHra(moje, "891E5C3AB4C57347C9DCDC6A263580C860B8EF3E", "16") && !NapOnlineHra.stejnaHra(moje, "891e5c3ab4c57347c9dcdc6a263580c860b8ef3e", "0")
                && !NapOnlineHra.stejnaHra(moje, "e33fe5db01a322a6d8c42763049d889c8b2f5cc4", "16") && !NapOnlineHra.stejnaHra("", "x", "16"),
                "   stejna hra = stejny otisk i nastaveni (region / TV)");
        // 4) stav z C++
        Map<String, String> st = NapOnlineHra.stav("faze=HRAJE\nkod=0427\ndruhy=bracha\npopis=ONLINE S bracha - ODEZVA 23 MS\nkonec=");
        over("HRAJE".equals(st.get("faze")) && "0427".equals(st.get("kod")) && "ONLINE S bracha - ODEZVA 23 MS".equals(st.get("popis")) && "".equals(st.get("konec")),
                "4) stav z C++ (radky klic=hodnota)");
        over(NapOnlineHra.serverId("sega-0123456789abcdef").length() == 21 && NapOnlineHra.serverId("sega-0123").isEmpty()
                && NapOnlineHra.serverId("x/../sega-0123456789abcdef").isEmpty(), "   id hry na serveru jen ve tvaru system-16hex");
        // 5) podpis spojeni (C++ otevre WebSocket s temito hlavickami)
        final Map<String, String> ul = new HashMap<>();
        NapServerKlient k = new NapServerKlient(new NapServerKlient.Uloziste() {
            @Override public String cti(String kl, String v) { String r = ul.get(kl); return r == null ? v : r; }
            @Override public void zapis(String kl, String v) { if (v == null) ul.remove(kl); else ul.put(kl, v); }
        }, "a1b2c3d4e5f60718", "Test Telefon", "14", "EMU10-B357");
        String[] h0 = k.hlavickyPro("GET", "/api/v1/online/pripoj?kod=0427&jmeno=x");
        over(h0.length == 1 && h0[0].startsWith("User-Agent: NapVision/EMU10-B357"), "5) nesparovany telefon: jen User-Agent");
        String token = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
        ul.put("token", token);
        ul.put("token_pro", "a1b2c3d4e5f60718");
        String cesta = z;
        long pred = System.currentTimeMillis() / 1000L;
        String[] h = k.hlavickyPro("get", cesta);
        Map<String, String> hm = new HashMap<>();
        for (String r : h) { int i = r.indexOf(": "); hm.put(r.substring(0, i), r.substring(i + 2)); }
        String cas = hm.get("X-NaP-Cas");
        boolean casOk = cas != null && Math.abs(Long.parseLong(cas) - pred) <= 2;
        String ocekavany = hmac(token, "a1b2c3d4e5f60718" + "\n" + cas + "\n" + "GET" + "\n" + cesta);   // = NapJadro.Overeni
        over(h.length == 4 && "a1b2c3d4e5f60718".equals(hm.get("X-NaP-Zarizeni")) && casOk && ocekavany.equals(hm.get("X-NaP-Podpis")),
                "   sparovany: User-Agent, X-NaP-Zarizeni, X-NaP-Cas, X-NaP-Podpis = HMAC jako engine (cesta i s ?dotazem)");
        boolean radky = true;
        for (String r : h) if (r.indexOf('\r') >= 0 || r.indexOf('\n') >= 0) radky = false;
        over(radky, "   zadna hlavicka nema konec radku (C++ je sklada do pozadavku)");
        ul.put("posun", "-3600");
        String[] h2 = k.hlavickyPro("GET", cesta);
        long cas2 = Long.parseLong(h2[2].substring("X-NaP-Cas: ".length()));
        over(Math.abs(cas2 - (pred - 3600)) <= 2, "   cas podle serveru (posun hodin z minuleho dotazu): " + (cas2 - pred) + " s");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
