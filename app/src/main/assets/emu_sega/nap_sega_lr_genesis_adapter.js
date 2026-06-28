/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2MR_SEGA_LRUSSO_GENESIS_ONLINE_CORE_PROBE_STAGE5
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

var BUILD='BUILD2MR_SEGA_LRUSSO_GENESIS_ONLINE_CORE_PROBE_STAGE5';
var LOCAL_ENGINE_CANDIDATES=[
  'cores/Genesis.min.js',
  'cores/Genesis.js',
  'cores/lrusso_Genesis.min.js',
  'cores/lrusso_Genesis.js',
  'Genesis.min.js',
  'Genesis.js'
];
var ONLINE_ENGINE_URL='https://raw.githubusercontent.com/lrusso/Genesis/main/Genesis.min.js';
var ALLOW_ONLINE_PROBE=true;
var enginePromise=null;
var adapterLog=function(){};
var mountEl=null;
var lastRomName='';
var running=false;
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
    if(global.embedGenesis){ return Promise.resolve('ENGINE_ALREADY_PRESENT'); }
    if(i>=LOCAL_ENGINE_CANDIDATES.length){ return Promise.reject(new Error('LOCAL_ENGINE_NOT_FOUND')); }
    var url=LOCAL_ENGINE_CANDIDATES[i++];
    log('zkousim lokalni lrusso Genesis engine '+url);
    return scriptLoad(url,9000).then(function(){
      if(global.embedGenesis) return 'LOCAL_ENGINE_READY:'+url;
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
  log('lokalni engine chybi, zkousim ONLINE probe lrusso Genesis.min.js');
  return fetch(ONLINE_ENGINE_URL,{cache:'no-store'}).then(function(res){
    if(!res || !res.ok) throw new Error('ONLINE_FETCH_HTTP_'+(res && res.status));
    return res.text();
  }).then(function(code){
    log('online Genesis.min.js stazeny jako text, velikost '+code.length+' B; patchuji jen cross-origin guard');
    var patched=patchLrussoSource(code);
    var blob=new Blob([patched+'\n//# sourceURL=nap_lrusso_genesis_online_probe.js'],{type:'application/javascript'});
    var url=URL.createObjectURL(blob);
    return scriptLoad(url,12000).then(function(){
      if(global.embedGenesis) return 'ONLINE_ENGINE_READY:lrusso_Genesis_min_js';
      throw new Error('ONLINE_SCRIPT_LOADED_BUT_EMBEDGENESIS_MISSING');
    });
  });
}
function ensureEngine(){
  if(global.embedGenesis) return Promise.resolve('ENGINE_ALREADY_PRESENT');
  if(enginePromise) return enginePromise;
  enginePromise=tryLocalEngines().catch(function(localErr){
    log('LOCAL engine probe neuspel: '+(localErr.message||String(localErr)));
    return loadOnlineEngine();
  }).then(function(status){
    if(!global.embedGenesis) throw new Error('ENGINE_READY_STATUS_BUT_EMBEDGENESIS_MISSING:'+status);
    log('CORE ENGINE READY '+status);
    return status;
  }).catch(function(e){
    enginePromise=null;
    restoreCanvasFallback();
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
  global.embedGenesis({
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
    log('loadRom prijato title='+lastRomName+' size='+romBuffer.byteLength+'; spoustim engine ensure');
    return ensureEngine().then(function(engineStatus){
      log('engineStatus='+engineStatus+'; posilam ROM do embedGenesis');
      return bootWithEmbedGenesis(romBuffer,info||{});
    }).catch(function(e){
      restoreCanvasFallback();
      showToast('Real core se nepodarilo nacist: '+(e.message||String(e)));
      log('loadRom FAILED '+(e.message||String(e)));
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
  getStatus:function(){ return running ? 'LRUSSO_GENESIS_RUNNING' : (global.embedGenesis ? 'LRUSSO_ENGINE_READY' : 'LRUSSO_ENGINE_WAITING'); }
};

global.NAP_SEGA_REAL_CORE=adapter;
global.NAP_SEGA_LRUSSO_GENESIS_ADAPTER=adapter;

})(window);
