package eu.atarihelp.emu10;

/**
 * B354: prirazeni hracu ovladacum (NapOvladac.Hraci - cista trida, bez Androidu). Rene: "dokazal by si napojit
 * 2 bt dpady na jeden mobil ... plno her je 1vs1".
 * Preklad a spusteni: viz ../JAK_SPUSTIT.txt krok 4                              -> VYSTUP_TestB354Hraci.txt
 */
public class TestB354Hraci {
    static int ok = 0, chyb = 0;
    static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); }

    public static void main(String[] a) {
        System.out.println("B354 - NapOvladac.Hraci: prvni ovladac = hrac 1, druhy = hrac 2 (podle prvniho stisku)");
        NapOvladac.Hraci h = new NapOvladac.Hraci();
        final int A = 11, B = 22, C = 33, D = 44;
        // 1) jeden ovladac = hrac 1 (jako drive)
        over(h.hrac(A, false) == -1, "1a) neprirazeny ovladac v klidu (sum packy, pusteni) nic neposila (-1)");
        over(h.hrac(A, true) == 0 && h.hrac(A, false) == 0 && h.hrac(A, true) == 0, "1b) prvni stisk ovladace A = hrac 1, pak vse z A hrac 1 (i pusteni)");
        // 2) druhy ovladac
        over(h.hrac(B, false) == -1, "2a) druhy ovladac B v klidu: nic (nesmi prepsat osy hrace 1)");
        over(h.hrac(B, true) == 1 && h.hrac(B, false) == 1 && h.hrac(A, true) == 0, "2b) prvni stisk na B = hrac 2; A zustava hrac 1");
        // 3) treti ovladac = za hrace 1
        over(h.hrac(C, false) == 0 && h.hrac(C, true) == 0 && h.obsazeno(0) && h.obsazeno(1), "3) treti ovladac C hraje za hrace 1 (i jeho pusteni - nic nezustane drzet)");
        // 4) odpojeni a znovu
        over(h.odpoj(A) == 0 && !h.obsazeno(0) && h.hrac(B, true) == 1, "4a) A odpojen -> misto hrace 1 volne, B porad hrac 2");
        over(h.hrac(C, false) == -1 && h.hrac(C, true) == 0, "4b) C (dosud navic) se stiskem dostane volne misto hrace 1");
        over(h.odpoj(B) == 1 && h.odpoj(B) == -1 && h.hrac(D, true) == 1, "4c) B odpojen -> D se stiskem = hrac 2; odpojit znovu nic");
        over(h.odpoj(77) == -1, "4d) odpojeni neznameho zarizeni nic nemeni");
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
