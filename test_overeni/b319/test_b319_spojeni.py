#!/usr/bin/env python3
"""B319: MYS Z PC JEDNIM SPOJENIM (keep-alive) - SKUTECNY KOD SERVERU Z MainActivity.java V JVM + Chromium.

Rene (log B318): "mam pocit, ze doslo ke zpomaleni pri prenosu LAN". Mys z PC posilala ~25 zprav za
sekundu a kazda otevrela nove TCP spojeni (SYN, SYN-ACK, ACK ... FIN - navic pakety pres Wi-Fi vedle
obrazu). B319: /mys spojeni zustava a dalsi zpravy jdou jim.

  - z MainActivity.java se vytahnou PRESNE metody napTvWebHandleClient (smycka spojeni),
    napTvWebCtiPozadavek, napTvWebDrzetSpojeni, napTvWebHeader a konstanta NAP_TV_DRZ_SPOJENI_MS;
    obsluha pozadavku (napTvWebObsluz) je v testu zjednodusena se STEJNOU smlouvou jako v appce
    (/mys: drzet = napTvWebDrzetSpojeni(req), hlavicka close = !drzet; /status: hlavicka keep-alive,
    drzet = napTvWebDrzetSpojeni(req); vse ostatni jako drive),
  - JVM server (ServerSocket + zasoba vlaken jako v appce) pocita spojeni a pozadavky na kazdem,
  - 1) primo pres socket: 3 zpravy jednim spojenim, hlavicka po kouskach, "Connection: close",
       HTTP/1.0, jiny pozadavek po /mys, 5 s ticha = appka spojeni zavre,
  - 2) Chromium: stranka posila /mys jako stranka WEB TV (XMLHttpRequest, jedna zprava naraz, ~30 / s)
       + /status kazdou sekundu, uprostred 6 s pauza; pocita se, kolik spojeni na to bylo potreba.
Pouziti: python3 test_b319_spojeni.py [pracovni adresar]   -> VYSTUP_test_b319_spojeni.txt
"""
import os
import re
import subprocess
import sys
import time

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JV = os.environ.get('JAVA_HOME', '/usr/lib/jvm/java-21-openjdk-amd64')
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


def metoda(src, hlavicka):
    """zdrojak metody od hlavicky po odpovidajici }"""
    i = src.index(hlavicka)
    j = src.index('{', i)
    hloubka = 0
    k = j
    while True:
        c = src[k]
        if c == '{':
            hloubka += 1
        elif c == '}':
            hloubka -= 1
            if hloubka == 0:
                return src[i:k + 1]
        elif c == '"':
            k += 1
            while src[k] != '"':
                if src[k] == '\\':
                    k += 1
                k += 1
        elif c == "'":
            k += 1
            while src[k] != "'":
                if src[k] == '\\':
                    k += 1
                k += 1
        k += 1


JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
KONST = re.search(r'private static final int NAP_TV_DRZ_SPOJENI_MS = \d+;', JAVA).group(0)
M_KLIENT = metoda(JAVA, 'private void napTvWebHandleClient(Socket s)')
M_CTI = metoda(JAVA, 'static int napTvWebCtiPozadavek(InputStream in, byte[] buf)')
M_DRZET = metoda(JAVA, 'static boolean napTvWebDrzetSpojeni(String req)')
M_HLAV = metoda(JAVA, 'private void napTvWebHeader(OutputStream out, String status, String type, int len, boolean close)')

STRANKA = r"""<!doctype html><html><head><meta charset='utf-8'><title>B319 spojeni</title></head><body><script>
// jako stranka WEB TV (pm.posli): XMLHttpRequest, jedna zprava naraz, dalsi nejdriv 33 ms po minule
window.__v={ok:0,chyb:0,poslano:0,status:0,hotovo:false};
var N1=120,N2=60,pauza=6000,posl=0;
function jedna(i,dal){var x=new XMLHttpRequest();posl=Date.now();x.open('GET','/mys?dx='+i+'&dy=-'+i+'&b=0&t='+posl,true);x.timeout=4000;
 x.onload=function(){if(x.responseText==='PS1:MYS')__v.ok++;else __v.chyb++;dal();};
 x.onerror=x.ontimeout=function(){__v.chyb++;dal();};__v.poslano++;x.send();}
function rada(n,konec){var i=0;(function dalsi(){if(i>=n){konec();return;}i++;var d=33-(Date.now()-posl);
 setTimeout(function(){jedna(i,dalsi);},d>1?d:1);})();}
var st=setInterval(function(){fetch('/status?'+Date.now()).then(function(r){return r.text();}).then(function(){__v.status++;});},1000);
rada(N1,function(){setTimeout(function(){rada(N2,function(){clearInterval(st);__v.hotovo=true;});},pauza);});
</script></body></html>"""

