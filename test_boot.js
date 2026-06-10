// test_boot.js - headless proof: boot to READY, SELF TEST, and BASIC math via keyboard.
'use strict';
const fs=require('fs');
const CPU6502=require('./cpu.js');
const Atari130XE=require('./machine.js');
const os=new Uint8Array(fs.readFileSync('os.bin'));
const bas=new Uint8Array(fs.readFileSync('basic.bin'));

function screenText(M){
  // projdi skutecny display list jako ANTIC a vytahni textove radky (mody 2-7)
  let dl=M.antic.dlist, lms=0, out=[], guard=0;
  while(guard++<256){
    const ins=M.anticRead(dl); dl=(dl&0xFC00)|((dl+1)&0x3FF);
    const m=ins&0x0F;
    if(m===0) continue;
    if(m===1){ if(ins&0x40) break; const lo=M.anticRead(dl),hi=M.anticRead(dl+1); dl=(hi<<8)|lo; continue; }
    if(ins&0x40){ const lo=M.anticRead(dl); dl=(dl&0xFC00)|((dl+1)&0x3FF);
                  const hi=M.anticRead(dl); dl=(dl&0xFC00)|((dl+1)&0x3FF); lms=(hi<<8)|lo; }
    const nb=(m===6||m===7)?20:40;
    if(m>=2&&m<=7){ let line='';
      for(let c=0;c<nb;c++){ let sc=M.anticRead((lms+c)&0xFFFF)&0x7F;
        let a; if(sc<64)a=sc+32; else if(sc<96)a=sc-64; else a=sc;
        line+=(a>=32&&a<127)?String.fromCharCode(a):'.'; }
      out.push(line); }
    if(m>=2) lms=(lms+nb)&0xFFFF;
  }
  return out.join('\n');
}
function runFrames(M,n){ for(let i=0;i<n;i++){ M.runFrame(); if(M.cpu.jam){ console.log('JAM op $'+M.cpu.jamOp.toString(16)+' at $'+M.cpu.jamPc.toString(16)+' frame '+M.frame); return false; } } return true; }

// XL/XE POKEY scan codes
const SCAN={'l':0,'j':1,';':2,'k':5,'+':6,'*':7,'o':8,'p':10,'u':11,'\n':12,'i':13,'-':14,'=':15,
'v':16,'c':18,'b':21,'x':22,'z':23,'4':24,'3':26,'6':27,'5':29,'2':30,'1':31,
',':32,' ':33,'.':34,'n':35,'m':37,'/':38,'r':40,'e':42,'y':43,'t':45,'w':46,'q':47,
'9':48,'0':50,'7':51,'8':53,'f':56,'h':57,'d':58,'g':61,'s':62,'a':63,'?':38|0x40};
function typeLine(M,str){
  for(const chr of str){
    const sc=SCAN[chr]; if(sc===undefined){ console.log('no scan for',chr); continue; }
    M.keyDown(sc); runFrames(M,3); M.keyUp(); runFrames(M,3);
  }
}

let pass=true;
// ---- TEST 1: BASIC boot -> READY ----
{
  const M=Atari130XE(os,bas,CPU6502);
  M.coldStart();
  let ready=false, fr=0;
  for(; fr<1500; fr+=10){ if(!runFrames(M,10)){pass=false;break;}
    if(screenText(M).includes('READY')){ ready=true; break; } }
  console.log('TEST1 BASIC READY:', ready?('OK ve frame ~'+fr+' ('+(fr/50).toFixed(1)+' s) PORTB=$'+M.portB().toString(16).toUpperCase()):'FAIL');
  if(!ready){ console.log(screenText(M)); console.log('PC=$'+M.cpu.pc.toString(16)+' PORTB=$'+M.portB().toString(16)+' DMACTL='+M.antic.dmactl+' DL=$'+M.antic.dlist.toString(16)+' NMIEN='+M.antic.nmien); pass=false; }
  else {
    // ---- TEST 2: type ? 1+1 -> 2 ----
    typeLine(M,'? 1+1\n'); runFrames(M,60);
    const txt=screenText(M);
    const ok=/\?\s?1\+1/.test(txt)&&txt.split('\n').some((l,i,A)=>/\?\s?1\+1/.test(l)&&A[i+1]&&A[i+1].trim()==='2');
    console.log('TEST2 BASIC ? 1+1 = 2:', ok?'OK':'FAIL');
    if(!ok){ console.log(txt); pass=false; }
  }
}
// ---- TEST 3: SELF TEST (OPTION held) ----
{
  const M=Atari130XE(os,bas,CPU6502);
  M.coldStart({option:true});
  let st=false, fr=0;
  for(; fr<2000; fr+=10){ if(!runFrames(M,10)){pass=false;break;}
    const t=screenText(M);
    if(/SELF\s?TEST|MEMORY|AUDIO.?VISUAL|KEYBOARD|ALL\s?TESTS/i.test(t)){ st=true; break; } }
  console.log('TEST3 SELF TEST:', st?('OK ve frame ~'+fr+' PORTB=$'+M.portB().toString(16).toUpperCase()):'FAIL');
  if(!st){ console.log(screenText(M)); console.log('PC=$'+M.cpu.pc.toString(16)+' PORTB=$'+M.portB().toString(16)+' DL=$'+M.antic.dlist.toString(16)); pass=false; }
}
console.log(pass?'ALL TESTS PASS':'TESTS FAILED');
process.exit(pass?0:1);
