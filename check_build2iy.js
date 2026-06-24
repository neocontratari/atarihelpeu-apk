const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
if (!fs.existsSync(p)) {
  console.error('CHYBA: Nenalezen ' + p + '. Spust check z korene projektu.');
  process.exit(1);
}
const s = fs.readFileSync(p, 'utf8');
const need = [
  'BUILD2IY_VBXE_SURFACE_FALLBACK_MADPASCAL_SAFE',
  'VBXE SURFACE CANDIDATE BUILD2IY',
  'VBXE SURFACE FALLBACK BUILD2IY',
  'BCB21 ONLY rollback po rozbitem BUILD2IW'
];
let ok = true;
for (const n of need) {
  if (!s.includes(n)) {
    console.error('CHYBI MARKER: ' + n);
    ok = false;
  }
}
if (s.includes('VBXE BCB DECODE BUILD2IW: auto BCB19')) {
  console.error('CHYBA: zustal rozbity BUILD2IW BCB19 auto decode marker.');
  ok = false;
}
if (!ok) process.exit(2);
console.log('OK: BUILD2IY overlay je pritomny, BCB19 auto decode neni aktivni.');
