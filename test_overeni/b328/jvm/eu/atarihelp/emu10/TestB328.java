package eu.atarihelp.emu10;

/**
 * B328: texty "nova verze" (okno misto Toastu) a zapis prijimace po aktualizaci do logu
 * (SKUTECNY NapAktualizaceTexty.java, cisty Java).
 */
public class TestB328 {
    static int ok = 0, chyb = 0;

    static void kontrola(String nazev, boolean b, Object detail) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + nazev + (b ? "" : "  -> " + detail));
    }

    public static void main(String[] a) {
        System.out.println("B328 - okno nove verze a log prijimace po aktualizaci (skutecny NapAktualizaceTexty)");
        System.out.println();
        kontrola("T1 titulek okna cesky / anglicky", "Aktualizace hotová".equals(NapAktualizaceTexty.novaVerzeTitulek(false))
                && "Update done".equals(NapAktualizaceTexty.novaVerzeTitulek(true)), "");
        String z = NapAktualizaceTexty.novaVerze(false, "EMU10-B328-MERENI (371)");
        kontrola("T2 text okna: verze i cislo", "✔ Nainstalována nová verze: EMU10-B328-MERENI (371)".equals(z), z);
        long ted = 1_800_000_000_000L;
        String p0 = NapAktualizaceTexty.prijimacDoLogu(0L, null, ted);
        kontrola("T3 zadna zprava od systemu = jasne v logu", p0.equals("prijimac: zadna zprava od systemu (MY_PACKAGE_REPLACED neprisla)"), p0);
        String p1 = NapAktualizaceTexty.prijimacDoLogu(ted - 42_000L, "otevreni zadano (Android 9)", ted);
        kontrola("T4 Android 9: kdy a co prijimac udelal", p1.equals("prijimac: zprava od systemu pred 42 s, otevreni zadano (Android 9)"), p1);
        String p2 = NapAktualizaceTexty.prijimacDoLogu(ted - 5_000L, "Android 12 (API 31) - appka se sama neotevira", ted);
        kontrola("T5 Android 12: prijimac zpravu dostal, appku neotevira", p2.endsWith("pred 5 s, Android 12 (API 31) - appka se sama neotevira"), p2);
        kontrola("T6 cas v budoucnosti (zmena hodin) = 0 s, bez vysledku = ?",
                NapAktualizaceTexty.prijimacDoLogu(ted + 9_000L, null, ted).equals("prijimac: zprava od systemu pred 0 s, ?"), "");
        System.out.println();
        System.out.println("VYSLEDEK: " + (ok + chyb) + " kontrol, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
