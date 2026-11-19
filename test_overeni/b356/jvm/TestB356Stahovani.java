package eu.atarihelp.emu10;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/**
 * B356: NapStahovaniHry na mistnim HTTP serveru (com.sun.net.httpserver), ktery se chova jako server N&P
 * (Accept-Ranges: bytes, ETag, If-Range, 206 + Content-Range) a umi "zamrznout" uprostred souboru jako
 * pri FF8 (Rene, 19. 11.). Plus id souboru na serveru (vzorec serveru) na skutecnych id z logu a rozhodnuti
 * "prekonane stahovani" na casove ose FF8.
 */
public class TestB356Stahovani {

    static int ok = 0, chyb = 0;
    static void over(boolean podminka, String co) {
        if (podminka) { ok++; System.out.println("OK    " + co); }
        else { chyb++; System.out.println("CHYBA " + co); }
    }

    // ------------------------------------------------------------ server
    static final int VELIKOST = 3 * 1024 * 1024;
    static volatile byte[] data = vzor(VELIKOST, 7);
    static volatile String etag = "\"v1\"";
    static volatile String rezim = "normal";      // normal | zamrzni | utni | zmena | bezRange | vzdyZamrzni
    static final AtomicInteger dotazu = new AtomicInteger();
    static final List<String> rozsahy = new ArrayList<>();

    static byte[] vzor(int n, int semeno) {
        byte[] b = new byte[n];
        int x = semeno;
        for (int i = 0; i < n; i++) { x = x * 1103515245 + 12345; b[i] = (byte) (x >>> 16); }
        return b;
    }

    static void obsluz(HttpExchange ex) throws IOException {
        final int cislo = dotazu.incrementAndGet();
        final byte[] d = data;
        final String range = ex.getRequestHeaders().getFirst("Range");
        final String ifRange = ex.getRequestHeaders().getFirst("If-Range");
        synchronized (rozsahy) { rozsahy.add(range == null ? "-" : range + (ifRange == null ? "" : " if " + ifRange)); }
        long od = 0;
        boolean cast = false;
        if (range != null && !"bezRange".equals(rezim) && (ifRange == null || ifRange.equals(etag))) {
            od = Long.parseLong(range.substring(6, range.indexOf('-')));
            cast = true;
        }
        if (!"bezRange".equals(rezim)) {
            ex.getResponseHeaders().add("Accept-Ranges", "bytes");
            ex.getResponseHeaders().add("ETag", etag);
        }
        final long n = d.length - od;
        if (cast) ex.getResponseHeaders().add("Content-Range", "bytes " + od + "-" + (d.length - 1) + "/" + d.length);
        ex.sendResponseHeaders(cast ? 206 : 200, n);
        OutputStream o = ex.getResponseBody();
        try {
            final boolean prvni = cislo == 1;
            final boolean zamrzni = ("zamrzni".equals(rezim) || "zmena".equals(rezim) || "bezRange".equals(rezim)) && prvni
                    || "vzdyZamrzni".equals(rezim);
            final boolean utni = "utni".equals(rezim) && prvni;
            if (zamrzni || utni) {
                int kolik = (int) Math.min(n, "vzdyZamrzni".equals(rezim) ? 100 * 1024 : 1024 * 1024);
                o.write(d, (int) od, kolik);
                o.flush();
                if (utni) throw new IOException("server spojeni utnul");
                if ("zmena".equals(rezim)) { data = vzor(VELIKOST + 4096, 99); etag = "\"v2\""; }   // soubor se na serveru zmenil
                try { Thread.sleep(8000); } catch (InterruptedException ignored) {}   // zamrzne (klient ma read timeout 1 s)
                return;
            }
            o.write(d, (int) od, (int) n);
        } finally {
            try { o.close(); } catch (Throwable ignored) {}
        }
    }

    static String adresa;

