package eu.atarihelp.emu10;

import android.view.KeyEvent;

import java.util.ArrayList;
import java.util.List;

/**
 * B355: TLACITKA OVLADACE - co ktere tlacitko herniho ovladace dela, zvlast pro Atari 130XE, Segu a PS1.
 * Rene: "musi byt v nastaveni u kazdeho emu co ktere tlacitko ma delat podle bt d-padu".
 *
 * Radky = tlacitka podle MISTA na ovladaci (A spodni, B prave, X leve, Y horni, L1 R1 L2 R2 L3 R3, START,
 * SELECT, MODE = HOME); smer (krizek, leva packa) zustava smerem. Co ktery cil znamena, jmena CZ / EN a
 * vychozi mapu dava C++ (nap_ovladac.h - katalog pres knihovnu Atari, ta je nactena vzdy), aby nic nebylo
 * opsane dvakrat. Tady jen: mapa jako text "A=FIRE;B=FIRE;X=K33;..." ("" = vychozi), zmena radku, verze
 * (jadro dostane mapu pred dalsim stiskem) a cil MENU (otevrit tohle nastaveni - do jadra nejde).
 * Cista trida bez Androidu (KeyEvent jen konstanty) - test test_overeni/b355/jvm.
 */
final class NapOvladacMapa {
    static final int ATARI = 0, SEGA = 1, PS1 = 2, JADER = 3;
    static final String[] JADRA = {"atari", "sega", "ps1"};
    static final String[] JADRA_JMENO = {"Atari 130XE", "Sega", "PS1"};
    static final String[] RADKY = {"A", "B", "X", "Y", "L1", "R1", "L2", "R2", "L3", "R3", "START", "SELECT", "MODE"};
    static final int R_MODE = 12;
    static final String MENU = "MENU";

    /** Katalog jednoho jadra z C++ (nap_ovladac.h katalog): radky "R\ttoken\tjmeno", cile "C\ttoken\tjmeno",
     *  "D\tvychozi mapa". */
    static final class Katalog {
        final String[] radekJmeno = new String[RADKY.length];
        final List<String> cilToken = new ArrayList<>();
        final List<String> cilJmeno = new ArrayList<>();
        String vychozi = "";

        boolean ok() { return !cilToken.isEmpty() && !vychozi.isEmpty(); }

        static Katalog z(String text) {
            final Katalog k = new Katalog();
            if (text != null) {
                for (String r : text.split("\n")) {
                    final String[] p = r.split("\t", 3);
                    if (p.length == 3 && p[0].equals("R")) {
                        final int i = radek(p[1]);
                        if (i >= 0) k.radekJmeno[i] = p[2];
                    } else if (p.length == 3 && p[0].equals("C")) {
                        if (!k.cilToken.contains(p[1])) { k.cilToken.add(p[1]); k.cilJmeno.add(p[2]); }
                    } else if (p.length >= 2 && p[0].equals("D")) {
                        k.vychozi = p[1].trim();
                    }
                }
            }
            for (int i = 0; i < RADKY.length; i++) if (k.radekJmeno[i] == null) k.radekJmeno[i] = RADKY[i];
            return k;
        }

        String jmenoCile(String token) {
            final int i = cilToken.indexOf(token);
            return i >= 0 ? cilJmeno.get(i) : token;
        }
    }

    static int radek(String token) {
        for (int i = 0; i < RADKY.length; i++) if (RADKY[i].equals(token)) return i;
        return -1;
    }

    /** Radek mapy pro klavesu ovladace (kod uz podle mista - NapOvladac.kodPodleMista); -1 = smer / jine. */
    static int radekKlavesy(int kod) {
        switch (kod) {
            case KeyEvent.KEYCODE_BUTTON_A: return 0;
            case KeyEvent.KEYCODE_BUTTON_B: return 1;
            case KeyEvent.KEYCODE_BUTTON_X: return 2;
            case KeyEvent.KEYCODE_BUTTON_Y: return 3;
            case KeyEvent.KEYCODE_BUTTON_L1: return 4;
            case KeyEvent.KEYCODE_BUTTON_R1: return 5;
            case KeyEvent.KEYCODE_BUTTON_L2: return 6;
            case KeyEvent.KEYCODE_BUTTON_R2: return 7;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return 8;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: return 9;
            case KeyEvent.KEYCODE_BUTTON_START: return 10;
            case KeyEvent.KEYCODE_BUTTON_SELECT: return 11;
            case KeyEvent.KEYCODE_BUTTON_MODE: return 12;
            default: return -1;
        }
    }

