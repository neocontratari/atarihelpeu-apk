package eu.atarihelp.emu10;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

/**
 * B356: PROC APPKA SKONCILA - do logu i ty pady, ktere se tam dosud nedostaly.
 *
 * Rene, 19. 11.: "final fantasy ma asi taky problemy", log PS1 "Ps1Crash": oba logy (21:03 a 21:49) konci uprostred
 * behu bez jedineho radku o padu. Nativni pad PS1 (SIGSEGV ...) uz zapisuje NAP_CRASH (A10/A11) - nebyl tam.
 * Slepe byly: pad Javy (nezachycena vyjimka v kteremkoli vlakne), pad / zabiti vykreslovani stranek (WebView -
 * appka bez onRenderProcessGone pak skonci) a zabiti appky Androidem. Ted:
 *  - nezachycena vyjimka -> radek "B356 PAD JAVA" HNED na disk (log + nap_posledni_pad.txt), pak puvodni chovani,
 *  - WebView renderer pryc -> radek "B356 PAD WEBVIEW",
 *  - znacka behu (nap_beh.txt): appka v popredi a dalsi start bez PAUZA mezi tim = skoncila nahle,
 *  - Android 11+: duvod konce posledniho behu od Androidu (ApplicationExitInfo) - pad, malo pameti, ANR, uzivatel ...
 * Cista cast (texty, zapis) bez Androidu - testuje se v JVM (test_overeni/b356).
 */
final class NapPad {

    private NapPad() {}

    /** Duvod konce procesu (ApplicationExitInfo.REASON_*) cesky. */
    static String duvodKonce(int r) {
        switch (r) {
            case 1:  return "appka se ukoncila sama (EXIT_SELF)";
            case 2:  return "zabita signalem (SIGNALED)";
            case 3:  return "Android ji zavrel - malo pameti (LOW_MEMORY)";
            case 4:  return "PAD JAVY (CRASH)";
            case 5:  return "NATIVNI PAD - C++ (CRASH_NATIVE)";
            case 6:  return "ZAMRZLA - Android ji zavrel (ANR)";
            case 7:  return "nepovedl se start (INITIALIZATION_FAILURE)";
            case 8:  return "zmena opravneni (PERMISSION_CHANGE)";
            case 9:  return "prilis mnoho prostredku - Android ji zavrel (EXCESSIVE_RESOURCE_USAGE)";
            case 10: return "zavrel ji uzivatel (USER_REQUESTED - odsunuti z poslednich aplikaci / vynutit ukonceni)";
            case 11: return "uzivatel zastavil (USER_STOPPED)";
            case 12: return "spadla zavislost (DEPENDENCY_DIED)";
            case 13: return "jiny duvod (OTHER)";
            case 14: return "zmrazena (FREEZER)";
            case 15: return "zmena balicku (PACKAGE_STATE_CHANGE)";
            case 16: return "aktualizace appky (PACKAGE_UPDATED)";
            default: return "neznamy duvod (" + r + ")";
        }
    }

    /** Je to konec, o ktery se ma Rene zajimat (pad, zamrznuti, zabiti)? Bezne (aktualizace, uzivatel) ne. */
    static boolean dulezityKonec(int r) {
        return r == 2 || r == 3 || r == 4 || r == 5 || r == 6 || r == 7 || r == 9 || r == 12 || r == 13 || r == 0;
    }

    /** Radek o padu Javy: vlakno, vyjimka, nejvys 'radku' radku zasobniku, pricina. */
    static String popisPadu(String vlakno, Throwable t, int radku) {
        StringBuilder s = new StringBuilder();
        s.append("B356 PAD JAVA ve vlakne \"").append(vlakno).append("\": ").append(t);
        StackTraceElement[] st = t == null ? null : t.getStackTrace();
        if (st != null) for (int i = 0; i < Math.min(radku, st.length); i++) s.append("\n      at ").append(st[i]);
        Throwable p = t == null ? null : t.getCause();
        int hloubka = 0;
        while (p != null && hloubka++ < 3) {
            s.append("\n    pricina: ").append(p);
            StackTraceElement[] ps = p.getStackTrace();
            if (ps != null && ps.length > 0) s.append("\n      at ").append(ps[0]);
            p = p.getCause();
        }
        return s.toString();
    }

    /** Okamzity zapis (pripsat / prepsat) - v padu neni na frontu logu cas. Chyby ignoruje. */
    static void zapisHned(File f, String text, boolean pripsat) {
        if (f == null || text == null) return;
        try (FileOutputStream o = new FileOutputStream(f, pripsat)) {
            o.write(text.getBytes(StandardCharsets.UTF_8));
            o.flush();
            try { o.getFD().sync(); } catch (Throwable ignored) {}
        } catch (Throwable ignored) {}
    }

    /**
     * Znacka behu: "bezi|cas|verze" (appka v popredi), "pauza|cas|verze", "konec|cas|verze".
     * @return veta do logu pri startu o predchozim behu, nebo null (skoncil normalne / neni znacka)
     */
    static String predchoziBeh(String znacka) {
        if (znacka == null || znacka.trim().isEmpty()) return null;
        String[] p = znacka.trim().split("\\|", -1);
        String stav = p[0], cas = p.length > 1 ? p[1] : "?", verze = p.length > 2 ? p[2] : "?";
        if ("bezi".equals(stav))
            return "B356 PREDCHOZI BEH SKONCIL NAHLE: appka byla na obrazovce (od " + cas + ", " + verze + ") a skoncila bez PAUZA"
                    + " - pad, zamrznuti nebo zabiti Androidem (podrobnosti vys / v PS1_LOG_predchozi.txt)";
        return null;
    }
}
