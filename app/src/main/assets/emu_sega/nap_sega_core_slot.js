/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2MP_SEGA_CORE_PICKER_CLARITY_STAGE3
 *
 * Real-core slot loader. This does NOT emulate Sega Mega Drive and does NOT
 * draw fake frames. It only tries to load a real local JS/WASM adapter and then
 * lets nap_sega_core_bridge.js connect to it.
 */
(function(global){
'use strict';

var SLOT_BUILD = 'BUILD2MP_SEGA_CORE_PICKER_CLARITY_STAGE3';
var ADAPTER_NAMES = ['NAP_SEGA_REAL_CORE','NAP_REAL_SEGA_CORE','NAP_SEGA_ADAPTER','SegaMDCore','GenesisCore','GenesisPlusGX','GPGX','JSMooGenesis'];
var CANDIDATE_SCRIPTS = [
  'nap_sega_real_core.js',
  'sega_real_core.js',
  'sega_core.js',
  'genesis_core.js',
  'genesis_plus_gx.js',
  'cores/nap_sega_real_core.js',
  'cores/sega_real_core.js',
  'cores/sega_core.js',
  'cores/genesis_core.js',
  'cores/genesis_plus_gx.js'
];

function safeLog(logger, msg){
  try{
    if(typeof logger === 'function') logger(msg);
    else if(global.console && console.log) console.log('[NAP SEGA CORE SLOT] ' + msg);
  }catch(e){}
}
function detectAdapter(){
  for(var i=0;i<ADAPTER_NAMES.length;i++){
    var name = ADAPTER_NAMES[i];
    if(global[name]) return {found:true, name:name, adapter:global[name]};
  }
  return {found:false, name:'', adapter:null};
}
function loadScriptUrl(url, logger, done){
  var finished = false;
  var script = document.createElement('script');
  var timer = setTimeout(function(){
    if(finished) return;
    finished = true;
    try{ script.remove(); }catch(e){}
    done({ok:false, status:'SCRIPT_TIMEOUT', url:url});
  }, 4500);
  script.onload = function(){
    if(finished) return;
    finished = true;
    clearTimeout(timer);
    var d = detectAdapter();
    done({ok:d.found, status:d.found ? ('ADAPTER_FOUND:' + d.name) : 'SCRIPT_LOADED_BUT_NO_ADAPTER', url:url, adapterName:d.name});
  };
  script.onerror = function(){
    if(finished) return;
    finished = true;
    clearTimeout(timer);
    try{ script.remove(); }catch(e){}
    done({ok:false, status:'SCRIPT_NOT_FOUND_OR_LOAD_FAILED', url:url});
  };
  script.src = url;
  script.async = false;
  try{ document.head.appendChild(script); }
  catch(e){
    clearTimeout(timer);
    done({ok:false, status:'SCRIPT_APPEND_FAILED:' + (e.message || String(e)), url:url});
  }
}
function loadBuiltInCandidates(logger, done){
  var already = detectAdapter();
  if(already.found){
    safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': adapter uz existuje ' + already.name);
    done && done({ok:true, status:'ADAPTER_ALREADY_PRESENT:' + already.name, adapterName:already.name});
    return;
  }
  var i = 0;
  function next(){
    if(i >= CANDIDATE_SCRIPTS.length){
      safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': zadny lokalni core adapter soubor nenalezen v assets/emu_sega ani assets/emu_sega/cores');
      done && done({ok:false, status:'NO_CORE_FILE_IN_ASSETS', tried:CANDIDATE_SCRIPTS.slice()});
      return;
    }
    var url = CANDIDATE_SCRIPTS[i++];
    safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': zkousim lokalni adapter ' + url);
    loadScriptUrl(url, logger, function(res){
      if(res && res.ok){
        safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': adapter nalezen pres ' + url + ' -> ' + res.adapterName);
        done && done(res);
      } else {
        safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': ' + url + ' -> ' + ((res && res.status) || 'LOAD_FAILED'));
        next();
      }
    });
  }
  next();
}
function loadExternalCoreFile(file, logger, done){
  if(!file){ done && done({ok:false, status:'NO_FILE_SELECTED'}); return; }
  var name = file.name || 'external_core';
  var lower = name.toLowerCase();
  if(lower.endsWith('.wasm')){
    var wasmUrl = URL.createObjectURL(file);
    global.NAP_SEGA_WASM_BLOB_URL = wasmUrl;
    global.NAP_SEGA_WASM_FILE_NAME = name;
    safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': WASM ulozen jako blob URL, ale bez JS adapter/glue se nespusti: ' + name);
    done && done({ok:false, status:'WASM_STORED_JS_ADAPTER_REQUIRED', fileName:name, wasmUrl:wasmUrl});
    return;
  }
  if(!lower.endsWith('.js')){
    safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': nepodporovany core soubor ' + name + ' (pravdepodobne ROM vybrana do CORE pickeru)');
    done && done({ok:false, status:'UNSUPPORTED_CORE_FILE_EXTENSION', fileName:name, hint:'ROM_GOES_TO_GAMES_PICKER_CORE_PICKER_NEEDS_JS_OR_WASM_GLUE'});
    return;
  }
  var reader = new FileReader();
  reader.onerror = function(){
    done && done({ok:false, status:'EXTERNAL_JS_READ_FAILED', fileName:name});
  };
  reader.onload = function(){
    try{
      var code = String(reader.result || '');
      if(code.indexOf('NAP_SEGA_REAL_CORE') < 0 && code.indexOf('SegaMDCore') < 0 && code.indexOf('Genesis') < 0 && code.indexOf('GPGX') < 0){
        safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': JS nema jasny adapter symbol, stejne ho zkusim nahrat: ' + name);
      }
      var blob = new Blob([code + '\n//# sourceURL=' + name.replace(/[^a-z0-9_.$-]/gi,'_')], {type:'application/javascript'});
      var url = URL.createObjectURL(blob);
      loadScriptUrl(url, logger, function(res){
        if(res && res.ok){
          safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': externi JS adapter aktivni ' + res.adapterName + ' ze souboru ' + name);
          done && done({ok:true, status:'EXTERNAL_JS_ADAPTER_FOUND:' + res.adapterName, fileName:name, adapterName:res.adapterName});
        } else {
          safeLog(logger, 'SEGA CORE SLOT ' + SLOT_BUILD + ': externi JS nacten, ale adapter API nenalezeno: ' + name);
          done && done({ok:false, status:'EXTERNAL_JS_LOADED_BUT_NO_ADAPTER', fileName:name});
        }
      });
    }catch(e){
      done && done({ok:false, status:'EXTERNAL_JS_LOAD_FAILED:' + (e.message || String(e)), fileName:name});
    }
  };
  reader.readAsText(file);
}

global.NAP_SEGA_CORE_SLOT = {
  BUILD:SLOT_BUILD,
  ADAPTER_NAMES:ADAPTER_NAMES.slice(),
  CANDIDATE_SCRIPTS:CANDIDATE_SCRIPTS.slice(),
  detectAdapter:detectAdapter,
  loadBuiltInCandidates:loadBuiltInCandidates,
  loadExternalCoreFile:loadExternalCoreFile
};

})(window);
