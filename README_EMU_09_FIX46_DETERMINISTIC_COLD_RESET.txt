EMU-09 FIX46 DETERMINISTIC COLD RESET

Základ je FIX45/FIX44/FIX39, ale hlavní oprava je deterministický cold reset před každým načtením XEX. Čistí RAM, CPU stav, inputy, keyboard queue, autostart timery, DLI/PMG cache, framebuffer a pomocné JS stavy, aby první a druhé načtení stejné hry nedávalo jiné výsledky. PiTT-KiTT zůstává referenční hra. Pitstop II má stále vypnutý chybný generic PMG overlay a používá stabilní frame capture.
