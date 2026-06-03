FIX108_MULTIGAME_DLIST_ARKANOID_PMG_CORE

Cíl:
- Nehonit jednu hru dokola. FIX108 posouvá sdílené jádro: bezpečnější výběr DLISTu, Donkey Jr rollback z chybného scan-DLISTu, první Arkanoid gameplay stabilizační pokus a opatrnější Montezuma PMG trim.

Změny:
- Donkey Kong Junior: blokuje se chybná generic scan-DLIST promotion, která ve FIX107 povýšila $1FD3 přes použitelný runtime DLIST. Profil nejdřív zkusí živý shadow/HW DLIST a potom stabilní Donkey-family $8031/$8052.
- Generic DLIST arbitration: pokud je základní DLIST dlouhý a kreslitelný, krátký skenovaný kandidát už ho nepřepíše jen kvůli vyššímu skóre.
- Arkanoid III: přidán opatrný stable-DLIST probe $3503, aby po STARTu nebyl preferovaný krátký/přechodový DLIST $0A00. Menu zůstává ruční, bez auto-assistu.
- Montezuma: PMG trim je zmírněný během death/respawn stavu; cílem je držet duchy nízko, ale nevyhodit tělo postavy při znovuzrození.
- Cobra: žádné nové Cobra hacky; menu a FIX106 zlepšení kolizí zůstávají jen jako kontrolní reference.
- Donkey / Arkanoid menu: zachovaný baseline.

Test:
1) Donkey Kong Junior: screenshot a snapshot log první obrazovky a po START/FIRE.
2) Arkanoid III: potvrdit menu, pak START/FIRE do hry a screenshot/log hry.
3) Montezuma: nechat intro do bludiště, umřít a zkontrolovat duchy/respawn.
4) Donkey Kong + Cobra: jen regresní kontrola, že se nezhoršily.
