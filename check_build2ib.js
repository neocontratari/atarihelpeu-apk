const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p,'utf8');
const need = [
  'BUILD2IB_VBXE_TEXTMODE_HSCROLL_SHARP_SAFE',
  'VBXE TEXT XDL RENDER BUILD2IB',
  'var srcBit=(sx<<1)+hs',
  'var bitPos=srcBit+sample',
  'ATR D1 FAST LOAD BUILD2HW'
];
let ok = true;
for (const n of need) {
  if (!s.includes(n)) { console.error('CHYBI MARKER:', n); ok = false; }
}
if (!s.includes('BUILD2IA_VBXE_SHARP_PRESENTATION_DAC_SAFE')) {
  console.log('OK: IA offscreen scaler komentar/kod neni aktivni v tomto HZ-based buildu.');
}
if (!ok) process.exit(1);
console.log('BUILD2IB kontrola OK');
