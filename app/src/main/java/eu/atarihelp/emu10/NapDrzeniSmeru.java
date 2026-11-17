package eu.atarihelp.emu10;

/**
 * B340: PODRZENI SIPKY pro dalkove ovladace TV, ktere posilaji jen JEDNO tlacitko naraz.
 *
 * Rene: "at to jde hrat v zakladu podle dalkoveho ovladace". Vetsina ovladacu televize (infra i rada
 * Bluetooth) neumi dve tlacitka naraz: drzis VPRAVO a zmacknes OK -> ovladac VPRAVO "pusti" a posle
 * jen OK. Sonic se pak za behu zastavi a skoci na miste. Tady appka pozna, ze ovladac sipku pustil
 * a hned (do OKNO_MS) prisla akce - clovek sipku drzi dal. Sipku tedy znovu stiskne a drzi ji za nej,
 * dokud trva akce; po pusteni akce chvili (CEKANI_MS) pocka, jestli ji ovladac zase posle (pri
 * drzeni ji posila dal). Kdyz ne, pusti ji.
 *
 * Ovladace, ktere dve tlacitka naraz umi (gamepad, nektere Bluetooth), tohle nespusti: sipka se pri
 * akci nepusti, takze neni co podrzet. Bez zpozdeni: pusteni sipky jde do jadra hned (obnovi se jen,
 * kdyz prijde akce) - zastaveni postavy se nezpozdi.
 *
 * Cisty Java bez Androidu (JVM test test_overeni/b340). Vola se z vlakna UI (udalosti i casovac).
 */
final class NapDrzeniSmeru {

    /** Kam jdou klavesy (jadro konzole). */
    interface Jadro {
        void klavesa(int kod, boolean dolu);
    }

    /** Ovladac pustil sipku a do tolika ms prisla akce = clovek ji drzi dal (infra: obe udalosti naraz). */
    static final long OKNO_MS = 120;
    /** Po pusteni akce: kdyz ovladac sipku do tolika ms neposle znovu, clovek ji pustil. */
    static final long CEKANI_MS = 250;

    private int drzi = -1;               // sipka, kterou drzi ovladac (stisk prisel, pusteni ne)
    private int podrzena = -1;           // sipka, kterou drzi APPKA za ovladac (jadro ji ma stisknutou)
    private int pustena = -1;            // posledni sipka, kterou ovladac pustil
    private long pustenaV = 0;
    private int akci = 0;                // kolik akci (ne sipek) je ted stisknutych
    private long konecAkceV = -1;        // kdy se pustila posledni akce (ceka se na navrat sipky)
    private int podrzeni = 0;            // kolikrat appka sipku podrzela (log)

    /**
     * Udalost z ovladace - jen prvni stisk a pusteni (ne opakovani drzene klavesy).
     * @return true = appka prave zacala sipku podrzovat (pro log)
     */
    boolean udalost(int kod, boolean smer, boolean dolu, long t, Jadro j) {
        if (smer) {
            if (dolu) {
                if (podrzena == kod) {           // ovladac sipku zase posila - jadro ji ma porad stisknutou
                    podrzena = -1;
                    konecAkceV = -1;
                    drzi = kod;
                    return false;
                }
                if (podrzena >= 0) {             // jina sipka: podrzenou pustit
                    j.klavesa(podrzena, false);
                    podrzena = -1;
                    konecAkceV = -1;
                }
                drzi = kod;
                pustena = -1;
                j.klavesa(kod, true);
            } else {
                if (drzi == kod) drzi = -1;
                if (podrzena == kod) return false;
                j.klavesa(kod, false);           // hned (bez zpozdeni) - obnovi se jen, kdyz prijde akce
                pustena = kod;
                pustenaV = t;
            }
            return false;
        }
        boolean zacal = false;
        if (dolu) {
            if (drzi < 0 && podrzena < 0 && pustena >= 0 && t - pustenaV >= 0 && t - pustenaV <= OKNO_MS) {
                podrzena = pustena;
                j.klavesa(podrzena, true);       // clovek sipku drzi dal - ovladac ji jen "pustil"
                podrzeni++;
                zacal = true;
            }
            pustena = -1;
            akci++;
            konecAkceV = -1;
            j.klavesa(kod, true);
        } else {
            j.klavesa(kod, false);
            if (akci > 0) akci--;
            if (akci == 0 && podrzena >= 0) konecAkceV = t;
        }
        return zacal;
    }

    /** Casovac (kazdych ~50 ms, dokud cekaPusteni()): sipka se po akci nevratila -> pustit. */
    void tik(long t, Jadro j) {
        if (cekaPusteni() && t - konecAkceV >= CEKANI_MS) {
            j.klavesa(podrzena, false);
            podrzena = -1;
            konecAkceV = -1;
        }
    }

    /** Ceka se, jestli se podrzena sipka vrati (je treba casovac)? */
    boolean cekaPusteni() {
        return podrzena >= 0 && akci == 0 && konecAkceV >= 0;
    }

    /** Podrzena sipka (-1 = zadna). */
    int podrzena() {
        return podrzena;
    }

    /** Kolikrat appka sipku podrzela. */
    int podrzeni() {
        return podrzeni;
    }

    /** Menu, odchod ze hry: zapomenout vse (klavesy v jadru pusti MainActivity.ovladacPustVse). */
    void zrus() {
        drzi = -1;
        podrzena = -1;
        pustena = -1;
        akci = 0;
        konecAkceV = -1;
    }
}
