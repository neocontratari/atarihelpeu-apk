AtariHelp.eu EMU-09 FIX248_SELFTEST_ROMDLIST_BUS_RENDER_CORE

Cil:
- zachovat cisty FIX247 unwrap,
- zlepsit SELF TEST render bez fake menu/obrazovky,
- cist DLIST $5000-$57FF pres realne ROM/bus mapovani, ne raw RAM pole,
- kreslit DMA data ze skutecne RAM ($3000 apod.) podle DLISTu, SDMCTL, CHBASE a PRIOR.

Proc:
FIX247 snapshot ukazal DLIST $51D1 a ANTIC parser videl m6/m8, ale DLIST BYTES byly samé nuly, protoze snapshot/render cetl raw RAM. Self-test DLIST je v ROM okne $5000-$57FF, takze renderer musi pouzit stejne bus mapovani jako CPU/ANTIC, jinak vidi prazdno.

Bez fake:
- nekresli se hotove SELF TEST menu,
- nekresli se fake RAM kostky,
- ctou se ROM DLIST bajty pres optionalRomByteAt/fix213SelfTestRomByteAt,
- data pro obraz jdou z RAM podle LMS v DLISTu.

Log:
BUILD FIX248_SELFTEST_ROMDLIST_BUS_RENDER_CORE
FIX248 SELFTEST ROMDLIST BUS RENDER CHECK
DLIST BYTES BUS ...
DLIST BYTES RAW ...
SELFTEST FIX248 ROM-BUS DLIST render ... pixels ... dataNonZero ... RAM3000nonzero ...
RULE FIX248_ROM_BUS_DLIST_READ_FOR_SELFTEST / RAM_DMA_DATA_FROM_$3000 / SDMCTL_CHBASE_PRIOR_LIVE / OLD_FIX240_RUNTIME_BYPASSED / NO_DRAWN_SELFTEST_FAKE

KODY JSOU STEJNE
