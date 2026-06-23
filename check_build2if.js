const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const must=[
 'BUILD2IF_VBXE_HR_CANVAS_CHAIN_SAFE',
 'const CHAIN_LIMIT=1024',
 'vbxeRenderGraphRecordHi',
 'vbxeVisibleWidthHi',
 'VBXE HR/SR XDL RENDER BUILD2IF',
 'VBXE TEXT XDL RENDER BUILD2IF',
 'BUILD2IE',
 'BUILD2HW',
 'BUILD2HL'
];
let ok=true;
for(const m of must){ if(!s.includes(m)){ console.error('CHYBI:',m); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2IF kontrola OK');
