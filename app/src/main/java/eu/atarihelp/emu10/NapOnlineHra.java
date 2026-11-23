package eu.atarihelp.emu10;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * B357: ONLINE HRANI PRES ENGINE N&P - pomocne veci bez Androidu (test: test_overeni/b357/jvm).
 * Rene: "chci hrat hry online pomoci meho enginu".
 *
 * Samotne hrani je cele v C++ (online/nap_online*.h): spojeni s enginem (WebSocket), domluva telefonu, stisky
 * obou hracu ve stejnem snimku na obou telefonech (lockstep) a kontrola otisku stavu. Java jen sklada dotazy
 * na engine (tahle trida), podepisuje je (NapServerKlient.hlavickyPro) a ukazuje stav na strance Segy.
 *
 * Engine (NapServer v12.4): GET /api/v1/online/mistnosti = hry, ktere cekaji na hrace 2;
 * /api/v1/online/zaloz?... (hrac 1) a /api/v1/online/pripoj?kod=... (hrac 2) = WebSocket.
 */
final class NapOnlineHra {
    private NapOnlineHra() {}

    static final String CESTA_SEZNAM = "/api/v1/online/mistnosti";
    static final int JMENO_MAX = 24;

    /** Jmeno hrace (ukaze se druhemu hraci): bez ridicich znaku a znaku, ktere engine zahazuje; prazdne = "HRAC". */
    static String jmeno(String s) {
        StringBuilder b = new StringBuilder();
        if (s != null) {
            for (int i = 0; i < s.length() && b.length() < JMENO_MAX; i++) {
                char c = s.charAt(i);
                if (c < 0x20 || c == 0x7F || c == '<' || c == '>' || c == '"' || c == '\\' || Character.isISOControl(c)) continue;
                if (Character.isHighSurrogate(c)) {   // emoji apod.: cely par, nebo nic
                    if (i + 1 < s.length() && Character.isLowSurrogate(s.charAt(i + 1)) && b.length() + 2 <= JMENO_MAX) {
                        b.append(c).append(s.charAt(i + 1));
                    }
                    i++;
                    continue;
                }
                if (Character.isLowSurrogate(c)) continue;
                b.append(c);
            }
        }
        String r = b.toString().trim();
        return r.isEmpty() ? "HRAC" : r;
    }

    /** Hrac 1: /api/v1/online/zaloz?system=..&hra=..&hra_id=..&otisk=..&nastaveni=..&jmeno=.. */
    static String cestaZaloz(String system, String hra, String hraId, String otisk, String nastaveni, String jmeno) {
        return "/api/v1/online/zaloz?system=" + enc(system) + "&hra=" + enc(nazevHry(hra)) + "&hra_id=" + enc(serverId(hraId))
                + "&otisk=" + enc(otisk) + "&nastaveni=" + enc(nastaveni) + "&jmeno=" + enc(jmeno(jmeno));
    }

    /** Hrac 2: /api/v1/online/pripoj?kod=1234&jmeno=.. */
    static String cestaPripoj(String kod, String jmeno) {
        return "/api/v1/online/pripoj?kod=" + enc(kod(kod)) + "&jmeno=" + enc(jmeno(jmeno));
    }

    /** "sha1|nastaveni" z C++ (onlineOtisk) -> {otisk, nastaveni}; null = zadna hra. */
    static String[] otisk(String s) {
        if (s == null) return null;
        int i = s.indexOf('|');
        if (i != 40) return null;
        String o = s.substring(0, 40).toLowerCase(java.util.Locale.ROOT);
        if (!o.matches("[0-9a-f]{40}")) return null;
        String n = s.substring(41).trim();
        if (!n.matches("[0-9]{1,6}")) return null;
        return new String[] { o, n };
    }

    /** Radky klic=hodnota (NativeSegaCoreBridge.onlineStav) -> mapa. */
    static Map<String, String> stav(String radky) {
        Map<String, String> m = new LinkedHashMap<>();
        if (radky == null) return m;
        for (String r : radky.split("\n")) {
            int i = r.indexOf('=');
            if (i > 0) m.put(r.substring(0, i), r.substring(i + 1));
        }
        return m;
    }

    /** Id hry na serveru N&P ("sega-0123456789abcdef"), jinak "". */
    static String serverId(String s) {
        return s != null && s.matches("(atari|sega|ps1)-[0-9a-f]{16}") ? s : "";
    }

