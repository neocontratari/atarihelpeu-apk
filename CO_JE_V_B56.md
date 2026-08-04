# B56 — ZPĚT NA B53 + ÚKLID (versionCode 104)

## 1) Renderer vrácen přesně na B53

Ne opravený, ne přiblížený — **vzatý přímo z tvého repozitáře**, z commitu
`76d9714 emu10_B53_TEXTURA_JEDNOU_ZA_SNIMEK`. Tři soubory
(`naples2_gl.c`, `naples2_gl.h`, `gpulib_if.c`) jsou bajt po bajtu ty, které ti
běžely. Žádné tečky, žádné kousání — je to stav, který jsi sám potvrdil.

Zůstávají opravy, které byly mimo renderer a fungovaly:
odstraněný vadný rámec, evropský region, zavírání mechaniky.

## 2) Smazaný duplicitní bordel

`ps1ActivateNativeView()` obsahovala `if (true) return;` a pod tím celou
**starou zobrazovací cestu** (`NativePs1InPlaceView`, kreslení přes lockCanvas,
31–57 ms na snímek). Nikdy se nespouštěla, jen tam ležela „jako záloha".

Smazáno **319 řádků**: celá třída, její pole i mrtvé větve v zapínání
a vypínání. Obraz PS1 teď kreslí jedna jediná cesta — OpenGL.

Ověřeno: složené závorky v souboru sedí (87 odebraných otevíracích proti
87 zavíracím), na mrtvou třídu nikde nezůstal odkaz.

## 3) Co jsem udělal špatně

Šestnáctibitový formát z B54 měl opravit grafiku BIOSu. **Můj vlastní test
u mě ukázal, že ji neopravil** — zelená změť tam zůstala i po té změně.
Přesto jsem to poslal. To byla chyba a stály tě kvůli ní dva testy navíc.

Ta práce není zahozená, mám ji u sebe. Ale do tvého repozitáře půjde teprve
až u mě na obrazovce to menu BIOSu bude opravdu vypadat správně. Ne dřív.

## Co testovat

Hru a BIOS bez disku. **Musí to být přesně jako B53** — plynulý zvuk,
bez teček. Když ano, jsme na čisté zemi a jdu dál jen na grafiku BIOSu.
