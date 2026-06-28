/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2MX_SEGA_ORIGIN_GUARD_SEMICOLON_STAGE11
 *
 * This file is intentionally NOT a fake Mega Drive emulator.
 * It prepares and probes a real-core adapter safely: object adapter, factory,
 * constructor/class, and common JS-glue global names. If no real adapter with a
 * ROM-loading API is present, it refuses to draw fake video.
 *
 * Supported adapter shapes:
 *   window.NAP_SEGA_REAL_CORE = { init, loadRom, press/release, start/reset }
 *   window.GenesisPlusGX.create(options) -> adapter
 *   window.JSMooGenesis / window.GenesisCore constructor -> adapter
 *   window.<name>.default / .core / .instance -> adapter
 *
 * Minimal adapter API:
 *   optional init({canvas,width,height,sampleRate,log})
 *   loadRom(Uint8Array, info) / loadROM(...) / insertCartridge(...) / loadGame(...)
 *   optional start() / run() / resume()
 *   optional press(button) + release(button), or setButton(button,down)
 */
(function(global){
'use strict';

var BRIDGE_BUILD = 'BUILD2MX_SEGA_ORIGIN_GUARD_SEMICOLON_STAGE11';
var CANDIDATE_NAMES = [
  'NAP_SEGA_REAL_CORE','NAP_REAL_SEGA_CORE','NAP_SEGA_ADAPTER',
  'SegaMDCore','GenesisCore','GenesisPlusGX','GPGX','JSMooGenesis',
  'MegaDriveCore','MegadriveCore','MDCore','JSMD','GenesisEmulator'
];
var INIT_NAMES = ['init','initialize','open','attach','attachCanvas'];
var LOAD_NAMES = ['loadRom','loadROM','insertCartridge','loadCartridge','loadGame','load','loadBinary'];
var RUN_NAMES = ['start','run','resume','play','mainLoop'];
var PAUSE_NAMES = ['pause','stop','suspend'];
var RESET_NAMES = ['reset','hardReset','softReset'];
var DOWN_NAMES = ['press','keyDown','buttonDown','inputDown','joypadDown','down'];
var UP_NAMES = ['release','keyUp','buttonUp','inputUp','joypadUp','up'];
var SET_NAMES = ['setButton','setInput','input','button','joypad'];
var FACTORY_NAMES = ['create','createCore','createInstance','make','factory','newCore','newInstance'];

function safeLog(logger, msg){
  try{
    if(typeof logger === 'function') logger(msg);
    else if(global.console && console.log) console.log('[NAP SEGA BRIDGE] ' + msg);
  }catch(e){}
}
function isPromiseLike(v){ return !!(v && (typeof v === 'object' || typeof v === 'function') && typeof v.then === 'function'); }
function methodNames(obj){
  var out=[], seen={};
  function add(n){ if(n && !seen[n]){ seen[n]=1; out.push(n); } }
  try{ Object.keys(obj || {}).forEach(add); }catch(e){}
  try{
    var p = obj && Object.getPrototypeOf(obj);
    while(p && p !== Object.prototype){ Object.getOwnPropertyNames(p).forEach(add); p = Object.getPrototypeOf(p); }
  }catch(e2){}
  return out.filter(function(n){ try{return typeof obj[n] === 'function';}catch(e){return false;} });
}
function firstMethod(obj, names){
  if(!obj) return null;
  for(var i=0;i<names.length;i++){
    var n=names[i];
    try{ if(typeof obj[n] === 'function') return n; }catch(e){}
  }
  return null;
}
function callAny(obj, names, args){
  var n = firstMethod(obj,names);
  if(!n) return {called:false, value:null, name:null};
  return {called:true, value:obj[n].apply(obj,args || []), name:n};
}
function adapterApiReport(obj){
  if(!obj) return 'NO_OBJECT';
  var methods = methodNames(obj);
  var load = firstMethod(obj,LOAD_NAMES);
  var init = firstMethod(obj,INIT_NAMES);
  var down = firstMethod(obj,DOWN_NAMES) || firstMethod(obj,SET_NAMES);
  var up = firstMethod(obj,UP_NAMES) || firstMethod(obj,SET_NAMES);
  var run = firstMethod(obj,RUN_NAMES);
  var reset = firstMethod(obj,RESET_NAMES);
  return 'load='+(load||'MISSING')+', init='+(init||'optional-missing')+', inputDown='+(down||'missing')+', inputUp='+(up||'missing')+', run='+(run||'optional-missing')+', reset='+(reset||'optional-missing')+', methods='+(methods.slice(0,18).join('|')||'none');
}
function hasLoadApi(obj){ return !!firstMethod(obj, LOAD_NAMES); }
function optionsFor(canvas, logger){
  return {canvas:canvas,width:320,height:224,sampleRate:44100,log:logger,wasmUrl:global.NAP_SEGA_WASM_BLOB_URL || null,wasmFileName:global.NAP_SEGA_WASM_FILE_NAME || null};
}
function pushCandidate(list, obj, name, kind){
  if(!obj) return;
  list.push({obj:obj,name:name,kind:kind});
}
function buildCandidateObjects(raw, name, opts, logger){
  var list=[];
  pushCandidate(list, raw, name, 'global');
  try{ if(raw && raw.default) pushCandidate(list, raw.default, name+'.default', 'default'); }catch(e){}
  try{ if(raw && raw.core) pushCandidate(list, raw.core, name+'.core', 'core'); }catch(e2){}
  try{ if(raw && raw.instance) pushCandidate(list, raw.instance, name+'.instance', 'instance'); }catch(e3){}
  try{
    if(raw){
      for(var i=0;i<FACTORY_NAMES.length;i++){
        var f=FACTORY_NAMES[i];
        if(typeof raw[f] === 'function'){
          try{ pushCandidate(list, raw[f](opts), name+'.'+f+'()', 'factory'); }
          catch(fe){ safeLog(logger, 'SEGA CORE BRIDGE '+BRIDGE_BUILD+': factory '+name+'.'+f+'() failed: '+(fe.message||String(fe))); }
        }
      }
    }
  }catch(e4){}
  if(typeof raw === 'function'){
    try{ pushCandidate(list, new raw(opts), 'new '+name+'()', 'constructor-options'); }
    catch(ce){ safeLog(logger, 'SEGA CORE BRIDGE '+BRIDGE_BUILD+': constructor new '+name+'(options) failed: '+(ce.message||String(ce))); }
    try{ pushCandidate(list, new raw(opts.canvas, opts), 'new '+name+'(canvas,options)', 'constructor-canvas'); }
    catch(ce2){}
    // Calling a function directly can be valid for some JS glue factories. Only do it for the known core symbol list.
    try{ pushCandidate(list, raw(opts), name+'(options)', 'function-factory'); }
    catch(fe2){}
  }
  return list;
}

function NAPSegaCoreBridge(canvas, logger){
  this.canvas = canvas || null;
  this.ctx = null;
  this.logger = logger || null;
  this.core = null;
  this.coreName = '';
  this.coreKind = '';
  this.ready = false;
  this.romLoaded = false;
  this.running = false;
  this.lastRomInfo = null;
  this.pendingRomBytes = null;
  this.pendingRomInfo = null;
  this.status = 'CREATED';
  this.lastError = '';
  this.apiReport = '';
  this.width = 320;
  this.height = 224;
}

NAPSegaCoreBridge.prototype._log = function(msg){ safeLog(this.logger, 'SEGA CORE BRIDGE ' + BRIDGE_BUILD + ': ' + msg); };
NAPSegaCoreBridge.prototype._setStatus = function(status){ this.status=status; this._log(status); };
NAPSegaCoreBridge.prototype._clearCanvas = function(label){
  if(!this.canvas) return;
  try{
    this.canvas.width = this.width;
    this.canvas.height = this.height;
    this.ctx = this.canvas.getContext('2d', {alpha:false}) || this.canvas.getContext('2d');
    if(!this.ctx) return;
    this.ctx.fillStyle = '#000000';
    this.ctx.fillRect(0,0,this.width,this.height);
    this.ctx.fillStyle = 'rgba(143,216,255,0.62)';
    this.ctx.font = 'bold 13px monospace';
    this.ctx.textAlign = 'center';
    this.ctx.fillText(label || 'SEGA CORE BRIDGE READY', this.width/2, this.height/2 - 8);
    this.ctx.fillStyle = 'rgba(143,216,255,0.40)';
    this.ctx.font = 'bold 10px monospace';
    this.ctx.fillText('NO FAKE VIDEO - WAITING FOR REAL CORE', this.width/2, this.height/2 + 12);
    this.ctx.strokeStyle='rgba(80,180,255,.45)'; this.ctx.strokeRect(22,22,this.width-44,this.height-44);
    for(var y=26;y<this.height-26;y+=8){ this.ctx.fillStyle='rgba(40,140,255,.10)'; this.ctx.fillRect(24,y,this.width-48,1); }
  }catch(e){ this.lastError = e.message || String(e); }
};
NAPSegaCoreBridge.prototype._useAdapter = function(obj, name, kind){
  if(isPromiseLike(obj)){
    this.status = 'ADAPTER_FACTORY_PROMISE_PENDING:' + name;
    var self=this;
    obj.then(function(realObj){
      if(realObj && hasLoadApi(realObj)){
        self.core=realObj; self.coreName=name; self.coreKind=kind+'-promise'; self.ready=true; self.status='REAL_CORE_ADAPTER_READY_ASYNC:'+name; self._log(self.status);
        if(self.pendingRomBytes) self.loadRom(self.pendingRomBytes,self.pendingRomInfo||{});
      }else{
        self.apiReport=adapterApiReport(realObj); self.ready=false; self.status='ADAPTER_PROMISE_RESOLVED_BUT_API_MISMATCH:'+name+':'+self.apiReport; self._log(self.status);
      }
    }).catch(function(e){ self.ready=false; self.lastError=e && e.message ? e.message : String(e); self.status='ADAPTER_PROMISE_REJECTED:'+name+':'+self.lastError; self._log(self.status); });
    this._log(this.status);
    return false;
  }
  if(!obj) return false;
  this.apiReport = adapterApiReport(obj);
  if(!hasLoadApi(obj)){
    this.status = 'ADAPTER_FOUND_BUT_API_MISMATCH:' + name + ':' + this.apiReport;
    this._log(this.status);
    return false;
  }
  this.core = obj;
  this.coreName = name;
  this.coreKind = kind || 'unknown';
  try{
    var initCall = callAny(obj, INIT_NAMES, [optionsFor(this.canvas,this.logger)]);
    if(!initCall.called){ initCall = callAny(obj, INIT_NAMES, [this.canvas, optionsFor(this.canvas,this.logger)]); }
    if(initCall.called && isPromiseLike(initCall.value)){
      var self=this;
      this.status='ADAPTER_INIT_PROMISE_PENDING:' + name;
      this._log(this.status);
      initCall.value.then(function(){
        self.ready=true; self.status='REAL_CORE_ADAPTER_READY_ASYNC_INIT:'+name; self._log(self.status);
        if(self.pendingRomBytes) self.loadRom(self.pendingRomBytes,self.pendingRomInfo||{});
      }).catch(function(e){ self.ready=false; self.lastError=e && e.message ? e.message : String(e); self.status='CORE_ADAPTER_INIT_FAILED:'+name+':'+self.lastError; self._log(self.status); });
      return false;
    }
    this.ready = true;
    this.status = 'REAL_CORE_ADAPTER_READY:' + name + ':' + this.coreKind + ':' + (initCall.called ? ('init='+initCall.name) : 'init=not-required');
    this._log(this.status + ' api=' + this.apiReport);
    return true;
  }catch(e){
    this.core = null; this.coreName=''; this.coreKind=''; this.ready=false;
    this.lastError = e.message || String(e);
    this.status = 'CORE_ADAPTER_INIT_FAILED:' + name + ':' + this.lastError;
    this._log(this.status);
    return false;
  }
};
NAPSegaCoreBridge.prototype.init = function(){
  this._clearCanvas('SEGA MD VIDEO 320x224');
  var opts = optionsFor(this.canvas,this.logger);
  var mismatch=[];
  for(var i=0;i<CANDIDATE_NAMES.length;i++){
    var name=CANDIDATE_NAMES[i], raw=global[name];
    if(!raw) continue;
    this._log('ADAPTER_SYMBOL_FOUND name=' + name + ' type=' + (typeof raw));
    var variants = buildCandidateObjects(raw,name,opts,this.logger);
    for(var j=0;j<variants.length;j++){
      var v=variants[j];
      var ok=this._useAdapter(v.obj,v.name,v.kind);
      if(ok || this.status.indexOf('PROMISE_PENDING')>=0) return ok;
      if(this.status.indexOf('API_MISMATCH')>=0) mismatch.push(this.status);
    }
  }
  this.ready=false;
  if(mismatch.length){
    this.status='REAL_CORE_SYMBOL_FOUND_BUT_NO_COMPATIBLE_API';
    this.lastError=mismatch.slice(-2).join(' || ');
    this._log(this.status + ' details=' + this.lastError);
    return false;
  }
  this.status='NO_REAL_CORE_ADAPTER_FOUND';
  this._log('NO REAL CORE ADAPTER FOUND. Expected one of: ' + CANDIDATE_NAMES.join(', '));
  return false;
};
NAPSegaCoreBridge.prototype.loadRom = function(romBytes, info){
  this.lastRomInfo=info||null;
  this.romLoaded=false;
  if(!romBytes || !romBytes.length){ this.status='ROM_BYTES_MISSING'; this._log(this.status); return {ok:false,status:this.status}; }
  if(!this.ready || !this.core){
    this.pendingRomBytes=romBytes; this.pendingRomInfo=info||{};
    this.status = (this.core && !this.ready) ? 'ROM_WAITING_FOR_CORE_INIT' : 'ROM_VALIDATED_BUT_CORE_MISSING';
    this._log(this.status + ' size=' + romBytes.length + ' title=' + ((info && (info.overseas || info.domestic)) || 'unknown'));
    this._clearCanvas(this.core ? 'ROM OK - CORE INIT WAIT' : 'ROM OK - CORE MISSING');
    return {ok:false,status:this.status};
  }
  try{
    var r=callAny(this.core,LOAD_NAMES,[romBytes,info||{}]);
    if(!r.called){ this.status='REAL_CORE_FOUND_BUT_LOADROM_API_MISSING'; this._log(this.status+' adapter='+this.coreName+' api='+adapterApiReport(this.core)); return {ok:false,status:this.status}; }
    if(isPromiseLike(r.value)){
      var self=this;
      this.status='ROM_LOAD_PROMISE_PENDING:'+this.coreName+':'+r.name;
      this._log(this.status);
      r.value.then(function(){ self.romLoaded=true; self.status='ROM_SENT_TO_REAL_CORE_ASYNC:'+self.coreName+':'+r.name; self._log(self.status); self.start(); }).catch(function(e){ self.lastError=e && e.message ? e.message : String(e); self.status='REAL_CORE_LOADROM_FAILED:'+self.lastError; self._log(self.status); });
      return {ok:false,status:this.status};
    }
    this.romLoaded=true;
    this.status='ROM_SENT_TO_REAL_CORE:'+this.coreName+':'+r.name;
    this._log(this.status);
    this.start();
    return {ok:true,status:this.status};
  }catch(e){ this.lastError=e.message||String(e); this.status='REAL_CORE_LOADROM_FAILED:'+this.lastError; this._log(this.status); return {ok:false,status:this.status}; }
};
NAPSegaCoreBridge.prototype.reset = function(){
  if(!this.ready || !this.core){ this.status='RESET_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{ var r=callAny(this.core,RESET_NAMES,[]); this._log(r.called?('RESET_SENT:'+r.name):'RESET_API_MISSING'); return !!r.called; }
  catch(e){ this.lastError=e.message||String(e); this._log('RESET_FAILED:'+this.lastError); return false; }
};
NAPSegaCoreBridge.prototype.start = function(){
  if(!this.ready || !this.core){ this.status='START_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{ var r=callAny(this.core,RUN_NAMES,[]); this.running=!!r.called; this._log(r.called?('RUN_SENT:'+r.name):'RUN_API_MISSING_OPTIONAL'); return !!r.called; }
  catch(e){ this.lastError=e.message||String(e); this._log('RUN_FAILED:'+this.lastError); return false; }
};
NAPSegaCoreBridge.prototype.pause = function(){
  if(!this.ready || !this.core){ this.status='PAUSE_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{ var r=callAny(this.core,PAUSE_NAMES,[]); this.running=false; this._log(r.called?('PAUSE_SENT:'+r.name):'PAUSE_API_MISSING_OPTIONAL'); return !!r.called; }
  catch(e){ this.lastError=e.message||String(e); this._log('PAUSE_FAILED:'+this.lastError); return false; }
};
NAPSegaCoreBridge.prototype.press = function(button){
  if(!this.ready || !this.core) return false;
  try{
    var r=callAny(this.core,DOWN_NAMES,[button,1]);
    if(!r.called) r=callAny(this.core,SET_NAMES,[button,true,1]);
    if(r.called) this._log('INPUT_DOWN_SENT:'+button+':'+r.name);
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('INPUT_DOWN_FAILED:'+button+':'+this.lastError); return false; }
};
NAPSegaCoreBridge.prototype.release = function(button){
  if(!this.ready || !this.core) return false;
  try{
    var r=callAny(this.core,UP_NAMES,[button,0]);
    if(!r.called) r=callAny(this.core,SET_NAMES,[button,false,0]);
    if(r.called) this._log('INPUT_UP_SENT:'+button+':'+r.name);
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('INPUT_UP_FAILED:'+button+':'+this.lastError); return false; }
};
NAPSegaCoreBridge.prototype.getStatusLine = function(){
  var s=this.status + (this.coreName?(' / '+this.coreName):'') + (this.coreKind?(' / '+this.coreKind):'');
  if(this.apiReport && this.status.indexOf('API_MISMATCH')>=0) s += ' / API ' + this.apiReport;
  if(this.lastError) s += ' / ' + this.lastError;
  return s;
};
NAPSegaCoreBridge.BUILD=BRIDGE_BUILD;
NAPSegaCoreBridge.CANDIDATE_NAMES=CANDIDATE_NAMES.slice();
NAPSegaCoreBridge.LOAD_NAMES=LOAD_NAMES.slice();
NAPSegaCoreBridge.INPUT_NAMES={down:DOWN_NAMES.slice(),up:UP_NAMES.slice(),set:SET_NAMES.slice()};
global.NAPSegaCoreBridge=NAPSegaCoreBridge;

})(window);
