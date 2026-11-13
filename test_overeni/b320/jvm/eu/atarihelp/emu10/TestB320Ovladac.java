// B320: HERNI OVLADAC v JVM na PC - skutecne tridy z appky:
//  A) NapOvladac (ciste casti bez Androidu, android.jar jen na classpath): co je herni ovladac,
//     ktera osa je prava packa a ktera spoust (podle rozsahu os - Xbox, DualShock 4, 8BitDo, ovladac
//     s pravou packou na RX/RY), cteni os, schovani dotykoveho ovladani (AUTOMATICKY / VZDY DOTYK /
//     SKRYT DOTYK, dotyk na schovane tlacitko, odpojeni)
//  B) NativePs1CoreBridge se SKUTECNYM nap_ps1_native.cpp (libnapps1core.so z ../../b308/ps1_harness.cpp),
//     -Xcheck:jni: 32 native metod (B319 29 + 3 ovladac), ovladacKlavesaSafe / ovladacOsySafe /
//     ovladacPustSafe za behu "hry", denik PS1
//  C) mosty Segy a Atari bez knihovny: obaly *Safe nespadnou (vraci 0)
// Spusteni: viz ../vse.sh (javac -cp android.jar, java -Xcheck:jni -Djava.library.path=...)
package eu.atarihelp.emu10;

