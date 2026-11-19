package eu.atarihelp.emu10;

import java.io.EOFException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicReference;

/**
 * B356: STAHOVANI VELKE HRY (PS1) - odolne proti zaseknuti spojeni a proti dvema stahovanim naraz.
 *
 * Rene, log 19. 11. 2026 (Final Fantasy VIII, ZIP 552 MB, server N&P):
 *   21:01:58 prvni stahovani, asi po 6 s server prestal posilat data (spojeni zamrzlo),
 *   21:02:32 Rene odesel do seznamu her (stahovani zruseno - ale vlakno viselo v read() a zruseni
 *            se kontrolovalo az u dalsich dat, ktera neprisla),
 *   21:02:39 hru spustil znovu - druhe stahovani do STEJNE slozky a STEJNEHO souboru .part,
 *   21:03:14 prvni stahovani dostalo po 70 s timeout, bralo ho jako svou chybu a smazalo .part
 *            - ten, do ktereho uz psalo druhe stahovani,
 *   21:03:19 druhe stahovani dobehlo: "nejde prejmenovat .part" -> misto hry BIOS.
 *
 * Tady:
 *  - zamrzle nebo spadle spojeni se NAVAZE od mista, kde skoncilo (Range + If-Range). Server N&P to umi
 *    (Accept-Ranges: bytes, ETag; navazani se mu nepocita do denniho limitu). Server, ktery Range neumi,
 *    = chyba jako drive.
 *  - kdo stahovani rusi (odchod ze stranky PS1, jina hra), zavola disconnect() na spojeni v 'aktivni' -
 *    cteni skonci hned, ne az po timeoutu.
 *  - chyba zruseneho stahovani je Zruseno - volajici pak nic nehlasi a nic nemaze.
 *  - kazde stahovani ma vlastni soubor .part (jmeno dava volajici), dve stahovani se nemuzou potkat.
 * Bez Androidu - testuje se v JVM s mistnim HTTP serverem (test_overeni/b356).
 */
final class NapStahovaniHry {

    private NapStahovaniHry() {}

    /** Otevre spojeni na soubor od bajtu 'od' (0 = cely). 'shoda' = ETag / Last-Modified pro If-Range (muze byt null). */
    interface Spojeni { HttpURLConnection otevri(long od, String shoda) throws IOException; }
    /** Bylo stahovani zruseno (odchod ze stranky, jina hra)? */
    interface Zruseni { boolean zruseno(); }
    interface Prubeh { void prubeh(long mam, long celkem); }
    interface Log { void log(String s); }

    /** Stahovani zruseno - neni to chyba. */
    static final class Zruseno extends IOException { Zruseno(String s) { super(s); } }
    /** Soubor je vetsi, nez appka dovoli - nenavazuje se. */
    static final class PrilisVelky extends IOException { PrilisVelky(String s) { super(s); } }

    static final class Vysledek {
        long bajtu;        // velikost stazeneho souboru
        int navazani;      // kolikrat se navazovalo po zamrznuti / vypadku
        int odZacatku;     // kolikrat server poslal cely soubor znovu (200 misto 206)
    }

    /** ETag (prednost) nebo Last-Modified - podle toho server pozna, ze se soubor mezitim nezmenil. */
    static String shoda(HttpURLConnection c) {
        if (c == null) return null;
        String e = c.getHeaderField("ETag");
        if (e != null && !e.trim().isEmpty() && !e.trim().startsWith("W/")) return e.trim();
        String m = c.getHeaderField("Last-Modified");
        return m != null && !m.trim().isEmpty() ? m.trim() : null;
    }

    /** Server rekl "Accept-Ranges: bytes" a dal ETag / Last-Modified -> jde navazat. */
    static boolean umiNavazat(HttpURLConnection c) {
        if (c == null) return false;
        String r = c.getHeaderField("Accept-Ranges");
        return r != null && r.toLowerCase(Locale.ROOT).contains("bytes") && shoda(c) != null;
    }

    /** Content-Length jako long (-1 = nezname). */
    static long delka(HttpURLConnection c) {
        try {
            String h = c.getHeaderField("Content-Length");
            if (h == null) return -1L;
            return Long.parseLong(h.trim());
        } catch (Throwable t) {
            return -1L;
        }
    }

    /** "bytes 100-199/1000" -> 100, jinak -1. */
    static long zacatekRozsahu(String h) {
        if (h == null) return -1L;
        String s = h.trim().toLowerCase(Locale.ROOT);
        if (!s.startsWith("bytes")) return -1L;
        s = s.substring(5).trim();
        int pomlcka = s.indexOf('-');
        if (pomlcka <= 0) return -1L;
        try { return Long.parseLong(s.substring(0, pomlcka).trim()); } catch (Throwable t) { return -1L; }
    }

