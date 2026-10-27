#!/usr/bin/env python3
"""B318: STARA CESTA TV (JAVA) JEN KDYZ SE NA NI NEKDO DIVA + /mys + /status ps1mys - kontrola Javy.

Rene: "zase vidim JAVA - tok ... ve me furt neco vrta". Java snimala obraz pro TV (snimek ->
zvetseni -> JPEG / H.264 na hlavnim vlakne) porad, kdyz byl prenos zapnuty - i kdyz se nikdo
nedival, nebo kdyz prohlizec bral obraz primo z C++. Od B318 jen do 3 s po posledni zadosti
stareho prohlizece (MJPEG, /frame.jpg) nebo s pripojenym H.264.

  1) SKUTECNA metoda napTvWebStaraCestaKouka() a jeji pole se vytahnou z MainActivity.java,
     prelozi v JVM s nahradami (log, fronta H.264 klientu) a vyzkousi: nikdo -> spi (a zapomene
     posledni snimek), /frame.jpg nebo MJPEG -> snima, 3 s ticho -> spi, H.264 klient -> snima,
     hlasky do logu jen pri zmene,
  2) v Jave: snimani (intro / menu / Atari / PS1 / Sega + PixelCopy) az za touto podminkou, zvuk
     PS1 pro TV se vyzvedava porad (jinak by v jadre pretekal), /frame.jpg a smycka MJPEG hlasi
     zajem, /mys -> NativePs1CoreBridge.pcMysSafe jen s PS1 na obrazovce, /status ps1mys,
     AHPS1.ps1MysZrychleni -> mysZrychleniSafe.
Pouziti: python3 kontrola_java_b318.py PRAC     -> VYSTUP_kontrola_java_b318.txt
"""
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
MOST = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java'), encoding='utf-8').read()
JDK = os.environ.get('JAVA_HOME', '/usr/lib/jvm/java-21-openjdk-amd64')
vystup = []
kontrol = chyb = 0


def tisk(s):
    print(s, flush=True)
    vystup.append(s)


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    tisk(('OK     ' if ok else 'CHYBA  ') + co)


def metoda(src, hlava):
    """telo metody od hlavicky po odpovidajici zavorku"""
    i = src.index(hlava)
    j = src.index('{', i)
    h = 0
    for k in range(j, len(src)):
        if src[k] == '{':
            h += 1
        elif src[k] == '}':
            h -= 1
            if h == 0:
                return src[i:k + 1]
    raise RuntimeError('konec metody nenalezen: ' + hlava)


