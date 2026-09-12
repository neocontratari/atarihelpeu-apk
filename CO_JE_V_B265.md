# B265 — PS1: diagnostika regionu podle tvé hypotézy (versionCode 311)

Rene: "Opravdu sem udělal kontrolu na reálném PS1. Grafika v našem
emu není u této hry dokonalá. Může to být třeba i PAL vs SECAM."

===============================================================================
 PROČ JE TOHLE DOBRÁ STOPA
===============================================================================

Appka podporuje řadu různých BIOS souborů - americký/NTSC
(scph5501, scph1001), evropský/PAL (scph5502, scph7502), japonský
(scph5500) a další. Jádro emulátoru si podle regionu hry vybírá
KONKRÉTNÍ soubor z téhle nabídky.

Pokud je nahraný BIOS jiného regionu, než jaký má disk samotné hry
(například americký/NTSC BIOS s evropskou/PAL verzí Doomu), časování
grafického čipu může být nesouhlasné - přesně tím způsobem, jaký jsi
popsal ("jako špatná grafická karta").

===============================================================================
 CO CHYBĚLO
===============================================================================

Appka dosud vůbec nelogovala, **který konkrétní BIOS soubor** a
**jaký region (NTSC/PAL)** jádro doopravdy používá - jen že "BIOS
nebo hra běží". Bez týhle informace nejde tvoji hypotézu ani
potvrdit, ani vyvrátit.

===============================================================================
 CO JSEM PŘIDAL
===============================================================================

Malá pomocná funkce přímo ve vendor kódu (jen čte už existující
nastavení jádra, nic neupravuje) plus volání z naší vrstvy hned po
startu - zaloguje přesné jméno použitého BIOS souboru a detekovaný
region jako NTSC nebo PAL.

Žádná změna chování jádra - čisté přidání diagnostiky.

===============================================================================
 OVĚŘENÍ
===============================================================================

Ověřena syntaxe a rovnováha závorek v obou dotčených souborech.
Způsob čtení nastavení přesně odpovídá už existujícímu, fungujícímu
vzoru ve stejném souboru. Stejné omezení jako u všech PS1 změn -
nejde zkompilovat ani spustit OpenGL/Android řetězec v tomto
sandboxu.

===============================================================================
 DALŠÍ KROK
===============================================================================

Tvůj další test s tímhle logem přímo potvrdí nebo vyvrátí hypotézu o
regionu/BIOSu - uvidíme černé na bílém, jaký BIOS a jaký region se
skutečně použil.
