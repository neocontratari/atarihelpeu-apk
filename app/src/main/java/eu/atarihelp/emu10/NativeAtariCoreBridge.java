package eu.atarihelp.emu10;

/**
 * BUILD2SA14: most do jadra Atari v C++ (vrstvy 1-2: procesor a pamet).
 *
 * Zatim tu NENI obraz ani zvuk - ANTIC, GTIA, POKEY a VBXE hotove nejsou,
 * takze se OS Atari nema o co oprit. Jedine, co jadro umi, je overit samo
 * sebe: pustit procesor a pamet na tisicich nahodnych stavech a spocitat
 * kontrolni soucet.
 *
 * Smysl: vsechna mereni jsem delal na pocitaci (x86_64). Telefon je ARM64.
 * Kdyz se jadro mezi temi dvema rozejde, musi se to poznat TED, a ne az
 * podle spatneho obrazu.
 */
public final class NativeAtariCoreBridge {

    // Cisla, ktera vysla na pocitaci. Telefon musi dat stejna.
    public static final String OCEKAVANY_CPU_HASH = "51154C46";
    public static final String OCEKAVANY_MEM_HASH = "D3949DC5";
    public static final long   OCEKAVANO_INSTRUKCI = 51200L;
    public static final long   OCEKAVANO_CTENI     = 16252928L;

    // BUILD2SB49: KBCODE pro HELP - na 130XE je HELP klavesa klavesnicove
    // matice (NE konzolovy prepinac jako START/SELECT/OPTION). Hodnota
    // overena z Reneho JS reference (emu_vbxe/index.html): 'F1':17 /*HELP*/
    // a skutecne tlacitko HELP tam vola M.keyDown(17) - stejne cislo na
    // obou mistech, takze je spolehlive.
    public static final int KBCODE_HELP = 17;

    private static volatile boolean loaded = false;
    private static volatile String loadError = null;

    static {
        try {
            System.loadLibrary("napatari");
            loaded = true;
        } catch (Throwable t) {
            loadError = String.valueOf(t.getMessage());
        }
    }

    private NativeAtariCoreBridge() {}

    public static boolean isLoaded() { return loaded; }
    public static String loadError() { return loadError; }

    private static native String runSelfTest();
    private static native String bootNative(int snimku);
    private static native void   keyNative(int kod, int snimku);
    private static native void   consolNative(int maska, int snimku);
    // B283: jen DRZI konzolovou masku (bez beh snimku, bez automatickeho
    // pusteni zpet na 7 na konci) - viz komentar u konsolSetNative v
    // nap_atari_native.cpp. Umoznuje Jave rozdelit puvodni jeden
    // "drz+odbehni+pust" blok na "drz" / "odbehni SE ZACHYCENIM ZVUKU
    // (zachytitCsaveZvukSafe)" / "pust", misto aby se zvuk z odbehnuti
    // tise ztratil jako driv.
    private static native void   consolSetNative(int maska);
    private static native void   runNative(int snimku);
    // BUILD2SB59: RESET tlacitko - nemaze pamet/hardware, jen znovu
    // nahodi procesor (na rozdil od BOOT, ktery ted dela poradny
    // studeny start).
    private static native void   resetNative();
    private static native String screenNative();
    // BUILD2SB55: Rene - "zadne pomocne testovaci tlacitko - zvuk ma
    // bezet prubezne presne jak na realnem atari." Vraci JEN holy
    // base64 PCM16 (zadny JSON obal, zadna statistika - vola se ~50x/s,
    // musi byt lehke). Stav generatoru se MEZI VOLANIMI NEVYNULUJE -
    // navazuje presne tam, kde skoncil predchozi snimek.
    private static native String audioChunkNative(int pocetVzorku);
    // BUILD2SB76: Rene - "udelej realny WAV, budu ho testovat na
    // skutecnem Atari, ne na Altirre." Bezi po CELOU dobu operace
    // (ne jen kratky kousek jako normalni beh) a vraci VSECHNY
    // vygenerovane vzorky najednou jako base64 16-bit PCM.
    private static native String atariZachytitCsaveZvukNative(int celkemSnimku);
    // BUILD2SB55: kompaktni text "audf0,1,2,3|audc0,1,2,3|audctl" jen
    // pro ridke logovani (viz atariAudioChunk v MainActivity).
    private static native String regsNative();
    // BUILD2SB92: velmi lehke - jen 1 bit (PACTL/$D302 bit3, oficialne
    // zdokumentovany "Motor Control"). JS s tim pozna SKUTECNY konec
    // CSAVE (motor ZAPNUTY->VYPNUTY prechod) a ukonci WAV nahravani
    // hned, misto cekani na pevnych 50s - viz zahajNahravani() v
    // index.html a komentar u C++ funkce.
    private static native int motorZapnutyNative();

