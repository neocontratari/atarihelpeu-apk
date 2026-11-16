package eu.atarihelp.emu10;

import android.view.InputDevice;
import android.view.KeyEvent;

/**
 * B334: dalkovy ovladac TV - prevod klaves (NapDalkovy, cisty Java; konstanty z android.jar).
 *  D) co je dalkovy ovladac: zdroj DPAD / klavesnice bez pismen ANO; klavesnice s pismeny, herni
 *     ovladac, virtualni klavesnice NE,
 *  H) hry: krizek = smer, OK / Enter = A (spodni), PLAY / MENU = START, barevna = B / X / Y / SELECT,
 *     pretaceni / kanaly = L1 / R1, ostatni nic (ZPET, HOME, hlasitost zustanou Androidu),
 *  M) hlavni menu: vlevo / vpravo listuje, OK spusti, nahoru / MENU = OPTIONS, dolu jen spotrebovat,
 *  P) uvodni demo: OK / PLAY / START / A / mezernik preskoci, sipky ani ZPET ne.
 */
public final class TestB334 {
    static int ok = 0, chyb = 0;

    static void over(boolean b, String co) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + co);
    }

    static String j(int k) { return k < 0 ? "nic" : KeyEvent.keyCodeToString(k).replace("KEYCODE_", ""); }

    public static void main(String[] a) {
        System.out.println("B334 - dalkovy ovladac Android TV: prevod klaves (NapDalkovy)");
        final int DPAD = InputDevice.SOURCE_DPAD, KL = InputDevice.SOURCE_KEYBOARD, GP = InputDevice.SOURCE_GAMEPAD;
        over(NapDalkovy.zDalkoveho(DPAD | KL, InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC, false, false)
                && NapDalkovy.zDalkoveho(DPAD, InputDevice.KEYBOARD_TYPE_NONE, false, false)
                && NapDalkovy.zDalkoveho(KL, InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC, false, false)
                && NapDalkovy.zDalkoveho(DPAD | KL, -1, false, false),
                "D1) dalkovy ovladac TV (zdroj DPAD / klavesnice bez pismen, i nezname zarizeni) = ANO");
        over(!NapDalkovy.zDalkoveho(KL, InputDevice.KEYBOARD_TYPE_ALPHABETIC, false, false)
                && !NapDalkovy.zDalkoveho(DPAD | GP, InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC, false, true)
                && !NapDalkovy.zDalkoveho(DPAD | KL, InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC, true, false)
                && !NapDalkovy.zDalkoveho(InputDevice.SOURCE_TOUCHSCREEN, InputDevice.KEYBOARD_TYPE_NONE, false, false),
                "D2) klavesnice s pismeny (ma vlastni rozlozeni B308 / B320), herni ovladac (NapOvladac), virtualni klavesnice, dotyk = NE");

        int[][] hra = {
            {KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_UP}, {KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_DOWN},
            {KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_LEFT}, {KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT},
            {KeyEvent.KEYCODE_DPAD_DOWN_LEFT, KeyEvent.KEYCODE_DPAD_DOWN_LEFT},
            {KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_BUTTON_A}, {KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_BUTTON_A},
            {KeyEvent.KEYCODE_NUMPAD_ENTER, KeyEvent.KEYCODE_BUTTON_A},
            {KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, KeyEvent.KEYCODE_BUTTON_START}, {KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_BUTTON_START},
            {KeyEvent.KEYCODE_PROG_RED, KeyEvent.KEYCODE_BUTTON_B}, {KeyEvent.KEYCODE_PROG_GREEN, KeyEvent.KEYCODE_BUTTON_X},
            {KeyEvent.KEYCODE_PROG_YELLOW, KeyEvent.KEYCODE_BUTTON_Y}, {KeyEvent.KEYCODE_PROG_BLUE, KeyEvent.KEYCODE_BUTTON_SELECT},
            {KeyEvent.KEYCODE_MEDIA_REWIND, KeyEvent.KEYCODE_BUTTON_L1}, {KeyEvent.KEYCODE_CHANNEL_UP, KeyEvent.KEYCODE_BUTTON_R1},
            {KeyEvent.KEYCODE_BACK, -1}, {KeyEvent.KEYCODE_HOME, -1}, {KeyEvent.KEYCODE_VOLUME_UP, -1}, {KeyEvent.KEYCODE_A, -1},
        };
        StringBuilder sb = new StringBuilder();
        boolean vse = true;
        for (int[] p : hra) {
            final int r = NapDalkovy.proHru(p[0]);
            if (r != p[1]) { vse = false; sb.append(" SPATNE ").append(j(p[0])).append("->").append(j(r)); }
        }
        over(vse, "H1) hry (" + hra.length + " klaves): krizek = smer, OK / Enter = A, PLAY / MENU = START, cervena / zelena / zluta / modra = B / X / Y / SELECT,"
                + " pretaceni / kanaly = L1 / R1, ZPET / HOME / hlasitost / pismena = nic" + sb);

        int[][] menu = {
            {KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_LEFT}, {KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT},
            {KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_BUTTON_A}, {KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_BUTTON_A},
            {KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_BUTTON_SELECT}, {KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_BUTTON_SELECT},
            {KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_BUTTON_X},
            {KeyEvent.KEYCODE_MEDIA_REWIND, KeyEvent.KEYCODE_BUTTON_L2}, {KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, KeyEvent.KEYCODE_BUTTON_R2},
            {KeyEvent.KEYCODE_BACK, -1}, {KeyEvent.KEYCODE_PROG_RED, -1},
        };
        sb = new StringBuilder();
        vse = true;
        for (int[] p : menu) {
            final int r = NapDalkovy.proMenu(p[0]);
            if (r != p[1]) { vse = false; sb.append(" SPATNE ").append(j(p[0])).append("->").append(j(r)); }
        }
        over(vse, "M1) hlavni menu: vlevo / vpravo = listovat (krizek gamepadu), OK / Enter = A (spustit), nahoru / MENU = SELECT (OPTIONS),"
                + " dolu = X (jen spotrebovat), pretaceni = prvni / posledni karta, ZPET = Android" + sb);

        boolean d = NapDalkovy.preskociDemo(KeyEvent.KEYCODE_DPAD_CENTER) && NapDalkovy.preskociDemo(KeyEvent.KEYCODE_ENTER)
                && NapDalkovy.preskociDemo(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) && NapDalkovy.preskociDemo(KeyEvent.KEYCODE_BUTTON_A)
                && NapDalkovy.preskociDemo(KeyEvent.KEYCODE_BUTTON_START) && NapDalkovy.preskociDemo(KeyEvent.KEYCODE_SPACE)
                && !NapDalkovy.preskociDemo(KeyEvent.KEYCODE_DPAD_LEFT) && !NapDalkovy.preskociDemo(KeyEvent.KEYCODE_BACK)
                && !NapDalkovy.preskociDemo(KeyEvent.KEYCODE_VOLUME_DOWN);
        over(d, "P1) uvodni demo: OK / Enter / PLAY / A / START / mezernik preskoci; sipky, ZPET, hlasitost ne");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
