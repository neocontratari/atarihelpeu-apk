// Kontrolni script pro BUILD2ID_VBXE_TEXT_EDGE_SNAP_LOG_SAFE
// Spusteni v koreni projektu: node check_build2id.js
const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
function ok(cond,msg){ if(!cond){ console.error('FAIL:',msg); process.exit(1); } console.log('OK:',msg); }
ok(s.includes('BUILD2ID_VBXE_TEXT_EDGE_SNAP_LOG_SAFE'), 'BUILD2ID marker pritomen');
ok(s.includes('edge-snap sampler pro VBXE 80sl text'), 'edge-snap text sampler pritomen');
ok(s.includes('VBXE TEXT XDL RENDER BUILD2ID'), 'BUILD2ID text render log pritomen');
ok(s.includes('memac5FCount'), 'D65F MEMAC throttle pocitadlo pritomno');
ok(s.includes('VBXE MEMAC $D65F: dalsi rychle prepinani potlacuju'), 'D65F spam throttle log pritomen');
ok(s.includes('function vbxeTextFgVisible'), 'text foreground pravidla zustala');
ok(s.includes('function vbxeTextBgColor'), 'text background pravidla zustala');
ok(!s.includes('VBXE OV_COLOR_SHIFT BUILD2HY'), 'HY OV_COLOR_SHIFT neni pritomen');
ok(!s.includes('vbxeBlend2('), 'HY HR blend funkce neni pritomna');
ok(s.includes('ATR D1 FAST LOAD BUILD2HW') || s.includes('BUILD2HW_VBXE_ATR_D1_FASTLOAD_SAFE'), 'rychly ATR/D1 zaklad zustal');
console.log('BUILD2ID kontrola hotova.');
