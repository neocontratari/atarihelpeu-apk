#!/usr/bin/env python3
"""B324: aktualizace appky ze serveru N&P - kontrola opravy ve zdrojaku + logiky hlidani (JVM).

Rene: "po odsouhlaseni v apce to skace z apky ven pro dalsi potvrzeni - coz by asi nevadilo - ale pak
se to nesmyslne vraci zpet do apky a stahuje to znova - tedy zada dalsi souhlas".
PRICINA: vysledek instalace (PackageInstaller -> PendingIntent na MainActivity) - MainActivity ma
launchMode "standard" (manifest), takze kazde zavolani vytvorilo NOVOU kopii aktivity. Ta pri startu
(napOverStav "start") videla na serveru novejsi verzi (stara jeste nebyla nainstalovana) a nabidla
aktualizaci znovu; napNabidnutaVerze byla jen v kopii.

  K1 PendingIntent vysledku instalace ma FLAG_ACTIVITY_SINGLE_TOP | FLAG_ACTIVITY_CLEAR_TOP
     (vysledek dostane bezici MainActivity.onNewIntent, zadna nova kopie)
  K2 MainActivity v manifestu bez launchMode (standard) = proto je K1 nutne; jine spousteni MainActivity
     (PendingIntent / startActivity) v appce neni
  K3 Android 12+: setRequireUserAction(USER_ACTION_NOT_REQUIRED) jen pod SDK_INT >= 31
  K4 behem instalace se nenabizi znovu: instalaceBezi v nabidniZeServeru, zeptejSe, nainstaluj a
     MainActivity.napNabidniCekajici; nastavi se na zacatku nainstaluj, konci pri chybe / vysledku;
     rucne z OPTIONS (spustRucne) jde vzdy
  K5 MainActivity: napNabidnutaVerze static, onCreate zpracuje vysledek instalace jen bez
     savedInstanceState, uspech / chyba ukonci hlidani
  K6 JVM: NapAktualizace.instalaceBezi() - 10 minut, pak zase nabizet; instalaceSkoncila()
Pouziti: python3 kontrola_aktualizace.py <android.jar>   -> VYSTUP_kontrola_aktualizace.txt
"""
import os
import re
import subprocess
import sys
import tempfile

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.join(ZDE, '..', '..')
J = os.path.join(R, 'app', 'src', 'main', 'java', 'eu', 'atarihelp', 'emu10')
akt = open(os.path.join(J, 'NapAktualizace.java'), encoding='utf-8').read()
ma = open(os.path.join(J, 'MainActivity.java'), encoding='utf-8').read()
man = open(os.path.join(R, 'app', 'src', 'main', 'AndroidManifest.xml'), encoding='utf-8').read()
vystup = []


def kontrola(nazev, ok, detail=''):
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '   -> ' + str(detail)[:300]))
    print(vystup[-1], flush=True)


def telo(zdroj, hlavicka):
    """telo metody od hlavicky po odpovidajici zavorku"""
    i = zdroj.find(hlavicka)
    if i < 0:
        return ''
    j = zdroj.find('{', i)
    h = 0
    for k in range(j, len(zdroj)):
        h += {'{': 1, '}': -1}.get(zdroj[k], 0)
        if h == 0:
            return zdroj[i:k + 1]
    return ''


m = re.search(r'Intent i = new Intent\(a, MainActivity\.class\);(.*?)PendingIntent\.getActivity', akt, re.S)
kontrola('K1 PendingIntent vysledku instalace: FLAG_ACTIVITY_SINGLE_TOP | FLAG_ACTIVITY_CLEAR_TOP pred getActivity',
         m is not None and 'FLAG_ACTIVITY_SINGLE_TOP' in m.group(1) and 'FLAG_ACTIVITY_CLEAR_TOP' in m.group(1), m.group(1) if m else 'nenalezeno')

akt_ma = re.search(r'<activity\s+android:name="eu\.atarihelp\.emu10\.MainActivity".*?>', man, re.S)
spousteni = [x for x in re.findall(r'new Intent\([^)]*MainActivity\.class\)', akt + ma)]
kontrola('K2 MainActivity v manifestu bez launchMode (standard) a jedine spousteni MainActivity je vysledek instalace',
         akt_ma is not None and 'launchMode' not in akt_ma.group(0) and len(spousteni) == 1, (akt_ma.group(0) if akt_ma else '', spousteni))

