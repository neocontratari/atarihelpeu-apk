const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
 'BUILD2II_VBXE_RECORD_VC_TRANSPARENCY_SAFE',
 'vbxeTransparent8ForRecord',
 'vbxeTransparent4ForRecord',
 'vbxeTextBgColorForRecord',
 'nt=',
 't15=',
 'xc=',
 'BUILD2IH_VBXE_XDL_NONBLANK_LASTGOOD_SAFE'
];
let ok=true;
for(const n of need){ if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2II kontrola OK - record VC transparency markers found.');
