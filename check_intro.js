
(function(){
'use strict';
var stage=document.querySelector('.stage');
var hud=document.getElementById('introMp3Hud');
var counter=document.getElementById('introMp3Counter');
var seekHud=document.getElementById('introMp3SeekHud');
var volNeedle=document.getElementById('introVolNeedle');
var volValue=document.getElementById('introVolValue');
var volHot=document.getElementById('introVolHot');
var mp3Now=document.getElementById('introMp3Now');
var playlistOverlay=document.getElementById('introPlaylistOverlay');
var playlistInfo=document.getElementById('introPlaylistInfo');
var playlistItems=document.getElementById('introPlaylistItems');
var introVolume=1.0, volHideTimer=null;
var audio=null, audioUrl='', audioName='', counterTimer=null, seekTimer=null, pressTimers={};
var playlist=[], playlistIndex=-1, incomingPlaylist=false, pendingPlaylist=[];
function $(id){return document.getElementById(id);}
function pad3(n){n=Math.max(0,Math.min(999,Math.floor(n||0))); return (n<10?'00':n<100?'0':'')+n;}
function fmt(t){ if(!isFinite(t)||t<0)t=0; var m=Math.floor(t/60), s=Math.floor(t%60); return m+':'+(s<10?'0':'')+s; }
function log(t){ try{console.log('[INTRO MP3] '+t);}catch(e){} }
function showVol(){ if(stage){ stage.classList.add('volshow'); if(volHideTimer)clearTimeout(volHideTimer); volHideTimer=setTimeout(function(){stage.classList.remove('volshow');},1400);} }
function applyVolume(v,announce){ introVolume=Math.max(0,Math.min(1,Number(v)||0)); if(audio) audio.volume=introVolume; if(volNeedle){ var deg=-130+(introVolume*260); volNeedle.style.transform='rotate('+deg+'deg)'; } if(volValue) volValue.textContent='VOL '+Math.round(introVolume*100)+'%'; if(announce){ showVol(); log('MP3 VOLUME '+Math.round(introVolume*100)+'% — hlasitost uvodniho kazetaku nastavena.'); } }
function setVolumeFromEvent(e){ if(!volHot)return; var r=volHot.getBoundingClientRect(), cx=r.left+r.width/2, cy=r.top+r.height/2; var ang=Math.atan2((e.clientY||0)-cy,(e.clientX||0)-cx)*180/Math.PI; var deg=ang+90; if(deg<-180)deg+=360; if(deg>180)deg-=360; var v=(deg+135)/270; if(v<0||v>1){ var x=((e.clientX||0)-r.left)/Math.max(1,r.width); v=x; } applyVolume(v,true); }
function setMode(mode){
  if(!stage)return;
  stage.classList.remove('mp3play','mp3ffwd','mp3rew');
  if(mode) stage.classList.add(mode);
  if(counter) counter.classList.toggle('run', !!mode);
}
function updateNow(){
  if(mp3Now){
    if(playlist.length>0 && playlistIndex>=0){
      mp3Now.textContent='MP3 '+(playlistIndex+1)+'/'+playlist.length+': '+(audioName||'hudba.mp3');
    } else {
      mp3Now.textContent='MP3: PRIDAT';
    }
  }
}
function updatePlaylistUi(){
  updateNow();
  var total=playlist.length+(incomingPlaylist?pendingPlaylist.length:0);
  if(playlistInfo){
    if(incomingPlaylist){
      playlistInfo.textContent='NACITAM MP3... prijato '+pendingPlaylist.length+' nova/nove. Uz v playlistu: '+playlist.length+'. Po dokonceni se skladby pripoji za konec.';
    } else {
      playlistInfo.textContent = playlist.length ? ('Vybrano '+playlist.length+' MP3. MP3 PRIDAT prida dalsi skladbu/skladby za konec. EJECT vymaze playlist.') : 'Playlist je prazdny. Stiskni MP3 PRIDAT a v Downloads vyber MP3. Kdyz NOX vezme jen jednu, stiskni MP3 PRIDAT znovu.';
    }
  }
  if(!playlistItems) return;
  playlistItems.innerHTML='';
  if(!total){
    var empty=document.createElement('div'); empty.textContent='Playlist je prazdny.'; empty.style.color='#8fd8ff'; empty.style.padding='8px 0'; playlistItems.appendChild(empty); return;
  }
  playlist.forEach(function(t,i){
    var b=document.createElement('button'); b.type='button'; b.textContent=(i+1)+'. '+(t.name||'hudba.mp3');
    if(i===playlistIndex) b.className='active';
    b.addEventListener('click',function(){ setTrack(i,true); if(playlistOverlay){ playlistOverlay.classList.remove('open'); playlistOverlay.setAttribute('aria-hidden','true'); } });
    playlistItems.appendChild(b);
  });
  if(incomingPlaylist){
    pendingPlaylist.forEach(function(t,i){
      var b=document.createElement('button'); b.type='button'; b.disabled=true; b.className='pending';
      b.textContent='nacitam: '+(playlist.length+i+1)+'. '+(t.name||'hudba.mp3');
      playlistItems.appendChild(b);
    });
  }
}
function showPlaylist(){ updatePlaylistUi(); if(playlistOverlay){ playlistOverlay.classList.add('open'); playlistOverlay.setAttribute('aria-hidden','false'); } }
function updateCounter(){
  var val=0;
  if(audio && audio.src && isFinite(audio.duration) && audio.duration>0) val=Math.floor((audio.currentTime/audio.duration)*999);
  if(counter) counter.textContent=pad3(val);
  updateNow();
}
function startCounter(){ stopCounter(false); counterTimer=setInterval(updateCounter,250); updateCounter(); }
function stopCounter(clearMode){ if(counterTimer){clearInterval(counterTimer); counterTimer=null;} if(clearMode) setMode(''); updateCounter(); }
function press(id,ms){ var el=$(id); if(!el)return; el.classList.add('flash','down'); if(pressTimers[id])clearTimeout(pressTimers[id]); pressTimers[id]=setTimeout(function(){el.classList.remove('flash','down');},ms||280); }
function showSeek(txt){ if(!seekHud)return; seekHud.textContent=txt; seekHud.classList.add('show'); if(seekTimer)clearTimeout(seekTimer); seekTimer=setTimeout(function(){seekHud.classList.remove('show');},900); }
function trackInfo(){ return playlist.length>1 ? ' ['+(playlistIndex+1)+'/'+playlist.length+']' : ''; }
function playLoaded(){
  if(!audio || !audio.src){ pickMp3(); return; }
  try{
    var p=audio.play();
    if(p&&p.catch)p.catch(function(e){log('MP3 PLAY: prohlizec nedovolil spusteni — '+e.message);});
    setMode('mp3play'); startCounter(); log('MP3 PLAY — '+(audioName||'HUDBA')+trackInfo());
  }catch(e){ log('MP3 PLAY chyba: '+e.message); }
}
function stopMp3(reason,reset){
  try{ if(audio) audio.pause(); }catch(e){}
  if(reset && audio && audio.src){ try{audio.currentTime=0;}catch(e){} }
  setMode(''); stopCounter(false); updateCounter(); log((reason||'MP3 STOP')+' — hudba zastavena');
}
function pauseMp3(){ stopMp3('MP3 PAUSE', false); }
function seekMp3(dir){
  if(!audio || !audio.src){ log('MP3 '+(dir<0?'REWIND':'F.FWD')+': nejdriv stiskni PLAY a vyber MP3 z mobilu.'); showSeek(dir<0?'MP3 <<':'MP3 >>'); setMode(dir<0?'mp3rew':'mp3ffwd'); setTimeout(function(){setMode('');},650); return; }
  if(!(isFinite(audio.duration)&&audio.duration>0)){ log('MP3 SEEK: cekam na delku souboru, zkus znovu za chvilku.'); return; }
  var old=audio.currentTime||0, step=10, target=Math.max(0,Math.min(audio.duration,old+dir*step));
  try{ audio.currentTime=target; }catch(e){ log('MP3 SEEK chyba: '+e.message); return; }
  setMode(dir<0?'mp3rew':'mp3ffwd'); showSeek(dir<0?'MP3 REW <<':'MP3 F.FWD >>'); updateCounter();
  var wasPlaying=!audio.paused;
  setTimeout(function(){ if(wasPlaying){setMode('mp3play'); startCounter();} else {setMode(''); stopCounter(false);} },720);
  log('MP3 '+(dir<0?'REWIND':'F.FWD')+': realny posun '+fmt(old)+' -> '+fmt(target));
}
function revokePlaylist(){
  try{
    var seen={};
    playlist.forEach(function(t){ if(t&&t.url&&!seen[t.url]){ seen[t.url]=1; URL.revokeObjectURL(t.url); } });
    if(audioUrl && !seen[audioUrl]) URL.revokeObjectURL(audioUrl);
  }catch(e){}
  playlist=[]; playlistIndex=-1; audioUrl=''; audioName='';
}
function createTrack(name,u8){
  if(!/\.mp3$/i.test(name||'')) { log('MP3 PLAYER: vyber prosim jen MP3 soubor, ne '+(name||'neznamy soubor')+'.'); return null; }
  var url=URL.createObjectURL(new Blob([u8],{type:'audio/mpeg'}));
  return {name:name||'hudba.mp3', url:url};
}
function setTrack(index,autoplay){
  if(index<0 || index>=playlist.length) return;
  try{ if(audio) audio.pause(); }catch(e){}
  playlistIndex=index;
  var tr=playlist[playlistIndex];
  audioName=tr.name||'hudba.mp3'; audioUrl=tr.url;
  if(!audio) audio=new Audio();
  audio.volume=introVolume;
  audio.pause(); audio.src=audioUrl; audio.currentTime=0;
  audio.onloadedmetadata=function(){updateCounter(); updatePlaylistUi(); log('MP3 nacteno — '+audioName+trackInfo()+' — delka '+fmt(audio.duration)+' — REW/F.FWD realne pretaci.');};
  audio.onseeked=updateCounter;
  audio.onended=function(){
    setMode(''); stopCounter(false);
    if(playlist.length>1 && playlistIndex<playlist.length-1){
      log('MP3 dalsi skladba — '+(playlistIndex+2)+'/'+playlist.length);
      setTrack(playlistIndex+1,true);
    } else {
      log('MP3 konec — '+audioName+trackInfo()+' — COUNTER '+(counter?counter.textContent:'000'));
    }
  };
  updateCounter(); updatePlaylistUi();
  if(autoplay) playLoaded();
}
function replacePlaylist(tracks){
  stopMp3('MP3 RESET PRED NOVOU HUDBOU', true);
  revokePlaylist();
  playlist=tracks||[];
  playlistIndex=-1;
  log('MP3 playlist nahran — '+playlist.length+' skladba/skladeb.');
  if(playlist.length>0) setTrack(0,true);
  else {updateCounter(); updatePlaylistUi();}
}
function appendPlaylist(tracks){
  tracks=tracks||[];
  if(!tracks.length){ updatePlaylistUi(); return; }
  var wasEmpty=playlist.length===0 || playlistIndex<0 || !audio || !audio.src;
  var startCount=playlist.length;
  tracks.forEach(function(t){ if(t) playlist.push(t); });
  log('MP3 playlist: pripojeno '+tracks.length+' skladba/skladeb. Celkem '+playlist.length+'.');
  if(playlistOverlay){ playlistOverlay.classList.add('open'); playlistOverlay.setAttribute('aria-hidden','false'); }
  if(wasEmpty){ setTrack(startCount,true); }
  else { updatePlaylistUi(); log('MP3 playlist: nove skladby cekaji za aktualni pisnickou.'); }
}
function ejectMp3(){
  try{ if(audio){ audio.pause(); audio.removeAttribute('src'); audio.load(); } }catch(e){}
  setMode(''); stopCounter(false); revokePlaylist(); updateCounter(); updatePlaylistUi();
  log('MP3 EJECT: hudba/playlist vymazany. Dalsi PLAY otevre Downloads a muzes vybrat novou skladbu nebo vice skladeb.');
}
function loadBytes(name,u8){
  try{
    var tr=createTrack(name,u8);
    if(!tr) return;
    if(incomingPlaylist){ pendingPlaylist.push(tr); log('MP3 playlist: nacitam '+tr.name+' ('+pendingPlaylist.length+')'); updatePlaylistUi(); if(playlistOverlay){ playlistOverlay.classList.add('open'); playlistOverlay.setAttribute('aria-hidden','false'); } return; }
    replacePlaylist([tr]);
  }catch(e){ log('MP3 chyba: '+e.message); }
}
window.AHLOCAL_MP3_PLAYLIST_BEGIN=function(){
  incomingPlaylist=true; pendingPlaylist=[];
  log('MP3 playlist: prijimam skladby z Android pickeru. Stare skladby zustanou, nove se pripoji za konec.');
  if(playlistOverlay){ playlistOverlay.classList.add('open'); playlistOverlay.setAttribute('aria-hidden','false'); }
  updatePlaylistUi();
};
window.AHLOCAL_MP3_PLAYLIST_END=function(){
  try{
    var got=pendingPlaylist.slice(0);
    incomingPlaylist=false;
    pendingPlaylist=[];
    if(got.length>0) appendPlaylist(got);
    else { log('MP3 playlist: nic nevybrano.'); updatePlaylistUi(); }
  }catch(e){ incomingPlaylist=false; pendingPlaylist=[]; log('MP3 playlist chyba — '+e.message); updatePlaylistUi(); }
};
window.AHLOCAL_AUDIO_BEGIN=function(name){ window.__introMp3Name=name||'hudba.mp3'; window.__introMp3Parts=[]; log('MP3: prijimam z Android pickeru — '+window.__introMp3Name); };
window.AHLOCAL_AUDIO_PART=function(b64){ (window.__introMp3Parts||(window.__introMp3Parts=[])).push(b64); };
window.AHLOCAL_AUDIO_END=function(){
  try{ var bin=atob((window.__introMp3Parts||[]).join('')), u8=new Uint8Array(bin.length); for(var i=0;i<bin.length;i++)u8[i]=bin.charCodeAt(i)&255; loadBytes(window.__introMp3Name||'hudba.mp3',u8); }
  catch(e){ log('MP3 prijem selhal — '+e.message); }
  window.__introMp3Parts=[];
};
function pickMp3(){
  log('MP3 PRIDAT: oteviram Downloads. Vybrana MP3 se PRIDA do playlistu, EJECT maze playlist.');
  try{ if(window.AHPICK && window.AHPICK.pickMp3){ window.AHPICK.pickMp3(); return; } }catch(e){}
  try{ if(window.AHPICK && window.AHPICK.pickAudio){ window.AHPICK.pickAudio(); return; } }catch(e){}
  var inp=document.createElement('input'); inp.type='file'; inp.accept='.mp3,audio/mpeg,audio/mp3'; inp.multiple=true; inp.style.display='none'; document.body.appendChild(inp);
  inp.onchange=function(){
    var files=Array.prototype.slice.call((inp.files||[])); inp.remove();
    if(!files.length){log('MP3: soubor nevybran.'); return;}
    incomingPlaylist=true; pendingPlaylist=[]; updatePlaylistUi(); if(playlistOverlay){ playlistOverlay.classList.add('open'); playlistOverlay.setAttribute('aria-hidden','false'); }
    function readNext(i){
      if(i>=files.length){ window.AHLOCAL_MP3_PLAYLIST_END(); return; }
      var f=files[i]; var rd=new FileReader();
      rd.onload=function(){ loadBytes(f.name,new Uint8Array(rd.result)); readNext(i+1); };
      rd.onerror=function(){ log('MP3: cteni souboru selhalo — '+f.name); readNext(i+1); };
      rd.readAsArrayBuffer(f);
    }
    readNext(0);
  };
  inp.click();
}
function bind(id,fn){
  var el=$(id); if(!el)return; var moved=false,last=0;
  el.addEventListener('pointerdown',function(){moved=false;},{passive:true});
  el.addEventListener('pointermove',function(){moved=true;},{passive:true});
  el.addEventListener('pointerup',function(e){
    e.preventDefault(); e.stopPropagation();
    if(moved) return; last=Date.now(); fn();
  },{passive:false});
  el.addEventListener('click',function(e){
    e.preventDefault(); e.stopPropagation();
    if(Date.now()-last<450) return; last=Date.now(); fn();
  },true);
}
bind('introTapePlay',function(){press('introTapePlay',520); playLoaded(); });
bind('introTapeStop',function(){press('introTapeStop',620); stopMp3('MP3 STOP', true); log('MP3 STOP: zastaveno a vraceno na zacatek. Zmackni PLAY a hudba se znovu rozjede.');});
bind('introTapePause',function(){press('introTapePause',620); pauseMp3(); log('MP3 PAUSE: zastaveno na aktualni pozici. Zmackni PLAY a pokracuje.');});
bind('introTapeEject',function(){press('introTapeEject',760); ejectMp3();});
bind('introTapeRew',function(){press('introTapeRew',650); seekMp3(-1);});
bind('introTapeFwd',function(){press('introTapeFwd',650); seekMp3(1);});
bind('introMp3DownloadBtn',function(){press('introMp3DownloadBtn',260); pickMp3();});
bind('introMp3ListBtn',function(){press('introMp3ListBtn',260); showPlaylist();});
var plDown=$('introPlaylistDownload'), plEject=$('introPlaylistEject'), plClose=$('introPlaylistClose');
if(plDown) plDown.addEventListener('click',function(){ pickMp3(); },true);
if(plEject) plEject.addEventListener('click',function(){ ejectMp3(); updatePlaylistUi(); },true);
if(plClose && playlistOverlay) plClose.addEventListener('click',function(){ playlistOverlay.classList.remove('open'); playlistOverlay.setAttribute('aria-hidden','true'); },true);
if(volHot){
  var volDragging=false;
  volHot.addEventListener('pointerdown',function(e){e.preventDefault(); e.stopPropagation(); volDragging=true; volHot.classList.add('flash'); setVolumeFromEvent(e); try{volHot.setPointerCapture(e.pointerId);}catch(_){}} ,{passive:false});
  volHot.addEventListener('pointermove',function(e){ if(!volDragging)return; e.preventDefault(); e.stopPropagation(); setVolumeFromEvent(e); },{passive:false});
  volHot.addEventListener('pointerup',function(e){ e.preventDefault(); e.stopPropagation(); volDragging=false; volHot.classList.remove('flash'); setVolumeFromEvent(e); try{volHot.releasePointerCapture(e.pointerId);}catch(_){}} ,{passive:false});
  volHot.addEventListener('click',function(e){ e.preventDefault(); e.stopPropagation(); setVolumeFromEvent(e); },true);
}
applyVolume(1.0,false); updatePlaylistUi();
var introHelpBtn=$('introHelpBtn'), introHelpOverlay=$('introHelpOverlay'), introHelpClose=$('introHelpClose');
if(introHelpBtn && introHelpOverlay){ bind('introHelpBtn', function(){ press('introHelpBtn',220); introHelpOverlay.classList.add('open'); introHelpOverlay.setAttribute('aria-hidden','false'); }); }
if(introHelpClose && introHelpOverlay){ introHelpClose.addEventListener('click', function(){ introHelpOverlay.classList.remove('open'); introHelpOverlay.setAttribute('aria-hidden','true'); }); }
log('BUILD2AQ: MP3 PRIDAT nyni nacita/pripojuje skladby do playlistu; EJECT maze playlist.');
})();
