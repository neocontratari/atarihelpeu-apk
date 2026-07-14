public class DrawFallbackCheck {
    int drawCalls = 0;
    long pixelCopyDisabledUntil = 0;

    // Simulace MISTA 1: timeout detekce
    void timeoutBranch(boolean hqLiteScreen, long nowTick) {
        pixelCopyDisabledUntil = nowTick + (hqLiteScreen ? 2000L : 12000L);
        if (!hqLiteScreen) drawCalls++;
    }

    // Simulace MISTA 2: explicitni PixelCopy FAIL vysledek
    void explicitFailBranch(boolean hqLiteScreen, long nowTick) {
        pixelCopyDisabledUntil = nowTick + (hqLiteScreen ? 2000L : 5000L);
        if (!hqLiteScreen) drawCalls++;
    }

    // Simulace MISTA 3: pixelCopyAllowed==false, didTimeoutFallback==false (napr. cooldown z drivejska)
    void notAllowedBranch(boolean hqLiteScreen) {
        if (!hqLiteScreen) drawCalls++;
    }

    public static void main(String[] a) {
        System.out.println("=== PS1/Sega/Atari (hqLiteScreen=true): draw() se NIKDY nesmi zavolat ===");
        DrawFallbackCheck ps1 = new DrawFallbackCheck();
        ps1.timeoutBranch(true, 1000);
        ps1.explicitFailBranch(true, 2000);
        ps1.notAllowedBranch(true);
        ps1.notAllowedBranch(true);
        ps1.notAllowedBranch(true);
        System.out.println("drawCalls=" + ps1.drawCalls + "  (cekano: 0)");
        System.out.println(ps1.drawCalls == 0 ? "OK" : "CHYBA - draw() se zavolal pro nativni obsah!");

        System.out.println();
        System.out.println("=== DJ pult / ostatni (hqLiteScreen=false): chovani BEZE ZMENY, draw() funguje jako drive ===");
        DrawFallbackCheck dj = new DrawFallbackCheck();
        dj.timeoutBranch(false, 1000);
        dj.explicitFailBranch(false, 2000);
        dj.notAllowedBranch(false);
        dj.notAllowedBranch(false);
        System.out.println("drawCalls=" + dj.drawCalls + "  (cekano: 4 - vsechny 4 volani presla, presne jako pred touto zmenou)");
        System.out.println(dj.drawCalls == 4 ? "OK - existujici chovani nedotceno" : "CHYBA");

        System.out.println();
        System.out.println("=== Cooldown doba: kratsi pro nativni obsah (rychlejsi navrat k PixelCopy) ===");
        DrawFallbackCheck c1 = new DrawFallbackCheck();
        c1.timeoutBranch(true, 5000);
        System.out.println("PS1 cooldown do: " + c1.pixelCopyDisabledUntil + "  (cekano: 7000 = 5000+2000)");
        DrawFallbackCheck c2 = new DrawFallbackCheck();
        c2.timeoutBranch(false, 5000);
        System.out.println("DJ  cooldown do: " + c2.pixelCopyDisabledUntil + "  (cekano: 17000 = 5000+12000, BEZE ZMENY)");
    }
}
