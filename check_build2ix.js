const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
function must(x){ if(!s.includes(x)){ console.error('CHYBI: '+x); process.exit(1); } }
function mustNot(x){ if(s.includes(x)){ console.error('NESMI TAM BYT: '+x); process.exit(2); } }
must('BUILD2IX_VBXE_BCB19_REVERT_BCB21_SAFE');
must('BCB21 ONLY rollback');
must('function vbxeBlitReadBCB(addr)');
must('adr=(adr+21)&0x7FFFF');
mustNot('function vbxeDecodeBCB19');
mustNot('VBXE BCB DECODE BUILD2IW');
mustNot('auto BCB19');
console.log('BUILD2IX kontrola OK: BCB19 auto odstraneno, BCB21-only rollback aktivni.');
