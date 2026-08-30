// Napodobenina prohlizece pro spousteni stranky, jak rika protokol (3d).
var chyby = [], casovace = [];
function prvek(){ return { style:{}, className:'', textContent:'', tabIndex:0,
  tagName:'DIV', _h:{}, addEventListener:function(t,f){ this._h[t]=f; },
  removeEventListener:function(){}, appendChild:function(){}, focus:function(){},
  setAttribute:function(){}, getAttribute:function(){ return null; },
  removeAttribute:function(){}, click:function(){}, load:function(){},
  requestFullscreen:function(){ return {catch:function(){}}; },
  play:function(){ return {catch:function(){}}; },
  getContext:function(){ return new Proxy({},{get:function(){ return function(){}; }}); },
  querySelectorAll:function(){ return []; },
  classList:{add:function(){},remove:function(){},toggle:function(){},contains:function(){return false;}} }; }
var prvky = {};
global.document = { getElementById:function(id){ return prvky[id]||(prvky[id]=prvek()); },
  querySelector:function(){ return prvek(); }, querySelectorAll:function(){ return []; },
  createElement:function(){ return prvek(); }, addEventListener:function(){},
  removeEventListener:function(){}, body:prvek(), documentElement:prvek(),
  hidden:false, hasFocus:function(){ return true; }, activeElement:null,
  exitFullscreen:function(){}, fullscreenElement:null };
global.window = global;
global.navigator = { userAgent:'node' };
global.location = { search:'', href:'http://127.0.0.1:8765/', reload:function(){} };
global.localStorage = { _d:{}, getItem:function(k){ return this._d[k]||null; },
                        setItem:function(k,v){ this._d[k]=String(v); } };
global.addEventListener = function(){};
global.console = { log:function(){}, warn:function(){},
                   error:function(){ chyby.push([].slice.call(arguments).join(' ')); } };
global.fetch = function(){ return Promise.resolve({ ok:true,
  text:function(){ return Promise.resolve('running=true atari=0 sega=0 seq=1 fps=30'); },
  arrayBuffer:function(){ return Promise.resolve(new ArrayBuffer(8)); },
  headers:{ get:function(){ return '44100'; } },
  body:{ getReader:function(){ return { read:function(){ return Promise.resolve({done:true}); } }; } } }); };
function XHR(){ this.responseText=''; this.status=200; }
XHR.prototype.open=function(){}; XHR.prototype.send=function(){ if(this.onload) this.onload(); };
global.XMLHttpRequest = XHR;
global.AudioContext = function(){ return { sampleRate:44100, currentTime:0, destination:{},
  createBuffer:function(c,f,r){ return { getChannelData:function(){ return new Float32Array(f); }, duration:f/r }; },
  createBufferSource:function(){ return { buffer:null, connect:function(){}, start:function(){}, onended:null }; },
  createGain:function(){ return { gain:{value:1}, connect:function(){} }; },
  resume:function(){ return Promise.resolve(); } }; };
global.webkitAudioContext = global.AudioContext;
global.MediaSource = function(){ this.addEventListener=function(){}; };
global.MediaSource.isTypeSupported = function(){ return false; };
global.URL = { createObjectURL:function(){ return 'blob:x'; } };
global.setInterval = function(f,ms){ casovace.push({f:f,ms:ms||0}); return casovace.length; };
global.setTimeout  = function(f,ms){ casovace.push({f:f,ms:ms||0}); return casovace.length; };
global.clearInterval = function(){}; global.clearTimeout = function(){};
global.requestAnimationFrame = function(f){ casovace.push({f:f,ms:16}); return 1; };
global.performance = { now:function(){ return Date.now(); } };
