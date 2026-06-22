// Kontrolni script pro BUILD2IA_VBXE_SHARP_PRESENTATION_DAC_SAFE
// Spusteni v koreni projektu: node check_build2ia.js
const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
function ok(cond,msg){ if(!cond){ console.error('FAIL:',msg); process.exit(1); } console.log('OK:',msg); }
ok(s.includes('BUILD2IA_VBXE_SHARP_PRESENTATION_DAC_SAFE'), 'BUILD2IA marker pritomen');
ok(s.includes('presentSharpFrame'), 'nearest-neighbor prezentacni frame pritomen');
ok(s.includes('resizeSharpCanvas'), 'resize sharp canvas pritomen');
ok(s.includes('imageSmoothingEnabled=false'), 'image smoothing vypnute');
ok(s.includes('VBXE TEXT XDL RENDER BUILD2IA'), 'TEXT XDL log aktualizovan na IA');
ok(s.includes('ATR D1 FAST LOAD BUILD2HW') || s.includes('BUILD2HW_VBXE_ATR_D1_FASTLOAD_SAFE'), 'rychly ATR/D1 zaklad zustal');
ok(!s.includes('VBXE OV_COLOR_SHIFT BUILD2HY'), 'HY OV_COLOR_SHIFT experiment se nevratil');
ok(!s.includes('vbxeBlend2('), 'HY HR blend se nevratil');
ok(s.includes('https://atarihelp.eu/?page_id=207'), 'NET HRY odkaz zustava');
console.log('BUILD2IA kontrola hotova.');
