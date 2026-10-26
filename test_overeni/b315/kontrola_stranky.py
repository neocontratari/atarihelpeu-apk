#!/usr/bin/env python3
"""B315: STRANKA WEB TV = STRANKA B314 + JEN ZMENY ZVUKU.

Obe stranky se vytahnou z MainActivity.java (B314 = git 8629955, B315 = pracovni strom) presne
jako v testech, rozdeli na prikazy (za ; { }) a porovnaji (difflib). Kazde zmenene misto musi
patrit ke zvuku stranky (AVD / setAvd, gp, vy, mereni vystupu, stara cesta: cutover, rawPredej,
pollAudio, rawHlas, sipky v menu, apZvuk, hlaseni). Cokoli jineho = CHYBA (nechtena zmena).
Anglicka verze stranky (napTvJazykStranka) se nemeni - Java ji sklada z te ceske.

Pouziti: python3 kontrola_stranky.py   -> VYSTUP_kontrola_stranky.txt
"""
import difflib
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, ZDE)
import test_b315_zvuk_tv as T   # noqa: E402  (stranka_z_javy, cesty)

KLIC = re.compile(r"\bAVD0?\b|setAvd|\bgp\b|\bvy\b|\brawS\b|\brawN\b|zvMer|zvMerText|\bzm\b|zmB|rawHlas|rawPredej|cutover|"
                  r"napVMenu|active\.length>400|vystup|tagName==='INPUT'|_avdL|"
                  r"byteLength>=4&&!\(ap&&ap\.chce\)")   # pollAudio: kus /audio.raw po startu proudu z C++ se nehraje
vystup = []


def tisk(s):
    print(s)
    vystup.append(s)


def tokeny(s):
    return re.split(r'(?<=[;{}])', s)


def main():
    b314 = T.stranka_z_javy(subprocess.run(['git', '-C', T.KOREN, 'show', T.B314_COMMIT + ':' + T.JAVA_CESTA],
                                           capture_output=True, text=True, check=True).stdout)
    b315 = T.stranka_z_javy(open(os.path.join(T.KOREN, T.JAVA_CESTA), encoding='utf-8').read())
    a, b = tokeny(b314), tokeny(b315)
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    tisk('B315 - stranka WEB TV (MainActivity.napTvWebWriteHtml): B314 %d znaku / %d prikazu, B315 %d znaku / %d prikazu'
         % (len(b314), len(a), len(b315), len(b)))
    mist = chyb = 0
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == 'equal':
            continue
        mist += 1
        stare, nove = ''.join(a[i1:i2]), ''.join(b[j1:j2])
        ok = bool(KLIC.search(stare) or KLIC.search(nove))
        chyb += not ok
        tisk('%s misto %d (%s, prikazy B314 %d-%d -> B315 %d-%d):' % ('OK   ' if ok else 'CHYBA', mist, op, i1, i2, j1, j2))
        tisk('       B314: %s' % (stare[:300] + (' ...' if len(stare) > 300 else '') if stare else '(nic)'))
        tisk('       B315: %s' % (nove[:300] + (' ...' if len(nove) > 300 else '') if nove else '(nic)'))
    # co se nesmelo zmenit: zbytek stranky prikaz po prikazu stejny
    stejne = sum(i2 - i1 for op, i1, i2, j1, j2 in sm.get_opcodes() if op == 'equal')
    tisk('beze zmeny %d z %d prikazu B314' % (stejne, len(a)))
    tisk('VYSLEDEK: %d zmenenych mist, %d CHYBA (nechtena zmena mimo zvuk)' % (mist, chyb))
    open(os.path.join(ZDE, 'VYSTUP_kontrola_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb or not mist else 0)


if __name__ == '__main__':
    main()
