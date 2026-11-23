#!/usr/bin/env python3
"""B359: ZASADA RENEHO - "v kazde aktualizaci musi byt videt co je noveho" (23. 11. 2026).
Bezi v CI (Build APK, krok "Co je noveho") PRED sestavenim: kdyz nejnovejsi zaznam v assets/novinky/novinky.js
neni pro versionCode z app/build.gradle, APK se nesestavi. Kontroly:
  - prvni (nejnovejsi) zaznam: verze = versionCode, build = Bxxx z versionName
  - kazdy zaznam: verze (cislo), build, datum, cs i en = neprazdne seznamy neprazdnych vet
  - verze shora dolu klesaji, zadna dvakrat
Pouziti: python3 test_overeni/b359/kontrola_novinek.py   (z korene repozitare nebo odkudkoli)
"""
import json
import os
import re
import sys

KOREN = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
chyby = []


def chyba(t):
    chyby.append(t)
    print('CHYBA  ' + t)


gradle = open(os.path.join(KOREN, 'app', 'build.gradle'), encoding='utf-8').read()
m1 = re.search(r'versionCode\s+(\d+)', gradle)
m2 = re.search(r'versionName\s+"([^"]+)"', gradle)
kod = int(m1.group(1)) if m1 else -1
jmeno = m2.group(1) if m2 else ''
mb = re.search(r'\b(B\d{3,4})\b', jmeno.replace('-', ' '))
build = mb.group(1) if mb else ''
print('app/build.gradle: versionCode %d, versionName %s (build %s)' % (kod, jmeno, build or '?'))

js = open(os.path.join(KOREN, 'app', 'src', 'main', 'assets', 'novinky', 'novinky.js'), encoding='utf-8').read()
i = js.find('NAP_NOVINKY')
a = js.find('[', i) if i >= 0 else -1
b = js.rfind(']')
data = []
try:
    data = json.loads(js[a:b + 1]) if a >= 0 and b > a else []
except ValueError as e:
    chyba('novinky.js: pole za NAP_NOVINKY neni cisty JSON (%s)' % e)
if not isinstance(data, list) or not data:
    chyba('novinky.js: zadny zaznam')
    data = []
predchozi = None
for n, z in enumerate(data):
    kde = 'zaznam %d (%s)' % (n + 1, z.get('build', '?') if isinstance(z, dict) else '?')
    if not isinstance(z, dict):
        chyba(kde + ': neni objekt')
        continue
    if not isinstance(z.get('verze'), int):
        chyba(kde + ': verze neni cislo')
    for k in ('build', 'datum'):
        if not isinstance(z.get(k), str) or not z.get(k).strip():
            chyba(kde + ': chybi ' + k)
    for k in ('cs', 'en'):
        t = z.get(k)
        if not isinstance(t, list) or not t or not all(isinstance(x, str) and x.strip() for x in t):
            chyba(kde + ': ' + k + ' musi byt neprazdny seznam vet')
    if isinstance(z.get('verze'), int) and predchozi is not None and z['verze'] >= predchozi:
        chyba(kde + ': verze %d neni mensi nez predchozi %d (nejnovejsi patri nahoru)' % (z['verze'], predchozi))
    if isinstance(z.get('verze'), int):
        predchozi = z['verze']
if data and isinstance(data[0], dict):
    if data[0].get('verze') != kod:
        chyba('nejnovejsi zaznam je pro verzi %s, build ma versionCode %d - pridej nahoru zaznam pro tenhle build' % (data[0].get('verze'), kod))
    if build and data[0].get('build') != build:
        chyba('nejnovejsi zaznam je %s, build je %s' % (data[0].get('build'), build))
    if not chyby:
        print('OK     nejnovejsi zaznam %s (verze %d, %s): %d vet cesky, %d anglicky' % (data[0]['build'], kod, data[0]['datum'], len(data[0]['cs']), len(data[0]['en'])))
print('VYSLEDEK: %d zaznamu, %d CHYBA' % (len(data), len(chyby)))
sys.exit(1 if chyby else 0)
