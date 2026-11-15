package eu.atarihelp.emu10;

/**
 * B322: POSTUP HRÁČE bez Androidu (testuje se na PC v JVM - test_overeni/b322).
 * NapPostup je jen obal pro appku (SharedPreferences "nap_postup").
 *
 *   ATARI   od začátku otevřené
 *   SEGA    1500 řádků programu v Atari  - nebo hned po zaplacení (server N&P)
 *   PS1     120 hodin hraní na Seze      - nebo hned po zaplacení, ale až po Seze
 *
 * Co je "získané" (řádky, čas) si appka počítá sama. Co je "koupené", rozhoduje server N&P
 * (správce viděl platbu a klikl ODEMKNOUT) - appka si to jen pamatuje pro hraní bez sítě.
 * Postup se zálohuje na serveru (preinstalace, nový telefon): bere se vždy vyšší hodnota.
 */
final class NapPostupJadro {

    /** Kde je postup uložený (v appce SharedPreferences, v testu mapa). */
    interface Pamet {
        boolean ctiB(String k, boolean v);
        int ctiI(String k, int v);
        long ctiL(String k, long v);
        void zapisB(String k, boolean v);
        void zapisI(String k, int v);
        void zapisL(String k, long v);
    }

    static final String K_SEGA = "sega_odemcena";       // získaná řádky (od B310)
    static final String K_PS1 = "ps1_odemcena";         // získaná časem Segy
    static final String K_MINUTY = "sega_odehrano_ms";
    static final String K_RADKY = "napsano_radku";
    static final String K_SEGA_KOUPENO = "sega_koupeno"; // B322: zaplaceno (podle serveru N&P)
    static final String K_PS1_KOUPENO = "ps1_koupeno";
    static final String K_ZAMEK = "zamek_cislo";         // B323: kolikaty zamek ze serveru uz appka provedla

    /** Kolik řádků kódu otevře Segu (B322: 1500, dřív 50). */
    static final int SEGA_POTREBA_RADKU = 1500;
    /** Kratší dávka se nepočítá - jinak by stačilo bouchat jeden řádek. */
    static final int NEJMENE_RADKU = 5;
    /** Kolik hraní na Seze otevře PlayStation (B322: 120 hodin, dřív 30 minut). */
    static final long PS1_POTREBA_MS = 120L * 60L * 60L * 1000L;

    private final Pamet p;

    NapPostupJadro(Pamet p) { this.p = p; }

    // ------------------------------------------------------------------ stav
    boolean segaZiskana() { return p.ctiB(K_SEGA, false); }
    boolean ps1Ziskana() { return p.ctiB(K_PS1, false); }
    boolean segaKoupena() { return p.ctiB(K_SEGA_KOUPENO, false); }
    boolean ps1Koupena() { return p.ctiB(K_PS1_KOUPENO, false); }

    boolean segaOdemcena() { return segaZiskana() || segaKoupena(); }
    /** PS1 až po Seze - i koupená PS1 potřebuje otevřenou Segu. */
    boolean ps1Odemcena() { return segaOdemcena() && (ps1Ziskana() || ps1Koupena()); }

    long segaOdehranoMs() { return Math.max(0L, p.ctiL(K_MINUTY, 0L)); }
    int napsanoRadku() { return Math.max(0, p.ctiI(K_RADKY, 0)); }

    int zbyvaRadku() { return Math.max(0, SEGA_POTREBA_RADKU - napsanoRadku()); }

    /** Kolik minut Segy ještě zbývá do otevření PS1 (zaokrouhleno nahoru). */
    int zbyvaMinutSegy() {
        long z = PS1_POTREBA_MS - segaOdehranoMs();
        return z <= 0 ? 0 : (int) ((z + 59999L) / 60000L);
    }

    // ------------------------------------------------------------------ získávání
    /** Dávka napsaných řádků. true = Sega se právě otevřela. Dávka pod 5 řádků se nepočítá. */
    boolean pripocitejRadky(int kolik) {
        if (kolik < NEJMENE_RADKU) return false;
        boolean bylo = segaOdemcena();
        long ted = Math.min((long) Integer.MAX_VALUE, (long) napsanoRadku() + kolik);
        p.zapisI(K_RADKY, (int) ted);
        if (!segaZiskana() && ted >= SEGA_POTREBA_RADKU) {
            p.zapisB(K_SEGA, true);
            return !bylo;
        }
        return false;
    }

