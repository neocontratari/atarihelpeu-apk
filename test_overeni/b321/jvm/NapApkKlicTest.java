import eu.atarihelp.emu10.NapApkKlic;
import java.util.Map;
import java.util.TreeMap;

/** Vypise klic NaP z APK tak, jak ho uvidi aplikace (trida z appky). */
public class NapApkKlicTest {
    public static void main(String[] a) {
        for (String f : a) {
            String t = NapApkKlic.precti(f);
            Map<String, String> m = new TreeMap<String, String>(NapApkKlic.hodnoty(f));
            System.out.println(f + " | " + (t == null ? "BEZ_KLICE" : m.toString()));
        }
    }
}
