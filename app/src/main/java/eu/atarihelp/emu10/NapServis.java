package eu.atarihelp.emu10;

import java.security.MessageDigest;
import java.util.Locale;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

/**
 * B312: SERVIS - Reneho tlacitka ODEMKNOUT DALSI a ZAMKNOUT VSE (postup hrace, NapPostup).
 *
 * Rene: "tlacitka schovat, ale nejak konstruktivne - treba pod kod, ktery bude vedet jen ja".
 * V OPTIONS uz tlacitka nejsou videt; servis se otevre az spravnym servisnim kodem a jen na
 * 10 minut. Hlavni je, ze to hlida JAVA: AHINTRO.odemkniDalsi / zamkniVse muze z JavaScriptu
 * zavolat kterakoli stranka ve WebView (i web) - bez otevreneho servisu je Java odmitne.
 *
 * Repozitar je verejny, proto tu NENI kod, jen nahodna sul a otisk PBKDF2-HMAC-SHA256
 * (60 000 opakovani). Kod: 10 znaku z 31 (bez 0 O 1 I L), pomlcky a mezery se nepocitaji,
 * velikost pismen taky ne. Po 3 spatnych pokusech minuta pauza. Kod ani jeho cast se
 * nikam nepise (ani do logu).
 *
 * Ciste Java (zadny Android) - testuje se na PC v JVM (test_overeni/b312).
 */
final class NapServis {

    private NapServis() {}

    static final int ITERACI = 60000;
    private static final byte[] SUL = hex("21532778f0738887e809ded53b15fd71");
    private static final byte[] OTISK = hex("fba6715faabf8c38a0ea10d886d1b0a1e36386210599a4fc6cbd284a6bf0d34e");

    static final long PLATNOST_MS = 10L * 60L * 1000L;
    static final int POKUSU_DO_PAUZY = 3;
    static final long PAUZA_MS = 60L * 1000L;

    private static long otevrenoDo = 0L;
    private static long pauzaDo = 0L;
    private static int spatnych = 0;

    static byte[] hex(String s) {
        byte[] o = new byte[s.length() / 2];
        for (int i = 0; i < o.length; i++) o[i] = (byte) Integer.parseInt(s.substring(2 * i, 2 * i + 2), 16);
        return o;
    }

    /** Velka pismena, bez mezer a pomlcek. */
    static String normalizuj(String kod) {
        if (kod == null) return "";
        StringBuilder sb = new StringBuilder(kod.length());
        for (int i = 0; i < kod.length(); i++) {
            char c = kod.charAt(i);
            if (c == ' ' || c == '-' || c == '\t' || c == '\n' || c == '\r') continue;
            sb.append(c);
        }
        return sb.toString().toUpperCase(Locale.US);
    }

    /** PBKDF2-HMAC-SHA256 (RFC 8018). Android ma PBKDF2WithHmacSHA256 az od API 26, appka od 24. */
    static byte[] pbkdf2(byte[] heslo, byte[] sul, int iteraci, int delka) throws Exception {
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(heslo, "HmacSHA256"));
        int hLen = mac.getMacLength();
        int bloku = (delka + hLen - 1) / hLen;
        byte[] out = new byte[delka];
        for (int i = 1; i <= bloku; i++) {
            mac.update(sul);
            mac.update(new byte[]{(byte) (i >>> 24), (byte) (i >>> 16), (byte) (i >>> 8), (byte) i});
            byte[] u = mac.doFinal();
            byte[] t = u.clone();
            for (int j = 1; j < iteraci; j++) {
                u = mac.doFinal(u);
                for (int k = 0; k < t.length; k++) t[k] ^= u[k];
            }
            System.arraycopy(t, 0, out, (i - 1) * hLen, Math.min(hLen, delka - (i - 1) * hLen));
        }
        return out;
    }

    /** Sedi kod s otiskem? (porovnani v konstantnim case) */
    static boolean kodSedi(String kod, byte[] sul, byte[] otisk, int iteraci) {
        String n = normalizuj(kod);
        if (n.length() < 6 || n.length() > 64) return false;
        try {
            return MessageDigest.isEqual(pbkdf2(n.getBytes("UTF-8"), sul, iteraci, otisk.length), otisk);
        } catch (Throwable t) {
            return false;
        }
    }

    static boolean kodSedi(String kod) {
        return kodSedi(kod, SUL, OTISK, ITERACI);
    }

    /** "OK" (servis otevren), "SPATNE" nebo "POCKEJ:sekund". */
    static synchronized String zkus(String kod, long ted) {
        if (ted < pauzaDo) return "POCKEJ:" + ((pauzaDo - ted + 999L) / 1000L);
        if (kodSedi(kod)) {
            spatnych = 0;
            otevrenoDo = ted + PLATNOST_MS;
            return "OK";
        }
        spatnych++;
        if (spatnych >= POKUSU_DO_PAUZY) {
            spatnych = 0;
            pauzaDo = ted + PAUZA_MS;
            return "POCKEJ:" + (PAUZA_MS / 1000L);
        }
        return "SPATNE";
    }

    static synchronized boolean otevreno(long ted) {
        return ted < otevrenoDo;
    }

    /** Kolik sekund jeste servis bezi (0 = zavreny). */
    static synchronized long zbyvaS(long ted) {
        return ted < otevrenoDo ? (otevrenoDo - ted + 999L) / 1000L : 0L;
    }

    static synchronized void zavri() {
        otevrenoDo = 0L;
    }

    /** Jen pro test na PC: vse zpet na zacatek. */
    static synchronized void vynulujProTest() {
        otevrenoDo = 0L;
        pauzaDo = 0L;
        spatnych = 0;
    }
}
