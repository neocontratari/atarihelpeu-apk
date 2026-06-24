const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
  'BUILD2IQ_VBXE_VSYNC_RESET_NONVBXE_SAFE',
  'vbxeResetRuntimeState',
  'VBXE XDL VSYNC APPLY BUILD2IQ',
  'VBXE NON-VBXE RESET BUILD2IQ',
  'VBXE AUDIT MATRIX BUILD2IQ',
  'xdlPendingEnabled',
  'ioHits'
];
let ok=true;
for(const n of need){ if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2IQ kontrola OK: VSYNC XDL timing + non-VBXE reset + audit markers jsou v index.html.');