    static String mb(long b) {
        if (b < 0) return "?MB";
        return String.format(Locale.US, "%.1fMB", b / 1048576.0);
    }

    private static String popis(IOException e) {
        if (e instanceof SocketTimeoutException) return "spojeni zamrzlo (server nic neposila)";
        String m = e.getMessage();
        return e.getClass().getSimpleName() + (m == null ? "" : ": " + m);
    }

    private static void cekej(long ms, Zruseni zr, String jmeno, long mam) throws Zruseno {
        long konec = System.currentTimeMillis() + ms;
        while (System.currentTimeMillis() < konec) {
            if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
            try { Thread.sleep(50); } catch (InterruptedException ie) { Thread.currentThread().interrupt(); throw new Zruseno("preruseno"); }
        }
    }

    /**
     * Stahne telo odpovedi 'prvni' (pripojene, kod 200) do 'part' (prepise ho).
     * Zamrzne-li spojeni (read timeout nastavil volajici) nebo spadne-li, a server umi Range, navaze od
     * stazeneho mista - nejvys maxNavazani-krat. Konec dat pred ocekavanou delkou = taky vypadek.
     * Aktivni spojeni je vzdy v 'aktivni' (kdo rusi, zavola disconnect()). Prvni spojeni zavira volajici.
     */
    static Vysledek stahni(HttpURLConnection prvni, File part, long ocekavano, Spojeni sp, Zruseni zr, Prubeh pr,
                           Log log, AtomicReference<HttpURLConnection> aktivni, int maxNavazani, long limit,
                           String jmeno) throws IOException {
        final Vysledek v = new Vysledek();
        final AtomicReference<HttpURLConnection> akt = aktivni != null ? aktivni : new AtomicReference<HttpURLConnection>();
        boolean umi = umiNavazat(prvni);
        String shoda = shoda(prvni);
        long celkem = ocekavano;
        long mam = 0L;
        HttpURLConnection c = prvni;
        IOException chybaOtevreni = null;
        FileOutputStream out = new FileOutputStream(part, false);
        try {
            while (true) {
                IOException chyba = chybaOtevreni;
                chybaOtevreni = null;
                if (c != null) {
                    akt.set(c);
                    InputStream in = null;
                    try {
                        if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
                        in = c.getInputStream();
                        byte[] buf = new byte[65536];
                        int n;
                        while ((n = in.read(buf)) >= 0) {
                            if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
                            if (n == 0) continue;
                            if (mam + n > limit) throw new PrilisVelky("prekrocen limit " + mb(limit));
                            out.write(buf, 0, n);
                            mam += n;
                            if (pr != null) pr.prubeh(mam, celkem);
                        }
                    } catch (Zruseno | PrilisVelky e) {
                        throw e;
                    } catch (IOException e) {
                        // zrusene stahovani (disconnect od rusiciho) neni chyba stahovani
                        if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
                        chyba = e;
                    } finally {
                        if (in != null) try { in.close(); } catch (Throwable ignored) {}
                    }
                    if (chyba == null && (celkem < 0 || mam >= celkem)) {
                        if (celkem >= 0 && mam > celkem) throw new IOException("prislo vic dat (" + mam + " B), nez ma soubor (" + celkem + " B)");
                        v.bajtu = mam;
                        return v;
                    }
                }
                // nedokonceno: chyba spojeni, nebo konec dat pred koncem souboru
                final String proc = chyba != null ? popis(chyba)
                        : "spojeni skoncilo po " + mb(mam) + " z " + mb(celkem);
                if (!umi || v.navazani >= maxNavazani) {
                    if (chyba != null) throw chyba;
                    throw new EOFException(proc);
                }
                v.navazani++;
                if (log != null) log.log("B356 STAHOVANI " + jmeno + ": " + proc + " - navazuji od " + mb(mam)
                        + " (pokus " + v.navazani + " z " + maxNavazani + ")");
                if (c != null) {
                    final HttpURLConnection stare = c;
                    try { stare.disconnect(); } catch (Throwable ignored) {}
                    akt.compareAndSet(stare, null);
                    c = null;
                }
                cekej(Math.min(4000L, 1000L * v.navazani), zr, jmeno, mam);
                HttpURLConnection n2 = null;
                try {
                    n2 = sp.otevri(mam, shoda);
                    akt.set(n2);
                    if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
                    final int kod = n2.getResponseCode();
                    if (kod == 206) {
                        final long od = zacatekRozsahu(n2.getHeaderField("Content-Range"));
                        if (od != mam) throw new IOException("navazani: server poslal od " + od + " misto " + mam);
                        c = n2;
                        n2 = null;
                    } else if (kod == 200) {
                        // soubor se na serveru zmenil (If-Range nesedi) nebo server Range nevzal -> znovu od zacatku
                        try { out.close(); } catch (Throwable ignored) {}
                        out = new FileOutputStream(part, false);
                        mam = 0L;
                        final long nova = delka(n2);
                        if (nova >= 0) celkem = nova;
                        final String s2 = shoda(n2);
                        if (s2 != null) shoda = s2;
                        umi = umiNavazat(n2);
                        v.odZacatku++;
                        if (log != null) log.log("B356 STAHOVANI " + jmeno + ": server poslal soubor znovu cely (zmenil se?) - stahuji od zacatku");
                        c = n2;
                        n2 = null;
                    } else {
                        throw new IOException("navazani: HTTP " + kod);
                    }
                } catch (Zruseno z) {
                    throw z;
                } catch (IOException e) {
                    if (zr != null && zr.zruseno()) throw new Zruseno("stahovani zruseno po " + mb(mam) + " (" + jmeno + ")");
                    final String m = e.getMessage();
                    if (m != null && m.startsWith("navazani:")) throw e;   // server odpovedel, ale jinak - nema smysl zkouset dal
                    chybaOtevreni = e;                                     // sit / timeout pri navazani -> dalsi pokus
                } finally {
                    if (n2 != null) {
                        final HttpURLConnection zavrit = n2;
                        try { zavrit.disconnect(); } catch (Throwable ignored) {}
                        akt.compareAndSet(zavrit, null);
                    }
                }
            }
        } finally {
            try { out.close(); } catch (Throwable ignored) {}
            if (c != null && c != prvni) {
                try { c.disconnect(); } catch (Throwable ignored) {}
            }
            if (c != null) akt.compareAndSet(c, null);
            akt.compareAndSet(prvni, null);
        }
    }

