package eu.atarihelp.emu10;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.Charset;
import java.util.Locale;
import java.util.Map;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

/**
 * B321: spojeni appky se serverem N&P (Reneho PC, engine v10).
 *
 *  - PAROVANI: POST /api/v1/sparuj s oznacenim telefonu (ANDROID_ID) a s klicem, ktery server
 *    vlozil do APK pri stazeni (NapApkKlic), nebo s osobnim kodem NAP-XXXX-XXXX. Server vrati
 *    tajny token telefonu; ten zustava jen v appce (SharedPreferences), nikam se neposila.
 *  - KAZDY DALSI DOTAZ je podepsany: X-NaP-Podpis = HMAC-SHA256(token,
 *    zarizeni + "\n" + cas + "\n" + METODA + "\n" + cesta s dotazem). Bez tokenu server nic neda.
 *  - HODINY: server posila server_cas; appka si pamatuje rozdil, takze spatne hodiny v telefonu
 *    nevadi (kdyz server odmitne cas, appka se srovna a zkusi to jednou znovu).
 *
 * Ciste Java bez Androidu (uloziste je rozhrani) - stejny kod se testuje na PC proti skutecnemu
 * serveru (test_overeni/b321).
 */
public final class NapServerKlient {

    public static final String VYCHOZI_SERVER = "http://46.33.114.139:8080";
    private static final Charset UTF8 = Charset.forName("UTF-8");
    private static final int MAX_ODPOVED = 8 * 1024 * 1024;

    /** Kam si appka uklada token a nastaveni (v appce SharedPreferences "nap_server"). */
    public interface Uloziste {
        String cti(String klic, String vychozi);
        void zapis(String klic, String hodnota);   // null = smazat
    }

    public interface Log { void rekni(String text); }

    /** Odpoved serveru. kod 0 = server nedostupny (sit). */
    public static final class Odpoved {
        public int kod;
        public byte[] data = new byte[0];
        public String typ = "";
        public Map<String, Object> json;
        public String chyba = "";

        public boolean ok() { return kod == 200 && (json == null || !json.containsKey("ok") || NapJson.ano(json, "ok")); }
        public String duvod() { return kod == 0 ? "sit" : NapJson.text(json, "duvod"); }
        public String zprava() {
            String z = NapJson.text(json, "zprava");
            return z.length() > 0 ? z : chyba;
        }
        public String text() { return new String(data, UTF8); }
    }

    private final Uloziste u;
    private final String zarizeni, model, android, verze;
    private volatile Log log;

    public NapServerKlient(Uloziste u, String zarizeni, String model, String android, String verze) {
        this.u = u;
        this.zarizeni = zarizeni == null ? "" : zarizeni;
        this.model = jedenRadek(model, 60);
        this.android = jedenRadek(android, 20);
        this.verze = jedenRadek(verze, 40);
    }

    public void nastavLog(Log l) { log = l; }

    private void rekni(String t) {
        Log l = log;
        if (l != null) try { l.rekni(t); } catch (Throwable ignored) { }
    }

    // ------------------------------------------------------------------ stav
    public String server() {
        String s = u.cti("server", "");
        return s == null || s.isEmpty() ? VYCHOZI_SERVER : s;
    }

    /** "46.33.114.139:8080/" -> "http://46.33.114.139:8080"; prazdne = vychozi server. */
    public static String normalizujServer(String url) {
        if (url == null) return "";
        String s = url.trim();
        if (s.isEmpty()) return "";
        if (!s.startsWith("http://") && !s.startsWith("https://")) s = "http://" + s;
        int z = s.indexOf("://") + 3;
        int lomitko = s.indexOf('/', z);
        if (lomitko > 0) s = s.substring(0, lomitko);
        return s;
    }

    public void nastavServer(String url) {
        String s = normalizujServer(url);
        u.zapis("server", s.isEmpty() || s.equals(VYCHOZI_SERVER) ? null : s);
        rekni("B321 SERVER nastaven na " + server());
    }

    public String zarizeni() { return zarizeni; }
    public String kodMaska() { return u.cti("kod", ""); }
    public boolean zablokovano() { return "1".equals(u.cti("blok", "")); }

