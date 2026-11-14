package eu.atarihelp.emu10;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

/**
 * B322: skutecne tridy appky bez Androidu.
 *  A) NapPostupJadro - Sega 1500 radku, PS1 120 hodin Segy, koupeno ze serveru, PS1 az po Seze,
 *     zaloha postupu (bere se vyssi), servis ODEMKNOUT DALSI / ZAMKNOUT VSE.
 *  B) NapPravidla - texty zamku cesky / anglicky (hodiny "118 h 47 min").
 *  C) NapServerKlient.post + NapJson.mapa proti SKUTECNEMU jadru serveru N&P v11 (NapTest.exe, mono):
 *     platba, ZAPLATIL JSEM, spravce odemkne, stav -> odemceno, zaloha postupu, PS1 az po Seze.
 * (v balicku eu.atarihelp.emu10 - tridy appky jsou package-private)
 * Pouziti: java -cp out eu.atarihelp.emu10.TestB322 <NapTest.exe> <pracovni slozka>
 */
public class TestB322 {
    static int ok = 0, chyb = 0;
    static String SERVER;
    static final int PORT = 18422;

    static void kontrola(String nazev, boolean podminka, Object detail) {
        if (podminka) { ok++; System.out.println("OK     " + nazev); }
        else { chyb++; System.out.println("CHYBA  " + nazev + "   -> " + detail); }
    }

    /** SharedPreferences jako mapa. */
    static final class MapaPamet implements NapPostupJadro.Pamet {
        final Map<String, Object> m = new HashMap<>();
        public boolean ctiB(String k, boolean v) { Object o = m.get(k); return o == null ? v : (Boolean) o; }
        public int ctiI(String k, int v) { Object o = m.get(k); return o == null ? v : (Integer) o; }
        public long ctiL(String k, long v) { Object o = m.get(k); return o == null ? v : (Long) o; }
        public void zapisB(String k, boolean v) { m.put(k, v); }
        public void zapisI(String k, int v) { m.put(k, v); }
        public void zapisL(String k, long v) { m.put(k, v); }
    }

    static final class Uloz implements NapServerKlient.Uloziste {
        final Map<String, String> m = new HashMap<>();
        public synchronized String cti(String k, String v) { String x = m.get(k); return x == null ? v : x; }
        public synchronized void zapis(String k, String v) { if (v == null) m.remove(k); else m.put(k, v); }
    }

