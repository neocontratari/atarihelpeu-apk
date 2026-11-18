#!/usr/bin/env python3
"""B345: test B308 (nahravaci okno stranky PS1) CESKY.

../b308/test_b308_ps1_stranka.py nema most AHJAZYK, takze stranka od B314 v Chromiu mluvi anglicky (jazyk systemu)
a 7 kontrol, ktere hledaji ceske texty (NAHRÁVÁM HRU, RESETUJI ...), pada uz na B344. Tenhle obal pusti STEJNY test
s AHJAZYK.kod() = 'cs' (nic jineho nemeni) - pak kontroluje to, co B308 kontrolovat chtel.
Na B344: 23 kontrol, 0 CHYB. Na B345: 1 CHYBA = zamerne - prubeh stahovani je v monitoru (okno STAHUJI HRU),
pruh pod monitorem uz jen pro chybu (viz test_b345_stranka.py S3, C1-C2).
Pouziti (z test_overeni/b345): python3 regrese_b308_cesky.py -> VYSTUP_regrese_b308_ps1_stranka_cesky.txt
"""
import os
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
B308 = os.path.normpath(os.path.join(ZDE, '..', 'b308'))
s = open(os.path.join(B308, 'test_b308_ps1_stranka.py'), encoding='utf-8').read()
s = s.replace("ZDE = os.path.dirname(os.path.abspath(__file__))", "ZDE = %r" % B308)
assert 'page.add_init_script(MOST)' in s
s = s.replace('page.add_init_script(MOST)',
              "page.add_init_script(\"window.AHJAZYK={kod:function(){return 'cs';}};\"); page.add_init_script(MOST)")
s = s.replace("os.path.join(ZDE, 'VYSTUP_test_b308_ps1_stranka.txt')",
              repr(os.path.join(ZDE, 'VYSTUP_regrese_b308_ps1_stranka_cesky.txt')))
sys.argv = [os.path.join(B308, 'test_b308_ps1_stranka.py')]
exec(compile(s, sys.argv[0], 'exec'), {'__name__': '__main__', '__file__': sys.argv[0]})