    /** Cislo hry (4 cislice), jinak "". */
    static String kod(String s) {
        if (s == null) return "";
        String k = s.trim();
        return k.matches("[0-9]{4}") ? k : "";
    }

    /** Nazev hry pro seznam her na enginu: jmeno souboru bez pripony, nejvys 60 znaku. */
    static String nazevHry(String soubor) {
        if (soubor == null) return "";
        String s = soubor;
        int l = Math.max(s.lastIndexOf('/'), s.lastIndexOf('\\'));
        if (l >= 0) s = s.substring(l + 1);
        int t = s.lastIndexOf('.');
        if (t > 0 && s.length() - t <= 5) s = s.substring(0, t);
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            b.append(c < 0x20 || c == 0x7F ? ' ' : c == '_' ? ' ' : c);
        }
        String r = b.toString().trim();
        return r.length() > 60 ? r.substring(0, 60) : r;
    }

    /**
     * B358: klic pro porovnani nazvu her: jen mala pismena a-z a cislice, vse ostatni oddeluje slova (jedna mezera).
     * Proc tak hrube: HRY uklada soubory pres safeFileName ([^A-Za-z0-9._-] -> '_'), hra z CARTRIDGE ma puvodni jmeno -
     * "Golden Axe II (World)" i "Golden_Axe_II__World_" tak daji "golden axe ii world". Ze je to opravdu tataz hra,
     * overi az otisk po spusteni. Stranka Segy (onlineKlic) dela presne totez.
     */
    static String klicNazvu(String nazev) {
        if (nazev == null) return "";
        StringBuilder b = new StringBuilder();
        boolean mezera = false;
        for (int i = 0; i < nazev.length(); i++) {
            char c = nazev.charAt(i);
            if (c >= 'A' && c <= 'Z') c = (char) (c + ('a' - 'A'));
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) { mezera = b.length() > 0; continue; }
            if (mezera) { b.append(' '); mezera = false; }
            b.append(c);
        }
        return b.toString();
    }

    /**
     * B358: hra hrace 1 je z jeho telefonu (ne ze serveru N&P) - ma ji tenhle telefon v HRY? Vrati jmeno souboru
     * z knihovny se stejnym nazvem hry (nazevHry), jinak null. Ze je to opravdu tataz hra, overi az otisk po spusteni.
     */
    static String najdiVKnihovne(java.util.List<String> soubory, String hraMistnosti) {
        String k = klicNazvu(hraMistnosti);
        if (k.isEmpty() || soubory == null) return null;
        for (String s : soubory) {
            if (s != null && !s.endsWith(".state") && klicNazvu(nazevHry(s)).equals(k)) return s;
        }
        return null;
    }

    /** B358: seznam her na enginu (JSON /api/v1/online/mistnosti) do logu: "2 (sega 1234, ps1 5555)", "0". */
    static String popisSeznamu(String json) {
        if (json == null) return "?";
        java.util.regex.Matcher k = java.util.regex.Pattern.compile("\"kod\"\\s*:\\s*\"([0-9]{4})\"").matcher(json);
        java.util.regex.Matcher s = java.util.regex.Pattern.compile("\"system\"\\s*:\\s*\"([a-z0-9]{1,10})\"").matcher(json);
        StringBuilder b = new StringBuilder();
        int n = 0;
        while (k.find()) {
            b.append(n++ == 0 ? " (" : ", ").append(s.find() ? s.group(1) : "?").append(' ').append(k.group(1));
        }
        return n + (n > 0 ? b.append(')').toString() : "");
    }

    /** Ma telefon tutez hru jako hrac 1? (otisk z C++ proti otisku z mistnosti) */
    static boolean stejnaHra(String mujOtisk, String otiskMistnosti, String nastaveniMistnosti) {
        String[] o = otisk(mujOtisk);
        if (o == null || otiskMistnosti == null) return false;
        if (!o[0].equalsIgnoreCase(otiskMistnosti.trim())) return false;
        return nastaveniMistnosti == null || nastaveniMistnosti.isEmpty() || o[1].equals(nastaveniMistnosti.trim());
    }

    private static String enc(String s) {
        try { return URLEncoder.encode(s == null ? "" : s, "UTF-8"); } catch (UnsupportedEncodingException e) { return ""; }
    }
}
