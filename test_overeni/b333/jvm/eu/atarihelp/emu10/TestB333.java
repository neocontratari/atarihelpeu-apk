package eu.atarihelp.emu10;

import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * B333: STRANKY APPKY NA TV Z C++ - Java strana.
 *  Z) NapTvStranka.zmensi: snimek stranky nejvys 1920 x 1080 (vic TV neukaze), sude rozmery, pomer stran,
 *  J) SKUTECNA trida NativeMenuBridge + SKUTECNY nap_menu_gl.cpp (knihovna napmenu pro PC: ../b313/tv_menu_lib_b313.cpp)
 *     pres JNI (spoustet s -Xcheck:jni): strankaOtevri / Stav / Snimek (prima pamet z Bitmapy) / Dalsi (male pole ->
 *     zaporna delka, paket pocka; vetsi pole -> presne ten paket) / Zvuk (48 kHz z prehravace) / Zavri / Log,
 *     obycejny (ne primy) ByteBuffer se odmitne bez padu.
 * Spusteni (z test_overeni/b333): viz JAK_SPUSTIT.txt (B).
 */
public final class TestB333 {
    static int ok = 0, chyb = 0;

    static void over(boolean b, String co) {
        if (b) ok++; else chyb++;
        System.out.println((b ? "OK     " : "CHYBA  ") + co);
    }

    static int u16(byte[] b, int o) { return (b[o] & 255) | ((b[o + 1] & 255) << 8); }
    static int u32(byte[] b, int o) { return u16(b, o) | (u16(b, o + 2) << 16); }

