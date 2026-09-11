# B261 — PS1: skutečná příčina nalezena (versionCode 307)

Rene: "Pojď ještě na tu PS1."

===============================================================================
 MŮJ OMYL Z B259 - A JAK JSEM HO NAŠEL
===============================================================================

V B259 jsem zkoumal a upravoval `plugins/gpu-gles/gpulib_if.c`. Problém:
**tenhle soubor se od buildu B82 vůbec nekompiluje.** Přímo v
`CMakeLists.txt` je komentář, který to říká - do B82 se překládal
ručně psaný `gpu_naples2` (1291 řádků), zatímco 19 377 řádků
hotového a "prověřeného" `gpu_neon` leželo nepoužitých. Od B82 se
používá `gpu_neon`.

Celá moje diagnostika z minula tedy šla do **mrtvého kódu** - proto
se v tvém Doom logu z ní nic neobjevilo. Když jsem si všiml tohohle
komentáře, přesměroval jsem se na skutečně aktivní `gpu_neon`.

===============================================================================
 SKUTEČNÁ PŘÍČINA
===============================================================================

Ve skutečně používaném kódu (`plugins/gpulib/gpu.c`, funkce
`GPUvBlank()`) je prokládaný režim podmíněný **třemi věcmi najednou**:

1. `allow_interlace` - nastavení z appky samotné (ne z jádra)
2. hra o prokládaný režim žádá (její vlastní GPU status)
3. hra chce dvojnásobnou výšku

Jádro se na bod 1 (`allow_interlace`) ptá přes standardní mechanismus
"nastavovacích proměnných" - konkrétně klíč
`pcsx_rearmed_neon_interlace_enable_v2`. A appka má seznam všech
nastavení, na která umí jádru odpovědět (bios, region, paměťové
karty...) - **tenhle konkrétní klíč v tom seznamu vůbec nebyl.**

Bez odpovědi jádro dostane "nenalezeno" a `allow_interlace` zůstane
na výchozí hodnotě - vypnuto. **Žádná hra tedy nikdy nedostala
prokládaný režim, ať o něj žádala jakkoli usilovně** - přesně to
vysvětluje tu rozsypanou grafiku.

===============================================================================
 OPRAVA
===============================================================================

Přidán jeden řádek do existujícího seznamu nastavení - přesně stejným
vzorem, jakým je přidáno všech ostatních ~7 už fungujících položek ve
stejné funkci (BIOS, region, paměťové karty...). Hodnota `"auto"`
(ne rovnou "enabled") - záměrně, protože komentář přímo v `gpu.c` to
vysvětluje: "interlace nevypadá dobře na progresivních displejích" (a
telefon JE progresivní displej) - a režim "auto" má navíc vestavěnou
ochranu proti hrám, které bit nastaví, ale ve skutečnosti video paměť
aktivně nečtou (takže by z prokládání neměly žádný přínos, jen
riziko artefaktů).

===============================================================================
 STEJNÉ OMEZENÍ JAKO PŘEDTÍM
===============================================================================

Nejde mi to tady zkompilovat ani spustit (OpenGL/Android). Ověřil
jsem syntaxi a shodu s už zavedeným, fungujícím vzorem téže funkce -
ne skutečný běh. Otestuješ přes GitHub Actions, jako vždy u PS1 změn.

===============================================================================
 PONAUČENÍ
===============================================================================

Než začnu zkoumat konkrétní soubor, vždycky nejdřív ověřit v
CMakeLists.txt/Android.mk, jestli se ten soubor vůbec kompiluje -
jinak může hodiny dlouhé pátrání směřovat do mrtvého kódu, přesně
jak se stalo v B259.
