public class Sk46Check {
    static boolean djScreenFor(String url) { return url != null && url.contains("/dj/"); }
    static boolean hqLiteScreenFor(String url) {
        return url != null && (url.contains("/emu_ps1/") || url.contains("/emu_sega/") || url.contains("/emu/") || url.contains("/emu_vbxe/") || url.contains("/dj/"));
    }

    public static void main(String[] a) {
        String[][] cases = {
            {"file:///android_asset/dj/index.html", "DJ pult"},
            {"file:///android_asset/emu_vbxe/index.html", "Atari"},
            {"file:///android_asset/emu_ps1/index.html", "PS1"},
            {"file:///android_asset/emu_sega/index.html", "Sega"},
            {"file:///android_asset/player/index.html", "MP3"},
            {"file:///android_asset/index.html", "Domovska"},
        };
        System.out.println("stranka       | djScreen | hqLiteScreen | metoda zachytavani");
        for (String[] c : cases) {
            boolean dj = djScreenFor(c[0]);
            boolean hq = hqLiteScreenFor(c[0]);
            System.out.printf("%-13s | %-8s | %-12s | %s%n", c[1], dj, hq, hq ? "PixelCopy" : "draw()");
        }
        System.out.println();
        boolean djStillTrue = djScreenFor("file:///android_asset/dj/index.html");
        boolean djNowHqLite = hqLiteScreenFor("file:///android_asset/dj/index.html");
        System.out.println("DJ pult: djScreen=" + djStillTrue + " (cekano true - rozliseni nedotceno) hqLiteScreen=" + djNowHqLite + " (cekano true - ted PixelCopy)");
        System.out.println((djStillTrue && djNowHqLite) ? "OK - DJ pult si drzi vlastni rozliseni A ted pouziva PixelCopy" : "CHYBA");
        boolean homeHq = hqLiteScreenFor("file:///android_asset/index.html");
        System.out.println("Domovska stranka: hqLiteScreen=" + homeHq + " (cekano false - tenhle build ji NErresi, zustava draw())");
    }
}
