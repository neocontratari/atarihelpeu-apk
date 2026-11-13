package eu.atarihelp.emu10;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * B321: maly cteni JSON (odpovedi serveru N&P) - bez knihoven, aby sel stejny kod
 * otestovat i na PC v cistem Jave. Vysledek: Map / List / String / Double / Boolean / null.
 */
public final class NapJson {
    private final String s;
    private int i;

    private NapJson(String s) { this.s = s; }

    /** Rozebere JSON; pri chybe vraci null. */
    public static Object cti(String text) {
        if (text == null) return null;
        try {
            NapJson p = new NapJson(text);
            p.mezery();
            Object v = p.hodnota();
            p.mezery();
            return p.i == p.s.length() ? v : null;
        } catch (RuntimeException e) {
            return null;
        }
    }

    /** JSON objekt (Map), nebo null. */
    @SuppressWarnings("unchecked")
    public static Map<String, Object> objekt(String text) {
        Object o = cti(text);
        return o instanceof Map ? (Map<String, Object>) o : null;
    }

    public static String text(Map<String, Object> m, String klic) {
        Object o = m == null ? null : m.get(klic);
        if (o == null) return "";
        if (o instanceof Double) {
            double d = (Double) o;
            if (d == Math.rint(d) && Math.abs(d) < 1e15) return String.valueOf((long) d);
        }
        return String.valueOf(o);
    }

    public static long cislo(Map<String, Object> m, String klic, long vychozi) {
        Object o = m == null ? null : m.get(klic);
        return o instanceof Double ? (long) (double) (Double) o : vychozi;
    }

    public static boolean ano(Map<String, Object> m, String klic) {
        return m != null && Boolean.TRUE.equals(m.get(klic));
    }

    /** Text do JSON vcetne uvozovek. */
    public static String uvoz(String t) {
        StringBuilder b = new StringBuilder("\"");
        if (t != null) for (int k = 0; k < t.length(); k++) {
            char c = t.charAt(k);
            switch (c) {
                case '"': b.append("\\\""); break;
                case '\\': b.append("\\\\"); break;
                case '\n': b.append("\\n"); break;
                case '\r': b.append("\\r"); break;
                case '\t': b.append("\\t"); break;
                default:
                    if (c < 0x20 || c == ' ' || c == ' ') b.append(String.format("\\u%04x", (int) c));
                    else b.append(c);
            }
        }
        return b.append('"').toString();
    }

    // ------------------------------------------------------------------
    private void mezery() {
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') i++;
            else break;
        }
    }

    private Object hodnota() {
        if (i >= s.length()) throw new IllegalStateException("konec");
        char c = s.charAt(i);
        if (c == '{') return objekt();
        if (c == '[') return pole();
        if (c == '"') return retezec();
        if (s.startsWith("true", i)) { i += 4; return Boolean.TRUE; }
        if (s.startsWith("false", i)) { i += 5; return Boolean.FALSE; }
        if (s.startsWith("null", i)) { i += 4; return null; }
        return cislo();
    }

    private Map<String, Object> objekt() {
        Map<String, Object> m = new LinkedHashMap<String, Object>();
        i++;
        mezery();
        if (i < s.length() && s.charAt(i) == '}') { i++; return m; }
        while (true) {
            mezery();
            if (s.charAt(i) != '"') throw new IllegalStateException("klic");
            String k = retezec();
            mezery();
            if (s.charAt(i) != ':') throw new IllegalStateException(":");
            i++;
            mezery();
            m.put(k, hodnota());
            mezery();
            char c = s.charAt(i++);
            if (c == '}') return m;
            if (c != ',') throw new IllegalStateException(",");
        }
    }

    private List<Object> pole() {
        List<Object> l = new ArrayList<Object>();
        i++;
        mezery();
        if (i < s.length() && s.charAt(i) == ']') { i++; return l; }
        while (true) {
            mezery();
            l.add(hodnota());
            mezery();
            char c = s.charAt(i++);
            if (c == ']') return l;
            if (c != ',') throw new IllegalStateException(",");
        }
    }

    private String retezec() {
        StringBuilder b = new StringBuilder();
        i++;
        while (true) {
            char c = s.charAt(i++);
            if (c == '"') return b.toString();
            if (c != '\\') { b.append(c); continue; }
            char e = s.charAt(i++);
            switch (e) {
                case '"': b.append('"'); break;
                case '\\': b.append('\\'); break;
                case '/': b.append('/'); break;
                case 'b': b.append('\b'); break;
                case 'f': b.append('\f'); break;
                case 'n': b.append('\n'); break;
                case 'r': b.append('\r'); break;
                case 't': b.append('\t'); break;
                case 'u': b.append((char) Integer.parseInt(s.substring(i, i + 4), 16)); i += 4; break;
                default: throw new IllegalStateException("escape");
            }
        }
    }

    private Double cislo() {
        int z = i;
        while (i < s.length() && "+-0123456789.eE".indexOf(s.charAt(i)) >= 0) i++;
        if (z == i) throw new IllegalStateException("hodnota");
        return Double.valueOf(s.substring(z, i));
    }
}
