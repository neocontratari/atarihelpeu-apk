package eu.atarihelp.emu10;

import android.view.KeyEvent;

import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Random;

/**
 * B340: HRA JEN DALKOVYM OVLADACEM TV (JVM, skutecne tridy appky NapDalkovy a NapDrzeniSmeru;
 * konstanty KeyEvent z android-34.jar).
 *  R1) PS1: cisla kolem petky jako ovladac PlayStation (2 = TROJUHELNIK ... 8 = KRIZEK), rohy L1 R1 L2 R2,
 *      5 SELECT, 0 START - rada i numericka klavesnice,
 *  R2) SEGA: kazda rada cisel = A B C (nap_ovladac.h: leve X = A, spodni A = B, prave B = C), 0 START,
 *  R3) ATARI: cislice -1 (jdou jako klavesy Atari - klavesnice B302), ostatni klavesy stejne jako B334,
 *  R4) cislice / smer krizku,
 *  H1) ovladac s JEDNIM tlacitkem naraz: drzim VPRAVO + OK -> VPRAVO se obnovi v tytez ms (jadro ho nepusti),
 *      po OK ovladac VPRAVO posle znovu -> beze zmeny, nakonec pusteni,
 *  H2) ovladac se dvema tlacitky naraz (gamepad): nic se nepodrzi - udalosti 1:1,
 *  H3) clovek sipku behem skoku pustil: po OK se nevratila -> pustena za CEKANI_MS (ne driv),
 *  H4) samostatna tuknuti (sipka, za 200 ms OK): nic se nepodrzi,
 *  H5) behem podrzeni jina sipka: podrzena se pusti, nova stiskne,
 *  H6) zrus (menu / jina stranka): nic se nepodrzuje,
 *  H7) nahodne sekvence (5000x, jeden i vic tlacitek naraz): po pusteni vseho nic nezustane stisknute.
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b340_java.txt
 */
