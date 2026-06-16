// BUILD2CJ syntax/audit check
const fs=require('fs');
const html=fs.readFileSync('app/src/main/assets/emu/index.html','utf8');
const js=(html.match(/<script>([\s\S]*)<\/script>/)||[])[1]||'';
new Function(js);
for (const m of [
  'BUILD2CJ_BY_BZ_GOLDEN_TRUE_BITSTREAM_CLOAD',
  'CLOAD TRUE BITSTREAM BUILD2CJ',
  'queueCassetteRawBitstream',
  'KODY JSOU STEJNE'
]) {
  if (!html.includes(m)) throw new Error('Missing marker: '+m);
}
console.log('BUILD2CJ OK');