m = re.search(r'if \(Build\.VERSION\.SDK_INT >= 31\) \{\s*try \{ sp\.setRequireUserAction\(PackageInstaller\.SessionParams\.USER_ACTION_NOT_REQUIRED\); \}', akt)
kontrola('K3 Android 12+ bez potvrzovani (USER_ACTION_NOT_REQUIRED) jen pod SDK_INT >= 31, v try', m is not None
         and akt.count('setRequireUserAction') == 1)

nab, zep, nai, ruc = (telo(akt, h) for h in ('public static void nabidniZeServeru(', 'private static void zeptejSe(',
                                              'private static void nainstaluj(', 'public static void spustRucne('))
cek = telo(ma, 'private void napNabidniCekajici(')
kontrola('K4 behem instalace se nenabizi znovu (nabidniZeServeru, zeptejSe, nainstaluj, napNabidniCekajici); rucne jde vzdy',
         all('instalaceBezi()' in x for x in (nab, zep, nai)) and 'NapAktualizace.instalaceBezi()' in cek
         and 'instalaceOd = System.currentTimeMillis();' in nai and nai.count('instalaceSkoncila()') >= 2 and 'instalaceSkoncila();' in ruc,
         [len(x) for x in (nab, zep, nai, ruc, cek)])

ins = telo(ma, 'private void napInstalace(Intent i)')
kontrola('K5 napNabidnutaVerze static, vysledek instalace jen pri novem startu (bez savedInstanceState), uspech i chyba ukonci hlidani',
         re.search(r'private static int napNabidnutaVerze = 0;', ma) is not None
         and 'if (savedInstanceState == null) napInstalace(getIntent());' in ma and ins.count('NapAktualizace.instalaceSkoncila();') == 2
         and 'STATUS_SUCCESS' in ins and 'STATUS_PENDING_USER_ACTION' in ins)

# K6: logika hlidani v JVM (android.jar jen kvuli prekladu - metody Androidu se nevolaji)
ajar = sys.argv[1] if len(sys.argv) > 1 else ''
if ajar and os.path.isfile(ajar):
    with tempfile.TemporaryDirectory() as d:
        test = os.path.join(d, 'TestHlidani.java')
        open(test, 'w', encoding='utf-8').write('''package eu.atarihelp.emu10;
public class TestHlidani {
    public static void main(String[] a) throws Exception {
        java.lang.reflect.Field f = NapAktualizace.class.getDeclaredField("instalaceOd");
        f.setAccessible(true);
        boolean nic = !NapAktualizace.instalaceBezi();
        f.setLong(null, System.currentTimeMillis());
        boolean bezi = NapAktualizace.instalaceBezi();
        f.setLong(null, System.currentTimeMillis() - 9L * 60 * 1000);
        boolean devet = NapAktualizace.instalaceBezi();
        f.setLong(null, System.currentTimeMillis() - 11L * 60 * 1000);
        boolean jedenact = !NapAktualizace.instalaceBezi();
        f.setLong(null, System.currentTimeMillis());
        NapAktualizace.instalaceSkoncila();
        boolean konec = !NapAktualizace.instalaceBezi();
        System.out.println(nic + " " + bezi + " " + devet + " " + jedenact + " " + konec);
    }
}
''')
        zdroje = [os.path.join(J, x) for x in os.listdir(J) if x.endswith('.java')] + \
                 [os.path.join(R, 'app', 'src', 'main', 'java', 'eu', 'atarihelp', 'nap', x)
                  for x in os.listdir(os.path.join(R, 'app', 'src', 'main', 'java', 'eu', 'atarihelp', 'nap')) if x.endswith('.java')]
        r = subprocess.run(['javac', '-proc:none', '--release', '17', '-nowarn', '-cp', ajar, '-d', d, test] + zdroje, capture_output=True, text=True)
        if r.returncode != 0:
            kontrola('K6 preklad testu', False, r.stderr[-400:])
        else:
            r = subprocess.run(['java', '-cp', d + os.pathsep + ajar, 'eu.atarihelp.emu10.TestHlidani'], capture_output=True, text=True)
            kontrola('K6 JVM: bez instalace nabizi; behem instalace (i po 9 min) ne; po 10 min / po skonceni zase ano',
                     r.stdout.strip() == 'true true true true true', (r.stdout.strip(), r.stderr[-300:]))
else:
    kontrola('K6 (preskoceno - chybi android.jar)', False, 'python3 kontrola_aktualizace.py <android.jar>')

ok = sum(1 for x in vystup if x.startswith('OK'))
vystup.append('')
vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (ok, len(vystup) - 1 - ok))
print(vystup[-1])
open(os.path.join(ZDE, 'VYSTUP_kontrola_aktualizace.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(0 if ok == len(vystup) - 2 else 1)
