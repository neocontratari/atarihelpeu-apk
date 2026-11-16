// ES5 kontrola vsech <script> bez src v HTML souborech (acorn ecmaVersion 5)
const fs = require('fs');
const acorn = require('/opt/npm-tools/node_modules/acorn');
let chyb = 0;
for (const f of process.argv.slice(2)) {
  const h = fs.readFileSync(f, 'utf8');
  const re = /<script(?![^>]*\bsrc=)[^>]*>([\s\S]*?)<\/script>/gi;
  let m, n = 0;
  while ((m = re.exec(h))) {
    n++;
    try { acorn.parse(m[1], { ecmaVersion: 5 }); }
    catch (e) { chyb++; console.log('CHYBA ' + f + ' skript ' + n + ': ' + e.message); }
  }
  console.log((chyb ? '' : 'OK     ') + f + ': skriptu ' + n + ' (ES5)');
}
for (const f of ['app/src/main/assets/jazyk/en_menu.js']) {
  try { acorn.parse(fs.readFileSync(f, 'utf8'), { ecmaVersion: 5 }); console.log('OK     ' + f + ' (ES5)'); }
  catch (e) { chyb++; console.log('CHYBA ' + f + ': ' + e.message); }
}
process.exit(chyb ? 1 : 0);
