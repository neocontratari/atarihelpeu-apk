package eu.atarihelp.emu10;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * B359: CO JE NOVEHO (test: test_overeni/b359). Rene 23. 11. 2026: "v kazde aktualizaci musi byt videt co je noveho -
 * to je zasada". Zdroj je assets/novinky/novinky.js (window.NAP_NOVINKY = pole JSON) - stejny soubor cte Napoveda.
 * Kazdy build pridava zaznam nahoru (verze = versionCode); CI bez nej APK nesestavi (kontrola_novinek.py).
 * Po aktualizaci appka jednou ukaze zaznamy, ktere telefon jeste nevidel (MainActivity.napNovinky). Ciste Java.
 */
final class NapNovinky {
    /** Telefon, ktery si jeste nic nepamatuje (appka pred B359): ukaze se vse po B357 - B358 nikdo nevidel. */
    static final int VIDENO_PRED_B359 = 400;

    static final class Zaznam {
        final int verze;
        final String build, datum;
        final List<String> cs, en;

        Zaznam(int verze, String build, String datum, List<String> cs, List<String> en) {
            this.verze = verze; this.build = build; this.datum = datum; this.cs = cs; this.en = en;
        }
    }

    /** Vsechny zaznamy ze souboru, novejsi prvni. Chyba v souboru = prazdny seznam (appka kvuli tomu nespadne). */
    static List<Zaznam> vse(String js) {
        List<Zaznam> r = new ArrayList<>();
        if (js == null) return r;
        int m = js.indexOf("NAP_NOVINKY");
        int a = m < 0 ? -1 : js.indexOf('[', m), b = js.lastIndexOf(']');
        if (a < 0 || b < a) return r;
        Object o = NapJson.cti(js.substring(a, b + 1));
        if (!(o instanceof List)) return r;
        for (Object x : (List<?>) o) {
            if (!(x instanceof Map)) continue;
            @SuppressWarnings("unchecked") Map<String, Object> z = (Map<String, Object>) x;
            int verze = (int) NapJson.cislo(z, "verze", -1);
            List<String> cs = texty(z.get("cs")), en = texty(z.get("en"));
            if (verze < 0 || (cs.isEmpty() && en.isEmpty())) continue;
            r.add(new Zaznam(verze, NapJson.text(z, "build"), NapJson.text(z, "datum"), cs, en));
        }
        Collections.sort(r, (p, q) -> Integer.compare(q.verze, p.verze));
        return r;
    }

    /** Zaznamy, ktere telefon jeste nevidel: videno &lt; verze &lt;= ted, novejsi prvni. */
    static List<Zaznam> nove(String js, int videno, int ted) {
        List<Zaznam> r = new ArrayList<>();
        for (Zaznam z : vse(js)) if (z.verze > videno && z.verze <= ted) r.add(z);
        return r;
    }

    /** Nadpis okna: "CO JE NOVÉHO · B359". */
    static String nadpis(List<Zaznam> z, boolean en) {
        String b = z.isEmpty() || z.get(0).build == null ? "" : z.get(0).build;
        return (en ? "WHAT'S NEW" : "CO JE NOVÉHO") + (b.isEmpty() ? "" : " · " + b);
    }

    /** Text okna: po verzich "B359 · 23. 11. 2026" a pod tim odrazky. Anglicky text chybi -> cesky (a naopak). */
    static String text(List<Zaznam> z, boolean en) {
        StringBuilder s = new StringBuilder();
        for (Zaznam x : z) {
            if (s.length() > 0) s.append("\n\n");
            s.append(x.build == null || x.build.isEmpty() ? "v" + x.verze : x.build);
            if (x.datum != null && !x.datum.isEmpty()) s.append(" · ").append(x.datum);
            List<String> t = en ? (x.en.isEmpty() ? x.cs : x.en) : (x.cs.isEmpty() ? x.en : x.cs);
            for (String r : t) s.append("\n• ").append(r);
        }
        return s.toString();
    }

    private static List<String> texty(Object o) {
        List<String> r = new ArrayList<>();
        if (o instanceof List) for (Object x : (List<?>) o) if (x instanceof String && !((String) x).trim().isEmpty()) r.add(((String) x).trim());
        return r;
    }
}
