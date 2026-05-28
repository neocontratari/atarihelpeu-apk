EMU-09 FIX6 INTRO AUDIO

Základ: EMU09 FIX5 PLAY FAST.

Změny:
- hlavní SPUSTIT už nedělá blokující bootGameStart skok přes intro; emulátor běží od RUNAD postupně přes requestAnimationFrame
- START se mačká normálně tlačítkem START, takže lze vidět intro
- renderVideo rozlišuje textové display-list obrazovky intro/lose/win/highscore a 2BPP herní obraz
- v herním režimu se neloguje každý snímek, aby WebView nezamrzalo
- defaultní rychlost je RYCHLEJI, protože na telefonu vyšla nejlépe
- přidán první jednoduchý POKEY/WebAudio zvuk přes AUDF/AUDC registry

Poznámka:
Zvuk není přesná POKEY emulace. Je to první funkční zvuková vrstva, aby bylo slyšet intro/hra/střely/skok.