    static String test(String cesta) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(SERVER + cesta).openConnection();
        try (InputStream in = c.getInputStream()) { return new String(in.readAllBytes(), StandardCharsets.UTF_8); }
        finally { c.disconnect(); }
    }

    static String enc(String s) throws Exception { return URLEncoder.encode(s, "UTF-8"); }

    static final long H = 3600_000L;

    public static void main(String[] a) throws Exception {
        postup();
        texty();
        if (a.length >= 2) server(a[0], Paths.get(a[1]));
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }

    // ------------------------------------------------------------------ A) postup hrace
    static void postup() {
        System.out.println("== A NapPostupJadro (Sega 1500 radku, PS1 120 h, koupeno, PS1 az po Seze)");
        kontrola("A0 prahy: Sega 1500 radku, PS1 120 hodin (432 000 000 ms), davka od 5 radku",
                NapPostupJadro.SEGA_POTREBA_RADKU == 1500 && NapPostupJadro.PS1_POTREBA_MS == 432_000_000L && NapPostupJadro.NEJMENE_RADKU == 5, "");
        MapaPamet m = new MapaPamet();
        NapPostupJadro j = new NapPostupJadro(m);
        kontrola("A1 novy hrac: Sega i PS1 zamcene, zbyva 1500 radku a 7200 minut",
                !j.segaOdemcena() && !j.ps1Odemcena() && j.zbyvaRadku() == 1500 && j.zbyvaMinutSegy() == 7200, j.stav());
        boolean o1 = j.pripocitejRadky(4);
        kontrola("A2 davka 4 radky se nepocita", !o1 && j.napsanoRadku() == 0, j.stav());
        boolean otevreno = false;
        for (int i = 0; i < 299; i++) otevreno |= j.pripocitejRadky(5);   // 1495
        kontrola("A3 1495 radku: Sega porad zamcena (dřív by stacilo 50)", !otevreno && !j.segaOdemcena() && j.zbyvaRadku() == 5, j.stav());
        boolean o2 = j.pripocitejRadky(5);
        kontrola("A4 1500 radku: Sega se prave otevrela (true), ziskana", o2 && j.segaOdemcena() && j.segaZiskana() && !j.segaKoupena(), j.stav());
        kontrola("A4 dalsi radky uz 'prave otevreno' nehlasi", !j.pripocitejRadky(10), j.stav());
        boolean p1 = j.pripocitejSegu(119 * H + 59 * 60_000L);
        kontrola("A5 119 h 59 min Segy: PS1 zamcena, zbyva 1 minuta", !p1 && !j.ps1Odemcena() && j.zbyvaMinutSegy() == 1, j.stav());
        boolean p2 = j.pripocitejSegu(60_000L);
        kontrola("A6 120 hodin: PS1 se prave otevrela", p2 && j.ps1Odemcena() && j.ps1Ziskana(), j.stav());

        // koupeno ze serveru
        MapaPamet m2 = new MapaPamet();
        NapPostupJadro k = new NapPostupJadro(m2);
        kontrola("A7 nastavKoupeno(false,false) u noveho = zadna zmena", !k.nastavKoupeno(false, false), "");
        kontrola("A8 server: zaplaceno PS1, Sega ne -> PS1 porad zamcena (PS1 az po Seze)",
                k.nastavKoupeno(false, true) && !k.ps1Odemcena() && !k.segaOdemcena(), k.stav());
        kontrola("A9 server: zaplaceno Sega i PS1 -> obe otevrene, stav koupeno=SP",
                k.nastavKoupeno(true, true) && k.segaOdemcena() && k.ps1Odemcena() && k.stav().endsWith("koupeno=SP"), k.stav());
        kontrola("A10 spravce zrusil odemceni Segy -> Sega i PS1 zase zamcene (nic ziskaneho)",
                k.nastavKoupeno(false, true) && !k.segaOdemcena() && !k.ps1Odemcena(), k.stav());
        k.nastavKoupeno(true, false);
        boolean p3 = k.pripocitejSegu(120 * H);
        kontrola("A11 koupena Sega + 120 hodin hrani -> PS1 ziskana a prave otevrena", p3 && k.ps1Odemcena() && k.ps1Ziskana(), k.stav());

        // zaloha postupu ze serveru
        MapaPamet m3 = new MapaPamet();
        NapPostupJadro s = new NapPostupJadro(m3);
        s.pripocitejRadky(100);
        kontrola("A12 server ma mene (50 radku) -> nic se nemeni", !s.slucPostup(50, 0) && s.napsanoRadku() == 100, s.stav());
        kontrola("A13 server ma 1500 radku (preinstalace) -> Sega otevrena z postupu", s.slucPostup(1500, 2 * H) && s.segaOdemcena()
                && s.napsanoRadku() == 1500 && s.segaOdehranoMs() == 2 * H, s.stav());
        kontrola("A14 server ma 121 hodin -> PS1 otevrena", s.slucPostup(0, 121 * H) && s.ps1Odemcena() && s.segaOdehranoMs() == 121 * H, s.stav());
        MapaPamet m4 = new MapaPamet();
        NapPostupJadro t = new NapPostupJadro(m4);
        kontrola("A15 server: 130 h Segy, ale jen 10 radku -> PS1 ne (Sega neni)", !t.slucPostup(10, 130 * H) && !t.ps1Odemcena(), t.stav());
        kontrola("A16 obrovsky pocet radku ze serveru se vejde do int", t.slucPostup(5_000_000_000L, 0) && t.napsanoRadku() == Integer.MAX_VALUE, t.stav());

        // servis
        MapaPamet m5 = new MapaPamet();
        NapPostupJadro v = new NapPostupJadro(m5);
        String d1 = v.odemkniDalsi(), d2 = v.odemkniDalsi(), d3 = v.odemkniDalsi();
        kontrola("A17 servis ODEMKNOUT DALSI: SEGA, PS1, UZ JE VSE OTEVRENE (+ radky a cas na prah)",
                "SEGA".equals(d1) && "PS1".equals(d2) && "UZ JE VSE OTEVRENE".equals(d3) && v.napsanoRadku() == 1500 && v.segaOdehranoMs() == 120 * H, v.stav());
        v.nastavKoupeno(true, true);
        v.zamkniVse();
        kontrola("A18 servis ZAMKNOUT VSE: vse zamcene, nuly, i zapamatovane platby", !v.segaOdemcena() && !v.ps1Odemcena()
                && v.napsanoRadku() == 0 && v.segaOdehranoMs() == 0 && !v.segaKoupena() && !v.ps1Koupena(), v.stav());

        // stara instalace (B321: 50 radku stacilo) - otevrena Sega zustava
        MapaPamet m6 = new MapaPamet();
        m6.m.put(NapPostupJadro.K_SEGA, true); m6.m.put(NapPostupJadro.K_RADKY, 60); m6.m.put(NapPostupJadro.K_PS1, true); m6.m.put(NapPostupJadro.K_MINUTY, 31 * 60_000L);
        NapPostupJadro st = new NapPostupJadro(m6);
        kontrola("A19 hrac z B321 (Sega po 50 radcich, PS1 po 30 min): co ma otevrene, zustava otevrene",
                st.segaOdemcena() && st.ps1Odemcena(), st.stav());
        kontrola("A20 stav do logu", st.stav().equals("radku=60/1500 sega=otevrena ps1=otevrena odehrano=31min/7200 koupeno=-"), st.stav());
    }

    // ------------------------------------------------------------------ B) texty
    static void texty() {
        System.out.println("== B NapPravidla - texty zamku");
        kontrola("B1 casText: 7200 min = 120 h, 7127 = 118 h 47 min, 47 = 47 min, 60 = 1 h",
                NapPravidla.casText(7200).equals("120 h") && NapPravidla.casText(7127).equals("118 h 47 min")
                        && NapPravidla.casText(47).equals("47 min") && NapPravidla.casText(60).equals("1 h") && NapPravidla.casText(-3).equals("0 min"), "");
        String s = NapPravidla.duvodKarty(NapPravidla.SEGA, false, 1437, 0, false);
        String se = NapPravidla.duvodKarty(NapPravidla.SEGA, false, 1, 0, true);
        kontrola("B2 Sega cesky / anglicky", s.equals("Napiš v Atari ještě 1437 řádků programu – nebo odemkni hned.")
                && se.equals("Type 1 more program line on the Atari – or unlock it now."), s + " | " + se);
        String p = NapPravidla.duvodKarty(NapPravidla.PS1, true, 0, 7127, false);
        String pe = NapPravidla.duvodKarty(NapPravidla.PS1, true, 0, 30, true);
        kontrola("B3 PS1 po Seze: hodiny a minuty", p.equals("Hraj na Seze ještě 118 h 47 min – nebo odemkni hned.")
                && pe.equals("Play the Sega for 30 min more – or unlock it now."), p + " | " + pe);
        String q = NapPravidla.duvodKarty(NapPravidla.PS1, false, 0, 7200, false);
        String qe = NapPravidla.duvodKarty(NapPravidla.PS1, false, 0, 7200, true);
        kontrola("B4 PS1 bez Segy: nejdriv Sega, 120 hodin (bez 'odemkni hned' - PS1 az po Seze)",
                q.equals("Nejdřív odemkni Segu, pak 120 hodin hraní na Seze.") && qe.equals("Unlock the Sega first, then play it for 120 hours."), q + " | " + qe);
        String z = NapPravidla.duvodZamku(NapPravidla.SEGA, false, 2, 0, false);
        kontrola("B5 hlaska u zamcene konzole z webu (nadpis + duvod)", z.equals("SEGA JE ZAMČENÁ\nNapiš v Atari ještě 2 řádky programu – nebo odemkni hned."), z);
    }

    // ------------------------------------------------------------------ C) proti serveru v11
    @SuppressWarnings("unchecked")
    static void server(String exe, Path prac) throws Exception {
        System.out.println("== C NapServerKlient (post, platba, zaloha postupu) proti skutecnemu serveru N&P v11");
        Path slozka = prac.resolve("server_jvm_b322");
        if (Files.exists(slozka)) try (var w = Files.walk(slozka)) { w.sorted(Comparator.reverseOrder()).forEach(x -> x.toFile().delete()); }
        for (String s : new String[]{"Atari", "Sega", "PS1"}) Files.createDirectories(slozka.resolve("Arcade").resolve(s));
        Path logCesta = prac.resolve("log_server_jvm_b322.txt");
        Process p = new ProcessBuilder("mono", exe, "--slozka", slozka.toString(), "--port", String.valueOf(PORT), "--test")
                .redirectErrorStream(true).redirectOutput(logCesta.toFile()).start();
        SERVER = "http://127.0.0.1:" + PORT;
        for (int i = 0; i < 200; i++) { try (Socket s = new Socket("127.0.0.1", PORT)) { break; } catch (IOException e) { Thread.sleep(50); } }
        Thread.sleep(300);
        try {
            String kod = (String) NapJson.objekt(test("/test/novy-kod?poznamka=JVM")).get("kod");
            Uloz u = new Uloz();
            NapServerKlient k = new NapServerKlient(u, "b322jvm00000001", "Samsung SM-G950F", "9", "EMU10-B322-PLATBY");
            k.nastavServer(SERVER);   // jinak by appka chodila na vychozi (Reneho) server
            NapServerKlient.Odpoved o = k.sparuj(kod, "");
            kontrola("C1 sparovano kodem", o.kod == 200 && k.sparovano(), o.kod + " " + o.text());

            o = k.get("/api/v1/stav");
            Map<String, Object> od = NapJson.mapa(o.json, "odemceno"), po = NapJson.mapa(o.json, "postup");
            kontrola("C2 stav: NapJson.mapa -> odemceno {sega:false, ps1:false}, postup {0, 0}, platby false",
                    o.ok() && od != null && !NapJson.ano(od, "sega") && od.containsKey("ps1") && po != null
                            && NapJson.cislo(po, "radku", -1) == 0 && o.json.get("platby") == Boolean.FALSE, o.text());
            kontrola("C2 NapJson.mapa na neexistujici / necim jinym = null", NapJson.mapa(o.json, "nic") == null && NapJson.mapa(o.json, "kod") == null, "");

            String r = test("/test/platby?ucet=" + enc("19-2000145399/0800") + "&jmeno=" + enc("René Nováček"));
            kontrola("C3 spravce nastavil ucet", r.contains("\"ok\":true"), r);
            o = k.get("/api/v1/platba?co=sega");
            kontrola("C4 platba: VS 10011, 75 Kc, SPD a EPC texty", o.ok() && "10011".equals(NapJson.text(o.json, "vs"))
                    && NapJson.cislo(o.json, "kc", 0) == 75 && NapJson.text(o.json, "spd").startsWith("SPD*1.0*ACC:CZ6508000000192000145399*AM:75.00*CC:CZK*X-VS:10011")
                    && NapJson.text(o.json, "epc").startsWith("BCD\n002\n1\nSCT\n"), o.text());

            o = k.post("/api/v1/zaplatil?co=ps1&sega=0");
            kontrola("C5 post ZAPLATIL ps1 bez Segy = 409 nejdriv_sega", o.kod == 409 && "nejdriv_sega".equals(o.duvod()), o.kod + " " + o.text());
            o = k.post("/api/v1/zaplatil?co=sega&sega=0");
            kontrola("C6 post ZAPLATIL sega = 200, zadost", o.ok() && NapJson.ano(o.json, "zadost"), o.kod + " " + o.text());
            o = k.get("/api/v1/stav");
            kontrola("C7 stav.zadost.sega = true", NapJson.ano(NapJson.mapa(o.json, "zadost"), "sega"), o.text());

            r = test("/test/odemkni?kod=" + enc(kod) + "&co=sega");
            o = k.get("/api/v1/stav");
            MapaPamet mp = new MapaPamet();
            NapPostupJadro j = new NapPostupJadro(mp);
            Map<String, Object> o2 = NapJson.mapa(o.json, "odemceno");
            boolean zmena = j.nastavKoupeno(NapJson.ano(o2, "sega"), NapJson.ano(o2, "ps1"));
            kontrola("C8 spravce odemkl -> stav.odemceno.sega -> appka: Sega otevrena (koupena), zadost pryc",
                    r.contains("\"ok\":true") && zmena && j.segaOdemcena() && j.segaKoupena() && !NapJson.ano(NapJson.mapa(o.json, "zadost"), "sega"), o.text());

            // zaloha postupu: stejne jako MainActivity.napPostupOdesli
            o = k.post("/api/v1/postup?radku=777&sega_ms=" + (3 * H) + "&sega=1&ps1=0");
            kontrola("C9 post zaloha postupu (podepsana adresa) -> server vraci 777 / 3 h a odemceno",
                    o.ok() && NapJson.cislo(o.json, "radku", 0) == 777 && NapJson.cislo(o.json, "sega_ms", 0) == 3 * H
                            && NapJson.ano(NapJson.mapa(o.json, "odemceno"), "sega"), o.text());
            // novy telefon (preinstalace): prazdny postup, server ho vrati
            Uloz u2 = new Uloz();
            NapServerKlient k2 = new NapServerKlient(u2, "b322jvm00000001", "Samsung SM-G950F", "9", "EMU10-B322-PLATBY");
            k2.nastavServer(SERVER);
            o = k2.sparuj(kod, "");
            NapServerKlient.Odpoved s2 = k2.get("/api/v1/stav");
            MapaPamet mp2 = new MapaPamet();
            NapPostupJadro j2 = new NapPostupJadro(mp2);
            Map<String, Object> pp = NapJson.mapa(s2.json, "postup"), oo = NapJson.mapa(s2.json, "odemceno");
            j2.nastavKoupeno(NapJson.ano(oo, "sega"), NapJson.ano(oo, "ps1"));
            j2.slucPostup(NapJson.cislo(pp, "radku", 0), NapJson.cislo(pp, "sega_ms", 0));
            kontrola("C10 preinstalace (stejny telefon, stejny kod): zaplacena Sega i postup 777 radku / 3 h se vrati",
                    o.kod == 200 && j2.segaOdemcena() && j2.napsanoRadku() == 777 && j2.segaOdehranoMs() == 3 * H && !j2.ps1Odemcena(), s2.text());
            o = k2.post("/api/v1/zaplatil?co=ps1&sega=1");
            kontrola("C11 ZAPLATIL ps1 (Sega zaplacena) = 200", o.ok(), o.kod + " " + o.text());
            o = k2.post("/api/v1/postup?radku=0&sega_ms=0&sega=0&ps1=0&reset=1");
            kontrola("C12 servis ZAMKNOUT VSE -> reset zalohy na serveru, zaplacene zustava",
                    o.ok() && NapJson.cislo(o.json, "radku", -1) == 0 && NapJson.ano(NapJson.mapa(o.json, "odemceno"), "sega"), o.text());

            // spatne hodiny telefonu: post se srovna podle serveru a zkusi znovu (jako get)
            u2.zapis("posun", "-3600");
            o = k2.post("/api/v1/postup?radku=5&sega_ms=0&sega=1&ps1=0");
            kontrola("C13 post se spatnymi hodinami telefonu: server 401 cas -> srovnat -> 200", o.ok() && Math.abs(Long.parseLong(u2.cti("posun", "0"))) < 60, o.kod + " posun=" + u2.cti("posun", "?"));
            String log = new String(Files.readAllBytes(logCesta), StandardCharsets.UTF_8);
            kontrola("C14 log serveru: ZAPLATIL? Sega a PS1 s modelem telefonu, ODEMČENO", log.contains("💳 ZAPLATIL?") && log.contains("SEGA 75 Kč")
                    && log.contains("PS1 125 Kč") && log.contains("Samsung SM-G950F") && log.contains("💳 ODEMČENO"), log.length());
        } finally {
            try { p.getOutputStream().write("konec\n".getBytes()); p.getOutputStream().flush(); p.waitFor(); } catch (Exception e) { p.destroy(); }
        }
    }
}
