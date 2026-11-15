package eu.atarihelp.emu10;

import java.util.Locale;

/**
 * B327: TEXTY OKNA AKTUALIZACE (cisty Java - bez Androidu, testuje se v JVM).
 *
 * René po B326: "člověk neví, jestli se provádí aktualizace - trvá to docela dlouho, než to
 * požádá telefon o povolení ... aplikace se nevypla a nedošlo k automatickému restartu".
 * Okno aktualizace proto od B327 zůstává otevřené od stažení až do instalace a slovy říká,
 * co se děje: kolik je staženo, co má člověk potvrdit a že se appka zavře.
 * Pravda o restartu: Android appku při instalaci zavře a sám ji znovu neotevře. Do Androidu 9
 * ji otevře NapPoAktualizaci (MY_PACKAGE_REPLACED), od Androidu 10 spouštění z pozadí systém
 * nedovolí - tam ji člověk otevře ikonou a appka při startu ukáže, že je nová verze.
 */
final class NapAktualizaceTexty {

    private NapAktualizaceTexty() {}

    /** Do Androidu 9 (API 28) se appka po aktualizaci otevře sama (NapPoAktualizaci). */
    static boolean otevreSeSama(int sdk) {
        return sdk <= 28;
    }

    /** "12,3" (česky) / "12.3" (anglicky) - megabajty s jedním desetinným místem. */
    static String mb(boolean en, long bajtu) {
        return String.format(en ? Locale.US : new Locale("cs", "CZ"), "%.1f", Math.max(0L, bajtu) / 1048576.0);
    }

    /** Během stahování: odkud, kolik z kolika, procenta, rychlost. celkem <= 0 = velikost neznámá. */
    static String stahovani(boolean en, boolean zeServeru, long mam, long celkem, long bajtuZaS) {
        String hlava = zeServeru
                ? (en ? "Downloading the new version from the N&P server…" : "Stahuji novou verzi ze serveru N&P…")
                : (en ? "Downloading the new version from atarihelp.eu…" : "Stahuji novou verzi z atarihelp.eu…");
        String kolik = celkem > 0
                ? mb(en, mam) + " / " + mb(en, celkem) + " MB  (" + Math.min(100L, mam * 100L / celkem) + " %)"
                : mb(en, mam) + " MB";
        String rychlost = bajtuZaS > 0 ? "  ·  " + mb(en, bajtuZaS) + " MB/s" : "";
        String rada = en ? "Keep the app open, it takes a while." : "Nech appku otevřenou, chvíli to trvá.";
        return hlava + "\n\n" + kolik + rychlost + "\n\n" + rada;
    }

    /** Staženo, předává se Androidu (commit relace). */
    static String predavam(boolean en) {
        return en ? "✔ Downloaded – handing it over to Android to install…"
                  : "✔ Staženo – předávám Androidu k instalaci…";
    }

    /** Po předání Androidu: co přijde a co má člověk udělat. */
    static String poPredani(boolean en, int sdk, long stazeno) {
        final boolean sama = otevreSeSama(sdk);
        if (en) return "✔ Downloaded (" + mb(true, stazeno) + " MB).\n\n"
                + "Android is installing it now – it may take a while.\n\n"
                + "• If the phone asks, tap INSTALL.\n"
                + (sama ? "• The app then closes and opens again by itself (if not, open it from its icon)."
                        : "• The app then closes – open it again from its icon. On start it shows that the new version is installed.");
        return "✔ Staženo (" + mb(false, stazeno) + " MB).\n\n"
                + "Teď instaluje Android – může to chvíli trvat.\n\n"
                + "• Když se telefon zeptá, klepni na INSTALOVAT.\n"
                + (sama ? "• Appka se pak zavře a sama se znovu otevře (kdyby ne, otevři ji ikonou)."
                        : "• Appka se pak zavře – otevři ji znovu ikonou. Při startu ukáže, že je nová verze nainstalovaná.");
    }

    /** Android otevřel své okno s povolením instalace (STATUS_PENDING_USER_ACTION). */
    static String potvrd(boolean en, int sdk) {
        final boolean sama = otevreSeSama(sdk);
        if (en) return "The phone asks whether to install – tap INSTALL.\n\n"
                + (sama ? "The app then closes and opens again by itself."
                        : "The app then closes – open it again from its icon.");
        return "Telefon se ptá, jestli instalovat – klepni na INSTALOVAT.\n\n"
                + (sama ? "Appka se pak zavře a sama se znovu otevře."
                        : "Appka se pak zavře – otevři ji znovu ikonou.");
    }

    /** Hláška při prvním startu nové verze (B328 okno, B327 Toast). */
    static String novaVerze(boolean en, String nazev) {
        return (en ? "✔ New version installed: " : "✔ Nainstalována nová verze: ") + nazev;
    }

    /** B328: titulek okna při prvním startu nové verze. */
    static String novaVerzeTitulek(boolean en) {
        return en ? "Update done" : "Aktualizace hotová";
    }

    /**
     * B328: do logu - co udělal přijímač MY_PACKAGE_REPLACED (NapPoAktualizaci). casMs = kdy (0 = žádná
     * zpráva od systému nepřišla), nyni = teď, vysledek = co zapsal.
     */
    static String prijimacDoLogu(long casMs, String vysledek, long nyni) {
        if (casMs <= 0) return "prijimac: zadna zprava od systemu (MY_PACKAGE_REPLACED neprisla)";
        return "prijimac: zprava od systemu pred " + Math.max(0L, (nyni - casMs) / 1000L) + " s, " + (vysledek == null ? "?" : vysledek);
    }

    /**
     * Je tenhle start prvním po aktualizaci? byla = verze uložená při minulém startu (0 = zatím
     * nikdy - appky do B326 ji neukládaly), ted = versionCode, prvniInstalace / posledniAktualizace
     * z PackageInfo, nyni = čas. Bez uložené verze: aktualizace (ne nová instalace) před méně než
     * 30 minutami - tak se pozná i přechod B326 -> B327.
     */
    static boolean prvniStartPoAktualizaci(int byla, int ted, long prvniInstalace, long posledniAktualizace, long nyni) {
        if (byla > 0) return byla < ted;
        return posledniAktualizace > prvniInstalace && nyni - posledniAktualizace >= 0
                && nyni - posledniAktualizace < 30L * 60L * 1000L;
    }
}
