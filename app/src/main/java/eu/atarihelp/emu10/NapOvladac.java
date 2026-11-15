package eu.atarihelp.emu10;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Handler;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * B320: HERNI OVLADAC - Retroid Pocket 6 (vestaveny), Bluetooth / USB gamepad.
 *
 * Rene: "proste to riskni ... je to pro nas vyzva Retroid pocket naslepo".
 * Uzivatel z fora: "jestli to pujde na Retro Pocket 6 s tim, ze by se na obrazovce
 * po pripojeni stahnul nas d-pad a najelo to na origo tlacitka".
 *
 * Tahle trida NEROZHODUJE, co tlacitko ve hre udela - to je v C++ (nap_ovladac.h,
 * spolecne pro PS1, Segu a Atari). Tady jen to, co umi jen Android:
 *  - ktera zarizeni jsou herni ovladace (InputManager, i pripojeni / odpojeni za behu)
 *    a VSE O NICH DO LOGU (jmeno, vyrobce, tlacitka, osy) - Retroid nemame, log od
 *    uzivatele z fora je jedine, z ceho pujde ladit,
 *  - je udalost z ovladace? (zdroj GAMEPAD / JOYSTICK, herni tlacitko, krizek ovladace),
 *  - osy: ktera osa je prava packa a ktera analogova spoust (podle rozsahu os -
 *    ovladace se lisi: Xbox Z/RZ = spouste, jine Z/RZ = prava packa, Retroid?),
 *  - dotykove ovladani: SCHOVAT, kdyz hraje ovladac (rezim AUTOMATICKY: hned po
 *    pripojeni skutecneho ovladace nebo pri prvnim stisku), UKAZAT po odpojeni nebo
 *    kdyz se nekdo dotkne mista tlacitka; v OPTIONS jde nastavit VZDY DOTYK / SKRYT DOTYK.
 *
 * Ciste casti (rozpoznani ovladace, rozlozeni os, stav schovani) jsou static a bez
 * Androidu - testuji se na PC (test_overeni/b320/jvm).
 */
final class NapOvladac implements InputManager.InputDeviceListener {

    /** Co ma udelat MainActivity (vola se na vlakne UI). */
    interface Posluchac {
        void ovladacLog(String radek);
        /** dotykove ovladani: stav 0 = videt, 1 = schovane (hraje ovladac), 2 = schovane vzdy */
        void ovladacUi(int stav, String jmeno, String duvod);
        /** ovladac odpojen - co drzel, uz nepusti (jadra: vse pustit) */
        void ovladacOdpojen();
    }

    // rezim z OPTIONS (SharedPreferences "nap_ovladac" / "rezim")
    static final int REZIM_AUTO = 0, REZIM_DOTYK = 1, REZIM_SKRYT = 2;
    // stav pro stranky a Atari (stranka PS1 / Sega: napOvladacStav, Atari: devCtl "ovladac:N")
    static final int UI_VIDET = 0, UI_SKRYTO = 1, UI_SKRYTO_VZDY = 2;

    // osy jako v nap_ovladac.h (O_* poradi, M_* maska)
    static final int O_LX = 0, O_LY = 1, O_RX = 2, O_RY = 3, O_HX = 4, O_HY = 5, O_L2 = 6, O_R2 = 7, O_POCET = 8;
    static final int M_HAT = 1, M_PRAVA = 2, M_SPOUSTE = 4, M_LEVA = 8;

    // =====================================================================
    //  CISTE FUNKCE (bez Androidu - testy na PC)
    // =====================================================================

