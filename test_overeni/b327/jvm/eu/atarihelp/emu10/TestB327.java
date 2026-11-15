package eu.atarihelp.emu10;

import android.content.Context;
import android.content.SharedPreferences;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * B327: (1) obrazovka volby jazyka jednou KAZDEMU (NapJazyk.obrazovkaHotova / oznacObrazovku),
 * (2) texty okna aktualizace a "prvni start po aktualizaci" (NapAktualizaceTexty).
 * SKUTECNE NapJazyk.java a NapAktualizaceTexty.java, Android jen nahrada (stub/android/content).
 */
public class TestB327 {
    static int ok = 0, chyb = 0;

    static void kontrola(String nazev, boolean b, Object detail) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + nazev + (b ? "" : "  -> " + detail));
    }

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

    static void restart() throws Exception {
        Field f = NapJazyk.class.getDeclaredField("kod");
        f.setAccessible(true);
        f.set(null, null);
    }

    /** MainActivity.onCreate (B327): ukazat obrazovku volby jazyka? */
    static boolean ukazeVolbu(Context c) { return !NapJazyk.obrazovkaHotova(c); }

    /** AHJAZYK.prvniVolba (B327): ulozit jazyk a oznacit obrazovku. */
    static void volbaNaObrazovce(Context c, String k) { NapJazyk.nastav(c, k); NapJazyk.oznacObrazovku(c); }

    public static void main(String[] a) throws Exception {
        System.out.println("B327 - volba jazyka jednou kazdemu + texty okna aktualizace (skutecne NapJazyk, NapAktualizaceTexty)");
        System.out.println();
        Locale.setDefault(Locale.forLanguageTag("cs-CZ"));
        Context c = new Telefon();

        // --- (1) obrazovka volby jazyka ---
        DISK.clear(); restart();
        kontrola("J1 nova instalace: obrazovka volby ANO, do te doby anglicky", ukazeVolbu(c) && "en".equals(NapJazyk.kod(c)), DISK);
        volbaNaObrazovce(c, "cs"); restart();
        kontrola("J2 po volbe CESTINA na obrazovce: dalsi start obrazovka NE, cesky", !ukazeVolbu(c) && "cs".equals(NapJazyk.kod(c)), DISK);

        DISK.clear(); DISK.computeIfAbsent("nap_jazyk", j -> new HashMap<>()).put("jazyk", "cs"); restart();
        kontrola("J3 Reneho S8 (cestina vybrana v OPTIONS v B314-B325): po aktualizaci na B327 obrazovka ANO, zatim cesky",
                ukazeVolbu(c) && "cs".equals(NapJazyk.kod(c)), DISK);
        volbaNaObrazovce(c, "en"); restart();
        kontrola("J3 tam zvoli ENGLISH: anglicky, obrazovka uz nikdy", !ukazeVolbu(c) && "en".equals(NapJazyk.kod(c)), DISK);

        DISK.clear(); DISK.computeIfAbsent("nap_jazyk", j -> new HashMap<>()).put("jazyk", "en"); restart();
        kontrola("J4 synuv telefon (obrazovka v B326, B326 si ji nepamatovala): obrazovka jeste jednou", ukazeVolbu(c), DISK);

        DISK.clear(); restart();
        volbaNaObrazovce(c, "cs");
        NapJazyk.nastav(c, "en"); restart();
        kontrola("J5 pozdejsi zmena v OPTIONS obrazovku nevrati (anglicky, obrazovka NE)", !ukazeVolbu(c) && "en".equals(NapJazyk.kod(c)), DISK);
        DISK.clear(); DISK.computeIfAbsent("nap_jazyk", j -> new HashMap<>()).put("obrazovka_volby", "x"); restart();
        kontrola("J6 poskozeny zaznam obrazovky = obrazovka ANO", ukazeVolbu(c), DISK);
        kontrola("J7 bez kontextu: obrazovka nehotova, nic nespadne", !NapJazyk.obrazovkaHotova(null), "");

        // --- (2) texty okna aktualizace ---
        String s = NapAktualizaceTexty.stahovani(false, true, 12_900_000L, 29_465_190L, 1_258_291L);
        kontrola("A1 stahovani cesky: odkud, MB s desetinnou carkou, procenta, rychlost, rada",
                s.startsWith("Stahuji novou verzi ze serveru N&P…") && s.contains("12,3 / 28,1 MB  (43 %)") && s.contains("1,2 MB/s")
                        && s.endsWith("Nech appku otevřenou, chvíli to trvá."), s);
        String e = NapAktualizaceTexty.stahovani(true, true, 12_900_000L, 29_465_190L, 1_258_291L);
        kontrola("A2 stahovani anglicky: desetinna tecka", e.startsWith("Downloading the new version from the N&P server…")
                && e.contains("12.3 / 28.1 MB  (43 %)") && e.contains("1.2 MB/s"), e);
        String z = NapAktualizaceTexty.stahovani(false, false, 5_000_000L, -1L, 0L);
        kontrola("A3 neznama velikost (zaloha atarihelp.eu): jen MB, bez procent a rychlosti", z.contains("z atarihelp.eu") && z.contains("\n\n4,8 MB\n\n")
                && !z.contains("%") && !z.contains("MB/s"), z);
        kontrola("A4 nikdy pres 100 % (zip: stazeno > velikost spojeni)", NapAktualizaceTexty.stahovani(false, false, 50_000_000L, 40_000_000L, 0L).contains("(100 %)"), "");

        String p9 = NapAktualizaceTexty.poPredani(false, 28, 29_465_190L), p12 = NapAktualizaceTexty.poPredani(false, 31, 29_465_190L);
        kontrola("A5 po predani Androidu, Android 9 (S8): INSTALOVAT + appka se sama znovu otevre",
                p9.startsWith("✔ Staženo (28,1 MB).") && p9.contains("klepni na INSTALOVAT") && p9.contains("sama se znovu otevře"), p9);
        kontrola("A6 po predani Androidu, Android 12 (syn): appka se zavre - otevrit ikonou, pri startu ukaze novou verzi",
                p12.contains("klepni na INSTALOVAT") && p12.contains("otevři ji znovu ikonou") && p12.contains("nová verze nainstalovaná")
                        && !p12.contains("sama se znovu otevře"), p12);
        kontrola("A7 anglicky Android 12", NapAktualizaceTexty.poPredani(true, 31, 1048576L).contains("open it again from its icon")
                && NapAktualizaceTexty.potvrd(true, 31).startsWith("The phone asks whether to install – tap INSTALL."), "");
        kontrola("A8 zadny text uz neslibuje, ze se appka na Androidu 10+ otevre sama",
                !p12.contains("otevře znovu sama") && !NapAktualizaceTexty.potvrd(false, 29).contains("sama"), NapAktualizaceTexty.potvrd(false, 29));
        kontrola("A9 do Androidu 9 (API 28) se appka otevre sama, od Androidu 10 (API 29) ne",
                NapAktualizaceTexty.otevreSeSama(28) && NapAktualizaceTexty.otevreSeSama(24) && !NapAktualizaceTexty.otevreSeSama(29)
                        && !NapAktualizaceTexty.otevreSeSama(34), "");
        kontrola("A10 toast nove verze", "✔ Nainstalována nová verze: EMU10-B327-INSTALACE".equals(NapAktualizaceTexty.novaVerze(false, "EMU10-B327-INSTALACE"))
                && "✔ New version installed: X".equals(NapAktualizaceTexty.novaVerze(true, "X")), "");

        // --- (3) prvni start po aktualizaci ---
        long ted = 1_800_000_000_000L, min = 60_000L;
        kontrola("P1 ulozena 369, ted 370 = prvni start po aktualizaci", NapAktualizaceTexty.prvniStartPoAktualizaci(369, 370, 0, 0, ted), "");
        kontrola("P2 ulozena 370, ted 370 = bezny start (zadny toast)", !NapAktualizaceTexty.prvniStartPoAktualizaci(370, 370, 0, ted - 5 * min, ted), "");
        kontrola("P3 B326 -> B327 (nic ulozeno), aktualizace pred 2 min = toast", NapAktualizaceTexty.prvniStartPoAktualizaci(0, 370, ted - 999 * min, ted - 2 * min, ted), "");
        kontrola("P4 nic ulozeno, nova instalace (prvni = posledni) = zadny toast", !NapAktualizaceTexty.prvniStartPoAktualizaci(0, 370, ted - 2 * min, ted - 2 * min, ted), "");
        kontrola("P5 nic ulozeno, aktualizace pred 2 hodinami = zadny toast", !NapAktualizaceTexty.prvniStartPoAktualizaci(0, 370, ted - 999 * min, ted - 120 * min, ted), "");
        kontrola("P6 navrat na starsi verzi (ulozena 371, ted 370) = zadny toast", !NapAktualizaceTexty.prvniStartPoAktualizaci(371, 370, 0, 0, ted), "");

        System.out.println();
        System.out.println("VYSLEDEK: " + (ok + chyb) + " kontrol, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
