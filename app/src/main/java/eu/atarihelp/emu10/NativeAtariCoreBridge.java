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
    private static native void   runNative(int snimku);
    private static native String screenNative();

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
    public static void consolSafe(int maska, int snimku) {
        if (!loaded) return;
        try { consolNative(maska, snimku); } catch (Throwable ignored) {}
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
