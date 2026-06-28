/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2MT_SEGA_LRUSSO_LIVE_IFRAME_FALLBACK_STAGE7
 *
 * NAP adapter for the lrusso Genesis/PicoDrive browser core API.
 * It does not include ROMs and it does not paint fake gameplay. It only tries
 * to start a real Mega Drive browser emulator using:
 *   1) local bundled engine: assets/emu_sega/cores/Genesis.min.js or Genesis.js
 *   2) online probe: raw GitHub Genesis.min.js, patched in memory only because
 *      that file blocks direct cross-origin <script> embedding.
 *
 * Final/offline production should bundle a legally usable core file locally in
 * assets/emu_sega/cores/ and preserve the matching license/source notices.
 */
(function(global){
'use strict';

var BUILD='BUILD2MT_SEGA_LRUSSO_LIVE_IFRAME_FALLBACK_STAGE7';
var LOCAL_ENGINE_CANDIDATES=[
  'cores/Genesis.min.js',
  'cores/Genesis.js',
  'cores/lrusso_Genesis.min.js',
  'cores/lrusso_Genesis.js',
  'Genesis.min.js',
  'Genesis.js'
];
var ONLINE_ENGINE_URL='https://raw.githubusercontent.com/lrusso/Genesis/main/Genesis.min.js';
var LIVE_EMULATOR_DEMO_URL='https://lrusso.github.io/Genesis/Genesis.htm?demo=';
var LIVE_EMULATOR_PICKER_URL='https://lrusso.github.io/Genesis/Genesis.htm';
var ALLOW_ONLINE_PROBE=true;
var enginePromise=null;
var adapterLog=function(){};
var mountEl=null;
var lastRomName='';
var running=false;
var liveFallback=false;
var liveIframe=null;
var liveFallbackReason='';
var keyState={};
var KEY_MAP={
  UP:{code:'ArrowUp',key:'ArrowUp',keyCode:38},
  DOWN:{code:'ArrowDown',key:'ArrowDown',keyCode:40},
  LEFT:{code:'ArrowLeft',key:'ArrowLeft',keyCode:37},
  RIGHT:{code:'ArrowRight',key:'ArrowRight',keyCode:39},
  START:{code:'Enter',key:'Enter',keyCode:13},
  A:{code:'KeyA',key:'a',keyCode:65},
  B:{code:'KeyS',key:'s',keyCode:83},
  C:{code:'KeyD',key:'d',keyCode:68},
  MODE:{code:'KeyZ',key:'z',keyCode:90},
  X:{code:'KeyQ',key:'q',keyCode:81},
  Y:{code:'KeyW',key:'w',keyCode:87},
  Z:{code:'KeyE',key:'e',keyCode:69}
};
function log(msg){ try{ adapterLog('SEGA LRUSSO ADAPTER '+BUILD+': '+msg); }catch(e){ try{ console.log('[NAP SEGA LRUSSO] '+msg); }catch(_e){} } }
function byId(id){ return global.document ? document.getElementById(id) : null; }
function hideOverlay(){
  try{ var ov=byId('monitorOverlay'); if(ov) ov.classList.remove('show'); }catch(e){}
}
function showToast(msg){
  try{
    var el=byId('toast');
    if(!el) return;
    el.textContent=msg; el.classList.add('show');
    clearTimeout(global.__segaToastTimer);
    global.__segaToastTimer=setTimeout(function(){ el.classList.remove('show'); },4200);
  }catch(e){}
}

var lastEmbedGenesisSource='MISSING';
function getEmbedGenesis(){
  try{
    if(typeof global.embedGenesis === 'function'){
      lastEmbedGenesisSource='window.embedGenesis';
      return global.embedGenesis;
    }
  }catch(e){}
  // Some browser bundles expose top-level let/const/function symbols in the
  // global lexical environment. They are callable by later scripts, but they do
  // NOT become window.embedGenesis. BUILD2MR missed this case and therefore
  // reported EMBEDGENESIS_MISSING even after the 2 MB engine script loaded.
  try{
    if(typeof global.eval === 'function'){
      var fn=global.eval('(typeof embedGenesis === "function") ? embedGenesis : null');
      if(typeof fn === 'function'){
        lastEmbedGenesisSource='global-lexical embedGenesis';
        return fn;
      }
    }
  }catch(e2){}
  try{
    var fn2=(0,eval)('(typeof embedGenesis === "function") ? embedGenesis : null');
    if(typeof fn2 === 'function'){
      lastEmbedGenesisSource='indirect-global-lexical embedGenesis';
      return fn2;
    }
  }catch(e3){}
  lastEmbedGenesisSource='MISSING';
  return null;
}
function embedSource(){ getEmbedGenesis(); return lastEmbedGenesisSource; }
function drawCoreScreen(title, detail, mode){
  try{
    var c=byId('mdVideo');
    if(!c || !c.getContext) return;
    c.style.display='block';
    if(mountEl) mountEl.style.display='none';
    var ctx=c.getContext('2d',{alpha:false}) || c.getContext('2d');
    if(!ctx) return;
    var w=c.width||320, h=c.height||224;
    var t=(Date.now()/1000)%1000;
    ctx.fillStyle='#020611'; ctx.fillRect(0,0,w,h);
    var grad=ctx.createRadialGradient(w*.5,h*.43,4,w*.5,h*.45,w*.68);
    grad.addColorStop(0,'#062f64'); grad.addColorStop(.62,'#010914'); grad.addColorStop(1,'#000000');
    ctx.fillStyle=grad; ctx.fillRect(0,0,w,h);
    for(var y=0;y<h;y+=4){ ctx.fillStyle=(y%8===0)?'rgba(28,120,255,.13)':'rgba(255,255,255,.025)'; ctx.fillRect(0,y,w,1); }
    for(var x=0;x<w;x+=16){ ctx.fillStyle='rgba(60,160,255,.055)'; ctx.fillRect(x,0,1,h); }
    var pulse=Math.floor((Math.sin(t*5)+1)*54)+64;
    ctx.strokeStyle='rgba(80,180,255,.55)'; ctx.lineWidth=2; ctx.strokeRect(10,10,w-20,h-20);
    ctx.strokeStyle='rgba(130,220,255,.30)'; ctx.lineWidth=1; ctx.strokeRect(16,16,w-32,h-32);
    ctx.fillStyle='rgb('+pulse+','+(pulse+70)+',255)';
    ctx.font='bold 15px monospace'; ctx.textAlign='center'; ctx.fillText('SEGA REAL CORE',w/2,42);
    ctx.fillStyle='rgba(225,245,255,.95)'; ctx.font='bold 12px monospace';
    wrapCanvasText(ctx,String(title||'CORE BOOT PROBE'),w/2,70,w-38,14);
    ctx.fillStyle='rgba(143,216,255,.80)'; ctx.font='bold 10px monospace';
    wrapCanvasText(ctx,String(detail||''),w/2,118,w-42,12);
    var bx=44, by=178, bw=232, bh=10;
    ctx.strokeStyle='rgba(110,200,255,.45)'; ctx.strokeRect(bx,by,bw,bh);
    var p=(mode==='error')?bw:Math.floor(((t*55)%bw));
    ctx.fillStyle=mode==='error'?'rgba(255,80,80,.75)':'rgba(80,180,255,.75)';
    ctx.fillRect(bx+1,by+1,Math.max(4,p),bh-2);
    ctx.fillStyle='rgba(180,230,255,.62)'; ctx.font='bold 9px monospace'; ctx.fillText(mode==='error'?'ENGINE FAILED - LOG ULOZENE':'NO FAKE VIDEO - REAL ENGINE ONLY',w/2,205);
  }catch(e){ log('DRAW CORE SCREEN FAILED '+(e.message||String(e))); }
}
function wrapCanvasText(ctx,text,x,y,maxWidth,lineHeight){
  var words=String(text||'').split(/\s+/), line='', lines=[];
  for(var n=0;n<words.length;n++){
    var test=line?line+' '+words[n]:words[n];
    if(ctx.measureText(test).width>maxWidth && line){ lines.push(line); line=words[n]; }
    else line=test;
  }
  if(line) lines.push(line);
  for(var i=0;i<Math.min(lines.length,5);i++) ctx.fillText(lines[i],x,y+i*lineHeight);
}
function prepareMount(){
  mountEl=byId('genesisMount');
  if(!mountEl){
    mountEl=document.createElement('div');
    mountEl.id='genesisMount';
    mountEl.style.cssText='position:absolute;left:8.45%;top:14.95%;width:83.2%;height:31.1%;z-index:6;border-radius:20px;overflow:hidden;background:#000;display:none;';
    var stage=document.querySelector('.stage') || document.body;
    stage.appendChild(mountEl);
  }
  mountEl.innerHTML='';
  mountEl.style.display='block';
  mountEl.style.background='#000';
  var mdVideo=byId('mdVideo');
  if(mdVideo) mdVideo.style.display='none';
  hideOverlay();
  return mountEl;
}
function restoreCanvasFallback(){
  try{
    var mdVideo=byId('mdVideo');
    if(mdVideo) mdVideo.style.display='block';
    if(mountEl) mountEl.style.display='none';
  }catch(e){}
}

function bootLiveIframeFallback(reason, info){
  try{
    liveFallback=true;
    liveFallbackReason=String(reason||'UNKNOWN');
    prepareMount();
    if(!mountEl) throw new Error('MOUNT_ELEMENT_MISSING');
    mountEl.innerHTML='';
    mountEl.style.display='block';
    mountEl.style.background='#000';
    mountEl.style.border='1px solid rgba(70,180,255,.55)';
    mountEl.style.boxShadow='0 0 18px rgba(0,130,255,.45) inset';
    var frameWrap=document.createElement('div');
    frameWrap.style.cssText='position:absolute;left:0;top:0;width:100%;height:100%;background:#000;overflow:hidden;border-radius:16px;';
    var infoBar=document.createElement('div');
    infoBar.style.cssText='position:absolute;left:0;right:0;bottom:0;z-index:3;padding:4px 6px;background:rgba(0,0,0,.72);color:#8fd8ff;font:bold 8px monospace;text-align:center;letter-spacing:.03em;pointer-events:none;';
    var romName=(info && info.name) ? info.name : lastRomName;
    infoBar.textContent='LIVE REAL CORE FALLBACK: klikni cerveny PLAY nebo upload ikonku uvnitr monitoru. Lokalni ROM '+(romName||'')+' nejde automaticky predat do cross-origin iframe.';
    var iframe=document.createElement('iframe');
    liveIframe=iframe;
    iframe.title='AtariHelp Sega live real core fallback';
    iframe.src=LIVE_EMULATOR_DEMO_URL;
    iframe.allow='autoplay; fullscreen; gamepad; clipboard-read; clipboard-write';
    iframe.referrerPolicy='no-referrer';
    iframe.style.cssText='position:absolute;left:0;top:0;width:100%;height:100%;border:0;background:#000;transform:scale(1);transform-origin:0 0;';
    iframe.onload=function(){
      log('LIVE IFRAME FALLBACK LOAD FIRED url='+LIVE_EMULATOR_DEMO_URL+' reason='+liveFallbackReason+'; klikni cerveny PLAY v monitoru pro realny demo core, nebo v dalsim kroku pouzijeme offline/local engine.');
      showToast('LIVE real core fallback nacten - klikni cerveny PLAY v monitoru.',5200);
    };
    frameWrap.appendChild(iframe);
    frameWrap.appendChild(infoBar);
    mountEl.appendChild(frameWrap);
    var mdVideo=byId('mdVideo');
    if(mdVideo) mdVideo.style.display='none';
    hideOverlay();
    log('LIVE IFRAME FALLBACK READY demoUrl='+LIVE_EMULATOR_DEMO_URL+' pickerUrl='+LIVE_EMULATOR_PICKER_URL+' reason='+liveFallbackReason+' selectedRom='+(romName||'none')+' NOTE=not fake; remote lrusso real core, local ROM not auto-passed due browser isolation');
    showToast('Oteviram LIVE real Genesis core v monitoru. Klikni cerveny PLAY.',6200);
    return true;
  }catch(e){
    liveFallback=false;
    log('LIVE IFRAME FALLBACK FAILED '+(e.message||String(e)));
    return false;
  }
}
function scriptLoad(url, timeoutMs){
  return new Promise(function(resolve,reject){
    var s=document.createElement('script');
    var done=false;
    var timer=setTimeout(function(){ if(done) return; done=true; try{s.remove();}catch(e){} reject(new Error('SCRIPT_TIMEOUT '+url)); }, timeoutMs||9000);
    s.onload=function(){ if(done) return; done=true; clearTimeout(timer); resolve('SCRIPT_LOADED '+url); };
    s.onerror=function(){ if(done) return; done=true; clearTimeout(timer); try{s.remove();}catch(e){} reject(new Error('SCRIPT_LOAD_FAILED '+url)); };
    s.async=false;
    s.src=url;
    document.head.appendChild(s);
  });
}
function tryLocalEngines(){
  var i=0;
  function next(){
    if(getEmbedGenesis()){ return Promise.resolve('ENGINE_ALREADY_PRESENT:'+embedSource()); }
    if(i>=LOCAL_ENGINE_CANDIDATES.length){ return Promise.reject(new Error('LOCAL_ENGINE_NOT_FOUND')); }
    var url=LOCAL_ENGINE_CANDIDATES[i++];
    log('zkousim lokalni lrusso Genesis engine '+url);
    return scriptLoad(url,9000).then(function(){
      if(getEmbedGenesis()) return 'LOCAL_ENGINE_READY:'+url+':'+embedSource();
      log(url+' nacten, ale embedGenesis symbol neni pritomen');
      return next();
    }).catch(function(e){
      log(url+' -> '+(e.message||String(e)));
      return next();
    });
  }
  return next();
}
function patchLrussoSource(code){
  var src=String(code||'');
  var before=src.length;
  // The upstream file contains a tiny anti-cross-origin script-tag guard.
  // For the online probe we fetch as text and run as a same-session blob, so
  // remove only that guard. This is not a gameplay patch and does not touch emulation logic.
  src=src.replace('if("undefined"!=typeof document&&document.currentScript&&document.currentScript.src.split("/").slice(0,3).join("/")!==window.location.origin)throw new Error("Error.");','');
  if(src.length===before){
    src=src.replace(/if\("undefined"!=typeof document&&document\.currentScript&&document\.currentScript\.src\.split\("\/"\)\.slice\(0,3\)\.join\("\/"\)!==window\.location\.origin\)throw new Error\("Error\."\);?/,'');
  }
  return src;
}
function loadOnlineEngine(){
  if(!ALLOW_ONLINE_PROBE) return Promise.reject(new Error('ONLINE_PROBE_DISABLED'));
  if(!global.fetch) return Promise.reject(new Error('FETCH_NOT_AVAILABLE_IN_WEBVIEW'));
  drawCoreScreen('ONLINE ENGINE PROBE','Stahuji lrusso Genesis.min.js jako realny engine. Pokud WebView povoli skript, musi vzniknout embedGenesis symbol.', 'boot');
  log('lokalni engine chybi, zkousim ONLINE probe lrusso Genesis.min.js');
  return fetch(ONLINE_ENGINE_URL,{cache:'no-store'}).then(function(res){
    if(!res || !res.ok) throw new Error('ONLINE_FETCH_HTTP_'+(res && res.status));
    return res.text();
  }).then(function(code){
    drawCoreScreen('ENGINE SCRIPT LOADED', 'Genesis.min.js stazen: '+code.length+' B. Hledam window nebo global-lexical embedGenesis.', 'boot');
    log('online Genesis.min.js stazeny jako text, velikost '+code.length+' B; patchuji jen cross-origin guard');
    var patched=patchLrussoSource(code);
    var blob=new Blob([patched+'\n//# sourceURL=nap_lrusso_genesis_online_probe.js'],{type:'application/javascript'});
    var url=URL.createObjectURL(blob);
    return scriptLoad(url,12000).then(function(){
      if(getEmbedGenesis()) return 'ONLINE_ENGINE_READY:lrusso_Genesis_min_js:'+embedSource();
      throw new Error('ONLINE_SCRIPT_LOADED_BUT_EMBEDGENESIS_MISSING:'+embedSource());
    });
  });
}
function ensureEngine(){
  if(getEmbedGenesis()) return Promise.resolve('ENGINE_ALREADY_PRESENT:'+embedSource());
  if(enginePromise) return enginePromise;
  enginePromise=tryLocalEngines().catch(function(localErr){
    log('LOCAL engine probe neuspel: '+(localErr.message||String(localErr)));
    return loadOnlineEngine();
  }).then(function(status){
    if(!getEmbedGenesis()) throw new Error('ENGINE_READY_STATUS_BUT_EMBEDGENESIS_MISSING:'+status+':'+embedSource());
    log('CORE ENGINE READY '+status);
    return status;
  }).catch(function(e){
    enginePromise=null;
    restoreCanvasFallback();
    drawCoreScreen('CORE ENGINE LOAD FAILED', (e.message||String(e)), 'error');
    log('CORE ENGINE LOAD FAILED '+(e.message||String(e)));
    throw e;
  });
  return enginePromise;
}
function toArrayBuffer(romBytes){
  if(!romBytes) throw new Error('ROM_BYTES_MISSING');
  if(romBytes instanceof ArrayBuffer) return romBytes.slice(0);
  if(romBytes.buffer instanceof ArrayBuffer){
    return romBytes.buffer.slice(romBytes.byteOffset||0,(romBytes.byteOffset||0)+(romBytes.byteLength||romBytes.length||0));
  }
  throw new Error('ROM_BYTES_UNSUPPORTED_TYPE');
}
function dispatchKey(type, button){
  if(liveFallback){
    try{ if(liveIframe && liveIframe.focus) liveIframe.focus(); }catch(_e){}
    log('LIVE_IFRAME_INPUT_NOT_BRIDGED '+String(button||'')+' '+type+' - iframe ma vlastni ovladani; tohle neni fake potvrzeni vstupu');
    return false;
  }
  var map=KEY_MAP[String(button||'').toUpperCase()];
  if(!map) return false;
  var id=map.code;
  if(type==='keydown'){
    if(keyState[id]) return true;
    keyState[id]=true;
  }else{
    keyState[id]=false;
  }
  try{
    var ev=new KeyboardEvent(type,{bubbles:true,cancelable:true,key:map.key,code:map.code,which:map.keyCode,keyCode:map.keyCode});
    document.dispatchEvent(ev);
    global.dispatchEvent(ev);
    if(mountEl) mountEl.dispatchEvent(ev);
    return true;
  }catch(e){
    try{
      var ev2=document.createEvent('Event');
      ev2.initEvent(type,true,true);
      ev2.key=map.key; ev2.code=map.code;
      document.dispatchEvent(ev2);
      return true;
    }catch(e2){ log('KEY DISPATCH FAILED '+button+' '+(e2.message||String(e2))); return false; }
  }
}
function bootWithEmbedGenesis(romBuffer, info){
  prepareMount();
  var title=(info && (info.overseas || info.domestic || info.name)) || lastRomName || 'SEGA Mega Drive ROM';
  showToast('Startuji realny Genesis/PicoDrive core: '+title);
  log('embedGenesis START title='+title+' size='+romBuffer.byteLength);
  var embedGenesisFn=getEmbedGenesis();
  if(!embedGenesisFn) throw new Error('EMBEDGENESIS_SYMBOL_MISSING_AT_BOOT:'+embedSource());
  log('embedGenesis symbol source='+embedSource());
  embedGenesisFn({
    container:'genesisMount',
    name:title,
    rom:romBuffer,
    soundEnabled:true,
    showMobileControls:false,
    backText:'ZPET',
    soundText:'ZVUK',
    loadText:'LOAD',
    saveText:'SAVE',
    player1:{
      up:'ArrowUp',down:'ArrowDown',left:'ArrowLeft',right:'ArrowRight',
      start:'Enter',mode:'KeyZ',a:'KeyA',b:'KeyS',c:'KeyD',x:'KeyQ',y:'KeyW',z:'KeyE'
    },
    player2:{
      up:'KeyI',down:'KeyK',left:'KeyJ',right:'KeyL',start:'KeyH',mode:'KeyV',a:'KeyB',b:'KeyN',c:'KeyM',x:'KeyU',y:'KeyO',z:'KeyP'
    },
    cbStarted:function(){
      running=true;
      hideOverlay();
      showToast('SEGA core nastartoval. Zkus START / A / B / C.');
      log('embedGenesis CALLBACK STARTED - real core reported start');
    }
  });
  return true;
}
var adapter={
  BUILD:BUILD,
  init:function(options){
    adapterLog=(options && typeof options.log==='function') ? options.log : adapterLog;
    log('adapter init OK; local engine candidates='+LOCAL_ENGINE_CANDIDATES.join(', ')+'; onlineProbe='+(ALLOW_ONLINE_PROBE?'YES':'NO'));
    return true;
  },
  loadRom:function(romBytes, info){
    lastRomName=(info && (info.overseas || info.domestic || info.name)) || 'SEGA ROM';
    var romBuffer=toArrayBuffer(romBytes);
    drawCoreScreen('BOOTING '+lastRomName, 'ROM overena, spoustim realny lrusso Genesis/PicoDrive engine. Zadny fake obraz.', 'boot');
    log('loadRom prijato title='+lastRomName+' size='+romBuffer.byteLength+'; spoustim engine ensure');
    return ensureEngine().then(function(engineStatus){
      log('engineStatus='+engineStatus+'; posilam ROM do embedGenesis');
      return bootWithEmbedGenesis(romBuffer,info||{});
    }).catch(function(e){
      var msg=(e && e.message) ? e.message : String(e);
      restoreCanvasFallback();
      drawCoreScreen('REAL CORE FAILED', msg, 'error');
      log('loadRom FAILED '+msg);
      if(bootLiveIframeFallback(msg, info||{})){
        log('loadRom FALLBACK MODE LIVE_IFRAME_READY - toto neni automaticky Sonic boot; je to realny lrusso core v iframe, ROM se v nem vybira samostatne nebo DEMO pres PLAY.');
        showToast('Primy embed selhal, ale oteviram realny live core fallback.',6200);
        return 'LIVE_IFRAME_FALLBACK_READY:'+msg;
      }
      showToast('Real core se nepodarilo nacist: '+msg);
      throw e;
    });
  },
  loadROM:function(romBytes,info){ return this.loadRom(romBytes,info); },
  insertCartridge:function(romBytes,info){ return this.loadRom(romBytes,info); },
  start:function(){ hideOverlay(); return true; },
  run:function(){ hideOverlay(); return true; },
  resume:function(){ hideOverlay(); return true; },
  reset:function(){
    log('reset requested; lrusso engine ma vlastni reset/refresh hotkey, provadim soft reload pres posledni ROM jen pokud bude znovu vybrana');
    return false;
  },
  press:function(button){ return dispatchKey('keydown',button); },
  release:function(button){ return dispatchKey('keyup',button); },
  buttonDown:function(button){ return dispatchKey('keydown',button); },
  buttonUp:function(button){ return dispatchKey('keyup',button); },
  setButton:function(button,down){ return dispatchKey(down?'keydown':'keyup',button); },
  getStatus:function(){ return liveFallback ? ('LRUSSO_LIVE_IFRAME_READY_PICK_OR_DEMO:'+liveFallbackReason) : (running ? 'LRUSSO_GENESIS_RUNNING' : (getEmbedGenesis() ? ('LRUSSO_ENGINE_READY:'+embedSource()) : 'LRUSSO_ENGINE_WAITING')); }
};

global.NAP_SEGA_REAL_CORE=adapter;
global.NAP_SEGA_LRUSSO_GENESIS_ADAPTER=adapter;

})(window);