public class TestB340 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    /** Falesne jadro: co je stisknute + zaznam udalosti. */
    static final class Jadro implements NapDrzeniSmeru.Jadro {
        final LinkedHashSet<Integer> drzi = new LinkedHashSet<>();
        final List<String> log = new ArrayList<>();
        long t;
        @Override public void klavesa(int kod, boolean dolu) {
            if (dolu) drzi.add(kod); else drzi.remove(kod);
            log.add(t + (dolu ? " +" : " -") + jm(kod));
        }
    }

    /** Jmeno klavesy (KeyEvent.keyCodeToString je v android.jar jen "Stub!" - jmena z konstant KEYCODE_*). */
    static final java.util.HashMap<Integer, String> JMENA = new java.util.HashMap<>();
    static String jm(int k) {
        if (JMENA.isEmpty()) {
            for (java.lang.reflect.Field f : KeyEvent.class.getFields()) {
                try {
                    if (f.getName().startsWith("KEYCODE_") && f.getType() == int.class && java.lang.reflect.Modifier.isStatic(f.getModifiers()))
                        JMENA.putIfAbsent(f.getInt(null), f.getName());
                } catch (Throwable ignored) {}
            }
        }
        String n = JMENA.getOrDefault(k, "KEYCODE_" + k);
        return n.replace("KEYCODE_", "").replace("DPAD_", "").replace("BUTTON_", "");
    }

    static final int R = KeyEvent.KEYCODE_DPAD_RIGHT, L = KeyEvent.KEYCODE_DPAD_LEFT, A = KeyEvent.KEYCODE_BUTTON_A;

    static void ev(NapDrzeniSmeru d, Jadro j, long t, int kod, boolean dolu) {
        j.t = t;
        d.udalost(kod, NapDalkovy.jeSmer(kod), dolu, t, j);
    }

    static void tik(NapDrzeniSmeru d, Jadro j, long t) {
        j.t = t;
        d.tik(t, j);
    }

    public static void main(String[] a) throws Exception {
        // ---- R1 PS1 ----
        int[] ps1 = new int[10], ps1n = new int[10];
        for (int c = 0; c <= 9; c++) {
            ps1[c] = NapDalkovy.proHru(KeyEvent.KEYCODE_0 + c, NapDalkovy.KONZOLE_PS1);
            ps1n[c] = NapDalkovy.proHru(KeyEvent.KEYCODE_NUMPAD_0 + c, NapDalkovy.KONZOLE_PS1);
        }
        int[] ps1Cekam = {KeyEvent.KEYCODE_BUTTON_START, KeyEvent.KEYCODE_BUTTON_L1, KeyEvent.KEYCODE_BUTTON_Y, KeyEvent.KEYCODE_BUTTON_R1,
                KeyEvent.KEYCODE_BUTTON_X, KeyEvent.KEYCODE_BUTTON_SELECT, KeyEvent.KEYCODE_BUTTON_B, KeyEvent.KEYCODE_BUTTON_L2,
                KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_BUTTON_R2};
        StringBuilder s1 = new StringBuilder();
        for (int c = 0; c <= 9; c++) s1.append(c).append('=').append(jm(ps1[c])).append(' ');
        over(java.util.Arrays.equals(ps1, ps1Cekam) && java.util.Arrays.equals(ps1n, ps1Cekam),
                "R1) PS1 cisla (rada i numericka klavesnice): " + s1.toString().trim()
                        + " -> nap_ovladac.h: Y = TROJUHELNIK, X = CTVEREC, B = KOLECKO, A = KRIZEK");

        // ---- R2 SEGA ----
        int[] sega = new int[10];
        for (int c = 0; c <= 9; c++) sega[c] = NapDalkovy.proHru(KeyEvent.KEYCODE_0 + c, NapDalkovy.KONZOLE_SEGA);
        boolean radky = true;
        for (int r = 0; r < 3; r++) {
            radky &= sega[1 + r * 3] == KeyEvent.KEYCODE_BUTTON_X && sega[2 + r * 3] == KeyEvent.KEYCODE_BUTTON_A
                    && sega[3 + r * 3] == KeyEvent.KEYCODE_BUTTON_B;
        }
        over(radky && sega[0] == KeyEvent.KEYCODE_BUTTON_START,
                "R2) SEGA cisla: 1 4 7 = " + jm(sega[1]) + " (Sega A), 2 5 8 = " + jm(sega[2]) + " (Sega B), 3 6 9 = " + jm(sega[3])
                        + " (Sega C), 0 = " + jm(sega[0]));

        // ---- R3 ATARI + beze zmeny ----
        boolean atari = true;
        for (int c = 0; c <= 9; c++) atari &= NapDalkovy.proHru(KeyEvent.KEYCODE_0 + c, NapDalkovy.KONZOLE_ATARI) == -1;
        int[] jine = {KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE,
                KeyEvent.KEYCODE_MEDIA_PLAY, KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_PROG_RED, KeyEvent.KEYCODE_PROG_BLUE,
                KeyEvent.KEYCODE_CHANNEL_UP, KeyEvent.KEYCODE_CHANNEL_DOWN, KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_SETTINGS};
        boolean stejne = true;
        for (int k : jine)
            for (int kon = 1; kon <= 3; kon++) stejne &= NapDalkovy.proHru(k, kon) == NapDalkovy.proHru(k);
        over(atari && stejne && NapDalkovy.proHru(KeyEvent.KEYCODE_BACK) == -1
                        && NapDalkovy.proHru(KeyEvent.KEYCODE_MEDIA_PLAY) == KeyEvent.KEYCODE_BUTTON_START,
                "R3) ATARI cislice -1 (klavesy Atari, B302); sipky, OK, PLAY / MENU = START, barvy, kanaly jako B334 u vsech konzoli; "
                        + "ZPET neni klavesa hry");

        // ---- R4 ----
        boolean cis = NapDalkovy.cislice(KeyEvent.KEYCODE_7) == 7 && NapDalkovy.cislice(KeyEvent.KEYCODE_NUMPAD_0) == 0
                && NapDalkovy.cislice(KeyEvent.KEYCODE_DPAD_CENTER) == -1 && NapDalkovy.cislice(KeyEvent.KEYCODE_A) == -1;
        boolean smer = NapDalkovy.jeSmer(R) && NapDalkovy.jeSmer(KeyEvent.KEYCODE_DPAD_UP_LEFT)
                && !NapDalkovy.jeSmer(KeyEvent.KEYCODE_DPAD_CENTER) && !NapDalkovy.jeSmer(A);
        over(cis && smer, "R4) cislice (rada / numericka), smer krizku (OK neni smer)");

        // ---- H1: jeden klavesa naraz ----
        NapDrzeniSmeru d = new NapDrzeniSmeru();
        Jadro j = new Jadro();
        ev(d, j, 0, R, true);
        ev(d, j, 1000, R, false);        // ovladac "pustil" VPRAVO ...
        ev(d, j, 1000, A, true);         // ... a poslal OK (infra: v tytez chvili)
        boolean drziPriSkoku = j.drzi.contains(R) && j.drzi.contains(A) && d.podrzena() == R;
        ev(d, j, 1300, A, false);        // OK pusteno
        boolean cekaNaNavrat = d.cekaPusteni() && j.drzi.contains(R);
        ev(d, j, 1340, R, true);         // ovladac VPRAVO zase posila (clovek ho drzi)
        int udalostiPredKoncem = j.log.size();
        ev(d, j, 2000, R, false);
        over(drziPriSkoku && cekaNaNavrat && !d.cekaPusteni() && udalostiPredKoncem == 5 && j.drzi.isEmpty()
                        && j.log.get(1).equals("1000 -RIGHT") && j.log.get(2).equals("1000 +RIGHT") && d.podrzeni() == 1,
                "H1) jedno tlacitko naraz: drzim VPRAVO, OK -> jadro: " + j.log + " (pusteni a znovu stisk v tytez ms - hra nepozna;"
                        + " VPRAVO drzeno pres cely skok, navrat VPRAVO od ovladace beze zmeny)");

        // ---- H2: dve tlacitka naraz ----
        d = new NapDrzeniSmeru(); j = new Jadro();
        ev(d, j, 0, R, true); ev(d, j, 500, A, true); ev(d, j, 800, A, false); ev(d, j, 1000, R, false);
        over(j.log.toString().equals("[0 +RIGHT, 500 +A, 800 -A, 1000 -RIGHT]") && d.podrzeni() == 0 && j.drzi.isEmpty(),
                "H2) ovladac se dvema tlacitky naraz: " + j.log + " - nic se nepodrzi");

        // ---- H3: sipka behem skoku pustena ----
        d = new NapDrzeniSmeru(); j = new Jadro();
        ev(d, j, 0, R, true); ev(d, j, 1000, R, false); ev(d, j, 1000, A, true); ev(d, j, 1300, A, false);
        tik(d, j, 1300 + NapDrzeniSmeru.CEKANI_MS - 10);
        boolean jesteDrzi = j.drzi.contains(R);
        tik(d, j, 1300 + NapDrzeniSmeru.CEKANI_MS);
        over(jesteDrzi && j.drzi.isEmpty() && !d.cekaPusteni(),
                "H3) VPRAVO se po skoku nevratilo: drzeno jeste " + (NapDrzeniSmeru.CEKANI_MS - 10) + " ms po OK, pusteno v "
                        + (1300 + NapDrzeniSmeru.CEKANI_MS) + " (" + j.log.get(j.log.size() - 1) + ")");

        // ---- H4: samostatna tuknuti ----
        d = new NapDrzeniSmeru(); j = new Jadro();
        ev(d, j, 0, R, true); ev(d, j, 100, R, false); ev(d, j, 100 + NapDrzeniSmeru.OKNO_MS + 80, A, true);
        ev(d, j, 400, A, false);
        over(d.podrzeni() == 0 && j.drzi.isEmpty() && j.log.size() == 4,
                "H4) tuknuti VPRAVO, OK az za " + (NapDrzeniSmeru.OKNO_MS + 80) + " ms: " + j.log + " - nic se nepodrzi");

        // ---- H5: jina sipka behem podrzeni ----
        d = new NapDrzeniSmeru(); j = new Jadro();
        ev(d, j, 0, R, true); ev(d, j, 1000, R, false); ev(d, j, 1000, A, true); ev(d, j, 1200, A, false);
        ev(d, j, 1250, L, true);
        over(j.drzi.size() == 1 && j.drzi.contains(L) && !d.cekaPusteni() && d.podrzena() == -1,
                "H5) behem podrzeni VPRAVO prijde VLEVO: " + j.log.subList(4, j.log.size()) + " - VPRAVO pusteno, VLEVO stisknuto");

        // ---- H6: zrus ----
        d = new NapDrzeniSmeru(); j = new Jadro();
        ev(d, j, 0, R, true); ev(d, j, 1000, R, false); ev(d, j, 1000, A, true);
        d.zrus();
        over(d.podrzena() == -1 && !d.cekaPusteni(), "H6) menu konzole / jina stranka: zrus() - nic se nepodrzuje (klavesy v jadru pusti ovladacPustVse)");

        // ---- H7: nahodne sekvence ----
        Random rnd = new Random(340);
        int[] klavesy = {R, L, KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN, A, KeyEvent.KEYCODE_BUTTON_B, KeyEvent.KEYCODE_BUTTON_START};
        int zaseknuto = 0, podrzeniCelkem = 0;
        for (int beh = 0; beh < 5000; beh++) {
            boolean jednoNaraz = beh % 2 == 0;
            d = new NapDrzeniSmeru(); j = new Jadro();
            LinkedHashSet<Integer> drzim = new LinkedHashSet<>();      // co drzi OVLADAC (jeho stisky)
            long t = 0;
            for (int krok = 0; krok < 40; krok++) {
                t += rnd.nextInt(4) == 0 ? 0 : rnd.nextInt(300);
                int k = klavesy[rnd.nextInt(klavesy.length)];
                if (drzim.contains(k)) { drzim.remove(k); ev(d, j, t, k, false); }
                else {
                    if (jednoNaraz) for (Integer x : new ArrayList<>(drzim)) { drzim.remove(x); ev(d, j, t, x, false); }
                    drzim.add(k); ev(d, j, t, k, true);
                }
                if (d.cekaPusteni() && rnd.nextBoolean()) tik(d, j, t + rnd.nextInt(400));
            }
            for (Integer x : new ArrayList<>(drzim)) ev(d, j, t, x, false);
            tik(d, j, t + NapDrzeniSmeru.CEKANI_MS + 1);
            if (!j.drzi.isEmpty() || d.cekaPusteni()) zaseknuto++;
            podrzeniCelkem += d.podrzeni();
        }
        over(zaseknuto == 0 && podrzeniCelkem > 0,
                "H7) 5000 nahodnych sekvenci (pulka jedno tlacitko naraz, pulka vic): po pusteni vseho zaseknuto " + zaseknuto
                        + ", podrzeni sipky celkem " + podrzeniCelkem);

        out.append("VYSLEDEK: ").append(testu).append(" testu, ").append(chyb).append(" CHYBA\n");
        System.out.println("VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA");
        try (FileOutputStream f = new FileOutputStream(a.length > 0 ? a[0] : "VYSTUP_test_b340_java.txt")) { f.write(out.toString().getBytes("UTF-8")); }
        System.exit(chyb == 0 ? 0 : 1);
    }
}
