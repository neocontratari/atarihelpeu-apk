FIX268 SLOW REAL KEY BASIC TYPE FORWARD CORE

Forward z FIX267/FIX261, zadny rollback.

Opravuje rychlozapis BASIC kodu jinak nez FIX266: FIX266 KGETCH PC hack se nepouziva.
Novy servisni helper posila text jako pomale realne virtualni klavesy po skutecnem BASIC READY.

Pravidla:
- zadny fake READY
- zadny fake SELF TEST
- zadny fake LOAD
- zadny RAM/program inject
- zadny screen RAM write pro rychlozapis
- zadny KGETCH PC jump
- zadny CH scan-code preklad pro FAST text

Stabilni UI/ROM quarantine z FIX267 zustava.