TEST = r"""
import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;

public class TestSpojeni {
  volatile boolean napTvWebRunning = true;
  final Map<Socket, Integer> pozadavkuNaSpojeni = new ConcurrentHashMap<>();
  final Map<Socket, Integer> mysiNaSpojeni = new ConcurrentHashMap<>();
  final AtomicInteger spojeni = new AtomicInteger();
  static String STRANKA = "";

  @@KONST@@

  @@M_KLIENT@@

  @@M_CTI@@

  @@M_DRZET@@

  @@M_HLAV@@

  // ---- zjednodusena obsluha se STEJNOU smlouvou jako MainActivity.napTvWebObsluz ----
  private boolean napTvWebObsluz(Socket s, InputStream in, OutputStream out, byte[] buf, int n) throws Exception {
    boolean drzet = false;
    String req = n > 0 ? new String(buf, 0, n, "ISO-8859-1") : "";
    String path = "/";
    int a = req.indexOf(' '), b = a < 0 ? -1 : req.indexOf(' ', a + 1);
    if (a >= 0 && b > a) path = req.substring(a + 1, b);
    int q = path.indexOf('?');
    if (q >= 0) path = path.substring(0, q);
    pozadavkuNaSpojeni.merge(s, 1, Integer::sum);
    if ("/".equals(path)) {
      byte[] body = STRANKA.getBytes("UTF-8");
      napTvWebHeader(out, "200 OK", "text/html; charset=utf-8", body.length, false);
      out.write(body);
    } else if ("/mys".equals(path)) {
      drzet = napTvWebDrzetSpojeni(req);
      mysiNaSpojeni.merge(s, 1, Integer::sum);
      byte[] body = "PS1:MYS".getBytes("UTF-8");
      napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", body.length, !drzet);
      out.write(body);
    } else if ("/status".equals(path)) {
      byte[] body = "running=true ps1=1 ps1mys=1\n".getBytes("UTF-8");
      napTvWebHeader(out, "200 OK", "text/plain; charset=utf-8", body.length, false);
      out.write(body);
      drzet = napTvWebDrzetSpojeni(req);
    } else {
      byte[] body = "not found\n".getBytes("UTF-8");
      napTvWebHeader(out, "404 Not Found", "text/plain; charset=utf-8", body.length, false);
      out.write(body);
    }
    out.flush();
    return drzet;
  }

  // ---- 1) primo pres socket ----
  static String odpoved(InputStream in) throws IOException {
    StringBuilder sb = new StringBuilder();
    int c, prazdne = 0;
    while ((c = in.read()) >= 0) {
      sb.append((char) c);
      if (sb.toString().endsWith("\r\n\r\n")) break;
    }
    if (c < 0) return null;
    java.util.regex.Matcher m = java.util.regex.Pattern.compile("Content-Length: (\\d+)").matcher(sb);
    int len = m.find() ? Integer.parseInt(m.group(1)) : 0;
    for (int i = 0; i < len; i++) sb.append((char) in.read());
    return sb.toString();
  }
  static boolean zavreno(Socket k, int cekejMs) throws IOException {
    k.setSoTimeout(cekejMs);
    try { return k.getInputStream().read() < 0; } catch (SocketTimeoutException e) { return false; }
  }
  static int ok = 0, chyb = 0;
  static void over(boolean b, String co) { if (b) ok++; else chyb++; System.out.println((b ? "OK     " : "CHYBA  ") + co); System.out.flush(); }

  public static void main(String[] args) throws Exception {
    STRANKA = new String(java.nio.file.Files.readAllBytes(java.nio.file.Paths.get(args[0])), "UTF-8");
    final TestSpojeni t = new TestSpojeni();
    ServerSocket ss = new ServerSocket(0, 50, InetAddress.getByName("127.0.0.1"));
    // zasoba vlaken jako v appce (napTvWebVlakna)
    ThreadPoolExecutor vlakna = new ThreadPoolExecutor(4, 24, 30L, TimeUnit.SECONDS, new SynchronousQueue<>());
    Thread prijem = new Thread(() -> {
      while (true) {
        try { Socket s = ss.accept(); t.spojeni.incrementAndGet(); vlakna.execute(() -> t.napTvWebHandleClient(s)); }
        catch (Throwable e) { return; }
      }
    });
    prijem.setDaemon(true);
    prijem.start();
    final int port = ss.getLocalPort();
    // ---- 1) primo pres socket ----
    try (Socket k = new Socket("127.0.0.1", port)) {
      OutputStream o = k.getOutputStream(); InputStream i = k.getInputStream();
      int dobre = 0;
      for (int n = 0; n < 3; n++) {
        o.write(("GET /mys?dx=" + n + "&dy=0&b=0 HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n").getBytes("ISO-8859-1")); o.flush();
        String r = odpoved(i);
        if (r != null && r.contains("Connection: keep-alive") && r.endsWith("PS1:MYS")) dobre++;
      }
      // hlavicka po kouskach (TCP ji muze rozdelit)
      o.write("GET /mys?dx=9&dy=0&b=1 HTTP/1.1\r\nHo".getBytes("ISO-8859-1")); o.flush();
      Thread.sleep(150);
      o.write("st: x\r\n\r\n".getBytes("ISO-8859-1")); o.flush();
      String r4 = odpoved(i);
      over(dobre == 3 && r4 != null && r4.endsWith("PS1:MYS") && !zavreno(k, 300),
           "1a) 3 zpravy /mys jednim spojenim (odpovedi s Connection: keep-alive), 4. zprava s hlavickou po kouskach taky - spojeni porad otevrene");
      // /status timtez spojenim (stranka se pta kazdou sekundu): odpoved, spojeni zustava
      o.write("GET /status HTTP/1.1\r\nHost: x\r\n\r\n".getBytes("ISO-8859-1")); o.flush();
      String r5 = odpoved(i);
      over(r5 != null && r5.contains("running=true") && !zavreno(k, 300), "1b) /status timtez spojenim: odpoved jako drive, spojeni zustava");
      // jiny pozadavek (napr. obrazek, stranka) timtez spojenim: obslouzi se jako drive a spojeni se zavre
      o.write("GET /neco HTTP/1.1\r\nHost: x\r\n\r\n".getBytes("ISO-8859-1")); o.flush();
      String r6 = odpoved(i);
      over(r6 != null && r6.contains("404") && zavreno(k, 2000), "1b) jiny pozadavek timtez spojenim: odpoved jako drive, pak spojeni zavrene");
    }
    try (Socket k = new Socket("127.0.0.1", port)) {
      k.getOutputStream().write("GET /mys?dx=1&dy=0&b=0 HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n".getBytes("ISO-8859-1"));
      String r = odpoved(k.getInputStream());
      over(r != null && r.contains("Connection: close") && zavreno(k, 2000), "1c) /mys s Connection: close - odpoved a spojeni zavrene (jako B318)");
    }
    try (Socket k = new Socket("127.0.0.1", port)) {
      k.getOutputStream().write("GET /mys?dx=1&dy=0&b=0 HTTP/1.0\r\nHost: x\r\n\r\n".getBytes("ISO-8859-1"));
      String r = odpoved(k.getInputStream());
      over(r != null && r.contains("Connection: close") && zavreno(k, 2000), "1d) HTTP/1.0 (stary prohlizec) - spojeni se zavre jako drive");
    }
    try (Socket k = new Socket("127.0.0.1", port)) {
      k.getOutputStream().write("GET /mys?dx=1&dy=0&b=0 HTTP/1.1\r\nHost: x\r\n\r\n".getBytes("ISO-8859-1"));
      String r = odpoved(k.getInputStream());
      long t0 = System.currentTimeMillis();
      boolean zav = zavreno(k, 8000);
      long ms = System.currentTimeMillis() - t0;
      over(r != null && zav && ms >= NAP_TV_DRZ_SPOJENI_MS - 300 && ms <= NAP_TV_DRZ_SPOJENI_MS + 1500,
           "1e) " + NAP_TV_DRZ_SPOJENI_MS / 1000 + " s ticha: appka spojeni zavre sama (za " + ms + " ms) - vlakno se vrati do zasoby");
    }
    // ---- 2) pro prohlizec ----
    t.pozadavkuNaSpojeni.clear(); t.mysiNaSpojeni.clear(); t.spojeni.set(0);
    System.out.println("PORT " + port);
    System.out.flush();
    BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
    stdin.readLine();
    int mysi = 0, spojMys = 0, nejvic = 0;
    for (int v : t.mysiNaSpojeni.values()) { mysi += v; spojMys++; nejvic = Math.max(nejvic, v); }
    System.out.println("PROHLIZEC spojeni " + t.spojeni.get() + " zprav_mys " + mysi + " spojeni_s_mysi " + spojMys + " nejvic_na_spojeni " + nejvic
                       + " aktivnich_vlaken " + vlakna.getActiveCount());
    System.out.println("VYSLEDEK_JVM " + ok + " " + chyb);
    System.out.flush();
    System.exit(0);
  }
}
"""


