#!/usr/bin/env python3
"""B346: test B314 stranky PS1 CZ / EN BEZ kroku PS1 CACHE CESTA a SMAZAT PS1 CACHE.

../b314/stranky_b314.py (STRANKY['ps1']) klika na #psCachePath a #psCacheClear - v B346 tlacitka Rene nechal
smazat ("vyvojarske tlacitka smaz"), takze test hlasi chybu JavaScriptu v tech krocich (c). Tenhle obal pusti STEJNY
test bez tech dvou kroku - nic jineho nemeni. Kontrola a) "texty jako v B313" pada uz na HEAD B345 (zakladna B313).
Pouziti (z test_overeni/b346): python3 regrese_b314_ps1_bez_cache.py -> VYSTUP_regrese_b314_ps1_bez_cache.txt
                                                                     (+ VYSTUP_regrese_b314_texty_ps1_en.txt)
"""
import os
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
B314 = os.path.normpath(os.path.join(ZDE, '..', 'b314'))
sys.path.insert(0, B314)
import stranky_b314                                   # noqa: E402

kroky = stranky_b314.STRANKY['ps1']['kroky']
pred = len(kroky)
kroky[:] = [k for k in kroky if 'psCachePath' not in k[1] and 'psCacheClear' not in k[1]]
assert pred - len(kroky) == 2, (pred, len(kroky))
s = open(os.path.join(B314, 'test_b314_jazyk_stranky.py'), encoding='utf-8').read()
a = "open(os.path.join(ZDE, jm), 'w', encoding='utf-8')"
assert s.count(a) == 1
s = s.replace(a, "open(%r, 'w', encoding='utf-8')" % os.path.join(ZDE, 'VYSTUP_regrese_b314_ps1_bez_cache.txt'))
a = "open(os.path.join(ZDE, 'VYSTUP_texty_%s_en.txt' % jm), 'w', encoding='utf-8')"
assert s.count(a) == 1
s = s.replace(a, "open(os.path.join(%r, 'VYSTUP_regrese_b314_texty_%%s_en.txt' %% jm), 'w', encoding='utf-8')" % ZDE)
sys.argv = [os.path.join(B314, 'test_b314_jazyk_stranky.py'), 'ps1']
exec(compile(s, sys.argv[0], 'exec'), {'__name__': '__main__', '__file__': sys.argv[0]})