    static HttpURLConnection otevri(long od, String shoda, int readTimeout) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(adresa).openConnection();
        c.setConnectTimeout(3000);
        c.setReadTimeout(readTimeout);
        if (od > 0) {
            c.setRequestProperty("Range", "bytes=" + od + "-");
            if (shoda != null) c.setRequestProperty("If-Range", shoda);
        }
        c.connect();
        return c;
    }

    static final class Vystup {
        NapStahovaniHry.Vysledek v; Throwable chyba; long ms; List<String> log = new ArrayList<>(); byte[] soubor;
    }

    static Vystup stahni(String r, int readTimeout, int maxNavazani, AtomicBoolean zrus,
                         AtomicReference<HttpURLConnection> akt, File part) throws Exception {
        rezim = r;
        dotazu.set(0);
        synchronized (rozsahy) { rozsahy.clear(); }
        data = vzor(VELIKOST, 7);
        etag = "\"v1\"";
        final Vystup vy = new Vystup();
        final long t0 = System.currentTimeMillis();
        HttpURLConnection prvni = otevri(0, null, readTimeout);
        akt.set(prvni);
        try {
            int kod = prvni.getResponseCode();
            if (kod != 200) throw new IOException("HTTP " + kod);
            vy.v = NapStahovaniHry.stahni(prvni, part, NapStahovaniHry.delka(prvni),
                    (od, shoda) -> otevri(od, shoda, readTimeout),
                    () -> zrus.get(),
                    null,
                    s -> { synchronized (vy.log) { vy.log.add(s); } },
                    akt, maxNavazani, 1800L * 1024 * 1024, "hra.zip");
        } catch (Throwable t) {
            vy.chyba = t;
        } finally {
            prvni.disconnect();
        }
        vy.ms = System.currentTimeMillis() - t0;
        vy.soubor = part.exists() ? Files.readAllBytes(part.toPath()) : null;
        return vy;
    }

    public static void main(String[] a) throws Exception {
        HttpServer srv = HttpServer.create(new InetSocketAddress("127.0.0.1", 0), 0);
        srv.createContext("/api/v1/hra", ex -> { try { obsluz(ex); } catch (IOException e) { ex.close(); } });
        srv.setExecutor(Executors.newCachedThreadPool(r -> { Thread t = new Thread(r); t.setDaemon(true); return t; }));
        srv.start();
        adresa = "http://127.0.0.1:" + srv.getAddress().getPort() + "/api/v1/hra?id=ps1-0123456789abcdef";
        File dir = Files.createTempDirectory("b356").toFile();
        AtomicReference<HttpURLConnection> akt = new AtomicReference<>();

        System.out.println("== 1) id souboru na serveru N&P (vzorec serveru) na skutecnych id z logu 19. 11.");
        over("ps1-016db3d9f2d626d3".equals(NapStahovaniHry.idNaServeru("ps1", "Final Fantasy VIII (USA) (Disc 1).bin")), "FF8 .bin = ps1-016db3d9f2d626d3 (log 21:23:43)");
        over("ps1-e64716b450ceca9d".equals(NapStahovaniHry.idNaServeru("ps1", "Final Fantasy VIII (USA) (Disc 1).cue")), "FF8 .cue = ps1-e64716b450ceca9d (log 21:23:24)");
        over("ps1-568ee09c19141a3b".equals(NapStahovaniHry.idNaServeru("ps1", "Final Fantasy VIII (USA) (Disc 1).zip")), "FF8 .zip = ps1-568ee09c19141a3b (log 21:01:58)");
        over("ps1-016db3d9f2d626d3".equals(NapStahovaniHry.idNaServeru("ps1", "FINAL FANTASY VIII (USA) (DISC 1).BIN")), "velka pismena jako server (ToLowerInvariant)");
        String u = NapStahovaniHry.adresaNaServeru("http://192.0.2.10:8080/api/v1/hra?id=ps1-e64716b450ceca9d", "ps1",
                "Final Fantasy VIII (USA) (Disc 1).bin");
        over("http://192.0.2.10:8080/api/v1/hra?id=ps1-016db3d9f2d626d3".equals(u), ".bin vedle .cue -> " + u + " (drive .../api/v1/<jmeno> = 404)");
        over("Hra (Track 2).bin".equals(NapStahovaniHry.jmenoZCue(" disk\\Hra (Track 2).bin ")) && "a.bin".equals(NapStahovaniHry.jmenoZCue("x/y/a.bin")),
                "jmeno z radku FILE bez cesty (obe lomitka)");
        over(NapStahovaniHry.adresaNaServeru("https://example.com/hry/a.cue", "ps1", "a.bin") == null, "cizi server (odkaz z webu) -> null = relativne jako drive");

        System.out.println("== 2) prekonane stahovani - casova osa FF8 21:01-21:03");
        // stahovani #1: gen 5, cislo 1 -> 21:02:32 odchod do seznamu her (gen 6, priznak false) -> #2: gen 7, cislo 2
        over(!NapStahovaniHry.jeAktualni(5, 7, 1, 2, true), "21:03:14 timeout stahovani #1 po startu #2 -> prekonane: nehlasi FAIL, nemaze slozku ani .part #2");
        over(!NapStahovaniHry.jeAktualni(5, 6, 1, 1, false), "zrusene (odchod ze stranky) a zadne dalsi -> prekonane (uklid jen svych souboru)");
        over(NapStahovaniHry.jeAktualni(7, 7, 2, 2, true), "#2 aktualni -> chybu hlasi a uklizi");

        System.out.println("== 3) normalni stazeni 3 MB");
        Vystup v = stahni("normal", 5000, 5, new AtomicBoolean(), akt, new File(dir, "a.d1.part"));
        over(v.chyba == null && v.v.bajtu == VELIKOST && v.v.navazani == 0 && Arrays.equals(v.soubor, data), "cele, bez navazani, obsah sedi (" + (v.chyba == null ? v.v.bajtu : v.chyba) + ")");

        System.out.println("== 4) spojeni zamrzne po 1 MB (FF8 21:02:05) -> navaze od 1 MB");
        v = stahni("zamrzni", 1000, 5, new AtomicBoolean(), akt, new File(dir, "b.d1.part"));
        over(v.chyba == null && v.v.navazani == 1 && v.v.bajtu == VELIKOST && Arrays.equals(v.soubor, data),
                "hotovo po 1 navazani za " + v.ms + " ms, obsah sedi" + (v.chyba != null ? " CHYBA " + v.chyba : ""));
        over(rozsahy.size() == 2 && rozsahy.get(1).equals("bytes=1048576- if \"v1\""), "druhy dotaz: Range bytes=1048576- a If-Range = ETag (" + rozsahy + ")");
        over(!v.log.isEmpty() && v.log.get(0).contains("spojeni zamrzlo") && v.log.get(0).contains("navazuji od 1.0MB"), "log: " + (v.log.isEmpty() ? "-" : v.log.get(0)));

        System.out.println("== 5) server spojeni utne po 1 MB -> navaze");
        v = stahni("utni", 5000, 5, new AtomicBoolean(), akt, new File(dir, "c.d1.part"));
        over(v.chyba == null && v.v.navazani == 1 && Arrays.equals(v.soubor, data), "hotovo po utnuti" + (v.chyba != null ? " CHYBA " + v.chyba : "") + " log " + v.log);

        System.out.println("== 6) soubor se na serveru mezitim zmenil (If-Range nesedi) -> 200 a znovu od zacatku");
        v = stahni("zmena", 1000, 5, new AtomicBoolean(), akt, new File(dir, "d.d1.part"));
        over(v.chyba == null && v.v.odZacatku == 1 && v.v.bajtu == VELIKOST + 4096 && Arrays.equals(v.soubor, data),
                "novy soubor cely, ne slepenec stareho a noveho" + (v.chyba != null ? " CHYBA " + v.chyba : ""));

        System.out.println("== 7) server bez Range (cizi web) -> zamrznuti = chyba jako drive");
        v = stahni("bezRange", 1000, 5, new AtomicBoolean(), akt, new File(dir, "e.d1.part"));
        over(v.chyba instanceof SocketTimeoutException && dotazu.get() == 1, "SocketTimeoutException, zadny dalsi dotaz (" + v.chyba + ")");

        System.out.println("== 8) zamrza porad -> po maxNavazani (2) konec");
        v = stahni("vzdyZamrzni", 700, 2, new AtomicBoolean(), akt, new File(dir, "f.d1.part"));
        over(v.chyba instanceof SocketTimeoutException && dotazu.get() == 3, "3 dotazy (1 + 2 navazani), pak chyba (" + v.chyba + ", dotazu " + dotazu.get() + ")");

        System.out.println("== 9) zruseni behem zamrznuti (odchod ze stranky PS1) -> Zruseno, zadne navazani, zadna chyba");
        // Pozn.: JDK HttpURLConnection.disconnect() z jineho vlakna ceka, az cteni skonci (MeteredStream je
        // synchronized) - tady tedy konec az po read timeoutu (2 s). Na Androidu (OkHttp) disconnect() zavre
        // socket a cteni konci hned; v JVM to overit nejde (OkHttp tu neni). Dulezite je, ze zrusene stahovani
        // skonci jako Zruseno (nic nehlasi, nemaze cizi soubory) a uz nenavazuje.
        final AtomicBoolean zrus = new AtomicBoolean();
        final AtomicReference<HttpURLConnection> akt2 = new AtomicReference<>();
        Thread rusic = new Thread(() -> {
            try { Thread.sleep(400); } catch (InterruptedException ignored) {}
            zrus.set(true);                                   // jako stopPs1SessionHard: priznak ...
            HttpURLConnection c = akt2.getAndSet(null);       // ... a ps1ZavriStahovani()
            if (c != null) c.disconnect();
        });
        rusic.start();
        v = stahni("zamrzni", 2000, 5, zrus, akt2, new File(dir, "g.d1.part"));
        rusic.join();
        over(v.chyba instanceof NapStahovaniHry.Zruseno && dotazu.get() == 1 && v.ms < 3500,
                "Zruseno za " + v.ms + " ms, dotazu " + dotazu.get() + " (bez navazani): " + v.chyba);

        System.out.println("== 10) dve stahovani te same hry, kazde vlastni .part (FF8: druhe prislo o .part)");
        File p1 = new File(dir, "Final_Fantasy.zip.d1.part"), p2 = new File(dir, "Final_Fantasy.zip.d2.part");
        Files.write(p1.toPath(), new byte[] { 1, 2, 3 });
        v = stahni("normal", 5000, 5, new AtomicBoolean(), akt, p2);
        p1.delete();   // prvni (zrusene) stahovani po chybe smaze SVUJ .part
        over(v.chyba == null && p2.isFile() && p2.length() == VELIKOST, "druhe stahovani ma svuj .part cely i po uklidu prvniho");

        srv.stop(0);
        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