import java.io.File;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class TestB320Ovladac {
    static int chyb = 0, ok = 0;

    static void over(boolean v, String co) {
        System.out.println((v ? "OK     " : "CHYBA  ") + co);
        if (v) ok++; else chyb++;
    }

    // MotionEvent.AXIS_* a InputDevice.SOURCE_* (pevna cisla API Androidu)
    static final int X = 0, Y = 1, Z = 11, RX = 12, RY = 13, RZ = 14, HX = 15, HY = 16, LT = 17, RT = 18, GAS = 22, BRAKE = 23;
    static final int SRC_GAMEPAD = 0x00000401, SRC_JOYSTICK = 0x01000010, SRC_KEYBOARD = 0x00000101, SRC_DPAD = 0x00000201;

    static NapOvladac.Rozlozeni roz(int[] osy, float[][] r) {
        float[] mi = new float[osy.length], ma = new float[osy.length];
        for (int i = 0; i < osy.length; i++) { mi[i] = r[i][0]; ma[i] = r[i][1]; }
        return NapOvladac.rozlozeni(osy, mi, ma);
    }
    static float[][] R(String s) {      // "b" = -1..1, "u" = 0..1
        float[][] r = new float[s.length()][];
        for (int i = 0; i < s.length(); i++) r[i] = s.charAt(i) == 'b' ? new float[]{-1f, 1f} : new float[]{0f, 1f};
        return r;
    }

    public static void main(String[] a) throws Exception {
        String prac = a.length > 0 ? a[0] : "/tmp/nap_b320_jvm";
        System.out.println("B320 - herni ovladac v JVM: NapOvladac (ciste casti) + NativePs1CoreBridge se skutecnym nap_ps1_native.cpp");

        // ================= A) NapOvladac =================
        over(NapOvladac.jeHerniTlacitko(96) && NapOvladac.jeHerniTlacitko(110) && NapOvladac.jeHerniTlacitko(188) && NapOvladac.jeHerniTlacitko(203)
                        && !NapOvladac.jeHerniTlacitko(4) && !NapOvladac.jeHerniTlacitko(66) && !NapOvladac.jeHerniTlacitko(19),
                "A1) herni tlacitka: BUTTON_A .. MODE, BUTTON_1 .. 16; ZPET, Enter a sipky ne");
        over(NapOvladac.klavesaZOvladace(SRC_KEYBOARD, 96, false)
                        && NapOvladac.klavesaZOvladace(SRC_GAMEPAD | SRC_DPAD, 19, false)
                        && NapOvladac.klavesaZOvladace(SRC_DPAD, 22, true)
                        && !NapOvladac.klavesaZOvladace(SRC_KEYBOARD, 19, false)
                        && !NapOvladac.klavesaZOvladace(SRC_DPAD, 21, false)
                        && !NapOvladac.klavesaZOvladace(SRC_GAMEPAD, 66, true),
                "A2) udalost z ovladace: herni tlacitko vzdy; sipka jen ze zdroje GAMEPAD / JOYSTICK nebo ze zarizeni, ktere je ovladac (sipky klavesnice jdou jako drive); Enter ne");
        over(NapOvladac.jeOvladac(SRC_GAMEPAD | SRC_JOYSTICK, false, true, true, true)
                        && NapOvladac.jeOvladac(SRC_GAMEPAD, false, true, true, false)
                        && NapOvladac.jeOvladac(SRC_JOYSTICK, false, true, false, true)
                        && !NapOvladac.jeOvladac(SRC_GAMEPAD | SRC_JOYSTICK, true, true, true, true)
                        && !NapOvladac.jeOvladac(SRC_KEYBOARD, false, true, true, false)
                        && !NapOvladac.jeOvladac(SRC_GAMEPAD, false, false, false, false),
                "A3) herni ovladac: GAMEPAD / JOYSTICK s tlacitky A a B (nebo packa + A); virtualni, klavesnice a \"gamepad\" telefonu bez tlacitek NE (dotyk zustava)");
        NapOvladac.Rozlozeni xbox = roz(new int[]{X, Y, Z, RZ, HX, HY, LT, RT, BRAKE, GAS}, R("bbbbbbuuuu"));
        over(xbox.leva && xbox.hat && xbox.pravaX == Z && xbox.pravaY == RZ && xbox.l2.length == 2 && xbox.r2.length == 2 && xbox.maska() == 15,
                "A4) Xbox / 8BitDo: prava packa Z/RZ, spouste LTRIGGER+BRAKE / RTRIGGER+GAS, krizek HAT - " + xbox.popis());
        NapOvladac.Rozlozeni ds4 = roz(new int[]{X, Y, Z, RZ, RX, RY, HX, HY, LT, RT}, R("bbbbbbbbuu"));
        over(ds4.pravaX == Z && ds4.pravaY == RZ && ds4.l2.length == 1 && ds4.l2[0] == LT && ds4.r2[0] == RT,
                "A5) DualShock 4: prava packa Z/RZ (RX/RY jsou tam taky - prednost ma Z/RZ), spouste LTRIGGER / RTRIGGER - " + ds4.popis());
        NapOvladac.Rozlozeni rxry = roz(new int[]{X, Y, RX, RY, Z, RZ, HX, HY}, R("bbbbuubb"));
        over(rxry.pravaX == RX && rxry.pravaY == RY && rxry.l2.length == 1 && rxry.l2[0] == Z && rxry.r2[0] == RZ,
                "A6) ovladac s pravou packou na RX/RY a spoustemi na Z/RZ (0..1): spravne rozpoznano - " + rxry.popis());
        NapOvladac.Rozlozeni holy = roz(new int[]{X, Y}, R("bb"));
        over(holy.leva && !holy.hat && holy.pravaX < 0 && holy.l2.length == 0 && holy.maska() == 8,
                "A7) jednoduchy ovladac (jen leva packa, tlacitka): maska jen leva packa - " + holy.popis());
        // cteni os: spoust s klidem na -1 (nektere ovladace) -> 0..1
        NapOvladac.Rozlozeni sp = roz(new int[]{X, Y, Z, RZ, BRAKE, GAS}, new float[][]{{-1, 1}, {-1, 1}, {-1, 1}, {-1, 1}, {-1, 1}, {0, 1}});
        float[] o = new float[NapOvladac.O_POCET];
        final java.util.Map<Integer, Float> hodnoty = new java.util.HashMap<>();
        hodnoty.put(X, 0.3f); hodnoty.put(Y, -0.7f); hodnoty.put(Z, 0.5f); hodnoty.put(RZ, -0.25f); hodnoty.put(BRAKE, -1f); hodnoty.put(GAS, 0.6f);
        int m = NapOvladac.nactiOsy(sp, ax -> hodnoty.getOrDefault(ax, 0f), o);
        over(o[NapOvladac.O_LX] == 0.3f && o[NapOvladac.O_LY] == -0.7f && o[NapOvladac.O_RX] == 0.5f && o[NapOvladac.O_RY] == -0.25f
                        && o[NapOvladac.O_L2] == 0f && Math.abs(o[NapOvladac.O_R2] - 0.6f) < 1e-6 && m == (8 | 2 | 4),
                "A8) osy -> 8 cisel pro C++: packy, spoust BRAKE v klidu (-1) = 0, GAS 0,6; maska " + m);
        hodnoty.put(BRAKE, 1f);
        NapOvladac.nactiOsy(sp, ax -> hodnoty.getOrDefault(ax, 0f), o);
        over(o[NapOvladac.O_L2] == 1f && NapOvladac.pohyb(o, m) && !NapOvladac.pohyb(new float[8], 15),
                "A9) spoust BRAKE naplno = 1; pohyb() = zamerny pohyb (packa / spoust pres polovinu), klid ne");
        // schovani dotykoveho ovladani
        NapOvladac.Schovani s = new NapOvladac.Schovani();
        boolean z1 = s.hraje();
        int u1 = s.ui();
        boolean z2 = s.hraje();
        boolean z3 = s.dotyk();
        int u3 = s.ui();
        boolean z4 = s.hraje();
        boolean z5 = s.odpojen();
        int u5 = s.ui();
        over(z1 && u1 == NapOvladac.UI_SKRYTO && !z2 && z3 && u3 == NapOvladac.UI_VIDET && z4 && z5 && u5 == NapOvladac.UI_VIDET,
                "A10) AUTOMATICKY: ovladac hraje -> schovano; dalsi stisk nic nemeni; dotyk na misto tlacitka -> videt; dalsi stisk -> zase schovano; odpojen -> videt");
        boolean d1 = s.nastav(NapOvladac.REZIM_DOTYK, true);
        int ud = s.ui();
        boolean d2 = s.hraje();
        boolean d3 = s.nastav(NapOvladac.REZIM_SKRYT, true);
        int us = s.ui();
        boolean d4 = s.dotyk();
        boolean d5 = s.nastav(NapOvladac.REZIM_AUTO, true);
        int ua = s.ui();
        boolean d6 = s.nastav(NapOvladac.REZIM_AUTO, false);
        int ua2 = s.ui();
        s.nastav(99, false);
        over(!d1 && ud == NapOvladac.UI_VIDET && !d2 && d3 && us == NapOvladac.UI_SKRYTO_VZDY && !d4 && d5 && ua == NapOvladac.UI_SKRYTO
                        && d6 && ua2 == NapOvladac.UI_VIDET && s.rezim == NapOvladac.REZIM_AUTO,
                "A11) OPTIONS: VZDY DOTYK = videt i kdyz ovladac hraje; SKRYT DOTYK = schovano vzdy (dotyk nic); AUTOMATICKY s pripojenym ovladacem = schovano, bez = videt; nesmysl = AUTOMATICKY");
        over("AUTOMATICKY".equals(NapOvladac.rezimText(0)) && "VZDY DOTYK".equals(NapOvladac.rezimText(1)) && "SKRYT DOTYK".equals(NapOvladac.rezimText(2))
                        && NapOvladac.jmenoZdroju(SRC_GAMEPAD | SRC_JOYSTICK).startsWith("GAMEPAD JOYSTICK") && "LTRIGGER".equals(NapOvladac.jmenoOsy(LT)),
                "A12) texty do logu: rezimy, zdroje (" + NapOvladac.jmenoZdroju(SRC_GAMEPAD | SRC_JOYSTICK) + "), osy");

        // ================= B) NativePs1CoreBridge =================
        new File(prac, "hra").mkdirs();
        try (FileWriter w = new FileWriter(new File(prac, "hra/Dune 2000.cue"))) {
            w.write("FILE \"Dune 2000.bin\" BINARY\n  TRACK 01 MODE2/2352\n");
        }
        over(NativePs1CoreBridge.isLoaded(), "B1) knihovna napps1core nactena skutecnou tridou NativePs1CoreBridge");
        int nativnich = 0;
        for (Method mm : NativePs1CoreBridge.class.getDeclaredMethods()) if (Modifier.isNative(mm.getModifiers())) nativnich++;
        over(nativnich == 32, "B2) native metod ve tride: " + nativnich + " (29 z B319 + ps1OvladacKlavesa / ps1OvladacOsy / ps1OvladacPust B320)");
        Method mk = NativePs1CoreBridge.class.getDeclaredMethod("ps1OvladacKlavesa", int.class, boolean.class);
        Method mo = NativePs1CoreBridge.class.getDeclaredMethod("ps1OvladacOsy", float[].class, int.class);
        Method mp = NativePs1CoreBridge.class.getDeclaredMethod("ps1OvladacPust");
        over(mk.getReturnType() == int.class && mo.getReturnType() == void.class && mp.getReturnType() == void.class
                        && Modifier.isPrivate(mk.getModifiers()) && Modifier.isStatic(mo.getModifiers()),
                "B3) podpisy: int ps1OvladacKlavesa(int, boolean), void ps1OvladacOsy(float[], int), void ps1OvladacPust() - private static native");
        NativePs1CoreBridge.setDiagLogPathSafe(prac + "/nap_tv_session_log.txt");
        String r = NativePs1CoreBridge.bootGameSafe(prac, prac, prac + "/hra/Dune 2000.cue");
        over("PS1_HRA_OK".equals(r), "B4) bootGameSafe -> " + r);
        Thread.sleep(300);
        int ta = NativePs1CoreBridge.ovladacKlavesaSafe(96, true);
        int tb = NativePs1CoreBridge.ovladacKlavesaSafe(97, true);
        int t0 = NativePs1CoreBridge.ovladacKlavesaSafe(4, true);
        float[] osy = {0.9f, 0f, 0f, 0f, -1f, 0f, 0.8f, 0f};
        NativePs1CoreBridge.ovladacOsySafe(osy, 15);
        NativePs1CoreBridge.ovladacOsySafe(new float[3], 15);       // kratke pole (jen leva packa)
        NativePs1CoreBridge.ovladacOsySafe(null, 15);               // null nespadne
        Thread.sleep(200);
        NativePs1CoreBridge.ovladacKlavesaSafe(96, false);
        NativePs1CoreBridge.ovladacKlavesaSafe(97, false);
        NativePs1CoreBridge.ovladacPustSafe();
        over(ta == 1 && tb == 2 && t0 == 0, "B5) ovladacKlavesaSafe za behu hry: A -> " + ta + " (T_A), B -> " + tb + " (T_B), ZPET -> " + t0
                + "; ovladacOsySafe (i kratke pole a null) a ovladacPustSafe bez chyby JNI");
        Thread.sleep(300);
        String dn = NativePs1CoreBridge.denikSafe();
        over(dn.contains("OVLADAC: klavesa 96 dolu -> A(spodni) -> PS1 KRIZEK") && dn.contains("OVLADAC: klavesa 97 dolu -> B(prave) -> PS1 KOLECKO"),
                "B6) denik PS1: \"OVLADAC: klavesa 96 dolu -> A(spodni) -> PS1 KRIZEK\"");
        NativePs1CoreBridge.stopSafe();

        // ================= C) mosty bez knihovny =================
        int rs = -1, ra = -1;
        boolean padSega = false, padAtari = false;
        try { rs = NativeSegaCoreBridge.ovladacKlavesaSafe(96, true); NativeSegaCoreBridge.ovladacPustSafe(); }
        catch (Throwable t) { padSega = true; }
        try { ra = NativeAtariCoreBridge.devOvladacKlavesaSafe(96, true); NativeAtariCoreBridge.devOvladacOsySafe(new float[8], 15); NativeAtariCoreBridge.devOvladacPustSafe(); }
        catch (Throwable t) { padAtari = true; }
        over(!padAtari && ra == 0, "C1) most Atari bez knihovny: devOvladac*Safe vraci 0 a nespadne");
        over(padSega || rs == 0, "C2) most Segy bez knihovny: trida se nenacte (System.loadLibrary bez try) - proto ji MainActivity vola jen v Seze"
                + " a pri odpojeni ovladace jen kdyz Sega ovladac dostala (ovlPouzitSega); " + (padSega ? "ExceptionInInitializerError zachycena" : "vraci 0"));

        System.out.println();
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " chyb");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
