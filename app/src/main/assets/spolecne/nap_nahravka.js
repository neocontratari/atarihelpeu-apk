/* B312: NAHRAVKA - spolecne pro MP3 prehravac (RECORD = mixtape) a DZUBOX (RECORD).
 *
 *  NapNahravka.typ()            nejlepsi format, ktery MediaRecorder v tomhle WebView umi:
 *                               M4A s AAC (prehraje vsechno), M4A s Opusem, WebM s Opusem
 *  NapNahravka.pripona(typ)     .m4a / .webm / .ogg
 *  NapNahravka.opravDelku(blob, ms, hotovo)
 *                               WebM z MediaRecorderu nema v hlavicce delku - prehravace pak
 *                               neukazou delku a spatne posouvaji. Doplni ji (element Duration
 *                               v Segment Info). Jiny format (M4A) se vrati beze zmeny.
 *  NapNahravka.uloz(blob, typ, volby)
 *                               do Javy po kouscich (AHSAVE.souborZacni / Pridej / Dokonci)
 *                               do Download/AtariHelp/<slozka>; v prohlizeci na PC (bez AHSAVE)
 *                               normalni stazeni souboru.
 *                               volby: {slozka, jmeno, kus (bajtu, jen test), prubeh(procent),
 *                                       hotovo(ok, text, stazeno)}
 *                               text pri chybe: 'uloziste' | 'zapis' | 'cteni' | odpoved Javy
 */
