// B304: vola VSECHNY native metody skutecne tridy NativeIntroBridge z appky
// (knihovna = skutecny nap_intro_gl.cpp prelozeny pro PC).
import eu.atarihelp.emu10.NativeIntroBridge;

public class TestPodpisu {
    public static void main(String[] a) throws Exception {
        if (!NativeIntroBridge.jeNactena()) { System.out.println("CHYBA knihovna: " + NativeIntroBridge.chybaNacteni()); System.exit(1); }
        int chyb = 0;
        NativeIntroBridge.introNastavAssety(new android.content.res.AssetManager());
        NativeIntroBridge.introStart();
        short[] buf = new short[2048];
        int n = NativeIntroBridge.introNaplnZvuk(buf, 1024);
        System.out.println("introNaplnZvuk pred pripravou -> " + n + " (ma byt 1024 ticha)");
        if (n != 1024) chyb++;
        NativeIntroBridge.introCasZvuku(0L);
        boolean h = NativeIntroBridge.introHotovo(), p = NativeIntroBridge.introPripraveno();
        long s = NativeIntroBridge.introSnimku();
        int c = NativeIntroBridge.introCasMs();
        int[] tv = new int[4];
        int g = NativeIntroBridge.introGrabFrame(tv);
        String info = NativeIntroBridge.introInfo();
        System.out.println("hotovo=" + h + " pripraveno=" + p + " snimku=" + s + " casMs=" + c + " grab=" + g + " info='" + info + "'");
        if (h || p || s != 0 || c != 0 || g != 0 || info == null) chyb++;
        NativeIntroBridge.introSetDisplaySurface(null);
        NativeIntroBridge.introStop();
        int n2 = NativeIntroBridge.introNaplnZvuk(buf, 1024);
        System.out.println("introNaplnZvuk po zastaveni -> " + n2 + " (ma byt -1)");
        if (n2 != -1) chyb++;
        // obaly *Safe
        System.out.println("Safe: grab=" + NativeIntroBridge.grabFrameSafe(tv) + " hotovo=" + NativeIntroBridge.hotovoSafe()
                + " pripraveno=" + NativeIntroBridge.pripravenoSafe() + " info='" + NativeIntroBridge.infoSafe() + "'");
        NativeIntroBridge.plochaSafe(null);
        NativeIntroBridge.stopSafe();
        System.out.println(chyb == 0 ? "VYSLEDEK: vsech 12 native metod se zavolalo, podpisy sedi (0 chyb)" : "VYSLEDEK: " + chyb + " CHYB");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
