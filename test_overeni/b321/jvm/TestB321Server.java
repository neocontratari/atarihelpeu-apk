import eu.atarihelp.emu10.NapApkKlic;
import eu.atarihelp.emu10.NapJson;
import eu.atarihelp.emu10.NapServerKlient;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

/**
 * B321: skutecne tridy appky (NapServerKlient, NapApkKlic, NapJson) proti skutecnemu jadru serveru
 * N&P v10 (NapTest.exe = NapServer.cs bez okna, mono, testovaci rezim).
 * Pouziti: java -cp out TestB321Server <NapTest.exe> <APK s podpisem v2/v3> <pracovni slozka>
 */
public class TestB321Server {
    static int ok = 0, chyb = 0;
    static String SERVER;
    static int PORT = 18421;

    static void kontrola(String nazev, boolean podminka, Object detail) {
        if (podminka) { ok++; System.out.println("OK     " + nazev); }
        else { chyb++; System.out.println("CHYBA  " + nazev + "   -> " + detail); }
    }

    static final class Pamet implements NapServerKlient.Uloziste {
        final Map<String, String> m = new HashMap<>();
        public synchronized String cti(String k, String v) { String x = m.get(k); return x == null ? v : x; }
        public synchronized void zapis(String k, String v) { if (v == null) m.remove(k); else m.put(k, v); }
    }

    static final class Odp { int kod; byte[] data; Map<String, List<String>> hl; String text() { return new String(data, StandardCharsets.UTF_8); } }

