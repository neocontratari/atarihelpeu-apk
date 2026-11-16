# B330: pri prenosu na TV appka nezada o "rovnomerny vykon" (staticka kontrola zdroju appky).
# Spusteni z test_overeni/b330: python3 kontrola_b330.py  -> VYSTUP_kontrola_b330.txt
import io, os, re, sys

KOREN = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
ok = chyb = 0


def kontrola(co, v):
    global ok, chyb
    print(('OK     ' if v else 'CHYBA  ') + co)
    if v:
        ok += 1
    else:
        chyb += 1


java = {}
for d, _, soubory in os.walk(os.path.join(KOREN, 'app/src/main/java')):
    for s in soubory:
        if s.endswith('.java'):
            p = os.path.join(d, s)
            java[os.path.relpath(p, KOREN)] = io.open(p, encoding='utf-8').read()
ma = java['app/src/main/java/eu/atarihelp/emu10/MainActivity.java']
bg = io.open(os.path.join(KOREN, 'app/build.gradle'), encoding='utf-8').read()

zapnuti = [p for p, t in java.items() if re.search(r'setSustainedPerformanceMode\(\s*true\s*\)', t)]
kontrola('T1 nikde v appce setSustainedPerformanceMode(true) (na S8 drzel velka jadra na 962 MHz)', not zapnuti)
st = ma[ma.index('private synchronized String napTvWebStart()'):ma.index('private synchronized String napTvWebStop(String why)')]
kontrola('T2 start prenosu na TV: rovnomerny vykon vypnout (false) a zapsat do logu B330',
         'getWindow().setSustainedPerformanceMode(false)' in st
         and 'B330 TV: rovnomerny vykon (sustained performance) NEZAPINAM' in st)
kontrola('T3 ostatni pri startu prenosu beze zmeny (Wi-Fi nizke latence, obrazovka vzhuru)',
         'napTvWifiNizkaLatence(true);' in st and 'FLAG_KEEP_SCREEN_ON' in st)
sp = ma[ma.index('private synchronized String napTvWebStop(String why)'):]
kontrola('T4 konec prenosu: vypnuti zustava (neskodi)', 'getWindow().setSustainedPerformanceMode(false)' in sp[:1500])
kontrola('V1 build.gradle: versionCode 373, versionName EMU10-B330-TV-VYKON',
         'versionCode 373' in bg and 'versionName "EMU10-B330-TV-VYKON"' in bg)
print('VYSLEDEK: %d OK, %d CHYBA' % (ok, chyb))
sys.exit(1 if chyb else 0)