    /** "A=FIRE;B=FIRE;..." -> cil pro kazdy radek (RADKY); co v mape chybi, je z vychozi mapy ("0" = nic). */
    static String[] rozloz(String mapa, String vychozi) {
        final String[] v = new String[RADKY.length];
        prepis(v, vychozi);
        prepis(v, mapa);
        for (int i = 0; i < v.length; i++) if (v[i] == null || v[i].isEmpty()) v[i] = "0";
        return v;
    }

    private static void prepis(String[] v, String mapa) {
        if (mapa == null || mapa.isEmpty()) return;
        for (String d : mapa.split("[;, \n\t]+")) {
            final int e = d.indexOf('=');
            if (e <= 0 || e + 1 >= d.length()) continue;
            final int i = radek(d.substring(0, e));
            if (i >= 0) v[i] = d.substring(e + 1);
        }
    }

    static String sloz(String[] v) {
        final StringBuilder b = new StringBuilder();
        for (int i = 0; i < RADKY.length; i++) {
            if (i > 0) b.append(';');
            b.append(RADKY[i]).append('=').append(i < v.length && v[i] != null && !v[i].isEmpty() ? v[i] : "0");
        }
        return b.toString();
    }

    /** Jmeno do logu: bez diakritiky a bez symbolu ("klávesa MEZERA" -> "klavesa MEZERA", "✕ křížek" -> "krizek"). */
    static String doLogu(String s) {
        if (s == null) return "";
        final String n = java.text.Normalizer.normalize(s, java.text.Normalizer.Form.NFD).replaceAll("\\p{M}", "");
        final StringBuilder b = new StringBuilder();
        for (int i = 0; i < n.length(); i++) {
            final char c = n.charAt(i);
            if (c >= 32 && c < 127) b.append(c);
            else if (c == '–' || c == '−') b.append('-');
        }
        return b.toString().replaceAll("\\s+", " ").trim();
    }

    // ---- stav: mapa kazdeho jadra ("" = vychozi), verze (jadro dostane mapu pred dalsim stiskem) ----
    private final String[] mapa = {"", "", ""};
    private final int[] verze = {1, 1, 1};

    synchronized String mapa(int j) { return mapa[platne(j)]; }
    synchronized int verze(int j) { return verze[platne(j)]; }

    /** Nova mapa jadra ("" = vychozi); vraci true, kdyz se neco zmenilo. */
    synchronized boolean nastav(int j, String m) {
        j = platne(j);
        final String nova = m == null ? "" : m;
        if (nova.equals(mapa[j])) return false;
        mapa[j] = nova;
        verze[j]++;
        return true;
    }

    /** Jeden radek -> cil (token). Mapa se ulozi cela (vsech 13 radku), at je v logu a v nastaveni videt cela. */
    synchronized boolean nastavRadek(int j, int radek, String cil, String vychozi) {
        j = platne(j);
        if (radek < 0 || radek >= RADKY.length || cil == null || cil.isEmpty()) return false;
        final String[] v = rozloz(mapa[j], vychozi);
        if (cil.equals(v[radek])) return false;
        v[radek] = cil;
        final String nova = sloz(v);
        return nastav(j, nova.equals(sloz(rozloz("", vychozi))) ? "" : nova);   // shodna s vychozi = ""
    }

    /** Cil radku (token) podle aktualni mapy jadra; vychozi = katalog z C++. */
    synchronized String cil(int j, int radek, String vychozi) {
        if (radek < 0 || radek >= RADKY.length) return "0";
        return rozloz(mapa[platne(j)], vychozi)[radek];
    }

    private static int platne(int j) { return j == SEGA || j == PS1 ? j : ATARI; }
}
