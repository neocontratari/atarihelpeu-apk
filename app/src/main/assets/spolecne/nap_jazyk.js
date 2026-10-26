/* nap_jazyk.js
 * ====================================================================
 *  B314: JAZYK CZ / EN PRO STRANKY APPKY (a stranku TV / PC)
 *
 *  Rene: "slo by dat vyber jazyka - volbu Cze a Ang? ... po volbe na ang
 *  by byla apka v anglictine? a naopak?"
 *
 *  Stranky zustavaji psane cesky (jejich kod se nemeni). Kdyz je zvolena
 *  anglictina, tenhle skript preklada to, co clovek vidi:
 *    - texty v HTML a texty, ktere stranka sama pozdeji vypise (hlidac
 *      MutationObserver) - podle slovniku: cela veta cesky -> anglicky,
 *    - vety s cislem nebo jmenem (vzory: regularni vyraz -> anglicky),
 *    - atributy placeholder / title / aria-label / alt, tlacitka <input>,
 *    - obrazky s namalovanym ceskym textem: <img data-en-src="..._en.png">,
 *    - dlouhe texty ve dvou verzich: <p data-cs-only>...</p><p data-en-only>...</p>,
 *    - nazev stranky, alert / confirm / prompt, texty kreslene do canvasu.
 *  V cestine nedela nic (zadny hlidac, nic se nemeni).
 *
 *  Jazyk: Java (AHJAZYK.kod() - volba v OPTIONS, poprve podle jazyka
 *  telefonu), na PC ?lang=en v adrese, jinak jazyk prohlizece.
 *  Slovniky: assets/jazyk/en_*.js  (NapJazyk.slovnik({...}), NapJazyk.vzory([...]))
 *  Neprekladat: prvek (a vse v nem) s atributem data-nepreklad.
 *  Psano v ES5 - bezi i v prohlizecich televizi.
 * ====================================================================
 */
