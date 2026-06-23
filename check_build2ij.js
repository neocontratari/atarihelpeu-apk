const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
for (const needle of ['BUILD2IJ_VBXE_XDL_STABLE_SPARSE_UNDERLAY_SAFE','VBXE XDL STABLE LATCH BUILD2IJ','VBXE PRE-XDL SPARSE UNDERLAY BUILD2IJ','VBXE XDL TOP STEP0 PREFACE CLIP BUILD2IJ']) { if(!s.includes(needle)){ console.error('MISSING '+needle); process.exit(2); } }
console.log('OK BUILD2IJ markers present');
