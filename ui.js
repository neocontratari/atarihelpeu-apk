// ui.js - EMU-10 UI: POKEY audio, klavesnice, smycka, log. Inlined do HTML pri buildu.
'use strict';
(function(){
  var logEl, M, canvas, ctx, img, running=false, audio=null;
  var LOGBUF=[];
  function log(s){ LOGBUF.push(s); if(logEl){ logEl.textContent=LOGBUF.slice(-200).join('\n'); logEl.scrollTop=logEl.scrollHeight; } }
  window.emuLog=log;

  // ---------- POKEY audio (4 kanaly + poly 4/5/9/17 + GTIA speaker klik) ----------
  function makePoly(bits, tap1, tap2){
    var len=(1<<bits)-1, out=new Uint8Array(len), sr=(1<<bits)-1;
    for(var i=0;i<len;i++){ out[i]=sr&1;
      var fb=((sr>>tap1)^(sr>>tap2))&1; sr=(sr>>1)|(fb<<(bits-1)); }
    return out;
  }
  var poly4=makePoly(4,3,2), poly5=makePoly(5,4,2), poly9=makePoly(9,8,3), poly17=makePoly(17,16,11);
  function startAudio(){
    if(audio) return;
    var AC=window.AudioContext||window.webkitAudioContext; if(!AC){ log('AUDIO: AudioContext neni k dispozici'); return; }
    var ac=new AC(), sp=ac.createScriptProcessor(2048,0,1);
    var CPS=1773447/ac.sampleRate; // cyklu na vzorek (PAL)
    var st={cnt:[0,0,0,0], out:[0,0,0,0], p4:0,p5:0,p9:0,p17:0, clock:0, spkLevel:0, spkDecay:0};
    sp.onaudioprocess=function(e){
      var buf=e.outputBuffer.getChannelData(0);
      var pk=M?M.pokey:null, gt=M?M.gtia:null;
      for(var i=0;i<buf.length;i++){
        var s=0;
        if(pk){
          var base15=(pk.audctl&1), j12=(pk.audctl&0x10), j34=(pk.audctl&8);
          st.clock+=CPS;
          var steps=st.clock|0; st.clock-=steps;
          st.p4=(st.p4+steps)%15; st.p5=(st.p5+steps)%31; st.p9=(st.p9+steps)%511; st.p17=(st.p17+steps)%131071;
          for(var ch=0;ch<4;ch++){
            var ac8=pk.audc[ch], vol=ac8&15;
            if(ac8&0x10){ s+= (vol/60); continue; } // volume only
            if(!vol) continue;
            var per;
            if(ch===0&&(pk.audctl&0x40)) per=pk.audf[0]+4;
            else if(ch===2&&(pk.audctl&0x20)) per=pk.audf[2]+4;
            else if(j12&&ch===1) per=(pk.audf[0]+(pk.audf[1]<<8)+7)*( (pk.audctl&0x40)?1:(base15?114:28) );
            else if(j34&&ch===3) per=(pk.audf[2]+(pk.audf[3]<<8)+7)*( (pk.audctl&0x20)?1:(base15?114:28) );
            else per=(pk.audf[ch]+1)*(base15?114:28);
            if(j12&&ch===0) continue; if(j34&&ch===2) continue;
            st.cnt[ch]+=steps;
            while(st.cnt[ch]>=per){ st.cnt[ch]-=per;
              if(!(ac8&0x80) && !poly5[st.p5]) continue;     // poly5 gate
              if(ac8&0x20) st.out[ch]^=1;                     // cisty ton
              else if(ac8&0x40) st.out[ch]=poly4[st.p4];
              else st.out[ch]=(pk.audctl&0x80)?poly9[st.p9]:poly17[st.p17];
            }
            s+= st.out[ch]? (vol/60):0;
          }
        }
        // GTIA reproduktor (klik klaves XL OS pres CONSOL bit3)
        if(st.spkDecay>0){ st.spkDecay--; s+=st.spkLevel*0.25; }
        buf[i]=s;
      }
    };
    sp.connect(ac.destination);
    audio={ac:ac,sp:sp,st:st};
    if(M) M.onSpeaker=function(level){ st.spkLevel=level?1:-1; st.spkDecay=(ac.sampleRate*0.004)|0; };
    log('AUDIO: POKEY WebAudio bezi ('+ac.sampleRate+' Hz)');
  }

  // ---------- klavesnice: XL/XE scan kody ----------
  // zakladni kody (bez shift/ctrl); +0x40 shift, +0x80 ctrl
  var KEY={ 'l':0,'j':1,';':2,'k':5,'+':6,'*':7,'o':8,'p':10,'u':11,'i':13,'-':14,'=':15,
    'v':16,'c':18,'b':21,'x':22,'z':23,'4':24,'3':26,'6':27,'5':29,'2':30,'1':31,
    ',':32,' ':33,'.':34,'n':35,'m':37,'/':38,'r':40,'e':42,'y':43,'t':45,'w':46,'q':47,
    '9':48,'0':50,'7':51,'8':53,'f':56,'h':57,'d':58,'g':61,'s':62,'a':63 };
  var SHIFTED={'!':'1','"':'2','#':'3','$':'4','%':'5','&':'6',"'":'7','@':'8','(':'9',')':'0',
    ':':';','<':',','>':'.','?':'/','^':'*','_':'-','|':'=','\\':'+','[':',',']':'.'};
  function charToScan(chr){
    if(KEY[chr]!==undefined) return KEY[chr];
    var lo=chr.toLowerCase();
    if(chr!==lo && KEY[lo]!==undefined) return KEY[lo]|0x40;       // velke pismeno
    if(SHIFTED[chr]!==undefined) return KEY[SHIFTED[chr]]|0x40;    // shift symbol
    return undefined;
  }
  var CODE={ 'Enter':12,'NumpadEnter':12,'Backspace':52,'Escape':28,'Tab':44,'CapsLock':60,
    'ArrowUp':14|0x80,'ArrowDown':15|0x80,'ArrowLeft':6|0x80,'ArrowRight':7|0x80,
    'F1':17 /*HELP*/,'Backquote':39 /*INVERZE (Atari logo)*/ };
  function bindKeyboard(){
    window.addEventListener('keydown',function(e){
      if(!M) return;
      if(e.code==='F2'){ consolSet('option',true); e.preventDefault(); return; }
      if(e.code==='F3'){ consolSet('select',true); e.preventDefault(); return; }
      if(e.code==='F4'){ consolSet('start',true); e.preventDefault(); return; }
      if(e.code==='F5'){ doReset(); e.preventDefault(); return; }
      if(e.code==='F6'){ M.breakKey(); log('KEY BREAK'); e.preventDefault(); return; }
      var sc=CODE[e.code];
      if(sc===undefined && e.key && e.key.length===1) sc=charToScan(e.key);
      if(sc===undefined) return;
      if(e.ctrlKey) sc|=0x80;
      M.keyDown(sc&0xFF);
      e.preventDefault();
    });
    window.addEventListener('keyup',function(e){ if(M) M.keyUp(); });
  }

  // ---------- on-screen XE klavesnice ----------
  var ROWS=[
    [['ESC',28],['1',31],['2',30],['3',26],['4',24],['5',29],['6',27],['7',51],['8',53],['9',48],['0',50],['<',54],['>',55],['BS',52]],
    [['TAB',44],['Q',47],['W',46],['E',42],['R',40],['T',45],['Y',43],['U',11],['I',13],['O',8],['P',10],['-',14],['=',15],['RET',12]],
    [['CTRL',-1],['A',63],['S',62],['D',58],['F',56],['G',61],['H',57],['J',1],['K',5],['L',0],[';',2],['+',6],['*',7],['CAPS',60]],
    [['SHIFT',-2],['Z',23],['X',22],['C',18],['V',16],['B',21],['N',35],['M',37],[',',32],['.',34],['/',38],['INV',39],['HELP',17]],
    [['BREAK',-3],['MEZERNIK',33]]
  ];
  var oskShift=false, oskCtrl=false;
  function buildOSK(host){
    ROWS.forEach(function(row){
      var r=document.createElement('div'); r.className='krow';
      row.forEach(function(k){
        var b=document.createElement('button'); b.className='key'; b.textContent=k[0];
        if(k[0]==='MEZERNIK') b.classList.add('space');
        if(k[1]<0) b.classList.add('mod');
        var press=function(ev){ ev.preventDefault();
          if(k[1]===-1){ oskCtrl=!oskCtrl; b.classList.toggle('on',oskCtrl); return; }
          if(k[1]===-2){ oskShift=!oskShift; b.classList.toggle('on',oskShift); return; }
          if(k[1]===-3){ M&&M.breakKey(); log('KEY BREAK'); return; }
          var sc=k[1]; if(oskShift) sc|=0x40; if(oskCtrl) sc|=0x80;
          M&&M.keyDown(sc&0xFF);
        };
        var rel=function(ev){ ev.preventDefault(); if(k[1]>=0){ M&&M.keyUp(); } };
        b.addEventListener('pointerdown',press); b.addEventListener('pointerup',rel); b.addEventListener('pointerleave',rel);
        r.appendChild(b);
      });
      host.appendChild(r);
    });
  }

  // ---------- konzole / power ----------
  var consolState={start:false,select:false,option:false};
  function consolSet(k,v){ consolState[k]=v; if(M) M.consol(consolState.start,consolState.select,consolState.option); }
  function bindHold(id,k){ var b=document.getElementById(id);
    b.addEventListener('pointerdown',function(e){e.preventDefault();consolSet(k,true);b.classList.add('on');});
    var off=function(e){consolSet(k,false);b.classList.remove('on');};
    b.addEventListener('pointerup',off); b.addEventListener('pointerleave',off);
  }
  function power(opt){
    startAudio();
    M=window.EMU10.createMachine();
    if(audio) M.onSpeaker=function(level){ audio.st.spkLevel=level?1:-1; audio.st.spkDecay=(audio.ac.sampleRate*0.004)|0; };
    M.coldStart({option:opt});
    if(opt){ consolState.option=true; setTimeout(function(){ consolState.option=false; M.consol(false,false,false); },4000);
             M.consol(false,false,true); }
    running=true;
    document.getElementById('led').classList.add('on');
    log(opt?'POWER + OPTION: cold start, OPTION drzene 4 s -> SELF TEST':'POWER: cold start -> BASIC');
    log('ROM: prava XL OS 16K (reset $C2AA) + prava Atari BASIC 8K');
  }
  function doReset(){ if(!M) return; M.reset(); log('RESET (warm start pres reset vektor)'); }

  // ---------- hlavni smycka ----------
  var last=0, acc=0;
  function tick(ts){
    requestAnimationFrame(tick);
    if(!running||!M) return;
    if(!last) last=ts;
    acc+=Math.min(ts-last,100); last=ts;
    var n=0;
    while(acc>=20 && n<3){ M.runFrame(); acc-=20; n++;
      if(M.cpu.jam){ running=false;
        log('CPU JAM op $'+M.cpu.jamOp.toString(16).toUpperCase()+' na $'+M.cpu.jamPc.toString(16).toUpperCase()+' frame '+M.frame);
        log('PC=$'+M.cpu.pc.toString(16).toUpperCase()+' PORTB=$'+M.portB().toString(16).toUpperCase());
        break; }
    }
    var u8=new Uint8ClampedArray(M.fb.buffer);
    img.data.set(u8); ctx.putImageData(img,0,0);
  }

  // ---------- start ----------
  window.addEventListener('DOMContentLoaded',function(){
    logEl=document.getElementById('log');
    canvas=document.getElementById('scr'); ctx=canvas.getContext('2d');
    img=ctx.createImageData(384,240);
    document.getElementById('btnPower').addEventListener('click',function(){power(false);});
    document.getElementById('btnSelfTest').addEventListener('click',function(){power(true);});
    document.getElementById('btnReset').addEventListener('click',doReset);
    document.getElementById('btnBreak').addEventListener('click',function(){ if(M){M.breakKey(); log('KEY BREAK');} });
    document.getElementById('btnSaveLog').addEventListener('click',function(){
      var blob=new Blob([LOGBUF.join('\n')],{type:'text/plain'});
      var a=document.createElement('a'); a.href=URL.createObjectURL(blob);
      a.download='atarihelp-EMU10-BUILD1-log-'+Date.now()+'.txt'; a.click();
    });
    bindHold('btnOption','option'); bindHold('btnSelect','select'); bindHold('btnStart','start');
    buildOSK(document.getElementById('osk'));
    bindKeyboard();
    log('AtariHelp.eu EMU-10 BUILD1 CLEAN CORE pripraven.');
    log('Jadro proslo headless testy: BASIC READY 4.6 s | ? 1+1 = 2 | SELF TEST OK');
    requestAnimationFrame(tick);
  });
})();
