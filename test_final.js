// Smoke test SKUTECNEHO artefaktu: script z emu10_130xe.html bootuje do READY i SELF TESTu.
const fs=require('fs');
const html=fs.readFileSync('app/src/main/assets/emu10_130xe.html','utf8');
const src=html.match(/<script>([\s\S]*?)<\/script>/)[1];
const stub={addEventListener(){},removeEventListener(){}};
global.window={addEventListener(){},AudioContext:undefined};
global.document={addEventListener(){},getElementById:()=>stub,createElement:()=>stub};
global.requestAnimationFrame=()=>{};
global.atob=(s)=>Buffer.from(s,'base64').toString('binary');
eval(src);
function screenHas(M,needle){
  let dl=M.antic.dlist,lms=0,txt='',g=0;
  while(g++<256){ const ins=M.anticRead(dl); dl=(dl&0xFC00)|((dl+1)&0x3FF); const m=ins&15;
    if(m===0)continue;
    if(m===1){ if(ins&0x40)break; const lo=M.anticRead(dl),hi=M.anticRead(dl+1); dl=(hi<<8)|lo; continue;}
    if(ins&0x40){const lo=M.anticRead(dl);dl=(dl&0xFC00)|((dl+1)&0x3FF);const hi=M.anticRead(dl);dl=(dl&0xFC00)|((dl+1)&0x3FF);lms=(hi<<8)|lo;}
    const nb=(m===6||m===7)?20:40;
    if(m>=2&&m<=7){for(let c=0;c<nb;c++){let sc=M.anticRead((lms+c)&0xFFFF)&0x7F;let a;if(sc<64)a=sc+32;else if(sc<96)a=sc-64;else a=sc;txt+=(a>=32&&a<127)?String.fromCharCode(a):'.';}txt+='\n';}
    if(m>=2)lms=(lms+nb)&0xFFFF;}
  return txt.includes(needle);
}
let M=window.EMU10.createMachine(); M.coldStart();
let ok1=false; for(let f=0;f<400;f++){M.runFrame(); if(f%10===9&&screenHas(M,'READY')){ok1=true;break;}}
console.log('FINAL HTML -> BASIC READY:',ok1?'OK':'FAIL');
M=window.EMU10.createMachine(); M.coldStart({option:true});
let ok2=false; for(let f=0;f<400;f++){M.runFrame(); if(f%10===9&&(screenHas(M,'SELF TEST')||screenHas(M,'MEMORY')||screenHas(M,'KEYBOARD'))){ok2=true;break;}}
console.log('FINAL HTML -> SELF TEST:',ok2?'OK':'FAIL');
process.exit(ok1&&ok2?0:1);
