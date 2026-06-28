/*
 * AtariHelp.eu EMU-10 / N&P VISION
 * BUILD2MO_SEGA_REAL_CORE_SLOT_STAGE2
 *
 * This file is intentionally NOT a fake Mega Drive emulator.
 * It prepares the real-core bridge, video canvas, ROM handoff and joypad input
 * for a separate real Sega Mega Drive core adapter.
 *
 * Expected external adapter names, in priority order:
 *   window.NAP_SEGA_REAL_CORE
 *   window.NAP_REAL_SEGA_CORE
 *   window.SegaMDCore
 *   window.GenesisCore
 *
 * Minimal adapter API supported:
 *   init({canvas, width, height, sampleRate, log}) or init(canvas, options)
 *   loadRom(Uint8Array, info) / loadROM(...) / insertCartridge(...)
 *   reset()
 *   start() / run() / resume()
 *   pause() / stop()
 *   press(button) / keyDown(button) / buttonDown(button)
 *   release(button) / keyUp(button) / buttonUp(button)
 */
(function(global){
'use strict';

var BRIDGE_BUILD = 'BUILD2MO_SEGA_REAL_CORE_SLOT_STAGE2';
var CANDIDATE_NAMES = ['NAP_SEGA_REAL_CORE','NAP_REAL_SEGA_CORE','NAP_SEGA_ADAPTER','SegaMDCore','GenesisCore','GenesisPlusGX','GPGX','JSMooGenesis'];

function safeLog(logger, msg){
  try{
    if(typeof logger === 'function') logger(msg);
    else if(global.console && console.log) console.log('[NAP SEGA BRIDGE] ' + msg);
  }catch(e){}
}

function callAny(obj, names, args){
  if(!obj) return {called:false, value:null, name:null};
  for(var i=0;i<names.length;i++){
    var n = names[i];
    if(typeof obj[n] === 'function'){
      return {called:true, value:obj[n].apply(obj,args || []), name:n};
    }
  }
  return {called:false, value:null, name:null};
}

function NAPSegaCoreBridge(canvas, logger){
  this.canvas = canvas || null;
  this.ctx = null;
  this.logger = logger || null;
  this.core = null;
  this.coreName = '';
  this.ready = false;
  this.romLoaded = false;
  this.running = false;
  this.lastRomInfo = null;
  this.status = 'CREATED';
  this.lastError = '';
  this.width = 320;
  this.height = 224;
}

NAPSegaCoreBridge.prototype._log = function(msg){
  safeLog(this.logger, 'SEGA CORE BRIDGE ' + BRIDGE_BUILD + ': ' + msg);
};

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
  }catch(e){
    this.lastError = e.message || String(e);
  }
};

NAPSegaCoreBridge.prototype.init = function(){
  this._clearCanvas('SEGA MD VIDEO 320x224');
  for(var i=0;i<CANDIDATE_NAMES.length;i++){
    var name = CANDIDATE_NAMES[i];
    var candidate = global[name];
    if(!candidate) continue;
    this.core = candidate;
    this.coreName = name;
    try{
      var init1 = callAny(candidate, ['init','initialize','open'], [{
        canvas:this.canvas,
        width:this.width,
        height:this.height,
        sampleRate:44100,
        log:this.logger
      }]);
      if(!init1.called){
        callAny(candidate, ['init','initialize','open'], [this.canvas,{width:this.width,height:this.height,sampleRate:44100,log:this.logger}]);
      }
      this.ready = true;
      this.status = 'REAL_CORE_ADAPTER_FOUND:' + name;
      this._log('REAL CORE ADAPTER FOUND name=' + name);
      return true;
    }catch(e){
      this.core = null;
      this.coreName = '';
      this.ready = false;
      this.lastError = e.message || String(e);
      this.status = 'CORE_ADAPTER_INIT_FAILED:' + name + ':' + this.lastError;
      this._log(this.status);
      return false;
    }
  }
  this.ready = false;
  this.status = 'NO_REAL_CORE_ADAPTER_FOUND';
  this._log('NO REAL CORE ADAPTER FOUND. Expected one of: ' + CANDIDATE_NAMES.join(', '));
  return false;
};

