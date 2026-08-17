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