(function (W, D) {
  'use strict';
  if (W.NapJazyk) return;

  var PISMENA = 'A-Za-z0-9\u00C0-\u024F';
  var RE_PREDPONA = new RegExp('^([^' + PISMENA + ']+)([\\s\\S]*[' + PISMENA + '][\\s\\S]*)$');
  var RE_PRIPONA = new RegExp('^([\\s\\S]*?[' + PISMENA + ')\\]])([\\s:\u203A\u00BB\u2026\u25B8\u25B6\u25C0\u2039\u00AB*\u2022\u00B7\\-\u2013\u2014]+)$');
  var PRESKOC = { SCRIPT: 1, STYLE: 1, NOSCRIPT: 1, TEXTAREA: 1, TEMPLATE: 1 };
  var ATRIBUTY = ['placeholder', 'title', 'aria-label', 'alt'];

  function zjisti() {
    try {
      if (W.AHJAZYK && typeof W.AHJAZYK.kod === 'function') {
        var k = String(W.AHJAZYK.kod());
        if (k === 'en' || k === 'cs') return k;
      }
    } catch (e) {}
    try {
      var q = /[?&#]lang=(cs|en)\b/.exec(String(W.location.search || '') + String(W.location.hash || ''));
      if (q) return q[1];
    } catch (e) {}
    if (W.NAP_JAZYK === 'en' || W.NAP_JAZYK === 'cs') return W.NAP_JAZYK;   // stranka TV z Javy
    try {
      var l = W.localStorage && W.localStorage.getItem('napJazyk');
      if (l === 'en' || l === 'cs') return l;
    } catch (e) {}
    var n = String((W.navigator && (W.navigator.language || W.navigator.userLanguage)) || 'cs').toLowerCase();
    return (n.indexOf('cs') === 0 || n.indexOf('sk') === 0) ? 'cs' : 'en';
  }

  var kod = zjisti();
  var slov = {};            // presny text (orezany) -> anglicky
  var slovMez = {};         // text se slitymi mezerami -> anglicky
  var vzory = [];           // [RegExp, retezec nebo funkce]
  var posluchaci = [];
  var orig = typeof W.WeakMap === 'function' ? new W.WeakMap() : null;   // uzel -> {cz, en}
  var hlidac = null;
  var puvAlert = W.alert, puvConfirm = W.confirm, puvPrompt = W.prompt;

  function sliMezery(s) { return s.replace(/\s+/g, ' ').replace(/^ | $/g, ''); }

  function hledej(s) {
    if (Object.prototype.hasOwnProperty.call(slov, s)) return slov[s];
    var m = sliMezery(s);
    if (Object.prototype.hasOwnProperty.call(slovMez, m)) return slovMez[m];
    for (var i = 0; i < vzory.length; i++) {
      var v = vzory[i];
      v[0].lastIndex = 0;
      if (v[0].test(s)) { v[0].lastIndex = 0; return s.replace(v[0], v[1]); }
      if (m !== s) {
        v[0].lastIndex = 0;
        if (v[0].test(m)) { v[0].lastIndex = 0; return m.replace(v[0], v[1]); }
      }
    }
    return null;
  }

  // cesky text -> anglicky (null = neni ve slovniku). Bez okrajovych mezer.
  function prelozJadro(s) {
    if (!s) return null;
    var r = hledej(s);
    if (r !== null) return r;
    var m = RE_PREDPONA.exec(s);
    if (m) {
      r = hledej(m[2]);
      if (r !== null) return m[1] + r;
    }
    m = RE_PRIPONA.exec(s);
    if (m) {
      r = hledej(m[1]);
      if (r !== null) return r + m[2];
      var m2 = RE_PREDPONA.exec(m[1]);
      if (m2) {
        r = hledej(m2[2]);
        if (r !== null) return m2[1] + r + m[2];
      }
    }
    return null;
  }

  // cely retezec (i s mezerami okolo) -> anglicky, nebo beze zmeny
  function tr(s) {
    if (kod !== 'en' || s === null || s === undefined) return s;
    s = String(s);
    var a = /^\s*/.exec(s)[0], b = /\s*$/.exec(s)[0];
    if (a.length === s.length) return s;
    var jadro = s.substring(a.length, s.length - b.length);
    var r = prelozJadro(jadro);
    return r === null ? s : a + r + b;
  }

  function preskocit(el) {
    for (var e = el; e && e.nodeType === 1; e = e.parentNode) {
      if (PRESKOC[e.nodeName]) return true;
      if (e.hasAttribute && (e.hasAttribute('data-nepreklad') || e.hasAttribute('data-cs-only') || e.hasAttribute('data-en-only'))) return true;
      if (e.isContentEditable) return true;
    }
    return false;
  }

  function zapamatuj(uzel, klic, cz, en) {
    if (orig) {
      var z = orig.get(uzel);
      if (!z) { z = {}; orig.set(uzel, z); }
      z[klic] = { cz: cz, en: en };
    } else {
      if (!uzel.__napJazyk) uzel.__napJazyk = {};
      uzel.__napJazyk[klic] = { cz: cz, en: en };
    }
  }

  function zaznam(uzel) {
    return orig ? orig.get(uzel) : uzel.__napJazyk;
  }

  function prelozUzelText(t) {
    var s = t.nodeValue;
    if (!s || !/[A-Za-z\u00C0-\u024F]/.test(s)) return;
    var r = tr(s);
    if (r !== s) { zapamatuj(t, '#', s, r); t.nodeValue = r; }
  }

  function prelozAtributy(el) {
    for (var i = 0; i < ATRIBUTY.length; i++) {
      var n = ATRIBUTY[i];
      if (!el.hasAttribute(n)) continue;
      var s = el.getAttribute(n), r = tr(s);
      if (r !== s) { zapamatuj(el, n, s, r); el.setAttribute(n, r); }
    }
    // obrazek s namalovanym textem: <img src="x.png" data-en-src="x_en.png">
    if (el.hasAttribute('data-en-src')) {
      var es = el.getAttribute('data-en-src'), ps = el.getAttribute('src');
      if (es && ps !== es) { zapamatuj(el, 'src', ps, es); el.setAttribute('src', es); }
    }
    if (el.nodeName === 'INPUT') {
      var typ = String(el.type || '').toLowerCase();
      if (typ === 'button' || typ === 'submit' || typ === 'reset') {
        var v = el.value, rv = tr(v);
        if (rv !== v) { zapamatuj(el, 'value', v, rv); el.value = rv; }
      }
    }
  }

  function preloz(koren) {
    if (kod !== 'en' || !koren) return;
    if (koren.nodeType === 3) {
      if (!preskocit(koren.parentNode)) prelozUzelText(koren);
      return;
    }
    if (koren.nodeType !== 1 && koren.nodeType !== 9 && koren.nodeType !== 11) return;
    if (koren.nodeType === 1 && preskocit(koren)) return;
    if (koren.nodeType === 1) prelozAtributy(koren);
    var w = D.createTreeWalker(koren, 1 | 4, {
      acceptNode: function (n) {
        if (n.nodeType === 1) {
          if (PRESKOC[n.nodeName] || (n.hasAttribute && (n.hasAttribute('data-nepreklad') ||
              n.hasAttribute('data-cs-only') || n.hasAttribute('data-en-only')))) return 2;   // FILTER_REJECT
          return 1;
        }
        return 1;
      }
    }, false);
    var n;
    while ((n = w.nextNode())) {
      if (n.nodeType === 3) prelozUzelText(n);
      else prelozAtributy(n);
    }
  }

  function prelozTitulek() {
    try {
      var s = D.title, r = tr(s);
      if (r !== s) { W.__napTitulCz = s; D.title = r; }
    } catch (e) {}
  }

  function poZmenach(zaznamy) {
    for (var i = 0; i < zaznamy.length; i++) {
      var z = zaznamy[i];
      if (z.type === 'childList') {
        for (var j = 0; j < z.addedNodes.length; j++) {
          var u = z.addedNodes[j];
          if (u.nodeType === 3) { if (!preskocit(u.parentNode)) prelozUzelText(u); }
          else if (u.nodeType === 1) preloz(u);
        }
        if (z.target && z.target.nodeName === 'TITLE') prelozTitulek();
      } else if (z.type === 'characterData') {
        var t = z.target;
        if (t.parentNode && t.parentNode.nodeName === 'TITLE') prelozTitulek();
        else if (!preskocit(t.parentNode)) prelozUzelText(t);
      } else if (z.type === 'attributes') {
        if (!preskocit(z.target)) prelozAtributy(z.target);
      }
    }
  }

  function hlidej() {
    if (hlidac || typeof W.MutationObserver !== 'function') return;
    hlidac = new W.MutationObserver(poZmenach);
    hlidac.observe(D.documentElement, {
      childList: true, subtree: true, characterData: true,
      attributes: true, attributeFilter: ATRIBUTY.concat(['value'])
    });
  }

  function nehlidej() {
    if (hlidac) { try { hlidac.disconnect(); } catch (e) {} hlidac = null; }
  }

  // ---- alert / confirm / prompt a texty v canvasu ----
  function hacky() {
    if (W.__napJazykHacky) return;
    W.__napJazykHacky = true;
    try {
      if (typeof puvAlert === 'function') W.alert = function (m) { return puvAlert.call(W, tr(m)); };
      if (typeof puvConfirm === 'function') W.confirm = function (m) { return puvConfirm.call(W, tr(m)); };
      if (typeof puvPrompt === 'function') W.prompt = function (m, d) { return puvPrompt.call(W, tr(m), d); };
    } catch (e) {}
    try {
      var P = W.CanvasRenderingContext2D && W.CanvasRenderingContext2D.prototype;
      if (P && !P.__napJazyk) {
        P.__napJazyk = true;
        var f = P.fillText, s = P.strokeText, mt = P.measureText;
        // canvas kresli treba kazdy snimek - jen slovnik (zadne vzory), at to nic nestoji
        var trC = function (x) {
          if (kod !== 'en' || typeof x !== 'string') return x;
          if (Object.prototype.hasOwnProperty.call(slov, x)) return slov[x];
          var k = x.replace(/^\s+|\s+$/g, '');
          return (k !== x && Object.prototype.hasOwnProperty.call(slov, k)) ? x.replace(k, slov[k]) : x;
        };
        P.fillText = function (t, x, y, m) {
          return arguments.length > 3 ? f.call(this, trC(t), x, y, m) : f.call(this, trC(t), x, y);
        };
        P.strokeText = function (t, x, y, m) {
          return arguments.length > 3 ? s.call(this, trC(t), x, y, m) : s.call(this, trC(t), x, y);
        };
        P.measureText = function (t) { return mt.call(this, trC(t)); };
      }
    } catch (e) {}
  }

  function vratHacky() {
    if (!W.__napJazykHacky) return;
    W.__napJazykHacky = false;
    try { W.alert = puvAlert; W.confirm = puvConfirm; W.prompt = puvPrompt; } catch (e) {}
    // canvas: obal se nevraci, ale v cestine nic neprelozi (kod !== 'en')
  }

  // ---- zpet do cestiny (zmena jazyka bez nacteni stranky - OPTIONS) ----
  function vratCestinu() {
    var w = D.createTreeWalker(D.documentElement, 1 | 4, null, false), n;
    while ((n = w.nextNode())) {
      var z = zaznam(n);
      if (!z) continue;
      for (var k in z) {
        if (!Object.prototype.hasOwnProperty.call(z, k)) continue;
        var p = z[k];
        if (k === '#') { if (n.nodeValue === p.en) n.nodeValue = p.cz; }
        else if (k === 'value') { if (n.value === p.en) n.value = p.cz; }
        else if (n.getAttribute(k) === p.en) n.setAttribute(k, p.cz);
      }
      if (orig) orig['delete'](n); else n.__napJazyk = null;
    }
    if (W.__napTitulCz && D.title === tr(W.__napTitulCz)) D.title = W.__napTitulCz;
  }

  // ---- skryt stranku, dokud neni prelozena (bez probliknuti cestiny) ----
  var cekaci = null;
  function skryj() {
    try {
      cekaci = D.createElement('style');
      cekaci.textContent = 'html.nap-en-cekej body{visibility:hidden!important}';
      (D.head || D.documentElement).appendChild(cekaci);
      D.documentElement.className += ' nap-en-cekej';
      W.setTimeout(ukaz, 1500);    // pojistka
    } catch (e) {}
  }
  function ukaz() {
    try {
      var h = D.documentElement;
      h.className = h.className.replace(/(^|\s)nap-en-cekej(\s|$)/g, ' ').replace(/^\s+|\s+$/g, '');
    } catch (e) {}
  }

  // ---- dlouhe texty ve dvou verzich: <div data-cs-only>cesky</div><div data-en-only>English</div> ----
  function tridaJazyka() {
    try {
      var h = D.documentElement;
      h.className = h.className.replace(/(^|\s)nap-jazyk-(cs|en)(\s|$)/g, ' ').replace(/^\s+|\s+$/g, '') + ' nap-jazyk-' + kod;
    } catch (e) {}
  }
  function stylVerzi() {
    try {
      var st = D.createElement('style');
      st.textContent = 'html:not(.nap-jazyk-en) [data-en-only]{display:none!important}' +
                       'html.nap-jazyk-en [data-cs-only]{display:none!important}';
      (D.head || D.documentElement).appendChild(st);
    } catch (e) {}
  }

  function start() {
    stylVerzi();
    tridaJazyka();
    if (kod !== 'en') return;
    try { D.documentElement.setAttribute('lang', 'en'); } catch (e) {}
    hacky();
    hlidej();
    if (D.readyState === 'loading') {
      skryj();
      D.addEventListener('DOMContentLoaded', function () {
        preloz(D.documentElement); prelozTitulek(); ukaz();
      });
    } else {
      preloz(D.documentElement); prelozTitulek();
    }
  }

  var api = {
    kod: function () { return kod; },
    en: function () { return kod === 'en'; },
    // dvojice primo v kodu stranky: NapJazyk.t('cesky', 'English')
    t: function (cs, en) { return kod === 'en' ? (en === undefined || en === null ? tr(cs) : en) : cs; },
    tr: tr,
    slovnik: function (o) {
      for (var k in o) {
        if (!Object.prototype.hasOwnProperty.call(o, k)) continue;
        var kk = String(k).replace(/^\s+|\s+$/g, '');
        slov[kk] = o[k];
        slovMez[sliMezery(kk)] = o[k];
      }
      if (kod === 'en' && D.body) preloz(D.body);
    },
    vzory: function (a) {
      for (var i = 0; i < a.length; i++) if (a[i] && a[i][0] instanceof RegExp) vzory.push(a[i]);
      if (kod === 'en' && D.body) preloz(D.body);
    },
    preloz: function (koren) { preloz(koren || D.documentElement); },
    // zmena jazyka bez nacteni stranky (OPTIONS): 'cs' / 'en'
    nastav: function (k) {
      if (k !== 'cs' && k !== 'en') return kod;
      if (k === kod) return kod;
      kod = k;
      tridaJazyka();
      try { D.documentElement.setAttribute('lang', k); } catch (e) {}
      if (k === 'en') { hacky(); hlidej(); preloz(D.documentElement); prelozTitulek(); }
      else { nehlidej(); vratHacky(); vratCestinu(); }
      for (var i = 0; i < posluchaci.length; i++) { try { posluchaci[i](k); } catch (e) {} }
      return kod;
    },
    priZmene: function (fn) { if (typeof fn === 'function') posluchaci.push(fn); },
    // pro testy: kolik je ve slovniku
    velikost: function () { var n = 0; for (var k in slov) if (Object.prototype.hasOwnProperty.call(slov, k)) n++; return { slova: n, vzory: vzory.length }; }
  };
  W.NapJazyk = api;
  start();
})(window, document);
