package eu.atarihelp.emu10;

import java.io.RandomAccessFile;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Map;

/**
 * B321: osobni klic, ktery server N&P vlozil do APK pri stazeni (dvojice ID 0x4E615056 v APK Signing
 * Block - zaznamy ZIP ani podpis aplikace se nemeni). Aplikace ho cte ze sveho vlastniho APK
 * (getApplicationInfo().sourceDir) a posle pri parovani telefonu. Hodnota je text UTF-8:
 * "nap=1\ntajemstvi=<32 hex>\napk_sha256=<64 hex>\n".
 */
public final class NapApkKlic {
    public static final int ID_NAP = 0x4E615056;
    private static final Charset UTF8 = Charset.forName("UTF-8");
    private static final Charset ASCII = Charset.forName("US-ASCII");

    private NapApkKlic() {}

    /** Text hodnoty, nebo null (APK bez klice, napr. stazene v LAN nebo z GitHubu). */
    public static String precti(String cestaApk) {
        RandomAccessFile f = null;
        try {
            f = new RandomAccessFile(cestaApk, "r");
            long n = f.length();
            if (n < 22) return null;
            int max = (int) Math.min(n, 22 + 65535);
            byte[] konec = new byte[max];
            f.seek(n - max);
            f.readFully(konec);
            int e = -1;
            for (int i = max - 22; i >= 0; i--) {
                if (konec[i] == 0x50 && konec[i + 1] == 0x4B && konec[i + 2] == 0x05 && konec[i + 3] == 0x06
                        && i + 22 + u16(konec, i + 20) == max) { e = i; break; }
            }
            if (e < 0) return null;
            long cd = u32(konec, e + 16);
            if (cd < 32 || cd > n) return null;
            byte[] pata = new byte[24];
            f.seek(cd - 24);
            f.readFully(pata);
            if (!"APK Sig Block 42".equals(new String(pata, 8, 16, ASCII))) return null;
            long vel = u64(pata, 0);
            if (vel < 24 || vel > cd - 8 || vel > 32L * 1024 * 1024) return null;
            byte[] pary = new byte[(int) (vel - 24)];
            f.seek(cd - vel);                       // zacatek bloku + 8 (prvni pole velikosti)
            f.readFully(pary);
            int p = 0;
            while (p + 12 <= pary.length) {
                long d = u64(pary, p);
                if (d < 4 || d > pary.length - p - 8) return null;
                if ((int) u32(pary, p + 8) == ID_NAP) return new String(pary, p + 12, (int) (d - 4), UTF8);
                p += 8 + (int) d;
            }
            return null;
        } catch (Exception ex) {
            return null;
        } finally {
            if (f != null) try { f.close(); } catch (Exception ignored) { }
        }
    }

    /** Hodnoty klice (tajemstvi, apk_sha256, ...) - prazdna mapa, kdyz APK klic nema. */
    public static Map<String, String> hodnoty(String cestaApk) {
        Map<String, String> m = new HashMap<String, String>();
        String t = precti(cestaApk);
        if (t == null) return m;
        for (String r : t.split("\n")) {
            int i = r.indexOf('=');
            if (i > 0) m.put(r.substring(0, i).trim(), r.substring(i + 1).trim());
        }
        return m;
    }

    private static int u16(byte[] a, int i) { return (a[i] & 0xFF) | ((a[i + 1] & 0xFF) << 8); }
    private static long u32(byte[] a, int i) {
        return (a[i] & 0xFFL) | ((a[i + 1] & 0xFFL) << 8) | ((a[i + 2] & 0xFFL) << 16) | ((a[i + 3] & 0xFFL) << 24);
    }
    private static long u64(byte[] a, int i) {
        long hi = u32(a, i + 4);
        return hi >= 0x80000000L ? -1 : u32(a, i) | (hi << 32);
    }
}
