const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
if(!fs.existsSync(p)){ console.error('CHYBI '+p); process.exit(1); }
const s=fs.readFileSync(p,'utf8');
for(const m of [
  'BUILD2IH_VBXE_XDL_NONBLANK_LASTGOOD_SAFE',
  'VBXE XDL QUICK NONBLANK LAST_GOOD BUILD2IH',
  'VBXE XDL BLANK_HOLD BUILD2IH',
  'AtariHelp.eu EMU-10 BUILD2IH_VBXE_XDL_NONBLANK_LASTGOOD_SAFE pripraven'
]){
  if(!s.includes(m)){ console.error('CHYBI MARKER: '+m); process.exit(2); }
}
console.log('BUILD2IH kontrola OK');
