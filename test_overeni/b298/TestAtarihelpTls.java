package eu.atarihelp.emu10;

import java.io.FileInputStream;
import java.security.KeyStore;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

/**
 * B298: test AtarihelpTls na PC (bez Androidu).
 *   ./mkcerts.sh 18765 certs ; (cd certs && python3 -m http.server 18765 --bind 127.0.0.1 &)
 *   javac -d out ../../app/src/main/java/eu/atarihelp/emu10/AtarihelpTls.java TestAtarihelpTls.java
 *   java -cp out eu.atarihelp.emu10.TestAtarihelpTls certs
 * "System" = duvera jen v testovacim koreni. Server posle jen list (bez
 * mezilehleho) -> system odmitne, AtarihelpTls dohleda mezilehly podle AIA
 * a retez projde. List od cizi CA (jiny klic, stejne jmeno vydavatele)
 * musi byt odmitnut i po dohledani.
 */
public class TestAtarihelpTls {
    static int chyb = 0, kontrol = 0;

    static void over(boolean ok, String co) {
        kontrol++;
        if (!ok) chyb++;
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
    }

    static X509Certificate cert(String f) throws Exception {
        try (FileInputStream in = new FileInputStream(f)) {
            return (X509Certificate) CertificateFactory.getInstance("X.509").generateCertificate(in);
        }
    }

    public static void main(String[] a) throws Exception {
        String d = a.length > 0 ? a[0] : "certs";
        X509Certificate root = cert(d + "/root.pem"), leaf = cert(d + "/leaf.pem"), eleaf = cert(d + "/eleaf.pem");
        KeyStore ks = KeyStore.getInstance(KeyStore.getDefaultType());
        ks.load(null, null);
        ks.setCertificateEntry("root", root);
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        tmf.init(ks);
        X509TrustManager sys = null;
        for (TrustManager tm : tmf.getTrustManagers()) if (tm instanceof X509TrustManager) sys = (X509TrustManager) tm;
        StringBuilder log = new StringBuilder();
        AtarihelpTls.pouzij(null, s -> log.append(s).append('\n'));   // jen nastavi logovani (null spojeni se preskoci)

        over("http://127.0.0.1:18765/inter.der".equals(AtarihelpTls.caIssuersUrl(leaf)), "AIA: adresa vydavatele prectena z certifikatu");

        boolean sysOdmitl = false;
        try { sys.checkServerTrusted(new X509Certificate[]{leaf}, "RSA"); } catch (CertificateException e) { sysOdmitl = true; }
        over(sysOdmitl, "system sam neuplny retez (jen list) odmitne - jako Android 9 v Reneho logu");

        AtarihelpTls.AiaTrustManager tm = new AtarihelpTls.AiaTrustManager(sys);
        boolean prijato = true;
        try { tm.checkServerTrusted(new X509Certificate[]{leaf}, "RSA"); } catch (CertificateException e) { prijato = false; System.out.println("  " + e); }
        over(prijato, "AtarihelpTls: mezilehly dohledan podle AIA a retez prijat");

        boolean cizi = false;
        try { tm.checkServerTrusted(new X509Certificate[]{eleaf}, "RSA"); } catch (CertificateException e) { cizi = true; }
        over(cizi, "list od cizi CA (stejne jmeno vydavatele, jiny klic) je odmitnut i po dohledani");

        over(AtarihelpTls.mistniServer(60000L) == null, "B299: cizi CA s CN atarihelp.eu se nehlasi jako mistni server (neni localhost/OSPanel)");

        boolean osp = false;
        try { tm.checkServerTrusted(new X509Certificate[]{cert(d + "/localhost.pem")}, "RSA"); } catch (CertificateException e) { osp = true; }
        String ms = AtarihelpTls.mistniServer(60000L);
        over(osp && ms != null && ms.contains("localhost") && ms.contains("ospanel"),
                "B299: certifikat localhost <- ospanel (jako v Reneho logu) odmitnut a hlasen jako MISTNI server: " + ms);

        boolean uplny = true;
        try { tm.checkServerTrusted(new X509Certificate[]{leaf, cert(d + "/inter.pem")}, "RSA"); } catch (CertificateException e) { uplny = false; }
        over(uplny, "uplny retez od serveru projde rovnou (beze zmeny)");

        System.out.print(log);
        System.out.println("VYSLEDEK: " + kontrol + " kontrol, " + chyb + " chyb");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
