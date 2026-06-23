const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const must=[
 'BUILD2IE_VBXE_TRUE_640_TEXT_CANVAS_SAFE',
 'vbxeRenderTextRecordHi',
 '640x240 canvasu',
 'VBXE TEXT XDL RENDER BUILD2IE',
 'BUILD2HW',
 'BUILD2HL'
];
let ok=true;
for(const m of must){ if(!s.includes(m)){ console.error('CHYBI:',m); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2IE kontrola OK');
