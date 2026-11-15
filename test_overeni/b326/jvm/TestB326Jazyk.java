import android.content.Context;
import android.content.SharedPreferences;
import eu.atarihelp.emu10.NapJazyk;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * B326: NapJazyk - dokud nikdo nevolil, plati ANGLICTINA (i na ceskem telefonu); zvoleno() rozhoduje,
 * jestli MainActivity ukaze obrazovku volby jazyka. SKUTECNY NapJazyk.java, Android jen jako nahrada
 * (stub/android/content). Pouziti: viz JAK_SPUSTIT.txt.
 */
public class TestB326Jazyk {
    static int ok = 0, chyb = 0;

    static void kontrola(String nazev, boolean b, Object detail) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + nazev + (b ? "" : "  -> " + detail));
    }

    /** SharedPreferences v pameti - jako soubor nap_jazyk.xml v telefonu (prezije restart appky). */
    static final Map<String, Map<String, String>> DISK = new HashMap<>();

    static class Pamet implements SharedPreferences {
        final Map<String, String> m;
        Pamet(Map<String, String> m) { this.m = m; }
        public String getString(String k, String def) { return m.containsKey(k) ? m.get(k) : def; }
        public Editor edit() {
            final Map<String, String> zmeny = new HashMap<>();
            return new Editor() {
                public Editor putString(String k, String v) { zmeny.put(k, v); return this; }
                public void apply() { m.putAll(zmeny); }
            };
        }
    }

    static class Telefon extends Context {
        public Context getApplicationContext() { return this; }
        public SharedPreferences getSharedPreferences(String jmeno, int rezim) {
            return new Pamet(DISK.computeIfAbsent(jmeno, j -> new HashMap<>()));
        }
    }

    /** Novy start appky: proces znovu = staticka pamet NapJazyk.kod prazdna (soubor zustava). */
    static void restart() throws Exception {
        Field f = NapJazyk.class.getDeclaredField("kod");
        f.setAccessible(true);
        f.set(null, null);
    }

    static void novaInstalace() throws Exception { DISK.clear(); restart(); }

    public static void main(String[] a) throws Exception {
        System.out.println("B326 - NapJazyk: nezvoleno = anglicky, volba pri prvnim spusteni (skutecny NapJazyk.java)");
        System.out.println();
        Context c = new Telefon();

        for (String tel : new String[] { "cs-CZ", "sk-SK", "en-US", "de-DE" }) {
            Locale.setDefault(Locale.forLanguageTag(tel));
            novaInstalace();
            kontrola("J1 nova instalace, telefon " + tel + ": nezvoleno -> obrazovka volby, do te doby anglicky",
                    !NapJazyk.zvoleno(c) && "en".equals(NapJazyk.kod(c)) && NapJazyk.en(c) && "CLOSE".equals(NapJazyk.t(c, "ZAVŘÍT", "CLOSE")),
                    NapJazyk.popis(c));
        }
        Locale.setDefault(Locale.forLanguageTag("cs-CZ"));
        novaInstalace();
        kontrola("J2 log pri startu: \"en (zatim nezvoleno - anglicky, telefon cs-CZ)\"",
                "en (zatim nezvoleno - anglicky, telefon cs-CZ)".equals(NapJazyk.popis(c)), NapJazyk.popis(c));

        boolean zmena = NapJazyk.nastav(c, "cs");
        kontrola("J3 volba CESTINA (prvniVolba -> nastav): zmena, cesky, zvoleno, ulozeno jazyk=cs",
                zmena && NapJazyk.zvoleno(c) && "cs".equals(NapJazyk.kod(c)) && "ZAVŘÍT".equals(NapJazyk.t(c, "ZAVŘÍT", "CLOSE"))
                        && "cs".equals(DISK.get("nap_jazyk").get("jazyk")), DISK);
        restart();
        kontrola("J4 dalsi start: cesky z pameti, obrazovka volby uz ne", NapJazyk.zvoleno(c) && "cs".equals(NapJazyk.kod(c))
                && "cs (zvoleno)".equals(NapJazyk.popis(c)), NapJazyk.popis(c));

        novaInstalace();
        kontrola("J5 volba ENGLISH na anglictine = bez zmeny jazyka (menu se neprekresluje), ale ulozena -> zvoleno",
                !NapJazyk.nastav(c, "en") && NapJazyk.zvoleno(c) && "en".equals(NapJazyk.kod(c)), DISK);
        restart();
        kontrola("J5 dalsi start: anglicky, obrazovka volby uz ne", NapJazyk.zvoleno(c) && "en".equals(NapJazyk.kod(c)), DISK);

        novaInstalace();
        kontrola("J6 neplatna volba (\"de\", null) se neulozi", !NapJazyk.nastav(c, "de") && !NapJazyk.nastav(c, null)
                && !NapJazyk.zvoleno(c) && "en".equals(NapJazyk.kod(c)), DISK);

        novaInstalace();
        DISK.computeIfAbsent("nap_jazyk", j -> new HashMap<>()).put("jazyk", "cs");   // B314-B325: volba v OPTIONS
        restart();
        kontrola("J7 aktualizace z B325, kde se v OPTIONS volilo cesky: zvoleno -> obrazovka volby NE, zustava cesky",
                NapJazyk.zvoleno(c) && "cs".equals(NapJazyk.kod(c)), DISK);

        novaInstalace();
        DISK.computeIfAbsent("nap_jazyk", j -> new HashMap<>()).put("jazyk", "xx");   // poskozeny zaznam
        restart();
        kontrola("J8 poskozeny zaznam = nezvoleno -> obrazovka volby, anglicky", !NapJazyk.zvoleno(c) && "en".equals(NapJazyk.kod(c)), DISK);

        restart();
        kontrola("J9 bez kontextu (null): anglicky, nezvoleno", "en".equals(NapJazyk.kod(null)) && !NapJazyk.zvoleno(null), "");

        System.out.println();
        System.out.println("VYSLEDEK: " + (ok + chyb) + " kontrol, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
