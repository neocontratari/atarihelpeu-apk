import android.content.Context;
import android.content.SharedPreferences;
import eu.atarihelp.emu10.NapPostup;
import java.util.HashMap;
import java.util.Map;

/**
 * B310: SKUTECNA trida NapPostup (app/src/main/java) - davky radku z C++ Atari
 * (devCtl "postup" -> "5,3,12") a odemceni Segy po 50 radcich.
 * Android Context / SharedPreferences nahrazeny pameti (stub/).
 */
public class TestPostup {
    static int kontrol = 0, chyb = 0;
    static void over(boolean ok, String co) { kontrol++; if (!ok) chyb++; System.out.println((ok ? "OK     " : "CHYBA  ") + co); }

    static class Pamet extends Context {
        final Map<String, Object> m = new HashMap<>();
        public SharedPreferences getSharedPreferences(String name, int mode) {
            return new SharedPreferences() {
                public boolean getBoolean(String k, boolean d) { Object v = m.get(k); return v == null ? d : (Boolean) v; }
                public int getInt(String k, int d) { Object v = m.get(k); return v == null ? d : (Integer) v; }
                public long getLong(String k, long d) { Object v = m.get(k); return v == null ? d : (Long) v; }
                public Editor edit() {
                    return new Editor() {
                        public Editor putBoolean(String k, boolean v) { m.put(k, v); return this; }
                        public Editor putInt(String k, int v) { m.put(k, v); return this; }
                        public Editor putLong(String k, long v) { m.put(k, v); return this; }
                        public void apply() {}
                    };
                }
            };
        }
    }

    public static void main(String[] a) {
        System.out.println("B310 - NapPostup.pripocitejDavky (skutecna trida z appky), davky z C++ Atari");
        Pamet c = new Pamet();
        StringBuilder log = new StringBuilder();
        over(!NapPostup.segaOdemcena(c) && NapPostup.napsanoRadku(c) == 0, "novy hrac: Sega zamcena, 0 radku");
        over(!NapPostup.pripocitejDavky(c, "", log) && NapPostup.napsanoRadku(c) == 0, "prazdna odpoved z C++ nic nepricte");
        boolean o = NapPostup.pripocitejDavky(c, "3", log);
        over(!o && NapPostup.napsanoRadku(c) == 0, "davka 3 radky se nepocita (pod 5)");
        o = NapPostup.pripocitejDavky(c, "5,12", log);
        over(!o && NapPostup.napsanoRadku(c) == 17, "davky 5 a 12 -> 17 radku");
        o = NapPostup.pripocitejDavky(c, "x, 20 ,4", log);
        over(!o && NapPostup.napsanoRadku(c) == 37, "nesmysl se preskoci, ' 20 ' se precte, 4 se nepocita -> 37");
        over(!NapPostup.segaOdemcena(c), "37 radku: Sega porad zamcena");
        o = NapPostup.pripocitejDavky(c, "13", log);
        over(o && NapPostup.segaOdemcena(c) && NapPostup.napsanoRadku(c) == 50, "davka 13 -> 50 radku: Sega PRAVE OTEVRENA (true)");
        o = NapPostup.pripocitejDavky(c, "6", log);
        over(!o && NapPostup.segaOdemcena(c), "dalsi davka uz Segu znovu neotevira (false), zustava otevrena");
        String l = log.toString();
        over(l.contains("B310 POSTUP_HRACE ATARI_RADKY +3 -> radku=0/50 sega=ZAMCENA") && l.contains("(davka pod 5 radku se nepocita)"), "log: davka pod 5 radku s vysvetlenim");
        over(l.contains("+13 -> radku=50/50 sega=otevrena") && l.contains("*** SEGA PRAVE OTEVRENA ***"), "log: SEGA PRAVE OTEVRENA");
        System.out.print("--- log appky ---\n" + l + "---\n");
        System.out.println("\nVYSLEDEK: " + kontrol + " kontrol, " + chyb + " chyb");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