    /** Odehraný čas Segy. true = PS1 se právě otevřela. */
    boolean pripocitejSegu(long ms) {
        if (ms <= 0) return false;
        boolean bylo = ps1Odemcena();
        long ted = segaOdehranoMs() + ms;
        p.zapisL(K_MINUTY, ted);
        if (!ps1Ziskana() && ted >= PS1_POTREBA_MS && segaOdemcena()) {
            p.zapisB(K_PS1, true);
            return !bylo;
        }
        return false;
    }

    /** Záloha postupu ze serveru N&P: bere se vyšší hodnota. true = něco se tím otevřelo. */
    boolean slucPostup(long radku, long ms) {
        boolean sega = segaOdemcena(), ps1 = ps1Odemcena();
        if (radku > napsanoRadku()) p.zapisI(K_RADKY, (int) Math.min(radku, (long) Integer.MAX_VALUE));
        if (ms > segaOdehranoMs()) p.zapisL(K_MINUTY, ms);
        if (!segaZiskana() && napsanoRadku() >= SEGA_POTREBA_RADKU) p.zapisB(K_SEGA, true);
        if (!ps1Ziskana() && segaOdemcena() && segaOdehranoMs() >= PS1_POTREBA_MS) p.zapisB(K_PS1, true);
        return (!sega && segaOdemcena()) || (!ps1 && ps1Odemcena());
    }

    /** Co je zaplacené podle serveru N&P (správce viděl platbu). true = změna. */
    boolean nastavKoupeno(boolean sega, boolean ps1) {
        boolean zmena = sega != segaKoupena() || ps1 != ps1Koupena();
        if (zmena) {
            p.zapisB(K_SEGA_KOUPENO, sega);
            p.zapisB(K_PS1_KOUPENO, ps1);
        }
        return zmena;
    }

    // ------------------------------------------------------------------ B323: zámek ze serveru (🔒 ZAMKNOUT TEST)
    /** Kolikátý zámek ze serveru N&P už appka provedla (0 = žádný). */
    int zamekCislo() { return Math.max(0, p.ctiI(K_ZAMEK, 0)); }

    /**
     * Správce na serveru klikl 🔒 ZAMKNOUT (TEST) - i Sega / PS1 otevřená zadarmo se zamkne.
     * sega = Sega i PS1 (jako servis ZAMKNOUT VŠE), jinak jen PS1 (Sega i řádky zůstanou).
     * Každý zámek (číslo) se provede jen jednou. true = provedeno teď.
     */
    boolean provedZamek(int cislo, boolean sega, boolean ps1) {
        if (cislo <= zamekCislo()) return false;
        if (sega) zamkniVse();
        else if (ps1) {
            p.zapisB(K_PS1, false);
            p.zapisB(K_PS1_KOUPENO, false);
            p.zapisL(K_MINUTY, 0L);
        }
        p.zapisI(K_ZAMEK, cislo);
        return true;
    }

    // ------------------------------------------------------------------ zadní vrátka pro Reného (servis)
    /** Odemkne DALŠÍ úroveň v pořadí (postupně, ať jde vyzkoušet každý krok). */
    String odemkniDalsi() {
        if (!segaOdemcena()) {
            p.zapisB(K_SEGA, true);
            if (napsanoRadku() < SEGA_POTREBA_RADKU) p.zapisI(K_RADKY, SEGA_POTREBA_RADKU);
            return "SEGA";
        }
        if (!ps1Odemcena()) {
            p.zapisB(K_PS1, true);
            if (segaOdehranoMs() < PS1_POTREBA_MS) p.zapisL(K_MINUTY, PS1_POTREBA_MS);
            return "PS1";
        }
        return "UZ JE VSE OTEVRENE";
    }

    /** Vše na začátek - pohled nového hráče (i zapamatované platby; server je při dalším dotazu vrátí). */
    void zamkniVse() {
        p.zapisB(K_SEGA, false);
        p.zapisB(K_PS1, false);
        p.zapisB(K_SEGA_KOUPENO, false);
        p.zapisB(K_PS1_KOUPENO, false);
        p.zapisI(K_RADKY, 0);
        p.zapisL(K_MINUTY, 0L);
    }

    /** Krátký popis stavu do logu. */
    String stav() {
        long ms = segaOdehranoMs();
        String koupeno = (segaKoupena() ? "S" : "") + (ps1Koupena() ? "P" : "");
        return "radku=" + napsanoRadku() + "/" + SEGA_POTREBA_RADKU
             + " sega=" + (segaOdemcena() ? "otevrena" : "ZAMCENA")
             + " ps1=" + (ps1Odemcena() ? "otevrena" : "ZAMCENA")
             + " odehrano=" + (ms / 60000L) + "min/" + (PS1_POTREBA_MS / 60000L)
             + " koupeno=" + (koupeno.isEmpty() ? "-" : koupeno);
    }
}
