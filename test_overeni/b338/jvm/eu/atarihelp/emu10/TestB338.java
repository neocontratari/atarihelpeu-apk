package eu.atarihelp.emu10;

import android.view.KeyEvent;

import java.io.FileOutputStream;
import java.util.Arrays;

/**
 * B338: ANDROID TV - VYKON A MENU KONZOLE (JVM, skutecne tridy appky NapPravidla, NapDalkovy).
 *  V1) slabe zarizeni (AUTO -> LOW): TV TCL z logu (jen 32 bitu, 4 jadra, 1100 MHz) ano; S8 (64 bitu,
 *      8 jader, 2314 MHz) ne; 64 bitu do 1,5 GHz ano; 2 jadra ano; takt nezjisten + 64 bitu + 8 jader ne,
 *  V2) obraz Segy na sirku 1920x1080 = [235,32][1685,1047] - presne gameDst z logu TV (B337 kreslil procesorem
 *      totez, B338 to dela grafika pres setTransform), 1280x720 a 960x540 se zachovanim pomeru 320:224,
 *  V3) klepnuti na tlacitko stranky z Javy (menu konzole na TV): JavaScript, jen id tlacitka,
 *  V4) ZPET dalkoveho ovladace neni klavesa hry (dojde do onBackPressed = menu konzole), OK = A, MENU = START.
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b338_java.txt
 */
public class TestB338 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    public static void main(String[] a) throws Exception {
        boolean tv = NapPravidla.jeSlabeZarizeni(false, 4, 1100);
        boolean s8 = NapPravidla.jeSlabeZarizeni(true, 8, 2314);
        boolean pomaly64 = NapPravidla.jeSlabeZarizeni(true, 4, 1400);
        boolean dve = NapPravidla.jeSlabeZarizeni(true, 2, 2000);
        boolean nezname = NapPravidla.jeSlabeZarizeni(true, 8, 0);
        over(tv && !s8 && pomaly64 && dve && !nezname,
                "V1) slabe zarizeni: TV TCL (32 bitu, 4 jadra, 1100 MHz) " + tv + ", S8 (64 bitu, 8 jader, 2314 MHz) " + s8
                        + ", 64 bitu 1400 MHz " + pomaly64 + ", 2 jadra " + dve + ", takt nezjisten " + nezname);

        int[] v1080 = NapPravidla.segaVyrez(1920, 1080);
        int[] v720 = NapPravidla.segaVyrez(1280, 720);
        int[] v540 = NapPravidla.segaVyrez(960, 540);
        boolean pomer = Math.abs((double) v720[2] / v720[3] - 320.0 / 224.0) < 0.01 && Math.abs((double) v540[2] / v540[3] - 320.0 / 224.0) < 0.01;
        over(Arrays.equals(v1080, new int[]{235, 32, 1450, 1015}) && pomer && v720[3] <= 720 * 0.94 + 1 && v720[0] * 2 + v720[2] <= 1281,
                "V2) obraz Segy 1920x1080 = [" + v1080[0] + "," + v1080[1] + "][" + (v1080[0] + v1080[2]) + "," + (v1080[1] + v1080[3])
                        + "] (log TV: gameDst=[235,32][1685,1047]), 1280x720 " + Arrays.toString(v720) + ", 960x540 " + Arrays.toString(v540));

        String js = NapPravidla.tvKlik("segaBtnSaveState");
        over(js.equals("(function(){var b=document.getElementById('segaBtnSaveState');if(b&&!b.disabled){b.click();return 'ok';}return 'chybi';})()"),
                "V3) klepnuti z Javy: " + js);

        int zpet = NapDalkovy.proHru(KeyEvent.KEYCODE_BACK);
        int ok = NapDalkovy.proHru(KeyEvent.KEYCODE_DPAD_CENTER);
        int menu = NapDalkovy.proHru(KeyEvent.KEYCODE_MENU);
        over(zpet == -1 && ok == KeyEvent.KEYCODE_BUTTON_A && menu == KeyEvent.KEYCODE_BUTTON_START,
                "V4) dalkovy ve hre: ZPET -> " + zpet + " (neni klavesa hry -> menu konzole), OK -> A, MENU -> START");

        out.append("VYSLEDEK: ").append(testu).append(" testu, ").append(chyb).append(" CHYBA\n");
        System.out.println("VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA");
        try (FileOutputStream f = new FileOutputStream(a.length > 0 ? a[0] : "VYSTUP_test_b338_java.txt")) { f.write(out.toString().getBytes("UTF-8")); }
        System.exit(chyb == 0 ? 0 : 1);
    }
}
