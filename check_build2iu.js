const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
  'BUILD2IU_CPU_BRANCH_NMI_TIMING_SAFE',
  'function br(cond,off)',
  'if(service()) return; // BUILD2IU',
  'case 0xD0: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!c.zf,t); break; // BNE rel BUILD2IU exact branch cycles',
  'case 0xF0: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!!c.zf,t); break; // BEQ rel BUILD2IU exact branch cycles'
];
let ok=true;
for(const n of need){ if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; } }
if(!ok) process.exit(1);
console.log('BUILD2IU kontrola OK: CPU branch/NMI timing patch nalezen.');
