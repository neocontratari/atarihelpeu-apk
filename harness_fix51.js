const fs=require('fs'), vm=require('vm');
const html=fs.readFileSync('/mnt/data/fix51/app/src/main/assets/emu09_pmg_gtia_overlay.html','utf8');
const js=html.match(/<script>([\s\S]*)<\/script>/)[1];
class Ctx{constructor(){this.fillStyle='';this.font='';this.textBaseline='';this.ops=[];} fillRect(){} fillText(){} createImageData(w,h){return {data:new Uint8ClampedArray(w*h*4),width:w,height:h};} putImageData(img){this.lastImage=img;}}
const elements={};
function elt(id){ if(!elements[id]) elements[id]={id, textContent:'', innerHTML:'', value:'', className:'', style:{}, appendChild(){}, addEventListener(){}, click(){}, getContext(){ if(!this.ctx)this.ctx=new Ctx(); return this.ctx;}}; return elements[id];}
const ctx={console, atob:(s)=>Buffer.from(s,'base64').toString('binary'), btoa:(s)=>Buffer.from(s,'binary').toString('base64'), Uint8Array, Uint8ClampedArray, Array, Date, Math, JSON, performance:{now:()=>Date.now()}, setTimeout:(fn,ms)=>0, clearTimeout:()=>{}, requestAnimationFrame:(fn)=>0, cancelAnimationFrame:()=>{}, prompt:()=> 'REN', alert:()=>{}, window:{}, document:{getElementById:elt, createElement:(tag)=>elt('created_'+tag+'_'+Math.random()), addEventListener(){}}, navigator:{}, location:{href:''}};
ctx.window=ctx; ctx.window.AudioContext=function(){this.createGain=()=>({gain:{value:0,setTargetAtTime(){}} ,connect(){}});this.createOscillator=()=>({frequency:{value:0,setTargetAtTime(){}},connect(){},start(){}});this.destination={};this.state='running';this.resume=()=>{};}; ctx.window.webkitAudioContext=ctx.window.AudioContext;
vm.createContext(ctx);
vm.runInContext(js,ctx,{filename:'emu09_fix51.js'});
function hex(v,n=4){return (v>>>0).toString(16).toUpperCase().padStart(n,'0')}
function step(frames){for(let i=0;i<frames;i++)ctx.playLoop();}
function summary(label){const ram=ctx.ram,cpu=ctx.cpu; console.log(label, 'build',ctx.EMU_BUILD_TAG,'profile',ctx.activeXexProfile,'PC',hex(cpu.pc),'steps',cpu.steps,'DL',hex(ctx.getDlistPtr()),'CH',hex(ram[0xD409],2),'dliStates',ctx.genericDliCache?ctx.genericDliCache.length:0,'snapSrc',ctx.pitstopLastRenderSource,'snapPhase',ctx.pitstopSnapshotPhase);}
ctx.loadBuiltinPitstopII(); ctx.startPlaying(); step(20); summary('pitstop20');
ctx.compatibilitySnapshot(); if(!String(elements.log.textContent||'').includes('FIX51_BLANK_DLI_PMG_CORE')) throw new Error('missing FIX50 log tag');
if(String(elements.log.textContent||'').includes('FIX49_PITSTOP')) throw new Error('old tag in log');
ctx.loadBuiltinRiverRaid(); ctx.startPlaying(); step(10); summary('river10');
ctx.loadBuiltinArkanoidIII(); ctx.startPlaying(); step(10); summary('arkanoid10');
console.log('OK headless');