    public static void main(String[] a) throws Exception {
        System.out.println("B333 - stranky appky na TV z C++: Java strana (NapTvStranka.zmensi, NativeMenuBridge pres JNI)");
        // ---- Z) zmenseni snimku stranky ----
        int[][] p = {
            {1440, 2672, 582, 1080},   // S8 na vysku (log B332)
            {2672, 1440, 1920, 1034},  // S8 na sirku
            {720, 1280, 608, 1080},
            {1080, 2220, 524, 1080},
            {1280, 720, 1280, 720},    // mensi nez Full HD - nezvetsuje se
            {3840, 2160, 1920, 1080},
            {1, 1, 2, 2},
            {0, 0, 0, 0},
        };
        StringBuilder sb = new StringBuilder();
        boolean vse = true;
        for (int[] r : p) {
            int[] z = NapTvStranka.zmensi(r[0], r[1]);
            boolean dobre = z[0] == r[2] && z[1] == r[3] && z[0] % 2 == 0 && z[1] % 2 == 0 && z[0] <= 1920 && z[1] <= 1080;
            if (r[0] > 2) dobre = dobre && Math.abs((double) z[0] / z[1] / ((double) r[0] / r[1]) - 1.0) < 0.005;
            vse &= dobre;
            sb.append(' ').append(r[0]).append('x').append(r[1]).append("->").append(z[0]).append('x').append(z[1]);
        }
        over(vse, "Z) zmenseni snimku stranky (nejvys 1920 x 1080, sude, pomer stran do 0,5 %):" + sb);
        over(NapTvStranka.ZMENA_MS == 66 && NapTvStranka.KLID_MS == 500,
                "Z) foceni: pri zmene nejvys " + (1000 / NapTvStranka.ZMENA_MS) + "x za sekundu, beze zmeny 2x za sekundu");

        // ---- J) JNI ----
        over(NativeMenuBridge.jeNactena(), "J) knihovna napmenu nactena (" + (NativeMenuBridge.jeNactena() ? "ok" : NativeMenuBridge.chybaNacteni()) + ")");
        if (!NativeMenuBridge.jeNactena()) { konec(); return; }
        int stav0 = NativeMenuBridge.strankaStavSafe();
        int id = NativeMenuBridge.strankaOtevriSafe();
        int stav1 = NativeMenuBridge.strankaStavSafe();
        over(id > 0 && (stav0 & 2) == 0 && (stav1 & 2) == 2, "J) strankaOtevri -> prohlizec " + id + ", stav bit1 (Java ma fotit) " + (stav0 & 2) + " -> " + (stav1 & 2));
        final int W = 582, H = 1080;
        ByteBuffer bb = ByteBuffer.allocateDirect(W * H * 4);
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                int i = (y * W + x) * 4;
                bb.put(i, (byte) (y / 54 * 12)); bb.put(i + 1, (byte) (x / 2)); bb.put(i + 2, (byte) 40); bb.put(i + 3, (byte) 255);
            }
        NativeMenuBridge.strankaSnimekSafe(bb, W, H, W * 4);
        byte[] male = new byte[1000];
        int r1 = NativeMenuBridge.strankaDalsiSafe(id, male, 500, 0);
        byte[] velke = new byte[Math.max(1, -r1)];
        int r2 = r1 < 0 ? NativeMenuBridge.strankaDalsiSafe(id, velke, 500, 0) : 0;
        boolean hlavicka = r2 > 32 && velke[4] == 'N' && velke[5] == 'A' && velke[6] == 3 && u16(velke, 8) == W && u16(velke, 10) == H
                && (velke[24] & 255) == 97 && (velke[25] & 255) == 180 && u32(velke, 0) + 4 <= r2;
        over(r1 < 0 && r2 == -r1 && hlavicka, "J) strankaDalsi: male pole -> " + r1 + " (paket pocka), pole " + velke.length + " B -> " + r2
                + " B, paket NA verze 3, " + u16(velke, 8) + "x" + u16(velke, 10) + ", pomer " + (velke[24] & 255) + ":" + (velke[25] & 255));
        int r3 = NativeMenuBridge.strankaDalsiSafe(id, velke, 60, 0);
        over(r3 == 0, "J) bez noveho snimku po 60 ms nic (" + r3 + ")");
        // zvuk prehravace 48 kHz mono (pushPlayerPcm16 -> strankaZvukSafe), pak snimek -> paket nese zvuk
        byte[] pcm = new byte[4800 * 2];
        for (int i = 0; i < 4800; i++) {
            short v = (short) Math.round(9000 * Math.sin(2 * Math.PI * 1000 * i / 48000.0));
            pcm[2 * i] = (byte) v; pcm[2 * i + 1] = (byte) (v >> 8);
        }
        NativeMenuBridge.strankaZvukSafe(pcm, 1, 48000);
        NativeMenuBridge.strankaSnimekSafe(bb, W, H, W * 4);
        byte[] pk = new byte[4 << 20];
        int r4 = NativeMenuBridge.strankaDalsiSafe(id, pk, 500, 0);
        int vz = -1;
        if (r4 > 0) {
            int o = 4 + 28 + 9;
            int dl = u32(pk, o);
            vz = u16(pk, o + 4 + dl);
        }
        over(r4 > 0 && vz >= 4400 && vz <= 4420, "J) zvuk 4800 vzorku 48 kHz -> v paketu " + vz + " vzorku 44,1 kHz (0,1 s)");
        // obycejny ByteBuffer (ne primy) - odmitne se, nic nespadne
        NativeMenuBridge.strankaSnimekSafe(ByteBuffer.allocate(W * H * 4), W, H, W * 4);
        NativeMenuBridge.strankaSnimekSafe(bb, W, H, W * 4 - 4);
        NativeMenuBridge.strankaSnimekSafe(null, W, H, W * 4);
        NativeMenuBridge.strankaZvukSafe(null, 1, 48000);
        int r5 = NativeMenuBridge.strankaDalsiSafe(id, pk, 60, 0);
        over(r5 == 0, "J) neprima pamet / kratky radek / null se odmitne bez padu (zadny novy snimek: " + r5 + ")");
        NativeMenuBridge.strankaZavriSafe(id);
        int stav2 = NativeMenuBridge.strankaStavSafe();
        int r6 = NativeMenuBridge.strankaDalsiSafe(id, pk, 10, 0);
        String log = NativeMenuBridge.strankaLogSafe();
        over((stav2 & 2) == 0 && r6 == Integer.MIN_VALUE && log != null && log.contains("TV PROUD STRANKA: prohlizec " + id + " pripojen")
                && log.contains("odpojen"), "J) strankaZavri: stav bit1 " + (stav2 & 2) + ", dalsi -> MIN_VALUE, log: "
                + (log == null ? "null" : log.trim().split("\n").length + " radky (pripojen / odpojen)"));
        konec();
    }

    static void konec() {
        System.out.println("VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
