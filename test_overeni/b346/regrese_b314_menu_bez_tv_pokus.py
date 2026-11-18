#!/usr/bin/env python3
"""B346: test B314 hlavni nabidky a OPTIONS CZ / EN BEZ kroku TV POKUS.

../b314/test_b314_jazyk_menu.py klika na OPTIONS -> TV POKUS (kroky "TV pokus 1" a "TV pokus 2") - v B346 to
tlacitko Rene nechal smazat ("vyvojarske tlacitka smaz"), takze test spadne vyjimkou a zbytek nezkontroluje.
Tenhle obal pusti STEJNY test bez tech dvou kroku a texty TV POKUS z B313 zaradi mezi zamerne zmeny
(UBYLO_PO_B313 'B346') - nic jineho nemeni. Na HEAD B345 puvodni test: 10 kontrol, 0 CHYBA.
Pouziti (z test_overeni/b346): python3 regrese_b314_menu_bez_tv_pokus.py -> VYSTUP_regrese_b314_menu_bez_tv_pokus.txt
"""
import os
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
B314 = os.path.normpath(os.path.join(ZDE, '..', 'b314'))
sys.path.insert(0, B314)
s = open(os.path.join(B314, 'test_b314_jazyk_menu.py'), encoding='utf-8').read()
for krok in ('''    ('TV pokus 1', "document.getElementById('stTvPokus').click()", 150),\n''',
             '''    ('TV pokus 2', "document.getElementById('stTvPokus').click()", 150),\n'''):
    assert s.count(krok) == 1, krok
    s = s.replace(krok, '')
a = 'UBYLO_PO_B313 = {\n'
assert s.count(a) == 1
s = s.replace(a, a + "    'TV POKUS: OBOJÍ': 'B346', 'TV POKUS: JEN ZVUK': 'B346', 'TV POKUS: JEN OBRAZ': 'B346',\n")
a = "open(os.path.join(ZDE, 'VYSTUP_test_b314_jazyk_menu.txt'), 'w', encoding='utf-8')"
assert s.count(a) == 1
s = s.replace(a, "open(%r, 'w', encoding='utf-8')" % os.path.join(ZDE, 'VYSTUP_regrese_b314_menu_bez_tv_pokus.txt'))
sys.argv = [os.path.join(B314, 'test_b314_jazyk_menu.py')]
exec(compile(s, sys.argv[0], 'exec'), {'__name__': '__main__', '__file__': sys.argv[0]})
