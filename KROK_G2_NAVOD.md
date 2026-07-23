# KROK G2 — doostření + zvuk bez tlačítka

## 1) Tlačítko AUDIO na TV je pryč

Odstraněno z pohledu — zvuk se teď zapíná sám hned po otevření stránky.

Poznámka pro jistotu: prohlížeče někdy zakazují automatické spuštění
zvuku, dokud uživatel na stránku neklikne. Ošetřil jsem to tak, že zvuk
naskočí sám, a kdyby ho prohlížeč zablokoval, stačí **kliknout kamkoliv
na stránku** — žádné tlačítko hledat nemusíš.

## 2) Doostření obrazu

Použil jsem osvědčený filtr, který už v tvé aplikaci byl (Laplace),
ale ve třetinově úsporné podobě:

- doostřuje se **malý originál** (320×240 / 640×480), ne velký 1280×720
  — mnohonásobně levnější
- rozdíl se počítá jen ze zelené složky (nejlepší zástupce jasu)
  a přidá se všem barvám — oko rozdíl nepozná, práce je třetinová

**Cena se měří** a každých 300 snímků se vypíše do logu jako
`G doostreni: X ms/snimek`. Podle toho poznáme, jestli se to vyplatí.

## Verze: EMU10-G2-DOOSTRENI

## Co mi pošli

1. Je obraz ostřejší? (hlavně u filmových sekvencí)
2. Kolik FPS teď ukazuje HIGH — spadly kvůli doostření?
3. Zvuk naskočí sám bez klikání?
4. Z logu řádek `G doostreni: ... ms/snimek`

Kdyby doostření sebralo moc FPS, snížíme sílu nebo ho pustíme jen na
menší rozlišení. Kdyby bylo naopak málo znát, přitvrdíme.