    private String token() { String t = u.cti("token", ""); return t == null ? "" : t; }

    /** Ma telefon platny token (sparovano s timhle oznacenim telefonu)? */
    public boolean sparovano() {
        return token().length() == 64 && zarizeni.length() > 0 && zarizeni.equals(u.cti("token_pro", ""));
    }

    public void zapomen(String proc) {
        u.zapis("token", null);
        u.zapis("token_pro", null);
        rekni("B321 PAROVANI zruseno: " + proc);
    }

    long posun() {
        try { return Long.parseLong(u.cti("posun", "0")); } catch (Exception e) { return 0; }
    }

    long ted() { return System.currentTimeMillis() / 1000L + posun(); }

    private void zapamatujCas(Map<String, Object> json) {
        long sc = NapJson.cislo(json, "server_cas", 0);
        if (sc > 1600000000L) {
            long p = sc - System.currentTimeMillis() / 1000L;
            if (Math.abs(p - posun()) > 2) u.zapis("posun", String.valueOf(p));
        }
    }

    public boolean jeNasUrl(String url) {
        return url != null && url.startsWith(server() + "/");
    }

    // ------------------------------------------------------------------ parovani
    /** Sparuje telefon klicem z APK (tajemstvi) nebo osobnim kodem. Blokuje - volat mimo UI vlakno. */
    public Odpoved sparuj(String kod, String tajemstvi) {
        StringBuilder t = new StringBuilder();
        t.append("zarizeni=").append(enc(zarizeni)).append("&model=").append(enc(model))
         .append("&android=").append(enc(android)).append("&verze=").append(enc(verze));
        if (tajemstvi != null && tajemstvi.length() > 0) t.append("&tajemstvi=").append(enc(tajemstvi));
        if (kod != null && kod.trim().length() > 0) t.append("&kod=").append(enc(kod.trim()));
        Odpoved o = posli("POST", "/api/v1/sparuj", t.toString().getBytes(UTF8), false);
        if (o.kod == 200 && NapJson.ano(o.json, "ok")) {
            String tok = NapJson.text(o.json, "token");
            if (tok.length() == 64) {
                u.zapis("token", tok);
                u.zapis("token_pro", zarizeni);
                u.zapis("kod", NapJson.text(o.json, "kod"));
                u.zapis("blok", null);
                rekni("B321 PAROVANI OK kod " + NapJson.text(o.json, "kod") + " (" + (kod != null && kod.trim().length() > 0 ? "zadany kod" : "klic z APK") + ")");
            }
        } else {
            if ("zablokovano".equals(o.duvod())) u.zapis("blok", "1");
            rekni("B321 PAROVANI ne: " + o.kod + " " + o.duvod());
        }
        return o;
    }

    // ------------------------------------------------------------------ podepsane dotazy
    /** Podepsany GET (stav, seznam her, obal). Blokuje. */
    public Odpoved get(String cesta) {
        Odpoved o = posli("GET", cesta, null, true);
        if (o.kod == 401 && "cas".equals(o.duvod())) {
            rekni("B321 HODINY telefonu srovnany podle serveru (posun " + posun() + " s), znovu");
            o = posli("GET", cesta, null, true);
        }
        zpracujStav(o);
        return o;
    }

    /** Po odpovedi serveru: neplatny token zapomenout, blokaci si zapamatovat. */
    public void zpracujStav(Odpoved o) {
        if (o.kod == 401 && ("nesparovano".equals(o.duvod()) || "podpis".equals(o.duvod()))) zapomen("server: " + o.duvod());
        else if (o.kod == 403 && "zablokovano".equals(o.duvod())) u.zapis("blok", "1");
        else if (o.kod == 200) u.zapis("blok", null);
    }

    /** Stav po chybe HTTP u stahovani (kod a telo chyby) - stejne jako u get(). */
    public void zpracujChybuStahovani(int kod, String teloChyby) {
        Odpoved o = new Odpoved();
        o.kod = kod;
        o.json = NapJson.objekt(teloChyby);
        if (o.json != null) zapamatujCas(o.json);
        zpracujStav(o);
    }

