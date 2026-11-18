package eu.atarihelp.emu10;

/**
 * B346: PROC SE PS1 HRA NESPUSTILA - veta pro hrace (JVM, skutecna trida appky NapPravidla).
 *  P1) "PS1_HRA_FAIL obraz" (C++ nap_ps1_obraz.h: soubor hry nema cele sektory CD) -> "obraz disku je poskozeny ..."
 *      s radou (smazat v CD ISO, stahnout znovu, jinak vadny soubor na serveru); CZ i EN
 *  P2) ostatni (PS1_HRA_FAIL jadro, null) -> technicky kod v zavorce jako drive, s diakritikou / anglicky
 *  P3) stranka PS1 z toho dela "HRA SE NESPUSTILA: <veta>" (status "PS1_REMOTE_FAIL boot <veta>") - veta se vejde
 *      do pruhu a zadna nekonci teckou ani nezacina velkym pismenem (navazuje za dvojtecku)
 * Pouziti: viz JAK_SPUSTIT.txt -> VYSTUP_test_b346_java.txt
 */
public class TestB346 {
    static int chyb = 0, testu = 0;
    static final StringBuilder out = new StringBuilder();

    static void over(boolean ok, String co) {
        testu++;
        if (!ok) chyb++;
        String r = (ok ? "OK     " : "CHYBA  ") + co;
        out.append(r).append('\n');
        System.out.println(r);
    }

    public static void main(String[] a) throws Exception {
        String cz = NapPravidla.ps1ProcNespustila("PS1_HRA_FAIL obraz", false);
        String en = NapPravidla.ps1ProcNespustila(" PS1_HRA_FAIL obraz ", true);
        over(cz.equals("obraz disku je poškozený (soubor hry nemá celé sektory CD) – smaž hru v CD ISO a stáhni ji znovu; "
                + "když to nepomůže, je vadný soubor na serveru – podrobnosti v LOGu (deník PS1)")
                && en.equals("the disc image is damaged (the game file does not have whole CD sectors) – delete the game in CD ISO "
                + "and download it again; if that does not help, the file on the server is bad – details in the LOG (PS1 diary)"),
                "P1) poskozeny obraz: \"" + cz + "\" / \"" + en + "\"");
        String j = NapPravidla.ps1ProcNespustila("PS1_HRA_FAIL jadro", false);
        String je = NapPravidla.ps1ProcNespustila("PS1_HRA_FAIL jadro", true);
        String n = NapPravidla.ps1ProcNespustila(null, false);
        over(j.equals("jádro hru nespustilo (PS1_HRA_FAIL jadro) – podrobnosti v LOGu (deník PS1)")
                && je.equals("the core did not start the game (PS1_HRA_FAIL jadro) – details in the LOG (PS1 diary)")
                && n.equals("jádro hru nespustilo () – podrobnosti v LOGu (deník PS1)"),
                "P2) ostatni: \"" + j + "\" / \"" + je + "\"");
        boolean p3 = true;
        for (String s : new String[]{cz, en, j, je}) {
            p3 &= Character.isLowerCase(s.charAt(0)) && !s.endsWith(".") && ("PS1_REMOTE_FAIL boot " + s).length() < 260;
        }
        over(p3, "P3) vety navazuji za \"HRA SE NESPUSTILA: \" (male pismeno, bez tecky), nejdelsi "
                + Math.max(cz.length(), en.length()) + " znaku");

        String souhrn = "VYSLEDEK: " + testu + " testu, " + chyb + " CHYBA";
        out.append(souhrn).append('\n');
        System.out.println(souhrn);
        if (a.length > 0) java.nio.file.Files.write(java.nio.file.Paths.get(a[0]), out.toString().getBytes("UTF-8"));
        System.exit(chyb == 0 ? 0 : 1);
    }
}