def main():
    prac = sys.argv[1] if len(sys.argv) > 1 else '/tmp'
    tisk('B318 - stara cesta TV (Java) jen kdyz se nekdo diva, /mys, /status ps1mys - kontrola Javy')

    # ===== 1) skutecna metoda v JVM =====
    pole = re.findall(r'\n\s*(private volatile (?:long|boolean) napTvWebStar\w+ = [^;]+;)', JAVA)
    met = metoda(JAVA, 'private boolean napTvWebStaraCestaKouka()')
    over(len(pole) == 2 and 'napTvWebH264ClientQueues.isEmpty()' in met and '3000L' in met,
         '1) z MainActivity.java: 2 pole (%s) + metoda napTvWebStaraCestaKouka (%d znaku)'
         % (', '.join(p.split()[3] for p in pole), len(met)))
    trida = """import java.util.*;
import java.util.concurrent.*;
public class TestB318StaraCesta {
    final List<LinkedBlockingQueue<byte[]>> napTvWebH264ClientQueues = new CopyOnWriteArrayList<>();
    volatile long napTvWebLastFrameMs = 777;
    volatile byte[] napTvWebJpeg = new byte[]{1, 2, 3};
    final List<String> log = new ArrayList<>();
    void appendNativeLog(String s) { log.add(s); }
    %s
    %s
    static int ok = 0, chyb = 0;
    static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); }
    public static void main(String[] a) {
        TestB318StaraCesta t = new TestB318StaraCesta();
        boolean k = t.napTvWebStaraCestaKouka();
        over(!k && t.napTvWebJpeg == null && t.log.size() == 1 && t.log.get(0).contains("spi - nikdo se nediva"),
             "   prenos zapnuty, nikdo se nediva: Java nesnima, posledni snimek zapomenut, 1 hlaska \\"" + t.log.get(0) + "\\"");
        for (int i = 0; i < 5; i++) t.napTvWebStaraCestaKouka();
        over(t.log.size() == 1, "   dalsich 5 kol smycky: zadna dalsi hlaska (jen pri zmene)");
        t.napTvWebStaryKlientMs = System.currentTimeMillis();   // /frame.jpg nebo smycka MJPEG
        k = t.napTvWebStaraCestaKouka();
        over(k && t.napTvWebLastFrameMs == 0 && t.log.size() == 2 && t.log.get(1).contains("snima - prohlizec bez proudu"),
             "   stary prohlizec chce obraz (/frame.jpg, MJPEG): Java snima, \\"" + t.log.get(1) + "\\" (mezera po spanku neni pomaly snimek)");
        t.napTvWebStaryKlientMs = System.currentTimeMillis() - 2500;
        over(t.napTvWebStaraCestaKouka(), "   2,5 s od posledni zadosti: porad snima");
        t.napTvWebStaryKlientMs = System.currentTimeMillis() - 3100;
        over(!t.napTvWebStaraCestaKouka() && t.log.size() == 3, "   3 s ticho (prohlizec odesel / prepnul na proud z C++): Java zase spi");
        t.napTvWebH264ClientQueues.add(new LinkedBlockingQueue<byte[]>());
        over(t.napTvWebStaraCestaKouka() && t.log.size() == 4, "   pripojeny prohlizec H.264: Java snima (enkoder potrebuje snimky)");
        t.napTvWebH264ClientQueues.clear();
        over(!t.napTvWebStaraCestaKouka() && t.log.size() == 5, "   H.264 odpojen: spi");
        System.out.println("JVM VYSLEDEK: " + ok + " OK, " + chyb + " CHYBA");
        System.exit(chyb == 0 ? 0 : 1);
    }
}
""" % ('\n    '.join(p.replace('private ', '') for p in pole), met.replace('private boolean', 'boolean'))
    d = os.path.join(prac, 'jvm_b318')
    os.makedirs(d, exist_ok=True)
    open(os.path.join(d, 'TestB318StaraCesta.java'), 'w', encoding='utf-8').write(trida)
    pr = subprocess.run([os.path.join(JDK, 'bin/javac'), '-d', d, os.path.join(d, 'TestB318StaraCesta.java')], capture_output=True, text=True)
    chyby = '\n'.join(r for r in pr.stderr.splitlines() if not r.startswith('Picked up JAVA_TOOL_OPTIONS'))
    over(pr.returncode == 0, '1) metoda z appky prelozena v JVM (javac)%s' % ((' ' + chyby[:200]) if chyby else ''))
    if pr.returncode == 0:
        r = subprocess.run([os.path.join(JDK, 'bin/java'), '-cp', d, 'TestB318StaraCesta'], capture_output=True, text=True)
        for radek in r.stdout.splitlines():
            if radek.startswith(('OK', 'CHYBA')):
                over(radek.startswith('OK'), radek.strip()[6:].strip() if radek.startswith('CHYBA') else radek[7:])
        tisk('   ' + (r.stdout.strip().splitlines() or ['?'])[-1])

    # ===== 2) napojeni v Jave =====
    i = JAVA.index('final boolean staraCestaKouka = napTvWebStaraCestaKouka();')
    blok = JAVA[i:i + 600]
    over(re.search(r'boolean gotFromCore = !staraCestaKouka\s*\|\|\s*napTvWebCaptureIntro\(bw, bh\)\s*\|\|\s*napTvWebCaptureAtariCpp\(\)\s*'
                   r'\|\|\s*napTvWebCaptureFromCore\(bw, bh\);', blok) is not None,
         '2) smycka TV: snimani intro / menu / Atari / PS1 / Sega jen kdyz se nekdo diva (jinak "hotovo" - ani PixelCopy okna)')
    za = JAVA[i:i + 2500]
    over('if (gotFromCore) {' in za and za.index('if (gotFromCore) {') < za.index('PixelCopy.request'),
         '   PixelCopy okna appky (DJ, MP3 ...) az za touto podminkou')
    zv = JAVA.rindex('NativePs1CoreBridge.pullTvAudioSafe(tvPs1Pcm)', 0, i)
    over(i - zv < 3000 and 'staraCestaKouka' not in JAVA[zv:i],
         '   zvuk PS1 pro TV se z jadra vyzvedava porad, pred podminkou (jinak by v jadre pretekal - viz komentar Vyzvedavat se musi VZDY)')
    over('napTvWebStaryKlientMs = System.currentTimeMillis();   // B318' in metoda(JAVA, 'private void napTvWebWriteFrame(OutputStream out)'),
         '   /frame.jpg hlasi zajem (napTvWebStaryKlientMs)')
    mj = metoda(JAVA, 'private void napTvWebWriteMjpeg(OutputStream out, Socket sock)')
    over('napTvWebStaryKlientMs = System.currentTimeMillis();' in mj[mj.index('while (napTvWebRunning)'):],
         '   smycka MJPEG hlasi zajem kazde kolo (dokud je prohlizec pripojeny)')
    m = metoda(JAVA, 'private void napTvWebMys(String fullPath, java.io.OutputStream out)')
    over('isPs1OwnerUrl(napTvWebCurrentUrl)' in m and 'NativePs1CoreBridge.pcMysSafe(dx, dy, b)' in m and '"NE_PS1"' in m and
         'napTvWebQueryLong(fullPath, "b", 0) & 3L' in m,
         '2) /mys: jen s PS1 na obrazovce -> NativePs1CoreBridge.pcMysSafe(dx, dy, b), tlacitka jen bit0 / bit1')
    over('} else if ("/mys".equals(path)) {' in JAVA and 'napTvWebMys(fullPath, out);' in JAVA, '   /mys v serveru TV (napTvWebHandleClient)')
    over('+ " ps1mys=" + (ps1Mys ? 1 : 0)' in JAVA and "ms.charAt(0) >= '1' && ms.charAt(0) <= '3'" in JAVA,
         '2) /status: ps1mys=1 jen s PS1 na obrazovce a mysi zapnutou (rezim 1-3 z ps1MysStav)')
    a = metoda(JAVA, 'public String ps1MysZrychleni(int zap)')
    over('NativePs1CoreBridge.mysZrychleniSafe(zap)' in a and '@JavascriptInterface\n        public String ps1MysZrychleni' in JAVA,
         '2) AHPS1.ps1MysZrychleni (stranka PS1) -> NativePs1CoreBridge.mysZrychleniSafe -> C++ ps1MysZrychleni')
    over('private static native String ps1MysZrychleni(int zap);' in MOST and 'public static String mysZrychleniSafe(int zap)' in MOST,
         '   NativePs1CoreBridge: native ps1MysZrychleni + obal mysZrychleniSafe (jen kdyz je knihovna nactena)')

    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    with open(os.path.join(ZDE, 'VYSTUP_kontrola_java_b318.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