    /** Podepsane spojeni pro stahovani (hra, APK). Volajici cte a zavre. */
    public HttpURLConnection otevri(String cesta, String metoda) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(server() + cesta).openConnection();
        c.setRequestMethod(metoda);
        c.setConnectTimeout(20000);
        c.setReadTimeout(60000);
        c.setUseCaches(false);
        podepis(c);
        return c;
    }

    /** Doplni hlavicky N&P (User-Agent + podpis) do pripojeni na server. Cesta a metoda se berou z pripojeni. */
    public void podepis(HttpURLConnection c) {
        c.setRequestProperty("User-Agent", ua());
        String t = token();
        if (t.length() != 64) return;
        String cesta = c.getURL().getFile();
        if (cesta == null || cesta.isEmpty()) cesta = "/";
        String cas = String.valueOf(ted());
        String metoda = c.getRequestMethod().toUpperCase(Locale.US);
        c.setRequestProperty("X-NaP-Zarizeni", zarizeni);
        c.setRequestProperty("X-NaP-Cas", cas);
        c.setRequestProperty("X-NaP-Podpis", hmac(t, zarizeni + "\n" + cas + "\n" + metoda + "\n" + cesta));
    }

    public String ua() {
        return ascii("NapVision/" + verze + " (Android " + android + "; " + model + ")");
    }

    private Odpoved posli(String metoda, String cesta, byte[] telo, boolean podepsat) {
        Odpoved o = new Odpoved();
        HttpURLConnection c = null;
        try {
            c = (HttpURLConnection) new URL(server() + cesta).openConnection();
            c.setRequestMethod(metoda);
            c.setConnectTimeout(15000);
            c.setReadTimeout(30000);
            c.setUseCaches(false);
            if (podepsat) podepis(c); else c.setRequestProperty("User-Agent", ua());
            if (telo != null) {
                c.setDoOutput(true);
                c.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
                c.setFixedLengthStreamingMode(telo.length);
                OutputStream out = c.getOutputStream();
                out.write(telo);
                out.close();
            }
            o.kod = c.getResponseCode();
            o.typ = c.getContentType() == null ? "" : c.getContentType();
            InputStream in = o.kod >= 400 ? c.getErrorStream() : c.getInputStream();
            o.data = in == null ? new byte[0] : precti(in, MAX_ODPOVED);
            if (o.typ.startsWith("application/json")) {
                o.json = NapJson.objekt(o.text());
                if (o.json != null) zapamatujCas(o.json);
            }
        } catch (IOException e) {
            o.kod = 0;
            o.chyba = "server nedostupný (" + e.getClass().getSimpleName() + ")";
        } finally {
            if (c != null) try { c.disconnect(); } catch (Throwable ignored) { }
        }
        if (o.kod != 200) rekni("B321 SERVER " + metoda + " " + cesta + " -> " + (o.kod == 0 ? o.chyba : o.kod + " " + o.duvod()));
        return o;
    }

    // ------------------------------------------------------------------ pomocne
    static byte[] precti(InputStream in, int max) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        byte[] buf = new byte[16384];
        int n;
        try {
            while ((n = in.read(buf)) > 0) {
                if (b.size() + n > max) throw new IOException("odpoved je moc velka");
                b.write(buf, 0, n);
            }
        } finally {
            try { in.close(); } catch (Throwable ignored) { }
        }
        return b.toByteArray();
    }

    static String hmac(String klic, String zprava) {
        try {
            Mac m = Mac.getInstance("HmacSHA256");
            m.init(new SecretKeySpec(klic.getBytes(UTF8), "HmacSHA256"));
            byte[] h = m.doFinal(zprava.getBytes(UTF8));
            StringBuilder s = new StringBuilder(64);
            for (byte x : h) s.append(String.format("%02x", x & 0xFF));
            return s.toString();
        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }

    private static String enc(String s) {
        try { return URLEncoder.encode(s == null ? "" : s, "UTF-8"); } catch (Exception e) { return ""; }
    }

    private static String jedenRadek(String s, int max) {
        if (s == null) return "";
        String v = s.replaceAll("[\\r\\n\\t]", " ").trim();
        return v.length() > max ? v.substring(0, max) : v;
    }

    private static String ascii(String s) {
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            b.append(c >= 0x20 && c < 0x7F ? c : '?');
        }
        return b.toString();
    }
}
