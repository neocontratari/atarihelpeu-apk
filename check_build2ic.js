// Kontrolni script pro BUILD2IC_VBXE_TEXT_NATIVE_DAC_ADDR_SAFE
// Spusteni v koreni projektu: node check_build2ic.js
const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
function ok(cond,msg){ if(!cond){ console.error('FAIL:',msg); process.exit(1); } console.log('OK:',msg); }
ok(s.includes('BUILD2IC_VBXE_TEXT_NATIVE_DAC_ADDR_SAFE'), 'BUILD2IC marker pritomen');
ok(s.includes('function vbxeTextBgColor'), 'text background pravidla pritomna');
ok(s.includes('function vbxeTextFgVisible'), 'text foreground pravidla pritomna');
ok(s.includes('foreground colour 0 is still a real colour'), 'foreground 0 neni filtrovany jako SR/LR transparentni 0');
ok(s.includes('skutecny XDL fetch po recordu posouva'), 'XDL OVADR advance kod pritomen');
ok(s.includes('phase='), 'XDL text phase log pritomen');
ok(!s.includes('VBXE OV_COLOR_SHIFT BUILD2HY'), 'HY OV_COLOR_SHIFT kod/log neni pritomen');
ok(!s.includes('vbxeBlend2('), 'HY HR blend funkce neni pritomna');
ok(s.includes('ATR D1 FAST LOAD BUILD2HW') || s.includes('BUILD2HW_VBXE_ATR_D1_FASTLOAD_SAFE'), 'rychly ATR/D1 zaklad zustal');
console.log('BUILD2IC kontrola hotova.');
