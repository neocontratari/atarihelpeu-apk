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

    // ------------------------------------------------------------------------------------------------
    //  B363: ONLINE ATARI 130XE (Rene 25. 11.: "uz pustit do online atari"). Hra jde druhemu telefonu ve stavu
    //  Atari (START ZE STAVU v C++) - otisk hry tedy engine nepotrebuje (chce 40 hex znaku: nuly), nastaveni =
    //  verze jadra (jen do seznamu). Hrac 2 nic nestahuje ani nespousti.
    // ------------------------------------------------------------------------------------------------
    static final String ATARI_OTISK = "0000000000000000000000000000000000000000";
    static final String ATARI_NASTAVENI = "363";

    /** "nazev\npripraveno\nproc" z C++ (NativeAtariCoreBridge.onlineHra) -> {nazev, "1"/"0", proc}; null = nic. */
    static String[] atariHra(String s) {
        if (s == null) return null;
        String[] c = s.split("\n", -1);
        if (c.length < 2) return null;
        return new String[] { c[0].trim(), "1".equals(c[1].trim()) ? "1" : "0", c.length > 2 ? c[2].trim() : "",
                              c.length > 3 ? c[3].trim() : "" };   // B364: [3] = kolikaty stroj (spusteni hry ze serveru)
    }

    /**
     * B364: hry jednoho systemu na serveru N&P (/api/v1/hry) pro ZALOZIT HRU - Rene: "kdyz nekdo zaklada hru jako
     * novacek - at nevybira ze sveho mobilu ale at najede na nas engine nase hry". {id, nazev, soubor}, serazene podle
     * nazvu (bez diakritiky a velikosti pismen), bez BIOSu (jako stranka HRY). Spatny JSON = prazdny seznam.
     */
    @SuppressWarnings("unchecked")
    static java.util.List<String[]> serveroveHry(String json, String system) {
        java.util.List<String[]> r = new java.util.ArrayList<>();
        Map<String, Object> o = NapJson.objekt(json);
        Object a = o == null ? null : o.get("hry");
        if (!(a instanceof java.util.List) || system == null) return r;
        for (Object x : (java.util.List<Object>) a) {
            if (!(x instanceof Map)) continue;
            Map<String, Object> m = (Map<String, Object>) x;
            if (!system.equals(NapJson.text(m, "system"))) continue;
            String id = NapJson.text(m, "id"), soubor = NapJson.text(m, "soubor"), nazev = NapJson.text(m, "nazev").trim();
            if (!id.matches(java.util.regex.Pattern.quote(system) + "-[0-9a-f]{16}")) continue;
            if (soubor.toLowerCase(java.util.Locale.ROOT).contains("bios")) continue;
            r.add(new String[] { id, nazev.isEmpty() ? nazevHry(soubor) : nazev, soubor });
        }
        r.sort((p, q) -> klicRazeni(p[1]).compareTo(klicRazeni(q[1])));
        return r;
    }

    static String klicRazeni(String s) {
        String t = java.text.Normalizer.normalize(s == null ? "" : s, java.text.Normalizer.Form.NFD).replaceAll("\\p{M}", "");
        return t.toLowerCase(java.util.Locale.ROOT);
    }

    /**
     * B364: v Atari bezi hra ze serveru? Jmeno souboru bez pripony (klicNazvu: HRY ukladaji soubory s '_', C++ jmeno
     * zbavi diakritiky); u archivu (ZIP / RAR) C++ spusti soubor z archivu pod jeho jmenem - staci, ze nebezi samotny
     * ATARI BASIC.
     */
    static boolean jeTaHra(String vAtari, String soubor) {
        if (vAtari == null || soubor == null || vAtari.isEmpty()) return false;
        String s = soubor.toLowerCase(java.util.Locale.ROOT);
        if (s.endsWith(".zip") || s.endsWith(".rar")) return !"ATARI BASIC".equals(vAtari);
        // C++ posila jmeno po bajtech ASCII (diakritika = "??") - klicNazvu bere oboje jako mezeru, takze "Ko??ka" =
        // "Kočka"; jmeno ale NEZKRACOVAT (nazevHry ma 60 znaku a C++ jmeno s "??" je delsi - useklo by se jinde)
        String a = klicSouboru(vAtari), b = klicSouboru(soubor);
        if (a.isEmpty() || b.isEmpty()) return false;
        if (a.equals(b)) return true;
        // C++ posila nejvys 120 bajtu - hodne dlouhe jmeno muze byt useknute (i s priponou)
        String kratsi = a.length() < b.length() ? a : b, delsi = a.length() < b.length() ? b : a;
        return vAtari.length() >= 100 && kratsi.length() >= 24 && delsi.startsWith(kratsi);
    }

    /** B364: klic jmena souboru (bez cesty a pripony, bez zkraceni) pro jeTaHra. */
    static String klicSouboru(String soubor) {
        if (soubor == null) return "";
        String s = soubor;
        int l = Math.max(s.lastIndexOf('/'), s.lastIndexOf('\\'));
        if (l >= 0) s = s.substring(l + 1);
        int t = s.lastIndexOf('.');
        if (t > 0 && s.length() - t <= 5) s = s.substring(0, t);
        return klicNazvu(s);
    }

    /** B364: cislo hry z radku hlidace "B358 ONLINE ceka na hrace 2 - cislo hry 1234" (jinak ""). */
    static String cisloZalozeneHry(String radek) {
        final String z = "B358 ONLINE ceka na hrace 2 - cislo hry ";
        if (radek == null || !radek.startsWith(z)) return "";
        return kod(radek.substring(z.length()).trim());
    }

    /** B364: hra ze serveru se po spusteni nahrala (novy stroj) - muze se zalozit online hra. */
    static boolean hraSpustena(String[] pred, String[] ted) {
        if (ted == null || !"1".equals(ted[1]) || ted[3].isEmpty()) return false;
        return pred == null || !ted[3].equals(pred[3]);
    }

    /**
     * Hry jednoho systemu, ktere na enginu cekaji na hrace 2 (JSON /api/v1/online/mistnosti): {kod, hra, hrac, ceka_s},
     * nejvys 20, v poradi enginu. Spatny JSON = prazdny seznam.
     */
    @SuppressWarnings("unchecked")
    static java.util.List<String[]> mistnosti(String json, String system) {
        java.util.List<String[]> r = new java.util.ArrayList<>();
        Map<String, Object> o = NapJson.objekt(json);
        Object a = o == null ? null : o.get("mistnosti");
        if (!(a instanceof java.util.List)) return r;
        for (Object x : (java.util.List<Object>) a) {
            if (!(x instanceof Map)) continue;
            Map<String, Object> m = (Map<String, Object>) x;
            if (system != null && !system.equals(NapJson.text(m, "system"))) continue;
            String k = kod(NapJson.text(m, "kod"));
            if (k.isEmpty()) continue;
            String h = NapJson.text(m, "hrac");
            r.add(new String[] { k, nazevHry(NapJson.text(m, "hra")), h.isEmpty() ? "?" : jmeno(h), NapJson.text(m, "ceka_s") });
            if (r.size() >= 20) break;
        }
        return r;
    }

    /** Radek seznamu her pro hrace: "1234 – Archon (SM-A125F, čeká 2 min)". */
    static String popisMistnosti(String[] m, boolean en) {
        long s = 0;
        try { s = Long.parseLong(m[3]); } catch (RuntimeException ignored) {}
        String ceka = s < 60 ? (en ? "waiting " + s + " s" : "čeká " + s + " s") : (en ? "waiting " + (s / 60) + " min" : "čeká " + (s / 60) + " min");
        return m[0] + " – " + (m[1].isEmpty() ? "Atari" : m[1]) + " (" + m[2] + ", " + ceka + ")";
    }

    /**
     * Stav online Atari pro hrace (okno ONLINE na pristroji), z radku C++ (onlineStav): faze, cislo hry, druhy hrac,
     * odezva, zpozdeni, start ze stavu, setrne kresleni, konec s duvodem.
     */
    static String atariPopis(Map<String, String> m, boolean en) {
        String faze = m.getOrDefault("faze", "NIC"), kod = m.getOrDefault("kod", ""), druhy = jmeno(m.getOrDefault("druhy", ""));
        String konec = m.getOrDefault("konec", "");
        if (konec.isEmpty()) konec = m.getOrDefault("chyba", "");
        boolean hrac1 = "1".equals(m.get("hrac1"));
        String startStav = m.getOrDefault("start_stav", "0");
        StringBuilder b = new StringBuilder();
        switch (faze) {
            case "PRIPOJUJE":
                b.append(en ? "Connecting to the N&P engine..." : "Připojuji se k enginu N&P...");
                break;
            case "CEKA_NA_HRACE":
                if (kod.isEmpty()) b.append(en ? "Connected, creating the game..." : "Spojeno, zakládám hru...");
                else b.append(en ? "Waiting for player 2.\nGAME NUMBER: " + kod + "\n\nTell it to the other player - they tap ONLINE → JOIN on their Atari. Meanwhile you can keep playing."
                                 : "Čekám na hráče 2.\nČÍSLO HRY: " + kod + "\n\nŘekni ho druhému hráči – na svém Atari klepne ONLINE → PŘIPOJIT SE. Zatím můžeš hrát dál.");
                break;
            case "HRAJE":
                if ("1".equals(startStav)) b.append(en ? "Player 2 joined (" + druhy + "). Your game is still loading - it will be sent as soon as it finishes."
                                                       : "Hráč 2 je tu (" + druhy + "). Hra se ještě nahrává – pošlu ji, jakmile doběhne.");
                else if ("2".equals(startStav)) b.append(en ? "Sending the whole Atari with the game to " + druhy + "..." : "Posílám hráči " + druhy + " celé Atari i s hrou...");
                else if ("3".equals(startStav)) b.append(en ? "Receiving the game from " + druhy + "..." : "Přijímám hru od hráče " + druhy + "...");
                else {
                    b.append(en ? "Playing online with " + druhy : "Hraješ online s " + druhy).append(hrac1 ? (en ? " (you are player 1)" : " (jsi hráč 1)") : (en ? " (you are player 2)" : " (jsi hráč 2)"));
                    b.append(en ? "\nResponse: " : "\nOdezva: ").append(m.getOrDefault("odezva", "?")).append(" ms");
                    b.append(en ? ", control delay: " : ", zpoždění ovládání: ").append(m.getOrDefault("zpozdeni_ted", m.getOrDefault("zpozdeni", "?"))).append(en ? " frames" : " snímků");
                    b.append("J1".equals(m.get("porty")) ? (en ? "\nBoth players on joystick 1." : "\nOba hráči na joysticku 1.")
                                                         : (en ? "\nPlayer 1 = joystick 1, player 2 = joystick 2." : "\nHráč 1 = joystick 1, hráč 2 = joystick 2."));
                    if ("1".equals(m.get("setrne"))) b.append(en ? "\nEconomy drawing: this phone shows every 2nd frame (the game itself runs in full)."
                                                                : "\nŠetrné kreslení: tenhle telefon ukazuje každý 2. snímek (hra sama běží celá).");
                }
                break;
            case "KONEC":
                b.append(en ? "Online ended: " : "Online skončilo: ").append(konec.isEmpty() ? "-" : konec);
                break;
            default:
                b.append(en ? "Online is off." : "Online neběží.");
        }
        return b.toString();
    }

    private static String enc(String s) {
        try { return URLEncoder.encode(s == null ? "" : s, "UTF-8"); } catch (UnsupportedEncodingException e) { return ""; }
    }
}
