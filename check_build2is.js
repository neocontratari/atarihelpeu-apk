const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
 'BUILD2IS_G2F_GTIA_COLOR_SEGMENTS_SAFE',
 'gtiaColorTrack(g,v)',
 'GTIA COLOR SEG BUILD2IS',
 'function gtiaRegAt(g,x)',
 'function pfColorAt(cls,x)'
];
let ok=true;
for(const n of need){ if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; } }
if(ok) console.log('BUILD2IS kontrola OK');
process.exit(ok?0:1);
