/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2NA_SEGA_NATIVE_BIND_NO_ART_STAGE14
 *
 * Real-core slot loader. This does NOT emulate Sega Mega Drive and does NOT
 * draw fake frames. It only loads local/external JS/WASM glue and lets
 * nap_sega_core_bridge.js probe whether a real adapter API exists.
 */
(function(global){
'use strict';

var SLOT_BUILD = 'BUILD2NA_SEGA_NATIVE_BIND_NO_ART_STAGE14';
var ADAPTER_NAMES = [
  'NAP_SEGA_REAL_CORE','NAP_REAL_SEGA_CORE','NAP_SEGA_ADAPTER',
  'SegaMDCore','GenesisCore','GenesisPlusGX','GPGX','JSMooGenesis',
  'MegaDriveCore','MegadriveCore','MDCore','JSMD','GenesisEmulator'
];
var CANDIDATE_SCRIPTS = [
  'nap_sega_real_core.js','sega_real_core.js','sega_core.js','genesis_core.js','genesis_plus_gx.js',
  'gpgx.js','jsmoo_genesis.js','jsmoo.js','megadrive_core.js','md_core.js','genesis_emulator.js',
  'cores/nap_sega_real_core.js','cores/sega_real_core.js','cores/sega_core.js','cores/genesis_core.js','cores/genesis_plus_gx.js',
  'cores/gpgx.js','cores/jsmoo_genesis.js','cores/jsmoo.js','cores/megadrive_core.js','cores/md_core.js','cores/genesis_emulator.js'
];
function safeLog(logger,msg){ try{ if(typeof logger==='function') logger(msg); else if(global.console&&console.log) console.log('[NAP SEGA CORE SLOT] '+msg); }catch(e){} }
function adapterApiProbe(obj){
  if(!obj) return 'NO_OBJECT';
  var load=['loadRom','loadROM','insertCartridge','loadCartridge','loadGame','load','loadBinary'];
  var input=['press','release','keyDown','keyUp','buttonDown','buttonUp','setButton','inputDown','inputUp'];
  var foundLoad=false, foundInput=false, methods=[];
  function check(o){
    if(!o) return;
    try{ Object.getOwnPropertyNames(o).forEach(function(k){ if(typeof obj[k]==='function') methods.push(k); }); }catch(e){}
  }
  check(obj); check(Object.getPrototypeOf(obj));
  for(var i=0;i<load.length;i++){ try{ if(typeof obj[load[i]]==='function') foundLoad=true; }catch(e){} }
  for(var j=0;j<input.length;j++){ try{ if(typeof obj[input[j]]==='function') foundInput=true; }catch(e2){} }
  return 'load='+(foundLoad?'YES':'NO')+', input='+(foundInput?'YES':'optional/NO')+', methods='+(methods.slice(0,14).join('|')||'none');
}
function detectAdapter(){
  for(var i=0;i<ADAPTER_NAMES.length;i++){
    var name=ADAPTER_NAMES[i];
    try{
      if(global[name]) return {found:true,name:name,adapter:global[name],api:adapterApiProbe(global[name])};
    }catch(e){}
  }
  return {found:false,name:'',adapter:null,api:''};
}
function loadScriptUrl(url, logger, done, asModule){
  var finished=false;
  var script=document.createElement('script');
  if(asModule) script.type='module';
  var timer=setTimeout(function(){
    if(finished) return; finished=true; try{script.remove();}catch(e){}
    done({ok:false,status:'SCRIPT_TIMEOUT',url:url});
  },6500);
  script.onload=function(){
    if(finished) return; finished=true; clearTimeout(timer);
    var d=detectAdapter();
    done({ok:d.found,status:d.found?('ADAPTER_SYMBOL_FOUND:'+d.name):'SCRIPT_LOADED_BUT_NO_ADAPTER_SYMBOL',url:url,adapterName:d.name,api:d.api});
  };
  script.onerror=function(){
    if(finished) return; finished=true; clearTimeout(timer); try{script.remove();}catch(e){}
    done({ok:false,status:'SCRIPT_NOT_FOUND_OR_LOAD_FAILED',url:url});
  };
  script.src=url;
  script.async=false;
  try{ document.head.appendChild(script); }
  catch(e){ clearTimeout(timer); done({ok:false,status:'SCRIPT_APPEND_FAILED:'+(e.message||String(e)),url:url}); }
}
function loadBuiltInCandidates(logger,done){
  var already=detectAdapter();
  if(already.found){ safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': adapter uz existuje '+already.name+' api='+already.api); done&&done({ok:true,status:'ADAPTER_ALREADY_PRESENT:'+already.name,adapterName:already.name,api:already.api}); return; }
  var i=0;
  function next(){
    if(i>=CANDIDATE_SCRIPTS.length){ safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': zadny lokalni core adapter soubor nenalezen v assets/emu_sega ani assets/emu_sega/cores'); done&&done({ok:false,status:'NO_CORE_FILE_IN_ASSETS',tried:CANDIDATE_SCRIPTS.slice()}); return; }
    var url=CANDIDATE_SCRIPTS[i++];
    safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': zkousim lokalni adapter '+url);
    loadScriptUrl(url,logger,function(res){
      if(res&&res.ok){ safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': adapter symbol nalezen pres '+url+' -> '+res.adapterName+' api='+res.api); done&&done(res); }
      else { safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': '+url+' -> '+((res&&res.status)||'LOAD_FAILED')); next(); }
    });
  }
  next();
}
function looksLikeRomByName(name){ return /\.(gen|bin|md|smd|sms|gg|zip|7z|rar)$/i.test(String(name||'')); }
function loadExternalCoreFile(file,logger,done){
  if(!file){ done&&done({ok:false,status:'NO_FILE_SELECTED'}); return; }
  var name=file.name||'external_core';
  var lower=name.toLowerCase();
  if(looksLikeRomByName(name)){
    safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': ROM vybrana omylem do CORE pickeru '+name);
    done&&done({ok:false,status:'ROM_SELECTED_IN_CORE_PICKER',fileName:name,hint:'ROM_GOES_TO_GAMES_PICKER_CORE_PICKER_NEEDS_JS_OR_WASM_GLUE'});
    return;
  }
  if(lower.endsWith('.wasm')){
    try{
      if(global.NAP_SEGA_WASM_BLOB_URL){ try{ URL.revokeObjectURL(global.NAP_SEGA_WASM_BLOB_URL); }catch(e0){} }
      var wasmUrl=URL.createObjectURL(file);
      global.NAP_SEGA_WASM_BLOB_URL=wasmUrl;
      global.NAP_SEGA_WASM_FILE_NAME=name;
      safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': WASM ulozen jako blob URL pro JS adapter: '+name);
      done&&done({ok:false,status:'WASM_STORED_JS_ADAPTER_REQUIRED',fileName:name,wasmUrl:wasmUrl,hint:'Now load matching .js glue/adapter that reads window.NAP_SEGA_WASM_BLOB_URL'});
    }catch(e){ done&&done({ok:false,status:'WASM_BLOB_FAILED:'+(e.message||String(e)),fileName:name}); }
    return;
  }
  if(!(lower.endsWith('.js') || lower.endsWith('.mjs'))){
    safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': nepodporovany core soubor '+name);
    done&&done({ok:false,status:'UNSUPPORTED_CORE_FILE_EXTENSION',fileName:name,hint:'CORE_PICKER_NEEDS_JS_OR_WASM_GLUE'});
    return;
  }
  var reader=new FileReader();
  reader.onerror=function(){ done&&done({ok:false,status:'EXTERNAL_JS_READ_FAILED',fileName:name}); };
  reader.onload=function(){
    try{
      var code=String(reader.result||'');
      var hasKnownSymbol=/NAP_SEGA_REAL_CORE|NAP_REAL_SEGA_CORE|SegaMDCore|GenesisCore|GenesisPlusGX|GPGX|JSMooGenesis|MegaDriveCore|MDCore|JSMD|GenesisEmulator/.test(code);
      if(!hasKnownSymbol) safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': JS nema znamy globalni adapter symbol, presto ho zkusim nahrat: '+name);
      var src=code+'\n//# sourceURL='+name.replace(/[^a-z0-9_.$-]/gi,'_');
      var blob=new Blob([src],{type: lower.endsWith('.mjs') ? 'text/javascript' : 'application/javascript'});
      var url=URL.createObjectURL(blob);
      loadScriptUrl(url,logger,function(res){
        if(res&&res.ok){ safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': externi JS adapter symbol aktivni '+res.adapterName+' ze souboru '+name+' api='+res.api); done&&done({ok:true,status:'EXTERNAL_JS_ADAPTER_SYMBOL_FOUND:'+res.adapterName,fileName:name,adapterName:res.adapterName,api:res.api}); }
        else { safeLog(logger,'SEGA CORE SLOT '+SLOT_BUILD+': externi JS nacten, ale adapter symbol nenalezen: '+name); done&&done({ok:false,status:'EXTERNAL_JS_LOADED_BUT_NO_ADAPTER_SYMBOL',fileName:name}); }
      }, lower.endsWith('.mjs'));
    }catch(e){ done&&done({ok:false,status:'EXTERNAL_JS_LOAD_FAILED:'+(e.message||String(e)),fileName:name}); }
  };
  reader.readAsText(file);
}

global.NAP_SEGA_CORE_SLOT={
  BUILD:SLOT_BUILD,
  ADAPTER_NAMES:ADAPTER_NAMES.slice(),
  CANDIDATE_SCRIPTS:CANDIDATE_SCRIPTS.slice(),
  detectAdapter:detectAdapter,
  loadBuiltInCandidates:loadBuiltInCandidates,
  loadExternalCoreFile:loadExternalCoreFile
};

})(window);
