// B356: spolecne/nap_obrazovka.js - klepnuti hned po otoceni obrazovky se zahodi (Rene, PS1 19. 11. 21:33:21-22:
// otoceni NA VYSKU a SAVE GAME 2x pod prstem). Skutecny skript ve vm s nahradou window / document.
// Spoustet: node test_b356_otoceni.js ../../app/src/main/assets/spolecne/nap_obrazovka.js
'use strict';
const fs = require('fs');
const vm = require('vm');

let ok = 0, chyb = 0;
function over(p, co) { if (p) { ok++; console.log('OK    ' + co); } else { chyb++; console.log('CHYBA ' + co); } }

const zdroj = fs.readFileSync(process.argv[2], 'utf8');
let cas = 100000;
const posluchaci = {};
const log = [];
function prvek(jmeno, atributy) {
  return {
    jmeno, attrs: atributy || {}, parentNode: null,
    getAttribute(n) { return Object.prototype.hasOwnProperty.call(this.attrs, n) ? this.attrs[n] : null; },
    setAttribute(n, v) { this.attrs[n] = v; },
    addEventListener() {}, querySelector() { return null; }, className: '', style: {}, innerHTML: ''
  };
}
const D = { readyState: 'complete', documentElement: { clientWidth: 0, clientHeight: 0 },
  querySelectorAll() { return []; }, addEventListener() {} };
const W = {
  innerWidth: 1344, innerHeight: 2768,            // na vysku (S8)
  addEventListener(typ, fn, zachyt) { (posluchaci[typ] = posluchaci[typ] || []).push({ fn, zachyt }); },
  jsLog(s) { log.push(s); },
  AHOBRAZ: { prepni() {} }
};
const ctx = { window: W, document: D, Date: { now: () => cas }, setTimeout: (f) => f(), console };
vm.createContext(ctx);
vm.runInContext(zdroj, ctx);

function klik(cil) {
  const e = { target: cil, zahozeno: false, preventDefault() { this.zahozeno = true; }, stopPropagation() {}, stopImmediatePropagation() {} };
  for (const p of posluchaci.click || []) p.fn(e);
  return e.zahozeno;
}
function otoc(naSirku) {
  W.innerWidth = naSirku ? 2768 : 1344; W.innerHeight = naSirku ? 1344 : 2768;
  for (const p of posluchaci.resize || []) p.fn({});
}

const save = prvek('btnSave');
save.parentNode = D;
const otocTl = prvek('btnPs1Otoc', { 'data-nap-otoc': '' });
otocTl.parentNode = D;
const ikona = prvek('svg'); ikona.parentNode = otocTl;

over((posluchaci.click || []).some(p => p.zachyt === true), 'posluchac click na okne ve fazi zachytu (drive nez tlacitka stranky)');
over(!klik(save), 'bez otoceni: SAVE GAME projde');

// 21:29:25 otoceni na sirku (tlacitkem), 21:33:21.624 otoceni na vysku tlacitkem, 21:33:22.100 telefon otocen
cas = 200000; otoc(true);
cas = 300000; W.NapObrazovka.prepni(null);
cas = 300476; otoc(false);
cas = 301055;
over(klik(save), '21:33:22.679 SAVE GAME 1055 ms po stisku otoceni (579 ms po otoceni telefonu) -> zahozen');
cas = 301107;
over(klik(save), '21:33:22.731 druhy SAVE GAME -> zahozen');
over(log.length === 2 && /B356 klepnuti \d+ ms po otoceni obrazovky zahozeno/.test(log[0]), 'do logu stranky: ' + log[0]);
cas = 300476 + 1300;
over(!klik(save), 'za 1,3 s po otoceni SAVE GAME zase funguje');

// otoceni bez tlacitka (telefon sam / Java po prichodu na stranku) - jen resize
cas = 400000; otoc(true);
cas = 400500;
over(klik(save), 'resize se zmenou na sirku/vysku = otoceni -> 0,5 s po nem klik zahozen');
cas = 500000; W.innerWidth = 2700; for (const p of posluchaci.resize || []) p.fn({});
cas = 500100;
over(!klik(save), 'resize BEZ zmeny orientace (klavesnice, lista) neni otoceni -> klik projde');

// tlacitko otoceni samo se nezahazuje (jeho dvojity klik resi prepni)
cas = 600000; otoc(false);
cas = 600200;
over(!klik(ikona), 'klik na tlacitko otoceni (i na jeho ikonu) se nezahazuje');
over(W.NapObrazovka.klidPoOtoceni() === true, 'NapObrazovka.klidPoOtoceni() = ano hned po otoceni');

console.log('');
console.log('VYSLEDEK: ' + ok + ' OK, ' + chyb + ' CHYB');
process.exit(chyb ? 1 : 0);
