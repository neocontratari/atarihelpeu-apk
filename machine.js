// machine130xe.js - AtariHelp.eu EMU-10 clean core. DOM-free, testable in Node.
'use strict';
function Atari130XE(osRom, basicRom, CPUctor){
  const M={};
  // ---------------- Memory ----------------
  const ram = new Uint8Array(65536);      // main 64K
  const ext = new Uint8Array(65536);      // 130XE extended 4x16K
  const os  = osRom;                      // 16K $C000-$FFFF (incl. self-test image at $1000-$17FF)
  const bas = basicRom;                   // 8K  $A000-$BFFF
  // ---------------- PIA -------------------
  const pia={ orA:0xFF, ddrA:0, orB:0xFF, ddrB:0, ctlA:0, ctlB:0 };
  function portB(){ return (pia.orB | (~pia.ddrB)) & 0xFF; }   // inputs pull high
  function portA(){ return (pia.orA | (~pia.ddrA)) & 0xFF; }   // joysticks idle
  // ---------------- GTIA ------------------
  const gtia={ regs:new Uint8Array(32), consol:7, speaker:0, prior:0,
               hposp:[0,0,0,0], hposm:[0,0,0,0], sizep:[0,0,0,0], sizem:0,
               grafp:[0,0,0,0], grafm:0, colpm:[0,0,0,0], colpf:[0,0,0,0], colbk:0,
               gractl:0, pal:0x01 /* PAL */ };
  // ---------------- POKEY -----------------
  const pokey={ audf:[0,0,0,0], audc:[0,0,0,0], audctl:0, skctl:0,
                irqen:0, irqpend:0, kbcode:0xFF, keyDown:false, shiftDown:false,
                outBusy:0, shiftBusy:0, random:0x12345, dirty:true };
  // Serial IRQ model podle chovani praveho POKEY (overeno disasm VSEROR $EAAD vasi ROM):
  // bit4 = JEDNORAZOVA udalost "byte presel z vystupniho do posuvneho registru",
  // bit3 = JEDNORAZOVA udalost "posuvny registr dobehl". Nejsou trvale aktivni v klidu.
  function pokeyStatus(){ return pokey.irqpend; }
  function pokeySync(){ cpu.irqLine = (pokey.irqen & pokey.irqpend)!==0 ? 1:0; }
  function pokeyRaise(bit){ pokey.irqpend|=bit; pokeySync(); }
  // ---------------- ANTIC -----------------
  const antic={ dmactl:0, chactl:2, dlist:0, hscrol:0, vscrol:0, pmbase:0, chbase:0,
                nmien:0, nmist:0x1F, vcountLine:0,
                dlptr:0, rowsLeft:0, mode:0, lms:0, dliPend:false, done:true,
                blankRows:0, width:0 };
  // ---------------- Machine state ---------
  M.frame=0; M.line=0; M.log=[];
  const CYC_LINE=114, LINES=312, VBI_LINE=248;
  M.fb=new Uint32Array(384*240);          // RGBA frame buffer (visible lines 8..247)
  // ---------------- Bus -------------------
  function hwRead(a){
    const r=a&0xFF, page=a&0xFF00;
    if(page===0xD000){ // GTIA
      const g=a&0x1F;
      if(g<=0x07) return 0x00;                 // collisions M0PF..P3PF (BUILD1: none)
      if(g>=0x08 && g<=0x0B) return 0x00;      // P/M playfield collisions
      if(g>=0x10 && g<=0x13) return g===0x13?0x00:0x01; // TRIG0-2 idle=1, TRIG3=0 (no cart)
      if(g===0x14) return gtia.pal;            // PAL/NTSC
      if(g===0x1F) return gtia.consol&0x07;    // console keys
      return 0x0F;
    }
    if(page===0xD200){ // POKEY
      const p=a&0x0F;
      if(p===0x09) return pokey.kbcode;        // KBCODE
      if(p===0x0A){ pokey.random=((pokey.random<<1)|(((pokey.random>>16)^(pokey.random>>11))&1))&0x1FFFF;
                    return (pokey.random>>1)&0xFF; } // RANDOM
      if(p===0x0D) return 0xFF;                // SERIN (no device)
      if(p===0x0E) return (~pokeyStatus())&0xFF; // IRQST active low
      if(p===0x0F){ // SKSTAT: bit3 shift(0=down), bit2 key down(0=down), serial idle bits high
        let v=0xFF; if(pokey.shiftDown)v&=~0x08; if(pokey.keyDown)v&=~0x04; return v; }
      return 0xFF;
    }
    if(page===0xD300){ // PIA
      const p=a&0x03;
      if(p===0){ return (pia.ctlA&0x04)? portA() : pia.ddrA; }
      if(p===1){ return (pia.ctlB&0x04)? portB() : pia.ddrB; }
      if(p===2) return pia.ctlA; return pia.ctlB;
    }
    if(page===0xD400){ // ANTIC
      const p=a&0x0F;
      if(p===0x0B) return (M.line>>1)&0xFF;    // VCOUNT
      if(p===0x0F) return antic.nmist;         // NMIST
      if(p===0x0C||p===0x0D) return 0x00;      // PENH/PENV
      return 0xFF;
    }
    return 0xFF; // $D100,$D500-$D7FF
  }
  function hwWrite(a,v){
    const page=a&0xFF00;
    if(page===0xD000){
      const g=a&0x1F;
      if(g<=0x03) gtia.hposp[g]=v;
      else if(g<=0x07) gtia.hposm[g-4]=v;
      else if(g<=0x0B) gtia.sizep[g-8]=v&3;
      else if(g===0x0C) gtia.sizem=v;
      else if(g<=0x10) gtia.grafp[g-0x0D]=v;
      else if(g===0x11) gtia.grafm=v;
      else if(g<=0x15) gtia.colpm[g-0x12]=v;
      else if(g<=0x19) gtia.colpf[g-0x16]=v;
      else if(g===0x1A) gtia.colbk=v;
      else if(g===0x1B) gtia.prior=v;
      else if(g===0x1D) gtia.gractl=v;
      else if(g===0x1E){ /* HITCLR */ }
      else if(g===0x1F){ const sp=(v>>3)&1; if(sp!==gtia.speaker){gtia.speaker=sp; pokey.dirty=true; M.onSpeaker&&M.onSpeaker(sp);} }
      return;
    }
    if(page===0xD200){
      const p=a&0x0F;
      if(p<=0x07){ if(p&1) pokey.audc[p>>1]=v; else pokey.audf[p>>1]=v; pokey.dirty=true; return; }
      if(p===0x08){ pokey.audctl=v; pokey.dirty=true; return; }
      if(p===0x09){ return; }                  // STIMER
      if(p===0x0A){ return; }                  // SKRES
      if(p===0x0D){ // SEROUT: byte opusti vystupni registr za ~10 radek, posuv dobehne za ~30
        pokey.outBusy=10; pokey.shiftBusy=30; pokey.irqpend&=~0x18; pokeySync(); return; }
      if(p===0x0E){ pokey.irqen=v; pokey.irqpend&=v; pokeySync(); return; }
      if(p===0x0F){ pokey.skctl=v; return; }
      return;
    }
    if(page===0xD300){
      const p=a&0x03;
      if(p===0){ if(pia.ctlA&0x04) pia.orA=v; else pia.ddrA=v; return; }
      if(p===1){ if(pia.ctlB&0x04) pia.orB=v; else pia.ddrB=v; return; }
      if(p===2){ pia.ctlA=v; return; } pia.ctlB=v; return;
    }
    if(page===0xD400){
      const p=a&0x0F;
      if(p===0x00) antic.dmactl=v;
      else if(p===0x01) antic.chactl=v;
      else if(p===0x02) antic.dlist=(antic.dlist&0xFF00)|v;
      else if(p===0x03) antic.dlist=(antic.dlist&0x00FF)|(v<<8);
      else if(p===0x04) antic.hscrol=v&15;
      else if(p===0x05) antic.vscrol=v&15;
      else if(p===0x07) antic.pmbase=v;
      else if(p===0x09) antic.chbase=v;
      else if(p===0x0A){ M.wsync=true; }       // WSYNC: stall CPU to line end
      else if(p===0x0E){ antic.nmien=v; }
      else if(p===0x0F){ antic.nmist=0x1F; }   // NMIRES
      return;
    }
  }
  function cpuRead(a){
    a&=0xFFFF;
    if(a>=0xD000 && a<0xD800) return hwRead(a);
    const pb=portB();
    if(a>=0xC000){ if(pb&1) return os[a-0xC000]; return ram[a]; }
    if(a>=0xA000){ if(!(pb&2)) return bas[a-0xA000]; return ram[a]; }
    if(a>=0x5000 && a<0x5800 && !(pb&0x80) && (pb&1)) return os[a-0x5000+0x1000]; // self test
    if(a>=0x4000 && a<0x8000 && !(pb&0x10))           // 130XE CPU ext bank
      return ext[(((pb>>2)&3)<<14)|(a-0x4000)];
    return ram[a];
  }
  function cpuWrite(a,v){
    a&=0xFFFF; v&=0xFF;
    if(a>=0xD000 && a<0xD800){ hwWrite(a,v); return; }
    const pb=portB();
    if(a>=0x4000 && a<0x8000 && !(pb&0x10)){ ext[(((pb>>2)&3)<<14)|(a-0x4000)]=v; return; }
    ram[a]=v; // RAM under ROM always writable
  }
  function anticRead(a){
    a&=0xFFFF;
    const pb=portB();
    if(a>=0xC000){ if(pb&1) return os[a-0xC000]; return ram[a]; }
    if(a>=0xA000 && !(pb&2)) return bas[a-0xA000];
    if(a>=0x5000 && a<0x5800 && !(pb&0x80) && (pb&1)) return os[a-0x5000+0x1000];
    if(a>=0x4000 && a<0x8000 && !(pb&0x20))           // ANTIC ext bank
      return ext[(((pb>>2)&3)<<14)|(a-0x4000)];
    return ram[a];
  }
  // ---------------- CPU -------------------
  const cpu=CPUctor(cpuRead,cpuWrite);
  M.cpu=cpu; M.ram=ram; M.portB=portB; M.pokey=pokey; M.gtia=gtia; M.antic=antic; M.anticRead=anticRead;
  // ---------------- Palette (PAL) ---------
  const palette=new Uint32Array(256);
  (function(){
    for(let i=0;i<256;i++){
      const hue=i>>4, lum=i&15;
      let y=lum/15, r,g,b;
      if(hue===0){ r=g=b=y; }
      else{
        const ang=((hue-1)*24-58)*Math.PI/180, sat=0.32;
        const u=sat*Math.cos(ang), v2=sat*Math.sin(ang);
        y=0.10+0.85*y;
        r=y+1.14*v2; g=y-0.395*u-0.581*v2; b=y+2.032*u;
      }
      const R=Math.max(0,Math.min(255,Math.round(r*255)));
      const G=Math.max(0,Math.min(255,Math.round(g*255)));
      const B=Math.max(0,Math.min(255,Math.round(b*255)));
      palette[i]=(0xFF<<24)|(B<<16)|(G<<8)|R;
    }
  })();
  M.palette=palette;
  // ---------------- ANTIC renderer --------
  const MODE={ // bytes/line (normal), scanlines, type
    0x02:{b:40,sl:8, t:'ch2'}, 0x03:{b:40,sl:10,t:'ch2'},
    0x04:{b:40,sl:8, t:'ch4'}, 0x05:{b:40,sl:16,t:'ch4'},
    0x06:{b:20,sl:8, t:'ch6'}, 0x07:{b:20,sl:16,t:'ch6'},
    0x08:{b:10,sl:8, t:'m2bpp',px:32}, 0x09:{b:10,sl:4,t:'m1bpp',px:32},
    0x0A:{b:20,sl:4, t:'m2bpp',px:16}, 0x0B:{b:20,sl:2,t:'m1bpp',px:16},
    0x0C:{b:20,sl:1, t:'m1bpp',px:16}, 0x0D:{b:40,sl:2,t:'m2bpp',px:8},
    0x0E:{b:40,sl:1, t:'m2bpp',px:8},  0x0F:{b:40,sl:1,t:'hires'} };
  function pfBytes(base){ const w=antic.dmactl&3; if(w===1)return base*0.8|0; if(w===3)return base*1.2|0; return base; }
  function startFrame(){
    antic.dlptr=antic.dlist; antic.rowsLeft=0; antic.done=!(antic.dmactl&0x20);
    antic.mode=0; antic.blankRows=0; antic.row=0;
  }
  function dlFetch(){ const v=anticRead(antic.dlptr); antic.dlptr=(antic.dlptr&0xFC00)|((antic.dlptr+1)&0x3FF); return v; }
  function nextInstr(){
    for(;;){
      if(antic.done) return;
      const ins=dlFetch();
      antic.instr=ins;
      const m=ins&0x0F;
      if(m===0){ antic.mode=0; antic.rowsLeft=((ins>>4)&7)+1; antic.dli=!!(ins&0x80); return; }
      if(m===1){
        const lo=dlFetch(), hi=dlFetch();
        if(ins&0x40){ antic.done=true; antic.dlist=(hi<<8)|lo; return; } // JVB
        antic.dlptr=(hi<<8)|lo; continue;                                  // JMP
      }
      if(ins&0x40){ const lo=dlFetch(), hi=dlFetch(); antic.lms=(hi<<8)|lo; }
      antic.mode=m; const md=MODE[m];
      antic.rowsLeft=md.sl; antic.row=0; antic.dli=!!(ins&0x80);
      return;
    }
  }
  function renderScanline(){
    const y=M.line-8; if(y<0||y>=240) return;
    const fb=M.fb, off=y*384, bk=palette[gtia.colbk];
    for(let x=0;x<384;x++) fb[off+x]=bk;
    if(antic.done||!(antic.dmactl&0x20)||antic.mode===0) { renderPMG(y); return; }
    const md=MODE[antic.mode]; if(!md){renderPMG(y);return;}
    const nb=pfBytes(md.b);
    const left=192-((nb*(md.t==='ch6'?16:(md.px? md.px*8/ (md.b===10?1: (md.b===20?2:4)) :8)))>>1); // computed below per type instead
    const row=antic.row;
    const ch=(antic.chbase&0xFC)<<8;
    const PF=[palette[gtia.colbk],palette[gtia.colpf[0]],palette[gtia.colpf[1]],palette[gtia.colpf[2]],palette[gtia.colpf[3]]];
    let xs;
    if(md.t==='ch2'){
      xs=192-nb*4;
      const fgL=gtia.colpf[1]&0x0F, bgC=gtia.colpf[2];
      const fg=palette[(bgC&0xF0)|fgL], bg=palette[bgC];
      for(let i=0;i<nb;i++){
        const sc=anticRead(antic.lms+i);
        let bits=os[0]===undefined?0:0;
        let line=row; if(antic.mode===3){ if(line>=8)line-=8; }
        bits=anticRead(ch+((sc&0x7F)<<3)+(line&7));
        if(sc&0x80){ if(antic.chactl&1) bits=0; else if(antic.chactl&2) bits^=0xFF; }
        for(let b=0;b<8;b++) fb[off+xs+i*8+b]=(bits&(0x80>>b))?fg:bg;
      }
    } else if(md.t==='ch4'){
      xs=192-nb*4;
      const l=(antic.mode===5)?(row>>1):row;
      for(let i=0;i<nb;i++){
        const sc=anticRead(antic.lms+i);
        const bits=anticRead(ch+((sc&0x7F)<<3)+(l&7));
        for(let p=0;p<4;p++){
          let v=(bits>>(6-p*2))&3;
          let col=v===0?PF[0]:(v===3&&(sc&0x80)?PF[4]:PF[v]);
          fb[off+xs+i*8+p*2]=col; fb[off+xs+i*8+p*2+1]=col;
        }
      }
    } else if(md.t==='ch6'){
      xs=192-nb*8;
      const l=(antic.mode===7)?(row>>1):row;
      for(let i=0;i<nb;i++){
        const sc=anticRead(antic.lms+i);
        const bits=anticRead(ch+((sc&0x3F)<<3)+(l&7));
        const col=PF[1+((sc>>6)&3)];
        for(let b=0;b<8;b++){ const on=bits&(0x80>>b);
          fb[off+xs+i*16+b*2]=on?col:PF[0]; fb[off+xs+i*16+b*2+1]=on?col:PF[0]; }
      }
    } else if(md.t==='m1bpp'){
      const pxw=320/(nb*8); xs=192-160;
      for(let i=0;i<nb;i++){ const bits=anticRead(antic.lms+i);
        for(let b=0;b<8;b++){ const col=(bits&(0x80>>b))?PF[1]:PF[0];
          const x0=xs+((i*8+b)*pxw)|0;
          for(let k=0;k<pxw;k++) fb[off+x0+k]=col; } }
    } else if(md.t==='m2bpp'){
      const pxw=320/(nb*4); xs=192-160;
      for(let i=0;i<nb;i++){ const bits=anticRead(antic.lms+i);
        for(let p=0;p<4;p++){ const v=(bits>>(6-p*2))&3; const col=PF[v];
          const x0=xs+((i*4+p)*pxw)|0;
          for(let k=0;k<pxw;k++) fb[off+x0+k]=col; } }
    } else if(md.t==='hires'){
      xs=192-nb*4;
      const fgL=gtia.colpf[1]&0x0F, bgC=gtia.colpf[2];
      const fg=palette[(bgC&0xF0)|fgL], bg=palette[bgC];
      for(let i=0;i<nb;i++){ const bits=anticRead(antic.lms+i);
        for(let b=0;b<8;b++) fb[off+xs+i*8+b]=(bits&(0x80>>b))?fg:bg; }
    }
    renderPMG(y);
  }
  function renderPMG(y){
    if(!(gtia.gractl&3) && !(gtia.grafp[0]|gtia.grafp[1]|gtia.grafp[2]|gtia.grafp[3]|gtia.grafm)) return;
    const fb=M.fb, off=y*384, line=M.line;
    const dbl=!(antic.dmactl&0x10);
    const pmDma=(antic.dmactl&0x08)&&(gtia.gractl&2), mDma=(antic.dmactl&0x04)&&(gtia.gractl&1);
    for(let p=0;p<4;p++){
      let g;
      if(pmDma){ const base=(antic.pmbase&(dbl?0xFC:0xF8))<<8;
        g=anticRead(base+(dbl?512:1024)+p*(dbl?128:256)+(dbl?(line>>1):line)&0xFFFF? base+(dbl?512:1024)+p*(dbl?128:256)+(dbl?(line>>1):line):0);
        g=anticRead(base+(dbl?512:1024)+p*(dbl?128:256)+(dbl?(line>>1):line));
      } else g=gtia.grafp[p];
      if(!g) continue;
      const col=palette[gtia.colpm[p]];
      const w=[1,2,1,4][gtia.sizep[p]&3];
      const x0=(gtia.hposp[p]-32)*2-32+24; // approx mapping color clocks->fb px
      for(let b=0;b<8;b++){ if(g&(0x80>>b)){
        for(let k=0;k<2*w;k++){ const x=x0+b*2*w+k; if(x>=0&&x<384) fb[off+x]=col; } } }
    }
    // missiles
    let gm;
    if(mDma){ const base=(antic.pmbase&(dbl?0xFC:0xF8))<<8;
      gm=anticRead(base+(dbl?384:768)+(dbl?(line>>1):line)); }
    else gm=gtia.grafm;
    if(gm){ for(let p=0;p<4;p++){ const bits=(gm>>(p*2))&3; if(!bits)continue;
      const col=palette[gtia.colpm[p]];
      const w=[1,2,1,4][(gtia.sizem>>(p*2))&3];
      const x0=(gtia.hposm[p]-32)*2-32+24;
      for(let b=0;b<2;b++){ if(bits&(2>>b)){
        for(let k=0;k<2*w;k++){ const x=x0+b*2*w+k; if(x>=0&&x<384) fb[off+x]=col; } } } } }
  }
  // ---------------- Scanline / frame ------
  M.wsync=false;
  function runScanline(){
    // DL bookkeeping at line start (display area 8..247)
    if(M.line===8) startFrame();
    let dli=false;
    if(M.line>=8 && M.line<248 && !antic.done && (antic.dmactl&0x20)){
      if(antic.rowsLeft===0) nextInstr();
      if(!antic.done && antic.rowsLeft===1 && antic.dli) dli=true;
    }
    if(dli && (antic.nmien&0x80)){ antic.nmist=(antic.nmist|0x80)&~0x40; cpu.nmiPending=true; }
    if(M.line===VBI_LINE){
      antic.nmist=(antic.nmist|0x40)&~0x80;
      if(antic.nmien&0x40) cpu.nmiPending=true;
    }
    // serial out udalosti
    if(pokey.outBusy>0 && --pokey.outBusy===0) pokeyRaise(0x10);   // vystupni registr prazdny
    if(pokey.shiftBusy>0 && --pokey.shiftBusy===0) pokeyRaise(0x08); // prenos dokoncen
    // CPU
    const target=cpu.cycles+CYC_LINE;
    while(cpu.cycles<target){
      if(M.wsync){ cpu.cycles=target; M.wsync=false; break; }
      cpu.step();
      if(cpu.jam){ cpu.cycles=target; break; }
    }
    // render
    if(M.line>=8 && M.line<248){
      renderScanline();
      if(!antic.done && antic.rowsLeft>0 && antic.mode!==0){ antic.rowsLeft--; antic.row++; if(antic.rowsLeft>0){} else { antic.lms=(antic.lms+pfBytes(MODE[antic.mode].b))&0xFFFF; } }
      else if(antic.rowsLeft>0) antic.rowsLeft--;
    }
    M.line++;
    if(M.line>=LINES){ M.line=0; M.frame++; }
  }
  M.runFrame=function(){ const f=M.frame; while(M.frame===f){ runScanline(); if(cpu.jam) { if(M.line===0)break; } } };
  M.runScanline=runScanline;
  // ---------------- Keyboard --------------
  M.keyDown=function(scan){ if((pokey.skctl&3)===0) return;
    pokey.kbcode=scan&0xFF; pokey.keyDown=true; pokey.shiftDown=!!(scan&0x40);
    pokeyRaise(0x40); };
  M.keyUp=function(){ pokey.keyDown=false; pokey.shiftDown=false; };
  M.breakKey=function(){ pokeyRaise(0x80); };
  M.consol=function(start,select,option){
    gtia.consol=( (start?0:1) | (select?0:2) | (option?0:4) )&7;
  };
  M.reset=function(){ cpu.reset(); };
  M.coldStart=function(opt){
    ram.fill(0); ext.fill(0);
    pia.orA=0xFF;pia.ddrA=0;pia.orB=0xFF;pia.ddrB=0;pia.ctlA=0;pia.ctlB=0;
    pokey.irqen=0;pokey.irqpend=0;pokey.kbcode=0xFF;pokey.keyDown=false;pokey.outBusy=0;pokey.shiftBusy=0;
    antic.dmactl=0;antic.nmien=0;antic.nmist=0x1F;antic.dlist=0;
    gtia.consol=7; if(opt&&opt.option) gtia.consol&=~4;
    M.line=0;M.frame=0;M.wsync=false;cpu.jam=false;cpu.irqLine=0;
    cpu.reset();
  };
  return M;
}
if(typeof module!=='undefined') module.exports=Atari130XE;
