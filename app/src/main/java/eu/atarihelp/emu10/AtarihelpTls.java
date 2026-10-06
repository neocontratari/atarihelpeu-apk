package eu.atarihelp.emu10;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

/**
 * B298: HTTPS na atarihelp.eu spolehlive i na starsim Androidu.
 *
 * Z Reneho logu (Android 9): stahovani her z atarihelp.eu obcas selze s
 * "Trust anchor for certification path not found" a o par sekund pozdeji
 * stejna adresa projde. atarihelp.eu bezi za WEDOS ochranou na dvou IP
 * adresach - typicky pripad, kdy server nekdy NEPOSLE mezilehly certifikat
 * certifikacni autority. Prohlizec (Chrome) si ho v takovem pripade dohleda
 * sam podle adresy v certifikatu (AIA, "CA Issuers"); Java v Androidu to
 * nedela a spojeni odmitne.
 *
 * Tohle dela totez co prohlizec: kdyz systemova kontrola selze, vezme z
 * certifikatu serveru adresu vydavatele (AIA), stahne mezilehly certifikat
 * a retez zkontroluje ZNOVU systemem proti duveryhodnym korenovym
 * certifikatum telefonu. Nic se "nepovoli naslepo": kdyz retez ani pak
 * nesedi, spojeni se odmitne jako drive (a do logu se napise, co server
 * poslal - podle toho se da najit dalsi pricina).
 */
final class AtarihelpTls {
    interface Log { void log(String s); }

    private static volatile SSLSocketFactory factory;
    private static volatile Log logger;
    private static final Map<String, List<X509Certificate>> stazene = new ConcurrentHashMap<>();
    private static final Map<String, Boolean> hlaseno = new ConcurrentHashMap<>();
    // B299: posledni odmitnuty certifikat, ktery NENI atarihelp.eu (napr. "localhost <- ospanel"
    // z Reneho logu = na jeho Wi-Fi vede atarihelp.eu na mistni server OSPanel na PC)
    private static volatile String mistniServer = null;
    private static volatile long mistniServerCas = 0;

    /** B299: popis ciziho serveru, ktery se v poslednich "ms" ozval misto atarihelp.eu, nebo null. */
    static String mistniServer(long ms) {
        String s = mistniServer;
        if (s == null || System.currentTimeMillis() - mistniServerCas > ms) return null;
        return s;
    }

    private AtarihelpTls() {}

    /** Nastavi spojeni na kontrolu certifikatu s dohledanim mezilehleho (jen HTTPS). */
    static void pouzij(HttpURLConnection c, Log log) {
        if (log != null) logger = log;
        if (!(c instanceof HttpsURLConnection)) return;
        try {
            SSLSocketFactory f = factory;
            if (f == null) {
                synchronized (AtarihelpTls.class) {
                    if (factory == null) {
                        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
                        tmf.init((KeyStore) null);
                        X509TrustManager sys = null;
                        for (TrustManager tm : tmf.getTrustManagers()) {
                            if (tm instanceof X509TrustManager) { sys = (X509TrustManager) tm; break; }
                        }
                        if (sys == null) return;
                        SSLContext ctx = SSLContext.getInstance("TLS");
                        ctx.init(null, new TrustManager[]{new AiaTrustManager(sys)}, null);
                        factory = ctx.getSocketFactory();
                    }
                    f = factory;
                }
            }
            ((HttpsURLConnection) c).setSSLSocketFactory(f);
        } catch (Throwable t) {
            loguj("B298 TLS nastaveni selhalo (pouzije se systemove): " + t);
        }
    }

    private static void loguj(String s) {
        Log l = logger;
        if (l != null) {
            try { l.log(s); } catch (Throwable ignored) {}
        }
    }

    private static String cn(javax.security.auth.x500.X500Principal p) {
        if (p == null) return "?";
        String s = p.getName();
        int i = s.indexOf("CN=");
        if (i >= 0) {
            int e = s.indexOf(',', i);
            return s.substring(i + 3, e < 0 ? s.length() : e);
        }
        return s.length() > 60 ? s.substring(0, 60) : s;
    }

    private static String popis(X509Certificate[] ch) {
        StringBuilder b = new StringBuilder();
        b.append(ch == null ? 0 : ch.length).append(" cert.");
        if (ch != null) {
            for (int i = 0; i < ch.length && i < 4; i++) {
                b.append(i == 0 ? ": " : " | ");
                b.append(cn(ch[i].getSubjectX500Principal())).append(" <- ").append(cn(ch[i].getIssuerX500Principal()));
            }
        }
        return b.toString();
    }