    /** Herni tlacitko Androidu (KEYCODE_BUTTON_A .. BUTTON_MODE, BUTTON_1 .. BUTTON_16). */
    static boolean jeHerniTlacitko(int k) {
        return (k >= KeyEvent.KEYCODE_BUTTON_A && k <= KeyEvent.KEYCODE_BUTTON_MODE)
                || (k >= KeyEvent.KEYCODE_BUTTON_1 && k <= KeyEvent.KEYCODE_BUTTON_16);
    }
    /** Smerova klavesa (krizek) vcetne uhlopricek a stredu. */
    static boolean jeSmer(int k) {
        return (k >= KeyEvent.KEYCODE_DPAD_UP && k <= KeyEvent.KEYCODE_DPAD_CENTER)
                || (k >= KeyEvent.KEYCODE_DPAD_UP_LEFT && k <= KeyEvent.KEYCODE_DPAD_DOWN_RIGHT);
    }
    /** Udalost klavesy je z herniho ovladace? zdroj = KeyEvent.getSource(),
     *  zarizeniOvladac = zarizeni udalosti je herni ovladac (viz jeOvladac). Sipky
     *  z klavesnice (zdroj KEYBOARD, zarizeni klavesnice) NE - ty jdou jako drive. */
    static boolean klavesaZOvladace(int zdroj, int keyCode, boolean zarizeniOvladac) {
        if (jeHerniTlacitko(keyCode)) return true;
        final boolean zdrojOvladac = (zdroj & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (zdroj & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
        return jeSmer(keyCode) && (zdrojOvladac || zarizeniOvladac);
    }
    /** Zarizeni je herni ovladac (ne klavesnice, ne dotyk, ne virtualni)? */
    static boolean jeOvladac(int zdroje, boolean virtualni, boolean maA, boolean maB, boolean maOsyXY) {
        if (virtualni) return false;
        final boolean gamepad = (zdroje & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD;
        final boolean joystick = (zdroje & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
        if (!gamepad && !joystick) return false;
        // skutecny ovladac ma tlacitka A a B, nebo aspon packu a jedno z nich (telefon
        // muze mit "gamepad" zarizeni bez tlacitek - pak dotykove ovladani NESCHOVAVAT)
        return (maA && maB) || (joystick && maOsyXY && (maA || maB));
    }

    /**
     * B329: "Pro Controller" (Nintendo Switch, vyrobce 057e produkt 2009 - hlasi se tak i levne ovladace,
     * Reneho Battletron) na Androidu BEZ jeho tabulky tlacitek. Google ji pridal az pro Android 10
     * (Vendor_057e_Product_2009.kl, 2019); Android 9 pouzije obecnou tabulku (Generic.kl) a tlacitka
     * jdou pod jinymi jmeny - Rene na S8: "jedine, co nefunguje, je tlacitko START" (chodilo jako R2).
     * Poznava se podle tlacitka C: obecna tabulka ho ma (kod 0x132), tabulka Androidu 10+ ne.
     */
    static boolean switchStaraTabulka(int vyrobce, int produkt, boolean maTlacitkoC) {
        return vyrobce == 0x057e && produkt == 0x2009 && maTlacitkoC;
    }

    /**
     * B329: kod z obecne tabulky Androidu (Generic.kl) -> kod podle MISTA na ovladaci, stejne jako
     * tabulka Androidu 10+ (Vendor_057e_Product_2009.kl). Poradi tlacitek v hlaseni ovladace: B (spodni),
     * A (prave), Y (leve), X (horni), L, R, ZL, ZR, minus, plus, leva packa, prava packa, HOME, snimek.
     * Obecna tabulka z nich dela A, B, C, X, Y, Z, L1, R1, L2, R2, SELECT, START, MODE, THUMBL.
     * HOME zustava MODE (tabulka Androidu 10 z nej dela systemove HOME = appka by odesla na plochu).
     */
    static int klavesaSwitch(int k) {
        switch (k) {
            case KeyEvent.KEYCODE_BUTTON_C: return KeyEvent.KEYCODE_BUTTON_X;           // leve (Y na Switchi, ctverec)
            case KeyEvent.KEYCODE_BUTTON_X: return KeyEvent.KEYCODE_BUTTON_Y;           // horni (X na Switchi, trojuhelnik)
            case KeyEvent.KEYCODE_BUTTON_Y: return KeyEvent.KEYCODE_BUTTON_L1;          // L
            case KeyEvent.KEYCODE_BUTTON_Z: return KeyEvent.KEYCODE_BUTTON_R1;          // R
            case KeyEvent.KEYCODE_BUTTON_L1: return KeyEvent.KEYCODE_BUTTON_L2;         // ZL
            case KeyEvent.KEYCODE_BUTTON_R1: return KeyEvent.KEYCODE_BUTTON_R2;         // ZR
            case KeyEvent.KEYCODE_BUTTON_L2: return KeyEvent.KEYCODE_BUTTON_SELECT;     // minus = SELECT
            case KeyEvent.KEYCODE_BUTTON_R2: return KeyEvent.KEYCODE_BUTTON_START;      // plus = START
            case KeyEvent.KEYCODE_BUTTON_SELECT: return KeyEvent.KEYCODE_BUTTON_THUMBL; // stisk leve packy
            case KeyEvent.KEYCODE_BUTTON_START: return KeyEvent.KEYCODE_BUTTON_THUMBR;  // stisk prave packy
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return KeyEvent.KEYCODE_BUTTON_MODE;   // snimek obrazovky
            default: return k;      // A (spodni), B (prave), MODE (HOME), krizek - stejne
        }
    }

    /** B329: kratke jmeno herni klavesy do logu (bez Androidu - KeyEvent.keyCodeToString je az v telefonu). */
    static String jmenoKlavesy(int k) {
        switch (k) {
            case KeyEvent.KEYCODE_BUTTON_A: return "A";
            case KeyEvent.KEYCODE_BUTTON_B: return "B";
            case KeyEvent.KEYCODE_BUTTON_C: return "C";
            case KeyEvent.KEYCODE_BUTTON_X: return "X";
            case KeyEvent.KEYCODE_BUTTON_Y: return "Y";
            case KeyEvent.KEYCODE_BUTTON_Z: return "Z";
            case KeyEvent.KEYCODE_BUTTON_L1: return "L1";
            case KeyEvent.KEYCODE_BUTTON_R1: return "R1";
            case KeyEvent.KEYCODE_BUTTON_L2: return "L2";
            case KeyEvent.KEYCODE_BUTTON_R2: return "R2";
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return "L3";
            case KeyEvent.KEYCODE_BUTTON_THUMBR: return "R3";
            case KeyEvent.KEYCODE_BUTTON_START: return "START";
            case KeyEvent.KEYCODE_BUTTON_SELECT: return "SELECT";
            case KeyEvent.KEYCODE_BUTTON_MODE: return "MODE";
            case KeyEvent.KEYCODE_DPAD_UP: return "nahoru";
            case KeyEvent.KEYCODE_DPAD_DOWN: return "dolu";
            case KeyEvent.KEYCODE_DPAD_LEFT: return "vlevo";
            case KeyEvent.KEYCODE_DPAD_RIGHT: return "vpravo";
            case KeyEvent.KEYCODE_DPAD_CENTER: return "stred";
            default:
                if (k >= KeyEvent.KEYCODE_BUTTON_1 && k <= KeyEvent.KEYCODE_BUTTON_16) return "BUTTON_" + (k - KeyEvent.KEYCODE_BUTTON_1 + 1);
                return "kod " + k;
        }
    }

    /** Kde ovladac ma ktere osy (podle rozsahu os zarizeni). */
    static final class Rozlozeni {
        boolean leva, hat;
        int pravaX = -1, pravaY = -1;          // osa Androidu (AXIS_*) prave packy
        int[] l2 = new int[0], r2 = new int[0];   // osy spousti
        float[] l2Min = new float[0], l2Max = new float[0], r2Min = new float[0], r2Max = new float[0];
        int maska() {
            return (leva ? M_LEVA : 0) | (hat ? M_HAT : 0) | (pravaX >= 0 ? M_PRAVA : 0)
                    | ((l2.length > 0 || r2.length > 0) ? M_SPOUSTE : 0);
        }
        String popis() {
            StringBuilder b = new StringBuilder();
            b.append("leva packa ").append(leva ? "X/Y" : "ne")
             .append(", krizek ").append(hat ? "HAT" : "klavesy")
             .append(", prava packa ").append(pravaX >= 0 ? jmenoOsy(pravaX) + "/" + jmenoOsy(pravaY) : "ne")
             .append(", spoust L2 ").append(seznam(l2)).append(", R2 ").append(seznam(r2));
            return b.toString();
        }
        private static String seznam(int[] o) {
            if (o.length == 0) return "ne (jen tlacitko)";
            StringBuilder b = new StringBuilder();
            for (int i = 0; i < o.length; i++) { if (i > 0) b.append('+'); b.append(jmenoOsy(o[i])); }
            return b.toString();
        }
    }

    /** Rozlozeni z os zarizeni: osy[i] = AXIS_*, min[i] / max[i] = rozsah. */
    static Rozlozeni rozlozeni(int[] osy, float[] min, float[] max) {
        Map<Integer, float[]> r = new HashMap<>();
        for (int i = 0; i < osy.length; i++) r.put(osy[i], new float[]{min[i], max[i]});
        Rozlozeni z = new Rozlozeni();
        z.leva = r.containsKey(MotionEvent.AXIS_X) && r.containsKey(MotionEvent.AXIS_Y);
        z.hat = r.containsKey(MotionEvent.AXIS_HAT_X) && r.containsKey(MotionEvent.AXIS_HAT_Y);
        // prava packa: osa se zapornym rozsahem (-1..1); Z/RZ (vetsina ovladacu), jinak RX/RY
        final boolean zrz = dvojsmerna(r, MotionEvent.AXIS_Z) && dvojsmerna(r, MotionEvent.AXIS_RZ);
        final boolean rxry = dvojsmerna(r, MotionEvent.AXIS_RX) && dvojsmerna(r, MotionEvent.AXIS_RY);
        if (zrz) { z.pravaX = MotionEvent.AXIS_Z; z.pravaY = MotionEvent.AXIS_RZ; }
        else if (rxry) { z.pravaX = MotionEvent.AXIS_RX; z.pravaY = MotionEvent.AXIS_RY; }
        // spouste: LTRIGGER / BRAKE, RTRIGGER / GAS; Z / RZ jen kdyz nejsou packa a jdou 0..1 (Xbox)
        int[] kandL = {MotionEvent.AXIS_LTRIGGER, MotionEvent.AXIS_BRAKE, zrz ? -1 : MotionEvent.AXIS_Z};
        int[] kandR = {MotionEvent.AXIS_RTRIGGER, MotionEvent.AXIS_GAS, zrz ? -1 : MotionEvent.AXIS_RZ};
        z.l2 = spouste(r, kandL); z.r2 = spouste(r, kandR);
        z.l2Min = rozsah(r, z.l2, 0); z.l2Max = rozsah(r, z.l2, 1);
        z.r2Min = rozsah(r, z.r2, 0); z.r2Max = rozsah(r, z.r2, 1);
        return z;
    }
    private static boolean dvojsmerna(Map<Integer, float[]> r, int osa) {
        float[] m = r.get(osa);
        return m != null && m[0] < -0.5f && m[1] > 0.5f;
    }
    private static int[] spouste(Map<Integer, float[]> r, int[] kand) {
        int n = 0;
        int[] v = new int[kand.length];
        for (int k : kand) {
            if (k < 0) continue;
            float[] m = r.get(k);
            if (m == null || m[1] - m[0] < 0.01f) continue;
            // Z / RZ jako spoust jen 0..1 (jinak je to neco jineho)
            if ((k == MotionEvent.AXIS_Z || k == MotionEvent.AXIS_RZ) && m[0] < -0.01f) continue;
            v[n++] = k;
        }
        int[] o = new int[n];
        System.arraycopy(v, 0, o, 0, n);
        return o;
    }
    private static float[] rozsah(Map<Integer, float[]> r, int[] osy, int i) {
        float[] o = new float[osy.length];
        for (int k = 0; k < osy.length; k++) o[k] = r.get(osy[k])[i];
        return o;
    }

    /** Hodnoty os (hodnota(AXIS_*)) -> 8 cisel pro C++ (O_*). Vraci masku M_*. */
    interface Hodnoty { float osa(int axis); }
    static int nactiOsy(Rozlozeni z, Hodnoty h, float[] ven) {
        for (int i = 0; i < O_POCET; i++) ven[i] = 0f;
        if (z.leva) { ven[O_LX] = h.osa(MotionEvent.AXIS_X); ven[O_LY] = h.osa(MotionEvent.AXIS_Y); }
        if (z.hat) { ven[O_HX] = h.osa(MotionEvent.AXIS_HAT_X); ven[O_HY] = h.osa(MotionEvent.AXIS_HAT_Y); }
        if (z.pravaX >= 0) { ven[O_RX] = h.osa(z.pravaX); ven[O_RY] = h.osa(z.pravaY); }
        ven[O_L2] = spoust(z.l2, z.l2Min, z.l2Max, h);
        ven[O_R2] = spoust(z.r2, z.r2Min, z.r2Max, h);
        return z.maska();
    }
    // spoust 0..1 (nektere ovladace maji v klidu -1) - nejvic vychylena z jejich os
    private static float spoust(int[] osy, float[] min, float[] max, Hodnoty h) {
        float v = 0f;
        for (int i = 0; i < osy.length; i++) {
            final float d = max[i] - min[i];
            if (d < 0.01f) continue;
            float x = (h.osa(osy[i]) - min[i]) / d;
            if (x != x) x = 0f;
            v = Math.max(v, Math.max(0f, Math.min(1f, x)));
        }
        return v;
    }
    /** Packa nebo spoust vychylena tak, ze to je zamerny pohyb (ne sum)? */
    static boolean pohyb(float[] o, int maska) {
        if ((maska & M_LEVA) != 0 && (Math.abs(o[O_LX]) > 0.5f || Math.abs(o[O_LY]) > 0.5f)) return true;
        if ((maska & M_PRAVA) != 0 && (Math.abs(o[O_RX]) > 0.5f || Math.abs(o[O_RY]) > 0.5f)) return true;
        if ((maska & M_HAT) != 0 && (Math.abs(o[O_HX]) > 0.5f || Math.abs(o[O_HY]) > 0.5f)) return true;
        return (maska & M_SPOUSTE) != 0 && (o[O_L2] > 0.5f || o[O_R2] > 0.5f);
    }

    /** Stav schovani dotykoveho ovladani (cisty - bez Androidu). */
    static final class Schovani {
        int rezim = REZIM_AUTO;
        boolean skryto = false;       // AUTOMATICKY: ovladac hraje -> schovane
        /** Co ma byt videt (UI_*). */
        int ui() { return rezim == REZIM_SKRYT ? UI_SKRYTO_VZDY : (rezim == REZIM_DOTYK ? UI_VIDET : (skryto ? UI_SKRYTO : UI_VIDET)); }
        /** Ovladac se ozval (stisk / packa) nebo se prave pripojil. Vraci true, kdyz se UI meni. */
        boolean hraje() { if (rezim != REZIM_AUTO || skryto) return false; skryto = true; return true; }
        /** Dotyk na schovane tlacitko - stranka ho uz ukazala sama. */
        boolean dotyk() { if (!skryto) return false; skryto = false; return rezim == REZIM_AUTO; }
        /** Posledni ovladac odpojen. */
        boolean odpojen() { if (!skryto) return false; skryto = false; return rezim == REZIM_AUTO; }
        boolean nastav(int r, boolean pripojen) {
            final int pred = ui();
            rezim = (r < REZIM_AUTO || r > REZIM_SKRYT) ? REZIM_AUTO : r;
            if (rezim == REZIM_AUTO) skryto = pripojen;
            return ui() != pred;
        }
    }

    static String jmenoOsy(int a) {
        switch (a) {
            case MotionEvent.AXIS_X: return "X";
            case MotionEvent.AXIS_Y: return "Y";
            case MotionEvent.AXIS_Z: return "Z";
            case MotionEvent.AXIS_RX: return "RX";
            case MotionEvent.AXIS_RY: return "RY";
            case MotionEvent.AXIS_RZ: return "RZ";
            case MotionEvent.AXIS_HAT_X: return "HAT_X";
            case MotionEvent.AXIS_HAT_Y: return "HAT_Y";
            case MotionEvent.AXIS_LTRIGGER: return "LTRIGGER";
            case MotionEvent.AXIS_RTRIGGER: return "RTRIGGER";
            case MotionEvent.AXIS_THROTTLE: return "THROTTLE";
            case MotionEvent.AXIS_RUDDER: return "RUDDER";
            case MotionEvent.AXIS_WHEEL: return "WHEEL";
            case MotionEvent.AXIS_GAS: return "GAS";
            case MotionEvent.AXIS_BRAKE: return "BRAKE";
            default: return "osa" + a;
        }
    }
    static String jmenoZdroju(int z) {
        StringBuilder b = new StringBuilder();
        if ((z & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) b.append("GAMEPAD ");
        if ((z & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) b.append("JOYSTICK ");
        if ((z & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) b.append("DPAD ");
        if ((z & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD) b.append("KEYBOARD ");
        if ((z & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN) b.append("TOUCHSCREEN ");
        if ((z & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE) b.append("MOUSE ");
        return b.append(String.format(Locale.US, "(0x%08x)", z)).toString();
    }

    // =====================================================================
    //  ANDROID
    // =====================================================================
    private static final int[] KLAVESY_LOG = {
            KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_BUTTON_B, KeyEvent.KEYCODE_BUTTON_X, KeyEvent.KEYCODE_BUTTON_Y,
            KeyEvent.KEYCODE_BUTTON_L1, KeyEvent.KEYCODE_BUTTON_R1, KeyEvent.KEYCODE_BUTTON_L2, KeyEvent.KEYCODE_BUTTON_R2,
            KeyEvent.KEYCODE_BUTTON_THUMBL, KeyEvent.KEYCODE_BUTTON_THUMBR, KeyEvent.KEYCODE_BUTTON_START,
            KeyEvent.KEYCODE_BUTTON_SELECT, KeyEvent.KEYCODE_BUTTON_MODE, KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_BACK,
            KeyEvent.KEYCODE_BUTTON_C, KeyEvent.KEYCODE_BUTTON_Z};
    private static final String[] KLAVESY_JMENA = {"A", "B", "X", "Y", "L1", "R1", "L2", "R2", "L3", "R3", "START", "SELECT", "MODE", "DPAD", "BACK", "C", "Z"};
    private static final int I_TLACITKO_C = 15;     // index BUTTON_C v KLAVESY_LOG (B329: stara tabulka Pro Controlleru)
    /** B329: nejvic ruznych tlacitek jednoho ovladace do logu (kazde poprve) */
    private static final int MAX_TLACITEK_LOG = 40;
    private static final int[] OSY_VSE = {
            MotionEvent.AXIS_X, MotionEvent.AXIS_Y, MotionEvent.AXIS_Z, MotionEvent.AXIS_RX, MotionEvent.AXIS_RY,
            MotionEvent.AXIS_RZ, MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y, MotionEvent.AXIS_LTRIGGER,
            MotionEvent.AXIS_RTRIGGER, MotionEvent.AXIS_THROTTLE, MotionEvent.AXIS_RUDDER, MotionEvent.AXIS_WHEEL,
            MotionEvent.AXIS_GAS, MotionEvent.AXIS_BRAKE};

    /** Co vime o jednom zarizeni (podle id). */
    private static final class Zarizeni {
        String jmeno = "?";
        boolean ovladac;
        Rozlozeni rozlozeni = new Rozlozeni();
        int osyHlaseny = 0;          // bity os (index v OSY_VSE), ktere uz sly do logu
        boolean hralo;               // prisel z nej stisk / pohyb packou (i kdyz neni rozpoznany jako ovladac)
        boolean switchStara;         // B329: Pro Controller se starou tabulkou Androidu - kody prekladat podle mista
        java.util.HashSet<Integer> hlaseno = new java.util.HashSet<>();   // B329: tlacitka, ktera uz sla do logu
    }

    private final Context ctx;
    private final Handler ui;
    private final Posluchac posluchac;
    private InputManager im;
    private final Map<Integer, Zarizeni> zarizeni = new HashMap<>();
    private final Schovani schovani = new Schovani();
    private volatile String jmenoOvladace = "";
    private final java.util.HashSet<Integer> hlasenoBezZarizeni = new java.util.HashSet<>();   // B329

    NapOvladac(Context c, Handler ui, Posluchac p) {
        this.ctx = c;
        this.ui = ui;
        this.posluchac = p;
        try {
            SharedPreferences sp = c.getSharedPreferences("nap_ovladac", Context.MODE_PRIVATE);
            schovani.rezim = sp.getInt("rezim", REZIM_AUTO);
        } catch (Throwable ignored) {}
    }

    /** Zacit sledovat ovladace (onCreate). */
    void start() {
        try {
            im = (InputManager) ctx.getSystemService(Context.INPUT_SERVICE);
            if (im != null) im.registerInputDeviceListener(this, ui);
        } catch (Throwable t) { log("B320 OVLADAC: InputManager nejde - " + t); }
        int n = 0, ovl = 0;
        try {
            for (int id : InputDevice.getDeviceIds()) {
                Zarizeni z = poznej(id, "pri startu");
                n++;
                if (z != null && z.ovladac) ovl++;
            }
        } catch (Throwable t) { log("B320 OVLADAC: seznam zarizeni nejde - " + t); }
        log("B320 OVLADAC: pri startu " + n + " vstupnich zarizeni, z toho hernich ovladacu " + ovl
                + " | rezim dotykoveho ovladani: " + rezimText(schovani.rezim));
        prepocitej("start");
    }

    /** Konec (onDestroy). */
    void stop() {
        try { if (im != null) im.unregisterInputDeviceListener(this); } catch (Throwable ignored) {}
    }

    // ---- InputManager.InputDeviceListener (vlakno UI) ----
    @Override public void onInputDeviceAdded(int id) {
        Zarizeni z = poznej(id, "pripojeno");
        if (z != null && z.ovladac) prepocitej("pripojen " + z.jmeno);
    }
    @Override public void onInputDeviceRemoved(int id) {
        Zarizeni z;
        synchronized (this) { z = zarizeni.remove(id); }
        if (z != null && (z.ovladac || z.hralo)) {
            log("B320 OVLADAC odpojen: " + z.jmeno + " (id " + id + ")");
            if (posluchac != null) try { posluchac.ovladacOdpojen(); } catch (Throwable ignored) {}
            prepocitej("odpojen " + z.jmeno);
        }
    }
    @Override public void onInputDeviceChanged(int id) {
        Zarizeni z = poznej(id, "zmena");
        if (z != null) prepocitej("zmena " + z.jmeno);
    }

    /** Zjistit, co zarizeni umi, a zapsat do logu (jen herni ovladace podrobne). */
    private Zarizeni poznej(int id, String proc) {
        InputDevice d;
        try { d = InputDevice.getDevice(id); } catch (Throwable t) { d = null; }
        if (d == null) return null;
        Zarizeni z = new Zarizeni();
        Zarizeni byl;
        synchronized (this) { byl = zarizeni.get(id); if (byl != null) { z.hralo = byl.hralo; z.osyHlaseny = byl.osyHlaseny; z.hlaseno = byl.hlaseno; } }
        z.jmeno = String.valueOf(d.getName());
        final int zdroje = d.getSources();
        boolean[] ma = new boolean[KLAVESY_LOG.length];
        try {
            boolean[] m = d.hasKeys(KLAVESY_LOG);
            if (m != null) System.arraycopy(m, 0, ma, 0, Math.min(m.length, ma.length));
        } catch (Throwable ignored) {}
        // osy
        int[] osy = new int[OSY_VSE.length];
        float[] min = new float[OSY_VSE.length], max = new float[OSY_VSE.length];
        int n = 0;
        StringBuilder po = new StringBuilder();
        for (int a : OSY_VSE) {
            InputDevice.MotionRange r = null;
            try { r = d.getMotionRange(a, InputDevice.SOURCE_JOYSTICK); } catch (Throwable ignored) {}
            if (r == null) continue;
            osy[n] = a; min[n] = r.getMin(); max[n] = r.getMax(); n++;
            po.append(' ').append(jmenoOsy(a)).append(String.format(Locale.US, "[%.2f..%.2f flat %.2f]", r.getMin(), r.getMax(), r.getFlat()));
        }
        int[] o2 = new int[n]; float[] mi2 = new float[n], ma2 = new float[n];
        System.arraycopy(osy, 0, o2, 0, n); System.arraycopy(min, 0, mi2, 0, n); System.arraycopy(max, 0, ma2, 0, n);
        z.rozlozeni = rozlozeni(o2, mi2, ma2);
        boolean virt = false;
        try { virt = d.isVirtual(); } catch (Throwable ignored) {}
        z.ovladac = jeOvladac(zdroje, virt, ma[0], ma[1], z.rozlozeni.leva);
        int vyrobce = 0, produkt = 0;
        try { if (Build.VERSION.SDK_INT >= 19) { vyrobce = d.getVendorId(); produkt = d.getProductId(); } } catch (Throwable ignored) {}
        z.switchStara = switchStaraTabulka(vyrobce, produkt, ma[I_TLACITKO_C]);
        synchronized (this) { zarizeni.put(id, z); }
        // do logu herni ovladace (a vse s GAMEPAD / JOYSTICK); zmena zarizeni (nektere ovladace ji hlasi
        // treba pri zmene baterie) jen kdyz se zmenilo, jestli je to ovladac
        final boolean zapsat = !"zmena".equals(proc) || byl == null || byl.ovladac != z.ovladac;
        if (zapsat && (z.ovladac || (zdroje & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                || (zdroje & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)) {
            StringBuilder b = new StringBuilder();
            b.append("B320 OVLADAC ").append(proc).append(": \"").append(z.jmeno).append("\" id ").append(id);
            try { if (Build.VERSION.SDK_INT >= 19) b.append(String.format(Locale.US, ", vyrobce %04x produkt %04x", d.getVendorId(), d.getProductId())); } catch (Throwable ignored) {}
            try { if (Build.VERSION.SDK_INT >= 29) b.append(d.isExternal() ? ", externi" : ", vestaveny"); } catch (Throwable ignored) {}
            b.append(", zdroje ").append(jmenoZdroju(zdroje)).append(virt ? ", VIRTUALNI" : "");
            b.append(" | tlacitka:");
            for (int i = 0; i < ma.length; i++) if (ma[i]) b.append(' ').append(KLAVESY_JMENA[i]);
            b.append(" | osy:").append(po.length() > 0 ? po.toString() : " zadne");
            b.append(" | ").append(z.rozlozeni.popis());
            b.append(" | ").append(z.ovladac ? "HERNI OVLADAC - dotykove ovladani se muze schovat" : "NENI herni ovladac (chybi tlacitka A/B) - dotyk zustava");
            if (z.switchStara)
                b.append(" | B329: Pro Controller se STAROU tabulkou tlacitek Androidu ").append(Build.VERSION.RELEASE)
                 .append(" - appka tlacitka preklada podle mista (START, SELECT, trojuhelnik, ctverec, L1/R1, L2/R2, L3/R3)");
            log(b.toString());
        }
        return z;
    }

    /** Je nejaky skutecny herni ovladac pripojeny? Prepocitat schovani. */
    private void prepocitej(String proc) {
        String jm = "";
        synchronized (this) {
            for (Zarizeni z : zarizeni.values()) if (z.ovladac || z.hralo) { jm = z.jmeno; break; }
        }
        final boolean pripojen = !jm.isEmpty();
        final boolean bylo = !jmenoOvladace.isEmpty();
        jmenoOvladace = jm;
        boolean zmena = false;
        synchronized (this) {
            if (pripojen && !bylo) zmena = schovani.hraje();          // pripojen skutecny ovladac -> schovat (AUTO)
            else if (!pripojen && bylo) zmena = schovani.odpojen();    // posledni odpojen -> ukazat
        }
        if (zmena || "start".equals(proc)) oznam(proc);
    }

    private void oznam(String duvod) {
        final int st;
        synchronized (this) { st = schovani.ui(); }
        if (posluchac != null) posluchac.ovladacUi(st, jmenoOvladace, duvod);
    }

    private Zarizeni zarizeniUdalosti(int id) {
        Zarizeni z;
        synchronized (this) { z = zarizeni.get(id); }
        if (z == null) z = poznej(id, "prvni udalost");     // zarizeni, ktere jsme jeste nevideli
        return z;
    }

    // ---- udalosti (vlakno UI, MainActivity.dispatchKeyEvent / dispatchGenericMotionEvent) ----

    /** Je udalost klavesy z herniho ovladace? */
    boolean zOvladace(KeyEvent e) {
        final int k = e.getKeyCode();
        if (jeHerniTlacitko(k)) return true;
        if (!jeSmer(k)) return false;
        Zarizeni z = zarizeniUdalosti(e.getDeviceId());
        return klavesaZOvladace(e.getSource(), k, z != null && z.ovladac);
    }

    /** Je pohyb os z joysticku / ovladace? */
    static boolean jeJoystick(MotionEvent e) {
        return (e.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                && e.getActionMasked() == MotionEvent.ACTION_MOVE;
    }

    /** Osy udalosti -> ven[8] pro C++; vraci masku. Prvni pouziti kazde osy do logu. */
    int osy(final MotionEvent e, float[] ven) {
        Zarizeni z = zarizeniUdalosti(e.getDeviceId());
        Rozlozeni r = z != null ? z.rozlozeni : rozlozeniBezZarizeni();
        final int maska = nactiOsy(r, e::getAxisValue, ven);
        if (z != null) {
            for (int i = 0; i < OSY_VSE.length; i++) {
                if ((z.osyHlaseny & (1 << i)) != 0) continue;
                final float v = e.getAxisValue(OSY_VSE[i]);
                if (Math.abs(v) > 0.5f) {
                    z.osyHlaseny |= 1 << i;
                    log(String.format(Locale.US, "B320 OVLADAC \"%s\": osa %s poprve %.2f", z.jmeno, jmenoOsy(OSY_VSE[i]), v));
                }
            }
        }
        if (pohyb(ven, maska)) ovladacHraje(z);
        return maska;
    }
    private static Rozlozeni rozlozeniBezZarizeni() {
        Rozlozeni r = new Rozlozeni();
        r.leva = true; r.hat = true;
        return r;
    }

    /**
     * B329: kod klavesy podle MISTA na ovladaci - Pro Controller se starou tabulkou Androidu prelozi
     * (klavesaSwitch), ostatni ovladace beze zmeny. MainActivity ho posila do PS1 / Segy / Atari / menu.
     */
    int kodPodleMista(KeyEvent e) {
        final int k = e.getKeyCode();
        Zarizeni z = zarizeniUdalosti(e.getDeviceId());
        return (z != null && z.switchStara) ? klavesaSwitch(k) : k;
    }

    /**
     * Klavesa ovladace zpracovana (kod = podle mista - kodPodleMista, kam = kam sla, t = logicka tlacitka
     * z C++, 0 = nic). B329: kazde tlacitko kazdeho ovladace POPRVE do logu, s kodem Androidu a prekladem
     * (do B328 jen prvnich 12 stisku vubec - Reneho hledani START uz v logu nebylo).
     */
    void klavesa(KeyEvent e, int kod, String kam, int t) {
        final boolean dolu = e.getAction() == KeyEvent.ACTION_DOWN;
        Zarizeni z = zarizeniUdalosti(e.getDeviceId());
        if (dolu && e.getRepeatCount() == 0) {
            final int puvodni = e.getKeyCode();
            boolean poprve;
            synchronized (this) {
                java.util.HashSet<Integer> h = z != null ? z.hlaseno : hlasenoBezZarizeni;
                poprve = h.size() < MAX_TLACITEK_LOG && h.add(puvodni);
            }
            if (poprve)
                log("B329 OVLADAC tlacitko " + jmenoKlavesy(puvodni) + " (kod " + puvodni + ") z \"" + (z != null ? z.jmeno : "?") + "\""
                        + (kod != puvodni ? " -> podle mista " + jmenoKlavesy(kod) : "")
                        + " -> " + kam + (t == 0 ? ": nic (jde dal Androidu)" : ""));
            if (t != 0) ovladacHraje(z);
        }
    }

    private void ovladacHraje(Zarizeni z) {
        if (z != null && !z.hralo) {
            z.hralo = true;
            if (jmenoOvladace.isEmpty()) jmenoOvladace = z.jmeno;
        }
        boolean zmena;
        synchronized (this) { zmena = schovani.hraje(); }
        if (zmena) oznam("stisk na ovladaci");
    }

    // ---- stranky / Atari / OPTIONS ----

    /** Dotyk na schovane tlacitko ukazal dotykove ovladani (stranka PS1 / Sega, Atari). */
    void dotyk(String kde) {
        boolean zmena;
        synchronized (this) { zmena = schovani.dotyk(); }
        if (zmena) {
            log("B320 OVLADAC: dotyk na schovane tlacitko (" + kde + ") - dotykove ovladani zase videt, dalsi stisk na ovladaci ho schova");
            oznam("dotyk " + kde);
        }
    }

    synchronized int ui() { return schovani.ui(); }
    synchronized int rezim() { return schovani.rezim; }
    String jmeno() { return jmenoOvladace; }

    void nastavRezim(int r) {
        boolean zmena;
        synchronized (this) { zmena = schovani.nastav(r, !jmenoOvladace.isEmpty()); }
        try { ctx.getSharedPreferences("nap_ovladac", Context.MODE_PRIVATE).edit().putInt("rezim", rezim()).apply(); } catch (Throwable ignored) {}
        log("B320 OVLADAC: rezim dotykoveho ovladani " + rezimText(rezim()) + (jmenoOvladace.isEmpty() ? " (zadny ovladac)" : " (ovladac \"" + jmenoOvladace + "\")"));
        if (zmena) oznam("OPTIONS");
    }

    static String rezimText(int r) {
        return r == REZIM_DOTYK ? "VZDY DOTYK" : (r == REZIM_SKRYT ? "SKRYT DOTYK" : "AUTOMATICKY");
    }

    private void log(String s) {
        if (posluchac != null) try { posluchac.ovladacLog(s); } catch (Throwable ignored) {}
    }
}