(function () {
  'use strict';

  var TYPY = ['audio/mp4;codecs=mp4a.40.2', 'audio/mp4', 'audio/webm;codecs=opus', 'audio/webm'];

  function typ() {
    try {
      if (!window.MediaRecorder || typeof MediaRecorder.isTypeSupported !== 'function') return '';
      for (var i = 0; i < TYPY.length; i++) if (MediaRecorder.isTypeSupported(TYPY[i])) return TYPY[i];
    } catch (e) {}
    return '';
  }

  function pripona(t) {
    t = String(t || '').toLowerCase();
    if (/mp4|aac|m4a/.test(t)) return '.m4a';
    if (/ogg/.test(t)) return '.ogg';
    return '.webm';
  }

  function datum() {
    var d = new Date(), p = function (n) { return (n < 10 ? '0' : '') + n; };
    return d.getFullYear() + p(d.getMonth() + 1) + p(d.getDate()) + '_' + p(d.getHours()) + p(d.getMinutes()) + p(d.getSeconds());
  }

  // ---- EBML (WebM): cteni a zapis cisel promenne delky ----
  function ctiId(b, p) {
    if (p >= b.length) return null;
    var x = b[p], d = (x & 0x80) ? 1 : (x & 0x40) ? 2 : (x & 0x20) ? 3 : (x & 0x10) ? 4 : 0;
    if (!d || p + d > b.length) return null;
    var id = 0;
    for (var i = 0; i < d; i++) id = id * 256 + b[p + i];
    return { d: d, id: id };
  }

  function ctiVelikost(b, p) {
    if (p >= b.length) return null;
    var x = b[p], d = 1, m = 0x80;
    while (d <= 8 && !(x & m)) { d++; m >>= 1; }
    if (d > 8 || p + d > b.length) return null;
    var v = x & (m - 1), jednicky = (v === m - 1);
    for (var i = 1; i < d; i++) {
      v = v * 256 + b[p + i];
      if (b[p + i] !== 255) jednicky = false;
    }
    return { d: d, v: v, neznama: jednicky };   // vsechny bity 1 = neznama velikost (zive nahravani)
  }

  function zapisVelikost(v, d) {
    if (v < 0 || v >= Math.pow(2, 7 * d) - 1) return null;   // 2^(7d)-1 = neznama velikost
    var o = new Uint8Array(d), x = v;
    for (var i = d - 1; i >= 0; i--) { o[i] = x % 256; x = Math.floor(x / 256); }
    o[0] |= (0x80 >> (d - 1));
    return o;
  }

  function velikostPro(v, chci) {
    var o = zapisVelikost(v, chci);
    for (var d = 1; !o && d <= 8; d++) o = zapisVelikost(v, d);
    return o;
  }

  function cislo(b, p, n) {
    var v = 0;
    for (var i = 0; i < n; i++) v = v * 256 + b[p + i];
    return v;
  }

  function spoj(casti) {
    var n = 0, i, p = 0;
    for (i = 0; i < casti.length; i++) n += casti[i].length;
    var o = new Uint8Array(n);
    for (i = 0; i < casti.length; i++) { o.set(casti[i], p); p += casti[i].length; }
    return o;
  }

  /* Zacatek WebM souboru (b) -> novy zacatek s delkou ms v Segment Info, nebo null, kdyz to
   * neni WebM ze ziveho nahravani (neznama velikost Segmentu, zadny SeekHead pred Info) -
   * pak se soubor necha, jak je (posunute pozice by ho rozbily). */
  function webmSDelkou(b, ms) {
    var id = ctiId(b, 0);
    if (!id || id.id !== 0x1A45DFA3) return null;                 // EBML hlavicka
    var s = ctiVelikost(b, id.d);
    if (!s || s.neznama) return null;
    var p = id.d + s.d + s.v;
    id = ctiId(b, p);
    if (!id || id.id !== 0x18538067) return null;                 // Segment
    var seg = ctiVelikost(b, p + id.d);
    if (!seg || !seg.neznama) return null;                        // jen zive nahravani
    var segData = p + id.d + seg.d;
    p = segData;
    while (p < b.length) {
      id = ctiId(b, p);
      if (!id) return null;
      s = ctiVelikost(b, p + id.d);
      if (!s || s.neznama) return null;
      var data = p + id.d + s.d, konec = data + s.v;
      if (konec > b.length) return null;
      // SeekHead / Cluster / Cues pred Info: pozice za Info by se posunuly - nesahat
      if (id.id === 0x114D9B74 || id.id === 0x1F43B675 || id.id === 0x1C53BB6B) return null;
      if (id.id === 0x1549A966) {                                 // Info
        var meritko = 1000000, q = data, deti = [];
        while (q < konec) {
          var cid = ctiId(b, q);
          if (!cid) return null;
          var cs = ctiVelikost(b, q + cid.d);
          if (!cs || cs.neznama) return null;
          var cd = q + cid.d + cs.d, ck = cd + cs.v;
          if (ck > konec) return null;
          if (cid.id === 0x2AD7B1 && cs.v >= 1 && cs.v <= 7) meritko = cislo(b, cd, cs.v) || 1000000;  // TimecodeScale (ns)
          if (cid.id !== 0x4489) deti.push(b.subarray(q, ck));      // stara Duration pryc
          q = ck;
        }
        var el = new Uint8Array(11);                              // Duration: 44 89, 8 bajtu float64
        el[0] = 0x44; el[1] = 0x89; el[2] = 0x88;
        new DataView(el.buffer).setFloat64(3, ms * 1e6 / meritko, false);
        deti.push(el);
        var obsah = spoj(deti), vel = velikostPro(obsah.length, s.d);
        if (!vel) return null;
        return spoj([b.subarray(0, p), b.subarray(p, p + id.d), vel, obsah, b.subarray(konec)]);
      }
      p = konec;
    }
    return null;
  }

  var HLAVA = 65536;   // Info je v prvnich stovkach bajtu - cte se jen zacatek souboru

  function opravDelku(blob, ms, hotovo) {
    var t = String((blob && blob.type) || '').toLowerCase();
    if (!blob || !(ms > 0) || t.indexOf('webm') < 0 || typeof FileReader === 'undefined') { hotovo(blob, false); return; }
    var fr = new FileReader();
    fr.onload = function () {
      var novy = null;
      try { novy = webmSDelkou(new Uint8Array(fr.result), ms); } catch (e) { novy = null; }
      if (!novy) { hotovo(blob, false); return; }
      hotovo(new Blob([novy, blob.slice(Math.min(HLAVA, blob.size))], { type: blob.type }), true);
    };
    fr.onerror = function () { hotovo(blob, false); };
    fr.readAsArrayBuffer(blob.slice(0, HLAVA));
  }

  function uloz(blob, t, v) {
    v = v || {};
    var hotovo = typeof v.hotovo === 'function' ? v.hotovo : function () {};
    var prubeh = typeof v.prubeh === 'function' ? v.prubeh : function () {};
    t = String(t || (blob && blob.type) || 'application/octet-stream').split(';')[0];
    var jm = String(v.jmeno || ('NP_VISION_' + datum() + pripona(t)));
    if (!(window.AHSAVE && typeof window.AHSAVE.souborZacni === 'function')) {
      // prohlizec na PC: normalni stazeni
      try {
        var a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = jm;
        document.body.appendChild(a);
        a.click();
        setTimeout(function () { try { URL.revokeObjectURL(a.href); a.remove(); } catch (e) {} }, 4000);
        hotovo(true, jm, true);
      } catch (e) { hotovo(false, 'uloziste'); }
      return;
    }
    var id = 0;
    try { id = window.AHSAVE.souborZacni(String(v.slozka || ''), jm, t); } catch (e) { id = 0; }
    if (!id) { hotovo(false, 'uloziste'); return; }
    // kousek 384 kB (base64 ~512 kB na jedno volani Javy)
    var KUS = v.kus > 0 ? v.kus : 384 * 1024, od = 0;
    prubeh(0);
    var dalsi = function () {
      if (od >= blob.size) {
        var r = '';
        try { r = String(window.AHSAVE.souborDokonci(id)); } catch (e) { r = 'CHYBA ' + e; }
        if (r.indexOf('OK:') === 0) hotovo(true, r.slice(3));
        else hotovo(false, r);
        return;
      }
      var fr = new FileReader();
      fr.onload = function () {
        var s = String(fr.result || ''), c = s.indexOf(','), ok = false;
        try { ok = window.AHSAVE.souborPridej(id, c >= 0 ? s.slice(c + 1) : s); } catch (e) { ok = false; }
        if (!ok) { try { window.AHSAVE.souborZrus(id); } catch (e) {} hotovo(false, 'zapis'); return; }
        od += KUS;
        prubeh(Math.min(99, Math.round(od * 100 / blob.size)));
        setTimeout(dalsi, 0);
      };
      fr.onerror = function () { try { window.AHSAVE.souborZrus(id); } catch (e) {} hotovo(false, 'cteni'); };
      fr.readAsDataURL(blob.slice(od, od + KUS));
    };
    dalsi();
  }

  window.NapNahravka = {
    typ: typ,
    pripona: pripona,
    datum: datum,
    opravDelku: opravDelku,
    webmSDelkou: webmSDelkou,
    uloz: uloz
  };
})();