def main():
    prac = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ZDE, 'prac_spojeni')
    os.makedirs(prac, exist_ok=True)
    src = (TEST.replace('@@KONST@@', KONST).replace('@@M_KLIENT@@', M_KLIENT).replace('@@M_CTI@@', M_CTI)
           .replace('@@M_DRZET@@', M_DRZET).replace('@@M_HLAV@@', M_HLAV))
    open(os.path.join(prac, 'TestSpojeni.java'), 'w', encoding='utf-8').write(src)
    open(os.path.join(prac, 'stranka.html'), 'w', encoding='utf-8').write(STRANKA)
    tisk('B319 - mys z PC jednim spojenim: skutecny kod serveru z MainActivity.java (napTvWebHandleClient %d znaku, '
         'napTvWebCtiPozadavek, napTvWebDrzetSpojeni, napTvWebHeader, %s)' % (len(M_KLIENT), KONST.split(' = ')[1].rstrip(';') + ' ms'))
    r = subprocess.run([os.path.join(JV, 'bin/javac'), '-d', prac, os.path.join(prac, 'TestSpojeni.java')], capture_output=True, text=True)
    chybyPrekladu = [l for l in (r.stdout + r.stderr).splitlines() if 'error' in l]
    over(r.returncode == 0, 'preklad v JVM (javac): %s' % ('bez chyby' if r.returncode == 0 else chybyPrekladu[:3]))
    if r.returncode != 0:
        tisk(r.stdout + r.stderr)
        return 1
    pr = subprocess.Popen([os.path.join(JV, 'bin/java'), '-cp', prac, 'TestSpojeni', os.path.join(prac, 'stranka.html')],
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
    port = None
    while True:
        l = pr.stdout.readline()
        if not l:
            break
        l = l.rstrip()
        if l.startswith('Picked up JAVA_TOOL_OPTIONS'):
            continue
        if l.startswith('PORT '):
            port = int(l.split()[1])
            break
        tisk(l)
        if l.startswith('OK') or l.startswith('CHYBA'):
            global kontrol, chyb
            kontrol += 1
            if l.startswith('CHYBA'):
                chyb += 1
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        chybyJs = []
        pg.on('pageerror', lambda e: chybyJs.append(str(e)))
        t0 = time.time()
        pg.goto('http://127.0.0.1:%d/' % port)
        v = None
        while time.time() - t0 < 40:
            v = pg.evaluate('window.__v')
            if v and v['hotovo']:
                break
            time.sleep(0.2)
        trvani = time.time() - t0
        b.close()
    pr.stdin.write('konec\n')
    pr.stdin.flush()
    vysl = ''
    for l in pr.stdout:
        l = l.rstrip()
        if l.startswith('PROHLIZEC'):
            vysl = l
    pr.wait(10)
    tisk('   prohlizec: %s (zprav %d, odpovedi PS1:MYS %d, chyb %d, /status %d, %.1f s)' % (vysl, v['poslano'], v['ok'], v['chyb'], v['status'], trvani))
    m = dict(zip(vysl.split()[1::2], map(int, vysl.split()[2::2]))) if vysl else {}
    over(v and v['hotovo'] and v['ok'] == v['poslano'] == 180 and v['chyb'] == 0 and not chybyJs,
         '2) Chromium: %d zprav /mys (jako stranka WEB TV, ~30 / s, uprostred 6 s pauza) - vsechny odpovedi PS1:MYS, zadna chyba' % (v['poslano'] if v else 0))
    over(m.get('zprav_mys') == 180 and m.get('spojeni_s_mysi', 99) <= 4 and m.get('nejvic_na_spojeni', 0) >= 100,
         '2) na 180 zprav mysi bylo potreba jen %d spojeni (nejvic %d zprav jednim) - B318: 180 spojeni'
         % (m.get('spojeni_s_mysi', -1), m.get('nejvic_na_spojeni', -1)))
    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b319_spojeni.txt'), 'w').write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
