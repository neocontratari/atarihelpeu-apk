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
    // B292: nove jadro presne po cyklech (nap_atari_6502.h + nap_atari_machine.h,
    // samokontrola v nap_atari_selftest.h) - cisla z test_overeni/b292/
    // test_b292_jni_host (x86_64): procesor vcetne poctu cyklu na sbernici,
    // pamet 130XE pres vsech 256 hodnot PORTB (CPU i ANTIC).
    public static final String OCEKAVANY_CPU_HASH = "29C55806";
    // (pamet: stejne cislo jako u stareho jadra - mapovani pameti 130XE pres
    // vsech 256 hodnot PORTB je po kontrole 130XE totozne)
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

    // B291: CELE zarizeni Atari 130XE ze schvaleneho navrhu (obrazovka,
    // klavesnice, konzole, kazetak, POWER, servisni tlacitka) kresli a ridi
    // C++ (nap_atari_device.h + vlakna v nap_atari_native.cpp) PRIMO na
    // displej pres SurfaceView (AtariDeviceView) - zadny JavaScript ani
    // base64 obrazek jako v B289/B290 (tam se stisk na telefonu nestihl
    // ukazat). Java sem jen preda plochu displeje a souradnice prstu.
    private static native void    devStartNative(boolean studeny);
    private static native void    devStopNative();
    private static native void    devSurfaceNative(android.view.Surface s, int w, int h);
    private static native int     devTouchNative(int pointerId, int akce, float x, float y);
    private static native int     devTypeTextNative(String text, boolean runPotom);
    private static native boolean devLoadXexNative(byte[] data, String jmeno);
    private static native void    devStatusNative(String zprava, int ms);
    private static native String  devPollLogNative();
    private static native byte[]  devTakeWavNative();
    private static native String  devInfoNative();
    // B292: kazeta (CLOAD) - WAV se v C++ demoduluje (FSK 5327/3995 Hz) a
    // stroj ho cte pri zapnutem motoru a PLAY, bajty sklada POKEY.
    private static native String  devLoadTapeNative(byte[] wav, String jmeno);
    private static native void    devEjectTapeNative();
    // B295: disketa ATR do mechaniky D1: (prikazy SIO v C++) + studeny start
    private static native String  devLoadAtrNative(byte[] atr, String jmeno);
    // B296: herni ovladac / klavesnice telefonu -> joystick 1 + START/SELECT/OPTION
    //  maska: bit0 nahoru, 1 dolu, 2 vlevo, 3 vpravo, 4 FIRE, 5 START, 6 SELECT, 7 OPTION,
    //  8 klavesa MEZERA, 9 klavesa RETURN
    private static native void    devPadNative(int maska);
    // B297: ovladani na sirku (D-pad a tlacitka jako u Segy): "get", "set:<nastaveni>",
    //  "edit", "mirror", "reset", "done", "stav" -> vraci nastaveni (pro SharedPreferences)
    private static native String  devCtlNative(String prikaz);
    // B299: program z TXT souboru (rezim 0 = ATARI BASIC, 1 = Turbo-BASIC XL - ten zavede Java
    //  pred tim pres devLoadXexNative); diagnostika pameti a cela RAM pro LOG/CHYBA
    private static native String  devProgramNative(byte[] data, String jmeno, int rezim);
    private static native String  devDiagNative();
    private static native byte[]  devRamDumpNative();
    // B301: obraz a zvuk Atari pro TV / PC (web prohlizec v appce) primo z jadra
    private static native int     devGrabFrameNative(int[] argb);
    private static native int     devPullTvAudioNative(short[] pcm);
    // B302: klavesa z prohlizece na TV / PC (e.code, e.key, mod: 1 Shift, 2 Ctrl, 4 Alt,
    //  8 AltGr, 16 opakovani; dolu) - mapovani, rezim PSANI/HRANI i drzene klavesy resi C++.
    //  code "STAV" = jen rezim. Vraci text pro prohlizec.
    private static native String  devPcKlavesaNative(String code, String znak, int mod, boolean dolu);

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

    /** B291: bezpecne wrappery pro zarizeni v HELP (nikdy nehodi vyjimku). */
    public static void devStartSafe(boolean studeny) {
        if (!loaded) return;
        try { devStartNative(studeny); } catch (Throwable ignored) {}
    }
    public static void devStopSafe() {
        if (!loaded) return;
        try { devStopNative(); } catch (Throwable ignored) {}
    }
    public static void devSurfaceSafe(android.view.Surface s, int w, int h) {
        if (!loaded) return;
        try { devSurfaceNative(s, w, h); } catch (Throwable ignored) {}
    }
    /** akce: 0 = prst dolu, 1 = nahoru, 2 = zruseno. Vraci servisni akci (0 = zadna). */
    public static int devTouchSafe(int pointerId, int akce, float x, float y) {
        if (!loaded) return 0;
        try { return devTouchNative(pointerId, akce, x, y); } catch (Throwable t) { return 0; }
    }
    public static int devTypeTextSafe(String text, boolean runPotom) {
        if (!loaded || text == null) return 0;
        try { return devTypeTextNative(text, runPotom); } catch (Throwable t) { return 0; }
    }
    public static boolean devLoadXexSafe(byte[] data, String jmeno) {
        if (!loaded || data == null) return false;
        try { return devLoadXexNative(data, jmeno); } catch (Throwable t) { return false; }
    }
    public static void devStatusSafe(String zprava, int ms) {
        if (!loaded) return;
        try { devStatusNative(zprava, ms); } catch (Throwable ignored) {}
    }
    public static String devPollLogSafe() {
        if (!loaded) return null;
        try { return devPollLogNative(); } catch (Throwable t) { return null; }
    }
    public static byte[] devTakeWavSafe() {
        if (!loaded) return null;
        try { return devTakeWavNative(); } catch (Throwable t) { return null; }
    }
    public static String devInfoSafe() {
        if (!loaded) return "B291 PRISTROJ knihovna napatari neni nactena";
        try { return devInfoNative(); } catch (Throwable t) { return "B291 PRISTROJ stav: chyba " + t.getMessage(); }
    }
    /** B292: vlozit kazetu (WAV) do magnetofonu. Vraci "OK ..." / "CHYBA ...". */
    public static String devLoadTapeSafe(byte[] wav, String jmeno) {
        if (!loaded) return "CHYBA knihovna napatari neni nactena";
        if (wav == null) return "CHYBA zadna data";
        try { String r = devLoadTapeNative(wav, jmeno); return r == null ? "CHYBA nic" : r; }
        catch (Throwable t) { return "CHYBA " + t.getMessage(); }
    }
    /** B295: disketa ATR do D1: a nabootovat. Vraci "OK ..." / "CHYBA ...". */
    public static String devLoadAtrSafe(byte[] atr, String jmeno) {
        if (!loaded) return "CHYBA knihovna napatari neni nactena";
        if (atr == null) return "CHYBA zadna data";
        try { String r = devLoadAtrNative(atr, jmeno); return r == null ? "CHYBA nic" : r; }
        catch (Throwable t) { return "CHYBA " + t.getMessage(); }
    }
    public static void devEjectTapeSafe() {
        if (!loaded) return;
        try { devEjectTapeNative(); } catch (Throwable ignored) {}
    }
    /** B296: stav herniho ovladace (joystick 1 + konzole). */
    public static void devPadSafe(int maska) {
        if (!loaded) return;
        try { devPadNative(maska); } catch (Throwable ignored) {}
    }
    /** B297: ovladani na sirku - prikaz / nastaveni; vraci aktualni nastaveni ("" pri chybe). */
    public static String devCtlSafe(String prikaz) {
        if (!loaded || prikaz == null) return "";
        try { String r = devCtlNative(prikaz); return r == null ? "" : r; } catch (Throwable t) { return ""; }
    }

    /** B299: TXT soubor -> program v ATARI BASICu (rezim 0) nebo Turbo-BASICu XL (rezim 1). */
    public static String devProgramSafe(byte[] data, String jmeno, int rezim) {
        if (!loaded) return "CHYBA knihovna napatari neni nactena";
        if (data == null) return "CHYBA zadna data";
        try { String r = devProgramNative(data, jmeno, rezim); return r == null ? "CHYBA nic" : r; }
        catch (Throwable t) { return "CHYBA " + t.getMessage(); }
    }
    /** B299: diagnostika pameti (stav, ukazatele BASICu, kontrolni soucty) pro log. */
    public static String devDiagSafe() {
        if (!loaded) return "B299 PAMET: knihovna napatari neni nactena";
        try { String r = devDiagNative(); return r == null ? "B299 PAMET: nic" : r; }
        catch (Throwable t) { return "B299 PAMET: chyba " + t.getMessage(); }
    }
    /** B299: cela pamet Atari (hlavicka + registry + 64 kB + rozsirenych 64 kB) nebo null. */
    public static byte[] devRamDumpSafe() {
        if (!loaded) return null;
        try { return devRamDumpNative(); } catch (Throwable t) { return null; }
    }

    /** B301: obraz Atari pro TV/PC: (sirka<<16)|vyska, zaporne = male pole, 0 = nic. */
    public static int devGrabFrameSafe(int[] argb) {
        if (!loaded || argb == null) return 0;
        try { return devGrabFrameNative(argb); } catch (Throwable t) { return 0; }
    }
    /** B301: zvuk Atari pro TV/PC (stereo int16 44100 Hz) - pocet shortu. */
    public static int devPullTvAudioSafe(short[] pcm) {
        if (!loaded || pcm == null) return 0;
        try { return devPullTvAudioNative(pcm); } catch (Throwable t) { return 0; }
    }
    /** B302: klavesa z prohlizece na TV/PC -> Atari 130XE (C++). Vraci text pro prohlizec
     *  ("PSANI"/"HRANI", "OK:...", "NEZNAMA:...") nebo null pri chybe knihovny. */
    public static String devPcKlavesaSafe(String code, String znak, int mod, boolean dolu) {
        if (!loaded || code == null) return null;
        try { return devPcKlavesaNative(code, znak == null ? "" : znak, mod, dolu); } catch (Throwable t) { return null; }
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
