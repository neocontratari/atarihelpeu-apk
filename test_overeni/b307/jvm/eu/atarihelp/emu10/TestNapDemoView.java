package eu.atarihelp.emu10;

import android.content.Context;
import android.content.res.AssetManager;
import android.view.MotionEvent;
import java.util.ArrayList;
import java.util.List;

/**
 * B307 test: SKUTECNA NapDemoView (dotyk) + SKUTECNA NativeIntroBridge + SKUTECNY C++
 * (libnapintro.so z ../tv_intro.cpp) v JVM. Stejne jako MainActivity.demoSpust:
 * introStart, new NapDemoView(this, this::demoKonec), dotyk prstem -> C++ rozhodne
 * (tlacitko SKIP / jinam / hned po startu nic) -> demoKonec(proc).
 * Telefon 1080 x 2340: tlacitko SKIP podle Demo::skipObd = [788,70]-[1042,159].
 */
public class TestNapDemoView {
    private final List<String> konce = new ArrayList<>();
    private void demoKonec(String proc) { konce.add(proc); }       // jako MainActivity.demoKonec (private)

    static int chyb = 0;
    static void over(boolean ok, String co) {
        System.out.println((ok ? "OK     " : "CHYBA  ") + co);
        if (!ok) chyb++;
    }
    static MotionEvent prst(int akce, float x, float y) { return MotionEvent.obtain(0, 0, akce, x, y, 0); }

    void beh() throws Exception {
        NativeIntroBridge.introNastavAssety(new AssetManager());
        final int st = NativeIntroBridge.introStart();
        NapDemoView v = new NapDemoView(new Context(), this::demoKonec);
        v.testVelikost(1080, 2340);
        v.onTouchEvent(prst(MotionEvent.ACTION_UP, 915f, 114f));          // prst, ktery appku spustil
        over(st == 3 && konce.isEmpty(), "start (bity " + st + "): dotyk hned po startu demo nepreskoci " + konce);
        Thread.sleep(800);
        v.onTouchEvent(prst(MotionEvent.ACTION_DOWN, 915f, 114f));        // polozeni prstu nic nedela
        v.onTouchEvent(prst(MotionEvent.ACTION_MOVE, 915f, 114f));
        over(konce.isEmpty(), "polozeni a posun prstu nic (jen zvednuti) " + konce);
        v.onTouchEvent(prst(MotionEvent.ACTION_UP, 915f, 114f));
        over(konce.size() == 1 && "tlacitko SKIP".equals(konce.get(0)), "zvednuti prstu na SKIP -> demoKonec(\"tlacitko SKIP\") " + konce);
        v.onTouchEvent(prst(MotionEvent.ACTION_UP, 540f, 1170f));
        over(konce.size() == 2 && "dotyk mimo SKIP".equals(konce.get(1)), "zvednuti prstu jinde -> demoKonec(\"dotyk mimo SKIP\") " + konce);
        v.testVelikost(2340, 1080);                                        // na sirku: tlacitko [2048,38]-[2302,126]
        v.onTouchEvent(prst(MotionEvent.ACTION_UP, 2175f, 82f));
        over(konce.size() == 3 && "tlacitko SKIP".equals(konce.get(2)), "na sirku: SKIP vpravo nahore " + konce);
        NativeIntroBridge.stopSafe();
    }

    public static void main(String[] a) throws Exception {
        if (!NativeIntroBridge.jeNactena()) { System.out.println("CHYBA napintro: " + NativeIntroBridge.chybaNacteni()); System.exit(1); }
        new TestNapDemoView().beh();
        System.out.println(chyb == 0 ? "VYSLEDEK: dotyk NapDemoView -> C++ -> demoKonec sedi (0 chyb)" : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
