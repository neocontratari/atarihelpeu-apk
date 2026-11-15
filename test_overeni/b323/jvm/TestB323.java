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
 * B323: 🔒 ZAMKNOUT (TEST) ze serveru N&P - skutecne tridy appky bez Androidu.
 *  A) NapPostupJadro.provedZamek: Sega i PS1 / jen PS1, i otevrene zadarmo (radky, hodiny, B321),
 *     kazdy zamek jen jednou, starsi cislo nic, po zamku slouceni zalohy ze serveru.
 *  C) proti SKUTECNEMU serveru N&P v11.2 (NapTest.exe, mono): spravce zamkne -> stav.zamek ->
 *     appka zamkne (jako MainActivity.napSlucServer) -> postup se zamek=N -> server potvrdi;
 *     postup z doby pred zamkem server nebere; jen PS1.
 * Pouziti: java -cp out eu.atarihelp.emu10.TestB323 [<NapTest.exe> <pracovni slozka>]
 */
public class TestB323 {
    static int ok = 0, chyb = 0;
    static String SERVER;
    static final int PORT = 18423;
    static final long H = 3600_000L;

    static void kontrola(String nazev, boolean podminka, Object detail) {
        if (podminka) { ok++; System.out.println("OK     " + nazev); }
        else { chyb++; System.out.println("CHYBA  " + nazev + "   -> " + detail); }
    }

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

    public static void main(String[] a) throws Exception {
        zamek();
        if (a.length >= 2) server(a[0], Paths.get(a[1]));
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }

    // ------------------------------------------------------------------ A) provedZamek
    static void zamek() {
        System.out.println("== A NapPostupJadro.provedZamek (🔒 ZAMKNOUT TEST ze serveru)");
        MapaPamet m = new MapaPamet();
        NapPostupJadro j = new NapPostupJadro(m);
        for (int i = 0; i < 300; i++) j.pripocitejRadky(5);              // 1500 radku -> Sega zadarmo
        j.pripocitejSegu(120 * H);                                        // 120 h -> PS1 zadarmo
        j.nastavKoupeno(true, true);
        kontrola("A1 hrac ma vse: Sega i PS1 zadarmo i zaplacene, zadny zamek", j.segaOdemcena() && j.ps1Odemcena() && j.zamekCislo() == 0, j.stav());
        boolean p1 = j.provedZamek(1, true, true);
        kontrola("A2 zamek c. 1 Sega i PS1: vse zamcene, 0 radku, 0 min, nic koupene, zamek 1",
                p1 && !j.segaOdemcena() && !j.ps1Odemcena() && j.napsanoRadku() == 0 && j.segaOdehranoMs() == 0
                        && !j.segaKoupena() && !j.ps1Koupena() && j.zamekCislo() == 1, j.stav());
        for (int i = 0; i < 10; i++) j.pripocitejRadky(5);
        boolean p2 = j.provedZamek(1, true, true);
        kontrola("A3 stejny zamek podruhe (dalsi stav ze serveru) = nic, postup po zamku zustane", !p2 && j.napsanoRadku() == 50, j.stav());
        boolean s = j.slucPostup(0, 0);
        kontrola("A4 slouceni zalohy ze serveru (po zamku 0 / 0) nic neotevre", !s && !j.segaOdemcena() && j.napsanoRadku() == 50, j.stav());

        MapaPamet m2 = new MapaPamet();
        m2.m.put(NapPostupJadro.K_SEGA, true); m2.m.put(NapPostupJadro.K_RADKY, 60); m2.m.put(NapPostupJadro.K_PS1, true); m2.m.put(NapPostupJadro.K_MINUTY, 31 * 60_000L);
        NapPostupJadro b = new NapPostupJadro(m2);
        boolean p3 = b.provedZamek(3, true, true);
        kontrola("A5 hrac z B321 (otevreno po 50 radcich / 30 min - zadarmo): zamek ho zamkne", p3 && !b.segaOdemcena() && !b.ps1Odemcena() && b.zamekCislo() == 3, b.stav());

        MapaPamet m3 = new MapaPamet();
        NapPostupJadro c = new NapPostupJadro(m3);
        for (int i = 0; i < 300; i++) c.pripocitejRadky(5);
        c.pripocitejSegu(121 * H);
        c.nastavKoupeno(false, true);
        boolean p4 = c.provedZamek(2, false, true);
        kontrola("A6 zamek jen PS1: Sega (1500 radku) zustane, PS1 zamcena, hodiny Segy 0, PS1 nekoupena",
                p4 && c.segaOdemcena() && !c.ps1Odemcena() && c.napsanoRadku() == 1500 && c.segaOdehranoMs() == 0 && !c.ps1Koupena() && c.zamekCislo() == 2, c.stav());
        boolean p5 = c.provedZamek(1, true, true);
        kontrola("A7 starsi zamek (c. 1 po c. 2) = nic", !p5 && c.segaOdemcena() && c.zamekCislo() == 2, c.stav());
        boolean p6 = c.provedZamek(0, true, true);
        kontrola("A8 cislo 0 (server bez zamku) = nic", !p6 && c.segaOdemcena(), c.stav());
        c.pripocitejSegu(120 * H);
        kontrola("A9 po zamku jde PS1 zase ziskat (120 h)", c.ps1Odemcena(), c.stav());
    }

