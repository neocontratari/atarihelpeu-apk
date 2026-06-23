const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
if(!fs.existsSync(p)){ console.error('CHYBI '+p); process.exit(1); }
const s=fs.readFileSync(p,'utf8');
function need(txt){ if(!s.includes(txt)){ console.error('CHYBI MARKER: '+txt); process.exit(2); } }
function forbid(txt){ if(s.includes(txt)){ console.error('NESMI TAM BYT POZDNI REGRESE: '+txt); process.exit(3); } }
need('BUILD2IL_VBXE_IF_APK_RENDER_BASELINE_SAFE');
need('const CHAIN_LIMIT=1024');
need('VBXE TEXT XDL RENDER BUILD2IF');
forbid('BUILD2IH_VBXE_XDL_NONBLANK_LASTGOOD_SAFE');
forbid('VBXE XDL BLANK_HOLD BUILD2IH');
forbid('vbxeQuickPromoteLastGoodXdl');
forbid('VBXE PRE-XDL UNDERLAY BUILD2IG');
console.log('OK BUILD2IL: IF/APK render baseline, bez 2IH blank-hold/quick-lastgood regrese.');