NAPSegaCoreBridge.prototype.loadRom = function(romBytes, info){
  this.lastRomInfo = info || null;
  this.romLoaded = false;
  if(!romBytes || !romBytes.length){
    this.status = 'ROM_BYTES_MISSING';
    this._log(this.status);
    return {ok:false, status:this.status};
  }
  if(!this.ready || !this.core){
    this.status = 'ROM_VALIDATED_BUT_CORE_MISSING';
    this._log(this.status + ' size=' + romBytes.length + ' title=' + ((info && (info.overseas || info.domestic)) || 'unknown'));
    this._clearCanvas('ROM OK - CORE MISSING');
    return {ok:false, status:this.status};
  }
  try{
    var r = callAny(this.core, ['loadRom','loadROM','insertCartridge','loadCartridge','load'], [romBytes, info || {}]);
    if(!r.called){
      this.status = 'REAL_CORE_FOUND_BUT_LOADROM_API_MISSING';
      this._log(this.status + ' adapter=' + this.coreName);
      return {ok:false, status:this.status};
    }
    this.romLoaded = true;
    this.status = 'ROM_SENT_TO_REAL_CORE:' + this.coreName + ':' + r.name;
    this._log(this.status);
    return {ok:true, status:this.status};
  }catch(e){
    this.lastError = e.message || String(e);
    this.status = 'REAL_CORE_LOADROM_FAILED:' + this.lastError;
    this._log(this.status);
    return {ok:false, status:this.status};
  }
};

NAPSegaCoreBridge.prototype.reset = function(){
  if(!this.ready || !this.core){ this.status='RESET_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{
    var r = callAny(this.core, ['reset','hardReset','softReset'], []);
    this._log(r.called ? ('RESET_SENT:' + r.name) : 'RESET_API_MISSING');
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('RESET_FAILED:' + this.lastError); return false; }
};

NAPSegaCoreBridge.prototype.start = function(){
  if(!this.ready || !this.core){ this.status='START_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{
    var r = callAny(this.core, ['start','run','resume','play'], []);
    this.running = !!r.called;
    this._log(r.called ? ('RUN_SENT:' + r.name) : 'RUN_API_MISSING');
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('RUN_FAILED:' + this.lastError); return false; }
};

NAPSegaCoreBridge.prototype.pause = function(){
  if(!this.ready || !this.core){ this.status='PAUSE_SKIPPED_CORE_MISSING'; this._log(this.status); return false; }
  try{
    var r = callAny(this.core, ['pause','stop','suspend'], []);
    this.running = false;
    this._log(r.called ? ('PAUSE_SENT:' + r.name) : 'PAUSE_API_MISSING');
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('PAUSE_FAILED:' + this.lastError); return false; }
};

NAPSegaCoreBridge.prototype.press = function(button){
  if(!this.ready || !this.core) return false;
  try{
    var r = callAny(this.core, ['press','keyDown','buttonDown','inputDown'], [button, 1]);
    if(r.called) this._log('INPUT_DOWN_SENT:' + button + ':' + r.name);
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('INPUT_DOWN_FAILED:' + button + ':' + this.lastError); return false; }
};

NAPSegaCoreBridge.prototype.release = function(button){
  if(!this.ready || !this.core) return false;
  try{
    var r = callAny(this.core, ['release','keyUp','buttonUp','inputUp'], [button, 0]);
    if(r.called) this._log('INPUT_UP_SENT:' + button + ':' + r.name);
    return !!r.called;
  }catch(e){ this.lastError=e.message||String(e); this._log('INPUT_UP_FAILED:' + button + ':' + this.lastError); return false; }
};

NAPSegaCoreBridge.prototype.getStatusLine = function(){
  return this.status + (this.coreName ? (' / ' + this.coreName) : '') + (this.lastError ? (' / ' + this.lastError) : '');
};

NAPSegaCoreBridge.BUILD = BRIDGE_BUILD;
NAPSegaCoreBridge.CANDIDATE_NAMES = CANDIDATE_NAMES.slice();
global.NAPSegaCoreBridge = NAPSegaCoreBridge;

})(window);
