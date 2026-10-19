#!/usr/bin/env python3
# B310: Java bez Androidu (na PC neni Android SDK - preklada az CI). javac prelozi
# zdrojaky appky z B309 (git 7b40d21) i B310 bez knihoven Androidu: chyby "nezname
# tridy Androidu" (Activity, Intent, WebView ...) jsou v obou stejne. NOVA chyba, ktera
# neni trida Androidu (chybejici vlastni metoda / promenna, preklep, syntaxe), by byla
# skutecna chyba B310 - musi jich byt 0. (Overeno pokusem: javac takove chyby hlasi
# i ve tride s neznamym predkem a v anonymnich tridach.)
#   python3 javac_porovnej.py chyby_b309.txt chyby_b310.txt
import collections, re, sys

def nacti(fn):
    L = open(fn, encoding='utf-8', errors='replace').read().split('\n')
    out = []
    i = 0
    while i < len(L):
        m = re.match(r'^(.*?\.java):(\d+): error: (.*)$', L[i])
        if m:
            extra = []
            j = i + 1
            while j < len(L) and not re.match(r'^(.*?\.java):(\d+): (error|warning)', L[j]):
                t = L[j].strip()
                if t.split(':')[0] in ('symbol', 'location', 'required', 'found', 'reason'):
                    extra.append(re.sub(r'\s+', ' ', t))
                j += 1
            out.append((m.group(1).split('/')[-1], m.group(3), ' | '.join(extra), int(m.group(2))))
            i = j
        else:
            i += 1
    return out

def je_android(k):
    _, msg, extra = k
    if msg.startswith('package android') or msg.startswith('package dalvik'):
        return True
    m = re.search(r'symbol: (class|variable) ([A-Za-z_]\w*)', extra)
    if msg == 'cannot find symbol' and m:
        # trida Androidu (Intent, Context, Uri, Base64 ...): jmeno s velkym pismenem
        return m.group(2)[0].isupper()
    return False

o = nacti(sys.argv[1]); n = nacti(sys.argv[2])
co = collections.Counter(x[:3] for x in o)
cn = collections.Counter(x[:3] for x in n)
nove = cn - co
print('B309: %d chyb javac (bez Androidu), B310: %d' % (len(o), len(n)))
skutecne = 0
for k, v in sorted(nove.items()):
    radky = [x[3] for x in n if x[:3] == k][:6]
    a = je_android(k)
    if not a:
        skutecne += v
    print('  %s %dx %s: %s [%s] radky %s' % ('trida Androidu (v novem kodu)' if a else 'SKUTECNA CHYBA', v, k[0], k[1], k[2], radky))
zmizele = co - cn
print('chyb, ktere zmizely se smazanym kodem: %d' % sum(zmizele.values()))
print('VYSLEDEK: skutecnych chyb (chybejici vlastni metoda / promenna, syntaxe): %d' % skutecne)
sys.exit(1 if skutecne else 0)
