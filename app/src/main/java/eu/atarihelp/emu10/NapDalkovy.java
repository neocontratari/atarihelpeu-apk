package eu.atarihelp.emu10;

import android.app.UiModeManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.view.InputDevice;
import android.view.KeyEvent;

/**
 * B334: DALKOVY OVLADAC TELEVIZE (Android TV) - Rene: "aby se to dalo ovladat ovladacem od TV nebo
 * gamepadem". Herni ovladac (gamepad) appka umi od B320 (NapOvladac + nap_ovladac.h). Dalkovy
 * ovladac TV ale neni "herni ovladac" (zdroj DPAD / klavesnice bez pismen) - jeho krizek a OK sly do
 * WebView a v hlavnim menu (C++), v uvodnim demu ani ve hrach nedelaly nic.
 *
 * Tady je jen PREVOD klaves dalkoveho ovladace (cisty Java, testuje se v JVM):
 *  - HRY (PS1, Sega, Atari): na klavesy gamepadu - co udelaji, rozhoduje C++ jako u gamepadu
 *    (krizek = smer, OK = spodni tlacitko A, PLAY/PAUSE nebo MENU = START, barevna tlacitka
 *    cervena / zelena / zluta / modra = B / X / Y / SELECT, pretaceni a kanaly = L1 / R1),
 *  - HLAVNI MENU (C++ karusel): vlevo / vpravo listuje, OK spusti, nahoru nebo MENU = OPTIONS,
 *  - UVODNI DEMO: OK / PLAY / START preskoci (dotykove tlacitko SKIP na TV nejde stisknout),
 *  - stranky (OPTIONS, napoveda, seznamy her ...) dostane WebView - sipky a OK tam resi
 *    assets/spolecne/nap_tv_ovladani.js.
 * Klavesnice s pismeny (Bluetooth / USB) sem nepatri - ta ma vlastni rozlozeni (B308 / B320).
 */
final class NapDalkovy {
    private NapDalkovy() {}

    /** Klavesa dalkoveho ovladace -> klavesa gamepadu pro hru (nap_ovladac.h), -1 = neni pro hru. */
    static int proHru(int k) {
        switch (k) {
            case KeyEvent.KEYCODE_DPAD_UP: case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_LEFT: case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_DPAD_UP_LEFT: case KeyEvent.KEYCODE_DPAD_UP_RIGHT:
            case KeyEvent.KEYCODE_DPAD_DOWN_LEFT: case KeyEvent.KEYCODE_DPAD_DOWN_RIGHT:
                return k;
            case KeyEvent.KEYCODE_DPAD_CENTER: case KeyEvent.KEYCODE_ENTER: case KeyEvent.KEYCODE_NUMPAD_ENTER:
                return KeyEvent.KEYCODE_BUTTON_A;
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE: case KeyEvent.KEYCODE_MEDIA_PLAY: case KeyEvent.KEYCODE_MEDIA_PAUSE:
            case KeyEvent.KEYCODE_MENU:
                return KeyEvent.KEYCODE_BUTTON_START;
            case KeyEvent.KEYCODE_PROG_RED:    return KeyEvent.KEYCODE_BUTTON_B;
            case KeyEvent.KEYCODE_PROG_GREEN:  return KeyEvent.KEYCODE_BUTTON_X;
            case KeyEvent.KEYCODE_PROG_YELLOW: return KeyEvent.KEYCODE_BUTTON_Y;
            case KeyEvent.KEYCODE_PROG_BLUE:   return KeyEvent.KEYCODE_BUTTON_SELECT;
            case KeyEvent.KEYCODE_MEDIA_REWIND: case KeyEvent.KEYCODE_CHANNEL_DOWN: case KeyEvent.KEYCODE_PAGE_DOWN:
                return KeyEvent.KEYCODE_BUTTON_L1;
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD: case KeyEvent.KEYCODE_CHANNEL_UP: case KeyEvent.KEYCODE_PAGE_UP:
                return KeyEvent.KEYCODE_BUTTON_R1;
            default:
                return -1;
        }
    }

    /** Klavesa dalkoveho ovladace -> klavesa gamepadu pro hlavni menu (MainActivity.ovladacMenu), -1 = nic. */
    static int proMenu(int k) {
        switch (k) {
            case KeyEvent.KEYCODE_DPAD_LEFT:  return KeyEvent.KEYCODE_DPAD_LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return KeyEvent.KEYCODE_DPAD_RIGHT;
            case KeyEvent.KEYCODE_DPAD_CENTER: case KeyEvent.KEYCODE_ENTER: case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE: case KeyEvent.KEYCODE_MEDIA_PLAY:
                return KeyEvent.KEYCODE_BUTTON_A;
            case KeyEvent.KEYCODE_DPAD_UP: case KeyEvent.KEYCODE_MENU: case KeyEvent.KEYCODE_PROG_BLUE:
                return KeyEvent.KEYCODE_BUTTON_SELECT;          // OPTIONS (ozubene kolecko je nahore)
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return KeyEvent.KEYCODE_BUTTON_X;               // nic (jen spotrebovat - focus nesmi utect)
            case KeyEvent.KEYCODE_MEDIA_REWIND: case KeyEvent.KEYCODE_PAGE_UP:
                return KeyEvent.KEYCODE_BUTTON_L2;              // prvni karta
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD: case KeyEvent.KEYCODE_PAGE_DOWN:
                return KeyEvent.KEYCODE_BUTTON_R2;              // posledni karta
            default:
                return -1;
        }
    }

    /** Preskoci uvodni demo (OK, PLAY, START, mezernik)? */
    static boolean preskociDemo(int k) {
        switch (k) {
            case KeyEvent.KEYCODE_DPAD_CENTER: case KeyEvent.KEYCODE_ENTER: case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_BUTTON_A: case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE: case KeyEvent.KEYCODE_MEDIA_PLAY: case KeyEvent.KEYCODE_SPACE:
                return true;
            default:
                return false;
        }
    }

    /**
     * Je udalost z dalkoveho ovladace (ne z klavesnice s pismeny, ne z herniho ovladace)?
     * zdroj = KeyEvent.getSource(), typKlavesnice = InputDevice.getKeyboardType() (-1 = zarizeni nezname),
     * virtualni = zarizeni je virtualni klavesnice (IME, adb).
     */
    static boolean zDalkoveho(int zdroj, int typKlavesnice, boolean virtualni, boolean herniOvladac) {
        if (herniOvladac || virtualni) return false;
        final boolean dpad = (zdroj & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
        final boolean klavesnice = (zdroj & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD;
        if (typKlavesnice == InputDevice.KEYBOARD_TYPE_ALPHABETIC) return false;   // klavesnice s pismeny
        return dpad || klavesnice;
    }

    /** Android TV: rezim televize (UiModeManager) nebo funkce leanback. */
    static boolean jeTv(Context c) {
        try {
            final UiModeManager um = (UiModeManager) c.getSystemService(Context.UI_MODE_SERVICE);
            if (um != null && um.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) return true;
        } catch (Throwable ignored) {}
        try {
            final PackageManager pm = c.getPackageManager();
            return pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK) || pm.hasSystemFeature("android.software.leanback_only");
        } catch (Throwable ignored) {}
        return false;
    }
}