    // BUILD2SC1: klavesnice+konzole v C++ (viz nap_atari_keyboard.h). Stejny
    // tvar jako screenNative() - C++ vykresli cely obrazek klavesnice jako
    // RGB a vrati base64, zadne HTML/CSS tlacitko. kbdTouchNative dostane
    // jen ID klavesy/tlacitka (jak ho vratil hitTest v JS - viz index.html)
    // a jestli je dole(1)/nahoru(0) - samo si v C++ poradi se SHIFT/CTRL
    // zamky, konzolovym drzenim i jednorazovymi akcemi (HELP, RESET, BREAK).
    private static native String kbdScreenNative();
    private static native int    kbdHitTestNative(int x, int y);
    private static native void   kbdTouchNative(int id, int dolu);

    // B287: Rene - "chci ciste jadro atari emu v c++ - Java odhaduje a to
    // je problem... vyzaduji aby... bylo opravdu emu atari pod tlacitkem
    // HELP ciste v c++." Zvuk uz nehraje JS Web Audio API (odhad/orezavani
    // fronty v index.html), ale primo jadro pres OpenSL ES (presne vzor
    // jako PS1 - "CESTA A: zvuk bez Javy"). Start/stop se vola ze
    // zivotniho cyklu obrazovky (viz MainActivity), diag je kratky
    // stavovy radek PRIMO z nativni fronty, zadny JS odhad.
    private static native void   atariAudioStartNative();
    private static native void   atariAudioStopNative();
    private static native String atariZvukDiagNative();

    /** KBCODE pro pismena a RETURN - potrebne, aby slo napsat BYE. */
    public static int kbcode(char c) {
        switch (Character.toUpperCase(c)) {
            case 'A': return 0x3F; case 'B': return 0x15; case 'C': return 0x12;
            case 'D': return 0x3A; case 'E': return 0x2A; case 'F': return 0x38;
            case 'G': return 0x3D; case 'H': return 0x39; case 'I': return 0x0D;
            case 'J': return 0x01; case 'K': return 0x05; case 'L': return 0x00;
            case 'M': return 0x25; case 'N': return 0x23; case 'O': return 0x08;
            case 'P': return 0x0A; case 'Q': return 0x2F; case 'R': return 0x28;
            case 'S': return 0x3E; case 'T': return 0x2D; case 'U': return 0x0B;
            case 'V': return 0x10; case 'W': return 0x2E; case 'X': return 0x16;
            case 'Y': return 0x2B; case 'Z': return 0x17;
            case '\n': return 0x0C;                  // RETURN
            case ' ': return 0x21;
            default: return -1;
        }
    }

