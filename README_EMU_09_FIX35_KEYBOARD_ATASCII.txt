FIX35: oprava Pitstop II keyboard. CH $02FC je scan code, OS keyboard rutina vrací ATASCII. Implementován CPU hook na $02A0 přes vektor $E424/$E425. PiTT-KiTT beze změny.
