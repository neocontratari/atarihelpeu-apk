// Kontrolni script pro BUILD2HZ_VBXE_ROLLBACK_TO_HX_COLOR_SAFE
// Spusteni v koreni projektu: node check_build2hz.js
const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
function ok(cond,msg){ if(!cond){ console.error('FAIL:',msg); process.exit(1); } console.log('OK:',msg); }
ok(s.includes('BUILD2HZ_VBXE_ROLLBACK_TO_HX_COLOR_SAFE'), 'BUILD2HZ marker pritomen');
ok(s.includes('HY paletovy experiment je odstraneny'), 'HY rollback text pritomen');
ok(!s.includes('VBXE OV_COLOR_SHIFT BUILD2HY'), 'HY OV_COLOR_SHIFT kod/log neni pritomen');
ok(!s.includes('vbxeBlend2('), 'HY HR blend funkce neni pritomna');
ok(s.includes('VBXE TEXT XDL RENDER BUILD2HZ'), 'TEXT XDL log aktualizovan na HZ');
ok(s.includes('ATR D1 FAST LOAD BUILD2HW') || s.includes('BUILD2HW_VBXE_ATR_D1_FASTLOAD_SAFE'), 'rychly ATR/D1 zaklad zustal');
console.log('BUILD2HZ kontrola hotova.');
