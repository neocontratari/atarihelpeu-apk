#!/usr/bin/env python3
# B313: POSTUP (ODEMCENE KONZOLE) SE PO NOVE INSTALACI NEVRACI
#
# Rene: "I pred zadanim kodu je aplikace odemcena ... apku vzdy mazu a nepreinstalovavam."
# Pricina: Android Auto Backup - appka nemela android:allowBackup, takze platilo vychozi "true"
# a Android po smazani a nove instalaci obnovil SharedPreferences ze zalohy Google (nap_postup =
# napsane radky Atari, minuty Segy -> Sega i PS1 odemcene; v logu B312 je videt i obnovena cesta
# ke Sonicovi a BIOSu "zapamatovan" hned po instalaci).
# B313: allowBackup="false" (a zadna pravidla zalohy, ktera by ho obesla), pri startu log
# "B313 POSTUP_PRI_STARTU ... nova instalace / aktualizace" - Rene to uvidi v 8765/log.
#   python3 kontrola_zalohy.py            -> VYSTUP_kontrola_zalohy.txt
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../../app/src/main'))
man = open(os.path.join(R, 'AndroidManifest.xml'), encoding='utf-8').read()
java = open(os.path.join(R, 'java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
postup = open(os.path.join(R, 'java/eu/atarihelp/emu10/NapPostup.java'), encoding='utf-8').read()
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


app = re.search(r'<application\b[^>]*>', man, re.S)
over(app is not None and re.search(r'android:allowBackup\s*=\s*"false"', app.group(0)) is not None,
     '1) AndroidManifest <application android:allowBackup="false"> - Android po nove instalaci nic neobnovi')
over(app is not None and 'fullBackupContent' not in app.group(0) and 'dataExtractionRules' not in app.group(0),
     '2) zadna pravidla zalohy (fullBackupContent / dataExtractionRules), ktera by zalohu zapnula zpet')
over('getSharedPreferences("nap_postup"' in postup or 'nap_postup' in postup,
     '3) postup je v SharedPreferences "nap_postup" (NapPostup) - to je to, co se drive obnovovalo')
over('B313 POSTUP_PRI_STARTU' in java and 'firstInstallTime == pi.lastUpdateTime' in java,
     '4) pri startu log "B313 POSTUP_PRI_STARTU <postup> | nova instalace / aktualizace" (videt v 8765/log)')
vystup.insert(0, 'B313 - postup se po smazani a nove instalaci appky nevraci (zaloha Androidu vypnuta)')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (4, chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_zalohy.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
