const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const must=[
 'BUILD2IG_VBXE_PREXDL_UNDERLAY_AUDIT_SAFE',
 'VBXE PRE-XDL UNDERLAY BUILD2IG',
 'VBXE VRAM PROBE SKIP BUILD2IG',
 'VBXE TEXT XDL RENDER BUILD2IG',
 'VBXE HR/SR XDL RENDER BUILD2IG',
 'var h4=function(v)',
 'const CHAIN_LIMIT=1024',
 'BUILD2IE',
 'BUILD2HW',
 'BUILD2HL'
];
let ok=true;
for(const m of must){ if(!s.includes(m)){ console.error('CHYBI:',m); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2IG kontrola OK');
