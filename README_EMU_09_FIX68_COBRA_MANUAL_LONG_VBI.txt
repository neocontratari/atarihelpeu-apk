EMU-09 FIX68 COBRA MANUAL + LONG VBI

Summary pro GitHub Desktop:
FIX68 cobra manual long vbi

Opravy:
- PiTT-KiTT Remaster: oprava pisma z FIX67 zustava jako referencni stav.
- Super Cobra: odstraneny automaticky START+FIRE helper, ktery preskakoval intro a delal samovolnou strelbu.
- Obecne XEX jadro: VBI budget zvysen pro dlouhe interrupt rutiny, aby se neukoncily napul.
- Obecne XEX jadro: pridany Atari OS CDTMV1-5 countdown casovace po video snimcich.
- CORE TEST TXT: automaticky TXT report a obnova puvodni hry zustava.

Test:
- PiTT-KiTT pismo.
- Super Cobra bez samovolne strelby; START/FIRE jen rucne.
- Donkey Kong intro nechat cekat, potom START; nesmi zustat zamcene na $9542.
- CORE TEST TXT automaticke ulozeni.
