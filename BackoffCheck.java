public class BackoffCheck {
    static long napTvWebFrameDelayMs = 75;

    // presna kopie nove logiky
    static long simulateTick(String exceptionMsg) {
        long extraDelay = 0;
        try {
            if (exceptionMsg != null) throw new RuntimeException(exceptionMsg);
        } catch (Throwable t) {
            String errMsg = t.getMessage();
            if (errMsg != null && errMsg.contains("backing surface")) {
                extraDelay = 700L;
            }
        }
        return Math.max(35, napTvWebFrameDelayMs) + extraDelay;
    }

    public static void main(String[] a) {
        System.out.println("=== Normalni tik (zadna vyjimka) ===");
        long d1 = simulateTick(null);
        System.out.println("delay=" + d1 + "  (cekano: 75 - beze zmeny)");

        System.out.println();
        System.out.println("=== Jina vyjimka (ne backing surface) ===");
        long d2 = simulateTick("nejaka jina chyba");
        System.out.println("delay=" + d2 + "  (cekano: 75 - normalni rychly retry, beze zmeny chovani)");

        System.out.println();
        System.out.println("=== Presne ta nahlasena vyjimka ===");
        long d3 = simulateTick("Window doesn't have a backing surface!");
        System.out.println("delay=" + d3 + "  (cekano: 775 = 75+700 - zpomaleny retry)");

        System.out.println();
        boolean ok = d1 == 75 && d2 == 75 && d3 == 775;
        System.out.println(ok ? "VSE OK" : "CHYBA");

        System.out.println();
        System.out.println("=== Kolik pokusu za 20 vterin driv vs. ted ===");
        System.out.println("Drive (kazdych ~47ms): " + (20000/47) + " pokusu");
        System.out.println("Ted (kazdych ~775ms):  " + (20000/775) + " pokusu");
    }
}