    // ------------------------------------------------------------------ C) proti serveru v11.2
    static void server(String exe, Path prac) throws Exception {
        System.out.println("== C zamek proti skutecnemu serveru N&P v11.2 (jako MainActivity.napSlucServer / napPostupOdesli)");
        Path slozka = prac.resolve("server_jvm_b323");
        if (Files.exists(slozka)) try (var w = Files.walk(slozka)) { w.sorted(Comparator.reverseOrder()).forEach(x -> x.toFile().delete()); }
        for (String x : new String[]{"Atari", "Sega", "PS1"}) Files.createDirectories(slozka.resolve("Arcade").resolve(x));
        Path logCesta = prac.resolve("log_server_jvm_b323.txt");
        Process p = new ProcessBuilder("mono", exe, "--slozka", slozka.toString(), "--port", String.valueOf(PORT), "--test")
                .redirectErrorStream(true).redirectOutput(logCesta.toFile()).start();
        SERVER = "http://127.0.0.1:" + PORT;
        for (int i = 0; i < 200; i++) { try (Socket so = new Socket("127.0.0.1", PORT)) { break; } catch (IOException e) { Thread.sleep(50); } }
        Thread.sleep(300);
        try {
            String kod = (String) NapJson.objekt(test("/test/novy-kod?poznamka=JVM-B323")).get("kod");
            Uloz u = new Uloz();
            NapServerKlient k = new NapServerKlient(u, "b323jvm00000001", "Pixel 8", "14", "EMU10-B323-HRY-ZAMEK");
            k.nastavServer(SERVER);
            NapServerKlient.Odpoved o = k.sparuj(kod, "");
            kontrola("C1 sparovano", o.kod == 200 && k.sparovano(), o.kod + " " + o.text());
            MapaPamet mp = new MapaPamet();
            NapPostupJadro j = new NapPostupJadro(mp);
            for (int i = 0; i < 320; i++) j.pripocitejRadky(5);   // 1600 radku -> Sega zadarmo
            o = k.post("/api/v1/postup?radku=" + j.napsanoRadku() + "&sega_ms=" + (2 * H) + "&sega=1&ps1=0&zamek=" + j.zamekCislo());
            kontrola("C2 zaloha postupu se zamek=0 (zadny zamek) se bere: 1600 radku", o.ok() && NapJson.cislo(o.json, "radku", 0) == 1600
                    && NapJson.cislo(NapJson.mapa(o.json, "zamek"), "cislo", -1) == 0, o.text());

            String r = test("/test/zamkni?kod=" + enc(kod) + "&co=vse");
            o = k.get("/api/v1/stav");
            Map<String, Object> z = NapJson.mapa(o.json, "zamek");
            kontrola("C3 spravce 🔒 ZAMKNOUT Sega i PS1 -> stav.zamek {cislo 1, sega, ps1}", r.contains("\"ok\":true") && z != null
                    && NapJson.cislo(z, "cislo", 0) == 1 && NapJson.ano(z, "sega") && NapJson.ano(z, "ps1"), o.text());

            // telefon jeste nevi (hraje bez site) a posle stary postup se zamek=0 -> server ho nebere
            o = k.post("/api/v1/postup?radku=1600&sega_ms=" + (2 * H) + "&sega=1&ps1=0&zamek=0");
            kontrola("C4 postup z doby pred zamkem (zamek=0) server nebere a v odpovedi posle zamek", o.ok() && NapJson.cislo(o.json, "radku", -1) == 0
                    && NapJson.cislo(NapJson.mapa(o.json, "zamek"), "cislo", 0) == 1, o.text());

            // MainActivity.napSlucServer: nejdriv zamek, pak odemceno a postup ze serveru
            boolean provedeno = j.provedZamek((int) NapJson.cislo(z, "cislo", 0), NapJson.ano(z, "sega"), NapJson.ano(z, "ps1"));
            j.nastavKoupeno(NapJson.ano(NapJson.mapa(o.json, "odemceno"), "sega"), NapJson.ano(NapJson.mapa(o.json, "odemceno"), "ps1"));
            j.slucPostup(NapJson.cislo(o.json, "radku", 0), NapJson.cislo(o.json, "sega_ms", 0));
            kontrola("C5 appka zamek provedla: Sega i PS1 zamcene, 0 radku", provedeno && !j.segaOdemcena() && !j.ps1Odemcena() && j.napsanoRadku() == 0, j.stav());
            o = k.post("/api/v1/postup?radku=" + j.napsanoRadku() + "&sega_ms=" + j.segaOdehranoMs() + "&sega=0&ps1=0&zamek=" + j.zamekCislo());
            String kj = test("/test/kod?kod=" + enc(kod));
            kontrola("C6 potvrzeni (postup se zamek=1) -> server: zamek potvrzen", o.ok() && kj.contains("\"zamek\":1") && kj.contains("\"zamek_potvrzeno\":1"), kj);
            for (int i = 0; i < 4; i++) j.pripocitejRadky(5);
            o = k.post("/api/v1/postup?radku=" + j.napsanoRadku() + "&sega_ms=0&sega=0&ps1=0&zamek=" + j.zamekCislo());
            kontrola("C7 po zamku se postup zase zalohuje (20 radku)", o.ok() && NapJson.cislo(o.json, "radku", 0) == 20, o.text());

            // jen PS1: spravce odemkne Segu (zaplaceno), hrac ma 3 h Segy, pak 🔒 jen PS1
            test("/test/odemkni?kod=" + enc(kod) + "&co=sega");
            test("/test/odemkni?kod=" + enc(kod) + "&co=ps1");
            o = k.get("/api/v1/stav");
            j.nastavKoupeno(NapJson.ano(NapJson.mapa(o.json, "odemceno"), "sega"), NapJson.ano(NapJson.mapa(o.json, "odemceno"), "ps1"));
            j.pripocitejSegu(3 * H);
            r = test("/test/zamkni?kod=" + enc(kod) + "&co=ps1");
            o = k.get("/api/v1/stav");
            z = NapJson.mapa(o.json, "zamek");
            boolean p2 = j.provedZamek((int) NapJson.cislo(z, "cislo", 0), NapJson.ano(z, "sega"), NapJson.ano(z, "ps1"));
            j.nastavKoupeno(NapJson.ano(NapJson.mapa(o.json, "odemceno"), "sega"), NapJson.ano(NapJson.mapa(o.json, "odemceno"), "ps1"));
            kontrola("C8 🔒 jen PS1: zamek c. 2 -> Sega zustane (zaplacena), PS1 zamcena, hodiny Segy 0",
                    p2 && NapJson.cislo(z, "cislo", 0) == 2 && !NapJson.ano(z, "sega") && j.segaOdemcena() && j.segaKoupena()
                            && !j.ps1Odemcena() && j.segaOdehranoMs() == 0 && j.zamekCislo() == 2, j.stav() + " | " + o.text());
            String log = new String(Files.readAllBytes(logCesta), StandardCharsets.UTF_8);
            kontrola("C9 log serveru: 🔒 ZAMKNOUT a 🔒 ZAMČENO (telefon Pixel 8 zamek c. 1 provedl)", log.contains("🔒 ZAMKNOUT")
                    && log.contains("🔒 ZAMČENO") && log.contains("Pixel 8 zámek č. 1 provedl"), log.length());
        } finally {
            try { p.getOutputStream().write("konec\n".getBytes()); p.getOutputStream().flush(); p.waitFor(); } catch (Exception e) { p.destroy(); }
        }
    }
}
