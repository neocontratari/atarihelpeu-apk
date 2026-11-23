// ES5 kontrola (acorn, ecmaVersion 5): inline skripty stranek + slovniky. Pouziti: node kontrola_es5.js <assets> soubor...
const acorn = require('/opt/npm-tools/node_modules/acorn');
const fs = require('fs'), path = require('path');
const assets = process.argv[2];
let chyb = 0, ok = 0;
for (const rel of process.argv.slice(3)) {
  const txt = fs.readFileSync(path.join(assets, rel), 'utf8');
  const skripty = [];
  if (rel.endsWith('.html')) {
    const re = /<script(\s[^>]*)?>([\s\S]*?)<\/script>/gi; let m, i = 0;
    while ((m = re.exec(txt))) { i++; if (m[1] && /\bsrc=/.test(m[1])) continue; skripty.push([rel + ' skript ' + i, m[2]]); }
  } else skripty.push([rel, txt]);
  for (const [jm, kod] of skripty) {
    try { acorn.parse(kod, {ecmaVersion: 5}); ok++; console.log('OK     ES5 ' + jm); }
    catch (e) { chyb++; console.log('CHYBA  ES5 ' + jm + ': ' + e.message); }
  }
}
console.log('VYSLEDEK: ' + ok + ' OK, ' + chyb + ' CHYBA');
process.exit(chyb ? 1 : 0);
