# B54 — VIDEOPAMĚŤ V 16 BITECH (versionCode 102)

## Co se změnilo v rendereru

Renderer bral textury **z paměti procesoru**. Změřil jsem, že to nemůže stačit:

    Stránky, ze kterých BIOS texturuje:
      x=640 -> 14/64 bloku od procesoru      x=832 -> 11/64
      x=704 -> 32/64                         x=896 -> 20/64
    Palety (y=384): 1/4 bloku od procesoru

Víc než polovinu textur a tři čtvrtiny palet si BIOS **kreslí grafikou** — a to
se do paměti procesoru nikdy nevrátí.

B54 to obrací: texturuje se **z obrazu**, ve kterém je všechno (zápisy
procesoru se do něj blitují, grafikou kreslené věci tam vznikají přímo).

Aby to šlo, musel obraz dostat přesný formát PlayStation: **5551**, tedy 5 bitů
na barvu a bit masky. Dřív byl 8 bitů na složku — vypadalo to stejně, ale
hodnota už nešla přečíst zpátky jako číslo, takže z něj nešly brát palety.
Přesně na tom ztroskotal B43.

Změny v `naples2_gl.c`:
1. `tex_out` i `tex_vram` mají formát 5551
2. stínovač skládá 16bitovou hodnotu z 5551 (a čte s otočeným Y)
3. texturovací textura se plní kopií z obrazu, ne přenosem z paměti jádra
4. kopie uvnitř videopaměti se dělá na grafice

## Jak jsem to ověřil

Postavil jsem si softwarové OpenGL ES a **přeložil jádro s naples2 u sebe**.
Nabootoval jsem BIOS bez disku a nechal tvůj renderer kreslit. Nejdřív jsem
reprodukoval přesně tvoji chybu (zelené bloky SONY), pak jsem to opravoval
a díval se na výsledek. Obrázek `BIOS_pred_a_po.png` je vlevo předtím,
vpravo potom — obojí z tvého rendereru, ne z cizího.

## Co to opravilo a co NE

Opravené: koule na pozadí se kreslí jako koule s hladkým přechodem, text
i rámeček MAIN MENU sedí, palety už se berou i z grafikou kreslených míst.

**NEOPRAVENÉ: zelená změť přes MEMORY CARD a CD PLAYER tam pořád je.**
Ověřil jsem, že nevzniká při kopiích uvnitř videopaměti (zkusil jsem je
přesunout na grafiku, nezměnilo to nic). Hledám dál — teď už ale s tím, že to
vidím u sebe a nemusím na to pálit tvoje testy.

## Riziko, které musíš vědět

Změna formátu obrazu se dotkne i **her**, a hru offline vyzkoušet neumím
(nemám herní disk). Podle mě budou hry vypadat spíš správněji, protože
PlayStation opravdu 5 bitů na barvu má — ale ověřit to musíš ty.

## Co testovat

1. **Hru** — jde obraz? Nezhoršil se? Jede zvuk?
2. BIOS bez disku — porovnej s obrázkem.

Když se hra zhorší, řekni a vrátím to; ten formát jde vypnout jedním místem.
