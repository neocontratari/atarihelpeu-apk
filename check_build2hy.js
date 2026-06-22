const fs = require('fs');
const path = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(path,'utf8');
const req = [
  'BUILD2HY_VBXE_COLOR_SHIFT_HR_BLEND_ON_HW',
  'VBXE OV_COLOR_SHIFT BUILD2HY',
  'function vbxeEffectiveColorIndex',
  'function vbxeBlend2',
  'st.colorShift=(o0>>4)&0x0F;',
  'fb[dst3+bx]=(!ht && !lt) ? vbxeBlend2(hc,lc)',
  'ATR D1 FAST LOAD BUILD2HW',
  'NET HRY BUILD2HW',
  'BUILD2HP',
  'BUILD2HR'
];
let ok = true;
for (const r of req) {
  if (!s.includes(r)) { console.error('MISSING:', r); ok = false; }
}
const js = s.slice(s.indexOf('<script>')+8, s.lastIndexOf('</script>'));
new Function(js);
if (!ok) process.exit(1);
console.log('BUILD2HY check OK: OV_COLOR_SHIFT + HR downsample blend + HX/HW/UI markers present.');