    /** Adresa "CA Issuers" z rozsireni AIA (1.3.6.1.5.5.7.1.1) nebo null. */
    static String caIssuersUrl(X509Certificate c) {
        byte[] ext = c.getExtensionValue("1.3.6.1.5.5.7.1.1");
        if (ext == null) return null;
        // OID id-ad-caIssuers 1.3.6.1.5.5.7.48.2 = 2B 06 01 05 05 07 30 02, za nim
        // [6] uniformResourceIdentifier: 0x86, delka, ASCII adresa
        final byte[] oid = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02};
        for (int i = 0; i + oid.length + 2 < ext.length; i++) {
            boolean shoda = true;
            for (int k = 0; k < oid.length; k++) if (ext[i + k] != oid[k]) { shoda = false; break; }
            if (!shoda) continue;
            int p = i + oid.length;
            if ((ext[p] & 0xFF) != 0x86) continue;
            int len = ext[p + 1] & 0xFF;
            int st = p + 2;
            if (len > 0x80) {                       // dlouha forma delky
                int nb = len & 0x7F; len = 0;
                for (int k = 0; k < nb && st < ext.length; k++) len = (len << 8) | (ext[st++] & 0xFF);
            }
            if (len <= 0 || st + len > ext.length) continue;
            String u = new String(ext, st, len, java.nio.charset.StandardCharsets.US_ASCII);
            if (u.startsWith("http://") || u.startsWith("https://")) return u;
        }
        return null;
    }

    private static List<X509Certificate> stahni(String url) throws Exception {
        List<X509Certificate> v = stazene.get(url);
        if (v != null) return v;
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        try {
            c.setConnectTimeout(8000);
            c.setReadTimeout(8000);
            c.setInstanceFollowRedirects(true);
            int code = c.getResponseCode();
            if (code < 200 || code >= 300) throw new java.io.IOException("HTTP " + code);
            ByteArrayOutputStream bo = new ByteArrayOutputStream();
            try (InputStream in = c.getInputStream()) {
                byte[] b = new byte[8192];
                int n;
                while ((n = in.read(b)) > 0 && bo.size() < 256 * 1024) bo.write(b, 0, n);
            }
            CertificateFactory cf = CertificateFactory.getInstance("X.509");
            Collection<? extends Certificate> cs = cf.generateCertificates(new java.io.ByteArrayInputStream(bo.toByteArray()));
            v = new ArrayList<>();
            for (Certificate x : cs) if (x instanceof X509Certificate) v.add((X509Certificate) x);
            if (v.isEmpty()) throw new CertificateException("na adrese neni certifikat");
            stazene.put(url, v);
            return v;
        } finally {
            try { c.disconnect(); } catch (Throwable ignored) {}
        }
    }

    // (package-private kvuli testu na PC: test_overeni/b298/TestAtarihelpTls.java)
    static final class AiaTrustManager implements X509TrustManager {
        private final X509TrustManager sys;

        AiaTrustManager(X509TrustManager sys) { this.sys = sys; }

        @Override
        public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException {
            sys.checkClientTrusted(chain, authType);
        }

        @Override
        public X509Certificate[] getAcceptedIssuers() {
            return sys.getAcceptedIssuers();
        }

        @Override
        public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException {
            try {
                sys.checkServerTrusted(chain, authType);
                return;
            } catch (CertificateException e) {
                if (chain == null || chain.length == 0) throw e;
                // retez doplnit o vydavatele podle AIA (max 3 urovne)
                List<X509Certificate> retez = new ArrayList<>();
                for (X509Certificate x : chain) retez.add(x);
                String zdroj = "";
                X509Certificate posl = chain[chain.length - 1];
                for (int krok = 0; krok < 3; krok++) {
                    if (posl.getSubjectX500Principal().equals(posl.getIssuerX500Principal())) break;   // koren
                    String url = caIssuersUrl(posl);
                    if (url == null) break;
                    X509Certificate vydavatel = null;
                    try {
                        for (X509Certificate k : stahni(url)) {
                            if (k.getSubjectX500Principal().equals(posl.getIssuerX500Principal())) { vydavatel = k; break; }
                        }
                    } catch (Throwable t) {
                        zdroj += " (stazeni " + url + " selhalo: " + t.getClass().getSimpleName() + ")";
                        break;
                    }
                    if (vydavatel == null) break;
                    retez.add(vydavatel);
                    zdroj += " +" + cn(vydavatel.getSubjectX500Principal()) + " z " + url;
                    posl = vydavatel;
                    try {
                        sys.checkServerTrusted(retez.toArray(new X509Certificate[0]), authType);
                        String klic = "ok:" + cn(chain[0].getSubjectX500Principal()) + popis(chain);
                        if (hlaseno.putIfAbsent(klic, Boolean.TRUE) == null)
                            loguj("B298 TLS atarihelp.eu: server poslal neuplny retez (" + popis(chain) + ") - doplnen" + zdroj + " -> DUVERYHODNY (system telefonu)");
                        return;
                    } catch (CertificateException ignored) {
                        // zkusit dalsi uroven
                    }
                }
                String klic = "bad:" + popis(chain);
                if (hlaseno.putIfAbsent(klic, Boolean.TRUE) == null)
                    loguj("B298 TLS odmitnuto: " + e.getMessage() + " | server poslal " + popis(chain) + zdroj);
                // B299: certifikat vubec neni na atarihelp.eu (localhost, OSPanel, vlastni podpis)
                // -> neni to chyba webu ani appky, ale sit telefonu vede jinam
                String jmeno = cn(chain[0].getSubjectX500Principal()).toLowerCase(java.util.Locale.US);
                String vydal = cn(chain[0].getIssuerX500Principal()).toLowerCase(java.util.Locale.US);
                if (!jmeno.contains("atarihelp") || vydal.contains("ospanel") || chain[0].getSubjectX500Principal().equals(chain[0].getIssuerX500Principal())) {
                    mistniServer = "certifikát " + cn(chain[0].getSubjectX500Principal()) + " od " + cn(chain[0].getIssuerX500Principal());
                    mistniServerCas = System.currentTimeMillis();
                    if (hlaseno.putIfAbsent("mistni:" + mistniServer, Boolean.TRUE) == null)
                        loguj("B299 TLS: misto atarihelp.eu odpovedel JINY server (" + mistniServer + ") - sit telefonu (Wi-Fi/DNS) "
                                + "vede atarihelp.eu jinam, napr. na mistni OSPanel na PC. Na mobilnich datech jde skutecny web.");
                }
                throw e;
            }
        }
    }
}