    /**
     * B356: smi stahovani, ktere skoncilo chybou, tu chybu hlasit a mazat slozku hry? Jen kdyz je porad AKTUALNI:
     * nebylo zruseno (generace zivota PS1 'gen' stejna, priznak stahovani 'aktivni') a nezacalo novejsi ('cislo').
     * FF8 19. 11.: zrusene stahovani (gen 1 -> zruseni -> nove stahovani) dostalo timeout az po startu noveho,
     * hlasilo PS1_REMOTE_FAIL a smazalo .part noveho. Ted je to "prekonane" stahovani: nic nehlasi, maze jen svoje.
     */
    static boolean jeAktualni(int mojeGen, int genTed, int mojeCislo, int cisloTed, boolean aktivni) {
        return mojeGen == genTed && mojeCislo == cisloTed && aktivni;
    }

    // ------------------------------------------------------------------ server N&P: .cue a jeho .bin

    /**
     * B356: id souboru na serveru N&P - presne jako server (NapServer.Katalog): system + "-" + prvnich 16 znaku
     * SHA-1 (hex, mala pismena) z "system/jmeno souboru malymi pismeny". Overeno na trech id Final Fantasy VIII
     * z logu 19. 11. (.zip, .cue, .bin). Server .bin k .cue jinak nedava (/api/v1/hra?id= je jen podle id).
     */
    static String idNaServeru(String system, String soubor) {
        if (system == null || soubor == null || soubor.isEmpty()) return null;
        try {
            final String s = system.toLowerCase(Locale.ROOT);
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            byte[] h = md.digest((s + "/" + soubor.toLowerCase(Locale.ROOT)).getBytes(StandardCharsets.UTF_8));
            StringBuilder b = new StringBuilder(s).append('-');
            for (int i = 0; i < 8; i++) b.append(String.format(Locale.ROOT, "%02x", h[i] & 0xff));
            return b.toString();
        } catch (Throwable t) {
            return null;
        }
    }

    /** Jmeno souboru z radku FILE v .cue - bez cesty (obe lomitka), bez mezer okolo. */
    static String jmenoZCue(String ref) {
        if (ref == null) return "";
        String r = ref.trim().replace('\\', '/');
        int i = r.lastIndexOf('/');
        return i >= 0 ? r.substring(i + 1).trim() : r;
    }

    /** Je to adresa hry na serveru N&P (".../api/v1/hra?id=...")? */
    static boolean jeHraNaServeru(String url) {
        return url != null && url.contains("/api/v1/hra?id=");
    }

    /** Adresa souboru vedle .cue na serveru N&P: stejny server, id podle jmena souboru. */
    static String adresaNaServeru(String urlCue, String system, String soubor) {
        if (!jeHraNaServeru(urlCue)) return null;
        final String id = idNaServeru(system, jmenoZCue(soubor));
        if (id == null) return null;
        return urlCue.substring(0, urlCue.indexOf("/api/v1/hra?id=")) + "/api/v1/hra?id=" + id;
    }
}