    static Odp http(String metoda, String cesta, String ip, String telo) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(SERVER + cesta).openConnection();
        c.setRequestMethod(metoda);
        if (ip != null) c.setRequestProperty("X-Nap-Test-Ip", ip);
        if (telo != null) {
            c.setDoOutput(true);
            c.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            c.getOutputStream().write(telo.getBytes(StandardCharsets.UTF_8));
        }
        Odp o = new Odp();
        o.kod = c.getResponseCode();
        InputStream in = o.kod >= 400 ? c.getErrorStream() : c.getInputStream();
        o.data = in == null ? new byte[0] : in.readAllBytes();
        o.hl = c.getHeaderFields();
        c.disconnect();
        return o;
    }

    static String sha(byte[] b) throws Exception {
        StringBuilder s = new StringBuilder();
        for (byte x : MessageDigest.getInstance("SHA-256").digest(b)) s.append(String.format("%02x", x & 0xFF));
        return s.toString();
    }

    @SuppressWarnings("unchecked")
    public static void main(String[] a) throws Exception {
        String exe = a[0];
        Path apk = Paths.get(a[1]), prac = Paths.get(a[2]);
        Path slozka = prac.resolve("server_jvm");
        if (Files.exists(slozka)) try (var w = Files.walk(slozka)) { w.sorted(Comparator.reverseOrder()).forEach(p -> p.toFile().delete()); }
        for (String s : new String[]{"Atari", "Sega", "PS1"}) Files.createDirectories(slozka.resolve("Arcade").resolve(s));
        Random r = new Random(321);
        byte[] moon = new byte[131088]; r.nextBytes(moon);
        byte[] crash = new byte[3000000]; r.nextBytes(crash); crash[0] = 'P'; crash[1] = 'K'; crash[2] = 3; crash[3] = 4;
        Files.write(slozka.resolve("Arcade/Atari/Moon Cresta v1.7 (PAL).car"), moon);
        Files.write(slozka.resolve("Arcade/PS1/Crash Bandicoot (Europe).zip"), crash);
        Files.write(slozka.resolve("Arcade/Atari/Moon Cresta v1.7 (PAL).jpg"), new byte[]{(byte) 0xFF, (byte) 0xD8, (byte) 0xFF, 1, 2, 3});
        Files.copy(apk, slozka.resolve("AtariHelp_Vision_v1.apk"));
        Path logCesta = prac.resolve("log_server_jvm.txt");
        Process p = new ProcessBuilder("mono", exe, "--slozka", slozka.toString(), "--port", String.valueOf(PORT), "--test")
                .redirectErrorStream(true).redirectOutput(logCesta.toFile()).start();
        SERVER = "http://127.0.0.1:" + PORT;
        for (int i = 0; i < 200; i++) { try (Socket s = new Socket("127.0.0.1", PORT)) { break; } catch (IOException e) { Thread.sleep(50); } }
        Thread.sleep(500);
        try {
            testy(slozka, moon, crash, logCesta);
        } finally {
            try { p.getOutputStream().write("konec\n".getBytes()); p.getOutputStream().flush(); } catch (IOException ignored) { }
            p.waitFor(20, java.util.concurrent.TimeUnit.SECONDS);
            p.destroy();
        }
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }

    @SuppressWarnings("unchecked")
    static void testy(Path slozka, byte[] moon, byte[] crash, Path logCesta) throws Exception {
        System.out.println("== J1 NapJson");
        Map<String, Object> j = NapJson.objekt("{\"a\":\"\\u017dlu\\u0165ou\\u010dk\\u00fd \\\"k\\u016f\\u0148\\\"\\n\",\"b\":[1,2.5,-3e2,true,null,{\"c\":[]}],\"d\":{}}");
        kontrola("J1 JSON: unicode, uvozovky, pole, cisla, vnoreni", j != null && "Žluťoučký \"kůň\"\n".equals(j.get("a"))
                && ((List<Object>) j.get("b")).size() == 6 && ((List<Object>) j.get("b")).get(2).equals(-300.0), j);
        kontrola("J1 JSON: neplatny text = null", NapJson.cti("{\"a\":1,}") == null && NapJson.cti("[1 2]") == null && NapJson.cti("{") == null, "");
        kontrola("J1 JSON: uvoz() a zpet", "a\"b\\c\nž\u2028".equals(NapJson.cti(NapJson.uvoz("a\"b\\c\nž\u2028"))), NapJson.uvoz("a\"b\\c\nž\u2028"));
        kontrola("J1 normalizujServer", NapServerKlient.normalizujServer(" 46.33.114.139:8080/ ").equals("http://46.33.114.139:8080")
                && NapServerKlient.normalizujServer("https://hry.atarihelp.eu/aktivace").equals("https://hry.atarihelp.eu")
                && NapServerKlient.normalizujServer("").equals(""), NapServerKlient.normalizujServer(" 46.33.114.139:8080/ "));

        System.out.println("== J2 kod na webu, APK s klicem, trida NapApkKlic");
        String k1 = (String) NapJson.objekt(http("GET", "/test/novy-kod?poznamka=JVM", null, null).text()).get("kod");
        Odp o = http("POST", "/aktivace", "88.1.1.1", "kod=" + URLEncoder.encode(k1, "UTF-8"));
        kontrola("J2 kod prijat na webu", o.kod == 200, o.kod);
        o = http("GET", "/apk?kod=" + k1, "88.1.1.1", null);
        Path stazene = slozka.resolveSibling("jvm_stazene.apk");
        Files.write(stazene, o.data);
        Map<String, String> klic = NapApkKlic.hodnoty(stazene.toString());
        String taj = klic.get("tajemstvi");
        kontrola("J2 NapApkKlic precte klic ze stazeneho APK", taj != null && taj.matches("[0-9a-f]{32}")
                && sha(Files.readAllBytes(slozka.resolve("AtariHelp_Vision_v1.apk"))).equals(klic.get("apk_sha256")), klic);
        kontrola("J2 NapApkKlic: APK bez klice = prazdne", NapApkKlic.hodnoty(slozka.resolve("AtariHelp_Vision_v1.apk").toString()).isEmpty()
                && NapApkKlic.precti(slozka.resolve("neni.apk").toString()) == null, "");

        System.out.println("== J3 parovani klientem appky");
        List<String> log = Collections.synchronizedList(new ArrayList<>());
        Pamet pam = new Pamet();
        pam.zapis("server", SERVER);
        NapServerKlient kl = new NapServerKlient(pam, "a1b2c3d4e5f60718", "Pixel 8", "14", "EMU10-B321");
        kl.nastavLog(log::add);
        kontrola("J3 pred parovanim: nesparovano", !kl.sparovano(), "");
        NapServerKlient.Odpoved od = kl.get("/api/v1/stav");
        kontrola("J3 nesparovany telefon: server 401 nesparovano", od.kod == 401 && "nesparovano".equals(od.duvod()), od.kod + " " + od.text());
        od = kl.sparuj("", taj);
        kontrola("J3 sparuj klicem z APK = OK, token ulozen, kod zamaskovany", od.ok() && kl.sparovano() && pam.cti("token", "").length() == 64
                && kl.kodMaska().equals(k1.substring(0, 6) + "**-****"), od.text());
        kontrola("J3 log appky bez tokenu", log.stream().noneMatch(l -> l.contains(pam.cti("token", "xxxxxxxx"))), log);

        System.out.println("== J4 podepsane dotazy");
        od = kl.get("/api/v1/stav");
        kontrola("J4 stav: ok, telefon, verze APK", od.ok() && "Pixel 8".equals(NapJson.text(od.json, "telefon"))
                && NapJson.text(od.json, "apk_sha256").length() == 64, od.text());
        od = kl.get("/api/v1/hry");
        List<Object> hry = (List<Object>) od.json.get("hry");
        kontrola("J4 seznam her: 2 hry", od.ok() && hry.size() == 2, od.text());
        String idMoon = null, idCrash = null;
        for (Object h : hry) {
            Map<String, Object> m = (Map<String, Object>) h;
            if ("atari".equals(m.get("system"))) idMoon = (String) m.get("id"); else idCrash = (String) m.get("id");
        }
        HttpURLConnection c = kl.otevri("/api/v1/hra?id=" + idMoon, "GET");
        byte[] stazeno = c.getInputStream().readAllBytes();
        kontrola("J4 stazeni hry podepsanym spojenim", c.getResponseCode() == 200 && Arrays.equals(stazeno, moon), c.getResponseCode());
        c = kl.otevri("/api/v1/hra?id=" + idCrash, "GET");
        c.setRequestProperty("Range", "bytes=1000000-");
        byte[] kus = c.getInputStream().readAllBytes();
        kontrola("J4 navazani stahovani (Range) podepsanym spojenim", c.getResponseCode() == 206 && Arrays.equals(kus, Arrays.copyOfRange(crash, 1000000, crash.length)), c.getResponseCode());
        c = (HttpURLConnection) new URL(SERVER + "/api/v1/obal?id=" + idMoon).openConnection();
        kl.podepis(c);
        kontrola("J4 podepis() do ciziho spojeni (jako configureGameHttpConnection)", c.getResponseCode() == 200 && "image/jpeg".equals(c.getContentType()), c.getResponseCode());
        kontrola("J4 jeNasUrl", kl.jeNasUrl(SERVER + "/api/v1/hra?id=x") && !kl.jeNasUrl("https://atarihelp.eu/x") && !kl.jeNasUrl(SERVER + "evil.cz/"), "");

        System.out.println("== J5 spatne hodiny v telefonu");
        pam.zapis("posun", "2000");
        od = kl.get("/api/v1/stav");
        kontrola("J5 hodiny o 2000 s mimo: appka se srovna a dotaz projde", od.ok() && Math.abs(Long.parseLong(pam.cti("posun", "999"))) < 5, od.kod + " posun=" + pam.cti("posun", ""));
        kontrola("J5 log: hodiny srovnany", log.stream().anyMatch(l -> l.contains("HODINY")), log);

        System.out.println("== J6 preposlana appka");
        Pamet pam2 = new Pamet();
        pam2.zapis("server", SERVER);
        NapServerKlient kl2 = new NapServerKlient(pam2, "f0e1d2c3b4a59687", "Galaxy A54", "15", "EMU10-B321");
        od = kl2.sparuj("", taj);
        kontrola("J6 stejny klic na jinem telefonu = kod_pouzit, nesparovano", od.kod == 403 && "kod_pouzit".equals(od.duvod()) && !kl2.sparovano()
                && od.zprava().contains("kopie aplikace"), od.text());
        od = kl2.sparuj("NAP-ZZZZ-ZZZZ", "");
        kontrola("J6 spatny kod = spatny_kod", od.kod == 403 && "spatny_kod".equals(od.duvod()), od.text());

        System.out.println("== J7 blokace a uvolneni");
        http("GET", "/test/zablokuj?kod=" + k1, null, null);
        od = kl.get("/api/v1/stav");
        kontrola("J7 zablokovany: 403 a appka si to pamatuje", od.kod == 403 && kl.zablokovano() && kl.sparovano(), od.kod);
        http("GET", "/test/odblokuj?kod=" + k1, null, null);
        od = kl.get("/api/v1/stav");
        kontrola("J7 odblokovany: zase funguje", od.ok() && !kl.zablokovano(), od.kod);
        http("GET", "/test/uvolni?kod=" + k1, null, null);
        od = kl.get("/api/v1/stav");
        kontrola("J7 uvolneny kod: 401 a appka token zapomene", od.kod == 401 && !kl.sparovano() && pam.cti("token", "").isEmpty(), od.kod);
        od = kl.sparuj(k1.toLowerCase(), "");
        kontrola("J7 znovu sparovat zadanym kodem (malymi pismeny)", od.ok() && kl.sparovano(), od.text());

        System.out.println("== J8 server vidi appku");
        String serverLog = new String(Files.readAllBytes(logCesta), StandardCharsets.UTF_8);
        kontrola("J8 log serveru: Appka N&P (User-Agent NapVision)", serverLog.contains("📱 Appka N&P"), "");
        kontrola("J8 log serveru: PREPOSLANA APPKA s modelem", serverLog.contains("PŘEPOSLANÁ APPKA") && serverLog.contains("Galaxy A54"), "");
        Pamet nikam = new Pamet();
        nikam.zapis("server", "http://127.0.0.1:9");
        kontrola("J8 server nedostupny = kod 0, ne pad", new NapServerKlient(nikam, "a1b2c3d4e5f60718", "x", "1", "y").get("/api/v1/stav").kod == 0, "");
    }
}
