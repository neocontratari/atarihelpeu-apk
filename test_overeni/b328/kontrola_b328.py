# B328 - kontrola zapojeni v kodu (bez telefonu): mereni vykonu PS1 (jen meri), oprava hlasky XA / CD audio,
# napoveda Retroid Pocket 6 a herni ovladac (CZ i EN), po aktualizaci okno + zapis prijimace do logu.
# Pouziti: python3 kontrola_b328.py   -> VYSTUP_kontrola_b328.txt
import os, re, sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.abspath(os.path.join(ZDE, '..', '..'))
vystup, chyb = [], 0


def kontrola(nazev, ok, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '  -> ' + str(detail)[:400]))


cti = lambda p: open(os.path.join(R, p), encoding='utf-8').read()
lr = cti('app/src/main/cpp/vendor/pcsx_rearmed/frontend/libretro.c')
nn = cti('app/src/main/cpp/nap_ps1_native.cpp')
cz = cti('app/src/main/assets/napoveda/index.html')
en = cti('app/src/main/assets/napoveda/en.html')
npa = cti('app/src/main/java/eu/atarihelp/emu10/NapPoAktualizaci.java')
na = cti('app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java')
vystup.append('B328 - zapojeni: mereni vykonu PS1, hlaska XA / CD audio, napoveda Pocket 6, po aktualizaci')
vystup.append('')

zap = lr[lr.index('int nap_mereni_zapni(void)'):lr.index('void nap_mereni_vypni(void)')]
vyp = lr[lr.index('void nap_mereni_vypni(void)'):lr.index('void nap_mereni_stav(')]
kontrola('M1 libretro.c: 13 obalu (7 GPU + 6 zvuk) v nap_mereni_zapni, vsech 13 vraci nap_mereni_vypni',
         zap.count('NAP_M_OBAL(') == 13 and vyp.count('NAP_M_VRAT(') == 13, (zap.count('NAP_M_OBAL('), vyp.count('NAP_M_VRAT(')))
obaly = re.findall(r'static [a-z0-9_ ]+CALLBACK (nap_w_\w+)\(([^)]*)\) \{([^}]*)\}', lr)
kontrola('M2 kazdy obal zavola puvodni funkci se stejnymi argumenty (a vrati jeji vysledek)',
         len(obaly) == 13 and all(('nap_o_' + jm[6:] + '(') in telo for jm, _, telo in obaly), [o[0] for o in obaly])
kontrola('M3 obal se neobali sam sebou (ukazatel != obal) a NULL se neobaluje',
         '#define NAP_M_OBAL(ukazatel, puvodni, obal) \\\n   if ((ukazatel) != NULL && (ukazatel) != (obal))' in lr)
kontrola('M4 predani obrazu: vout_flip = obal kolem puvodni nap_vout_flip_telo (stejne argumenty)',
         'nap_vout_flip_telo(vram_, vram_ofs, bgr24, x, y, w, h, dims_changed);' in lr and '.pl_vout_flip     = vout_flip,' in lr)
kontrola('M5 hlaska jadra: "zvuk XA %s, CD audio %s" s Config.Xa ? "VYPNUTY" : "ZAPNUTY" (B308 obracene "XA NE")',
         'Config.Xa ? "VYPNUTY" : "ZAPNUTY", Config.Cdda ? "VYPNUTY" : "ZAPNUTY"' in lr and 'zvuk XA %s, CD audio %s' in lr)
kontrola('M6 nap_ps1_native.cpp: mereni se zapne po kazdem startu hry (3 cesty startu)',
         nn.count('ps1_mereni_po_startu();') == 3 and nn.count('ps1_mys_po_startu(') == 4)
kontrola('M7 radek VYKON: hra + kresleni + predani obrazu + zvuk, pomale snimky, jadro procesoru (getcpu), TV divaku',
         'VYKON %.0f s: snimek prumer %.1f ms = hra (procesor PS1) %.1f + kresleni %.1f + predani obrazu %.1f' in nn
         and 'pomalych snimku %d z %llu (nad %.1f ms)' in nn and 'ps1_jadro_ted()' in nn and 'syscall(__NR_getcpu' in nn)
kontrola('M8 mereni nic nenastavuje: v nap_ps1_native.cpp zadna nova volba jadra (nap_core_option_value beze zmeny vuci B327)',
         'pcsx_rearmed_neon_enhancement_enable") == 0) return "enabled"' in nn and 'gpu_thread_rendering' not in nn)

for jaz, t, h3, ovl, verze in (('CZ', cz, '<h3>Herní ovladač a Retroid Pocket 6</h3>', '<h3>Herní ovladač</h3>', 'Pod nadpisem OPTIONS je napsaná verze appky'),
                               ('EN', en, '<h3>Game controller and Retroid Pocket 6</h3>', '<h3>Game controller</h3>', 'The app version is written under the OPTIONS heading')):
    kontrola('N1 napoveda %s: kapitola menu - blok Retroid Pocket 6 / herni ovladac' % jaz, t.count(h3) == 1)
    kontrola('N2 napoveda %s: Atari, Sega i PS1 maji cast "Retroid Pocket 6, Bluetooth, USB"' % jaz, t.count('(Retroid Pocket 6, Bluetooth, USB)</h') == 3,
             t.count('(Retroid Pocket 6, Bluetooth, USB)</h'))
    kontrola('N3 napoveda %s: OPTIONS - blok herni ovladac (3 rezimy) a veta o verzi appky' % jaz, t.count(ovl) == 1 and verze in t)

kontrola('U1 NapPoAktualizaci: zapise cas a vysledek (commit - proces muze hned skoncit)',
         '.putLong(K_CAS, System.currentTimeMillis()).putString(K_VYSLEDEK, vysledek).commit()' in npa)
ps = na[na.index('public static String poStartu(final Activity a)'):]
kontrola('U2 poStartu: okno "Aktualizace hotova" s OK (ne Toast), zapis prijimace do logu a smazani',
         'new AlertDialog.Builder(a).setTitle(NapAktualizaceTexty.novaVerzeTitulek(en))' in ps and 'Toast.makeText' not in ps[:ps.index('public static boolean smiInstalovat')]
         and 'NapAktualizaceTexty.prijimacDoLogu(' in ps and '.remove(NapPoAktualizaci.K_CAS)' in ps)
bg = cti('app/build.gradle')
kontrola('V1 build.gradle: versionCode 371, versionName EMU10-B328-MERENI', 'versionCode 371' in bg and 'versionName "EMU10-B328-MERENI"' in bg)

vystup.append('')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b328.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