    public static String bootSafe(int snimku) {
        if (!loaded) return "{\"chyba\":\"knihovna napatari se nenacetla\"}";
        try { String r = bootNative(snimku); return r == null ? "{\"chyba\":\"nic\"}" : r; }
        catch (Throwable t) { return "{\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"','\'') + "\"}"; }
    }
    public static void keySafe(int kod, int snimku) {
        if (!loaded || kod < 0) return;
        try { keyNative(kod, snimku); } catch (Throwable ignored) {}
    }
    public static void resetSafe() {
        if (!loaded) return;
        try { resetNative(); } catch (Throwable ignored) {}
    }
    public static void consolSafe(int maska, int snimku) {
        if (!loaded) return;
        try { consolNative(maska, snimku); } catch (Throwable ignored) {}
    }
    public static void consolSetSafe(int maska) {
        if (!loaded) return;
        try { consolSetNative(maska); } catch (Throwable ignored) {}
    }
    public static void runSafe(int snimku) {
        if (!loaded) return;
        try { runNative(snimku); } catch (Throwable ignored) {}
    }
    public static String screenSafe() {
        if (!loaded) return "{\"chyba\":\"knihovna napatari se nenacetla\"}";
        try { String r = screenNative(); return r == null ? "{\"chyba\":\"nic\"}" : r; }
        catch (Throwable t) { return "{\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"','\'') + "\"}"; }
    }

    /** BUILD2SB55: prubezny zvuk - stejny bezpecny vzor, ale bez JSON
     *  obalu (jen holy base64, at je co nejlehci pro casti volani ~50x/s). */
    public static String audioChunkSafe(int pocetVzorku) {
        if (!loaded) return "";
        try { String r = audioChunkNative(pocetVzorku); return r == null ? "" : r; }
        catch (Throwable t) { return ""; }
    }
    /** BUILD2SB92: bezpecny wrapper - 1=motor bezi (kazeta aktivni),
     *  0=motor stoji nebo knihovna neni nactena/chyba. */
    public static int motorZapnutySafe() {
        if (!loaded) return 0;
        try { return motorZapnutyNative(); } catch (Throwable t) { return 0; }
    }
    /** BUILD2SB76: bezpecny wrapper - zachyti VSECHNY vzorky behem
     *  cele operace (napr. celeho CSAVE) najednou. */
    public static String zachytitCsaveZvukSafe(int celkemSnimku) {
        if (!loaded) return "";
        try { String r = atariZachytitCsaveZvukNative(celkemSnimku); return r == null ? "" : r; }
        catch (Throwable t) { return ""; }
    }
    /** BUILD2SB55: bezpecny wrapper pro ridke logovani registru. */
    public static String regsSafe() {
        if (!loaded) return "?";
        try { String r = regsNative(); return r == null ? "?" : r; }
        catch (Throwable t) { return "?"; }
    }

    /** B287: bezpecne zapnuti/vypnuti nativniho OpenSL zvuku. Idempotentni
     *  (jadro samo hlida, jestli uz bezi/je zavrene) - bezpecne volat
     *  vicekrat za sebou z ruznych mist zivotniho cyklu obrazovky. */
    public static void audioStartSafe() {
        if (!loaded) return;
        try { atariAudioStartNative(); } catch (Throwable ignored) {}
    }
    public static void audioStopSafe() {
        if (!loaded) return;
        try { atariAudioStopNative(); } catch (Throwable ignored) {}
    }
    /** B287: kratky stav nativni zvukove fronty (podtekani/zpozdeni/stav) -
     *  nahrazuje stary JS odhad ZVUK_PODTEKANI. */
    public static String zvukDiagSafe() {
        if (!loaded) return null;
        try { return atariZvukDiagNative(); } catch (Throwable t) { return null; }
    }

    /** BUILD2SC1: bezpecne wrappery pro klavesnici+konzoli v C++. */
    public static String kbdScreenSafe() {
        if (!loaded) return "{\"chyba\":\"knihovna napatari se nenacetla\"}";
        try { String r = kbdScreenNative(); return r == null ? "{\"chyba\":\"nic\"}" : r; }
        catch (Throwable t) { return "{\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"','\'') + "\"}"; }
    }
    public static void kbdTouchSafe(int id, int dolu) {
        if (!loaded) return;
        try { kbdTouchNative(id, dolu); } catch (Throwable ignored) {}
    }
    public static int kbdHitTestSafe(int x, int y) {
        if (!loaded) return -1;
        try { return kbdHitTestNative(x, y); } catch (Throwable t) { return -1; }
    }

    /** Vrati vysledek jako JSON. Nikdy nehodi vyjimku. */
    public static String runSelfTestSafe() {
        if (!loaded) {
            return "{\"chyba\":\"knihovna napatari se nenacetla: "
                    + (loadError == null ? "neznamy duvod" : loadError.replace('"', '\'')) + "\"}";
        }
        try {
            String r = runSelfTest();
            return (r == null) ? "{\"chyba\":\"jadro nevratilo nic\"}" : r;
        } catch (Throwable t) {
            return "{\"chyba\":\"" + String.valueOf(t.getMessage()).replace('"', '\'') + "\"}";
        }
    }
}
