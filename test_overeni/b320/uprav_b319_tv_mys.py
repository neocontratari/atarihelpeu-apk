# B320 regrese: test B319 mysi na strance WEB TV - kontrola 4) "stranka = B318 + jen B319" rozsirena
# o 3 zmeny B320 (verze appky -> stranka se po aktualizaci obnovi sama). Git soubor vrati.
p = 'test_b319_tv_mys.py'
s = open(p, encoding='utf-8').read()
old = "    ocek = STARA318\n"
new = """    # B320: + verze appky (NAP_VERZE, napVerze v pollFps) - stranka se po aktualizaci appky obnovi sama
    _nv = CZ[CZ.index('function napVerze(t){'):CZ.index('function pollFps(){')]
    zmeny += [
        ("<!doctype html><html><head><meta charset='utf-8'>",
         "<!doctype html><html><head><meta charset='utf-8'><script>window.NAP_VERZE=0/*NAP_VERZE*/;</script>"),
        ("function pollFps(){if(window.napPollBezi", _nv + "function pollFps(){if(window.napPollBezi"),
        ("then(function(t){clearTimeout(tm);window.napPollBezi=0;",
         "then(function(t){clearTimeout(tm);window.napPollBezi=0;if(napVerze(t))return;"),
    ]
    ocek = STARA318
"""
assert s.count(old) == 1
s = s.replace(old, new)
old2 = "'4) cesky: stranka WEB TV = B318 + jen B319 (%d zmen: v=319 v adrese proudu, kopie a JPEG v polovicnim rozliseni, log, mys nejvys po 33 ms)'"
new2 = "'4) cesky: stranka WEB TV = B318 + jen B319 + B320 (%d zmen: v=319 v adrese proudu, kopie a JPEG v polovicnim rozliseni, log, mys nejvys po 33 ms; B320 verze appky - NAP_VERZE, napVerze)'"
assert s.count(old2) == 1
s = s.replace(old2, new2)
open(p, 'w', encoding='utf-8').write(s)
print('upraveno')
