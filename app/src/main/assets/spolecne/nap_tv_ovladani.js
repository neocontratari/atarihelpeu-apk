/* B334: STRANKY APPKY DALKOVYM OVLADACEM TV A GAMEPADEM (Rene: "aby se to dalo ovladat ovladacem
 * od TV nebo gamepadem" - appka primo na Android TV, kde neni dotykovy displej).
 *
 * Sipky (krizek dalkoveho ovladace / gamepadu, klavesnice) presouvaji zlaty ramecek mezi tim, na co
 * se da na strance klepnout; OK / Enter (gamepad A) to stiskne - jako prst. Kdyz uz ve smeru nic
 * neni, stranka se posune (dlouha napoveda). ZPET resi appka (Android).
 *  - co je "na klepnuti": odkazy, tlacitka, pole formulare, prvky s onclick / role / tabindex, prvky
 *    s kurzorem ruky a HLAVNE prvky, kterym stranka dala posluchace click (tenhle skript je na strance
 *    PRVNI a addEventListener si poznamena),
 *  - jen v nejvyssi vrstve (otevreny panel OPTIONS prekryva stranku - vybira se jen v nem),
 *  - stranka, ktera si sipky resi sama (karusel karet, volba jazyka), je resi dal: skript posloucha
 *    az po ni a udalost, kterou stranka uz zpracovala (preventDefault), nechava byt,
 *  - na telefonu se ramecek ukaze az po prvni sipce (dotyk nic nemeni); na TV ho zapne appka
 *    hned po nacteni stranky (NapTvOvladani.zapni()).
 * Psano v ES5 (starsi WebView v televizich). */
(function (W, D) {
  'use strict';
  if (W.NapTvOvladani) return;

  // ---- 1) kdo dostal posluchace klepnuti (musi byt pred skripty stranky) ----
  var UDALOSTI = { click: 1, mousedown: 1, mouseup: 1, pointerdown: 1, pointerup: 1, touchstart: 1, touchend: 1 };
  var klikaci = (typeof W.WeakSet === 'function') ? new W.WeakSet() : null;
  try {
    var puvodni = W.EventTarget && W.EventTarget.prototype.addEventListener;
    if (puvodni && klikaci) {
      W.EventTarget.prototype.addEventListener = function (typ, fn, volby) {
        try { if (UDALOSTI[typ] && this && this.nodeType === 1) klikaci.add(this); } catch (e) {}
        return puvodni.apply(this, arguments);
      };
    }
  } catch (e) {}

  var aktivni = false, fokus = null, stylPridan = false;
  var TRIDA = 'nap-tv-fokus';

  function styl() {
    if (stylPridan || !D.head) return;
    stylPridan = true;
    var s = D.createElement('style');
    s.id = 'napTvStyl';
    s.textContent = '.' + TRIDA + '{outline:3px solid #ffd479 !important;outline-offset:2px !important;' +
      'box-shadow:0 0 0 5px rgba(255,212,121,.35),0 0 18px rgba(255,212,121,.6) !important}';
    D.head.appendChild(s);
  }

  function interaktivniTag(el) {
    var t = el.tagName;
    if (t === 'A') return el.hasAttribute('href');
    if (t === 'BUTTON' || t === 'SELECT' || t === 'TEXTAREA' || t === 'SUMMARY') return true;
    if (t === 'INPUT') return (el.type || '').toLowerCase() !== 'hidden';
    return false;
  }

  function klikatelny(el) {
    if (!el || el.nodeType !== 1 || el === D.body || el === D.documentElement) return false;
    if (el.disabled || el.getAttribute('aria-disabled') === 'true') return false;
    if (interaktivniTag(el)) return true;
    // uvnitr odkazu / tlacitka je cilem ten odkaz (obrazek a text v dlazdici napovedy ne)
    for (var p = el.parentElement; p && p !== D.body; p = p.parentElement) {
      if (p.tagName === 'A' || p.tagName === 'BUTTON' || p.tagName === 'SUMMARY' || p.tagName === 'LABEL') return false;
    }
    if (el.hasAttribute('onclick') || typeof el.onclick === 'function') return true;
    var r = el.getAttribute('role');
    if (r && /^(button|link|tab|menuitem|checkbox|radio|option|switch)$/.test(r)) return true;
    var ti = el.getAttribute('tabindex');
    if (ti !== null && parseInt(ti, 10) >= 0) return true;
    if (klikaci && klikaci.has(el)) return true;
    try {
      // kurzor ruky nastaveny prave tomuto prvku (ne zdedeny od rodice)
      if (W.getComputedStyle(el).cursor === 'pointer' &&
          !(el.parentElement && W.getComputedStyle(el.parentElement).cursor === 'pointer')) return true;
    } catch (e) {}
    return false;
  }

  function viditelny(el) {
    var r = el.getBoundingClientRect();
    if (r.width < 4 || r.height < 4) return false;
    var cs = W.getComputedStyle(el);
    if (cs.visibility === 'hidden' || cs.display === 'none' || parseFloat(cs.opacity) < 0.05) return false;
    for (var p = el; p && p !== D.body; p = p.parentElement) {
      if (p.getAttribute && p.getAttribute('aria-hidden') === 'true') return false;
      var ps = W.getComputedStyle(p);
      if (ps.display === 'none' || parseFloat(ps.opacity) < 0.05) return false;
    }
    return true;
  }

  // nejvyssi vrstva: prekryvny panel (fixed, aspon pul obrazovky) uprostred obrazovky, jinak cela stranka
  function vrstva() {
    var e = null;
    try { e = D.elementFromPoint(W.innerWidth / 2, W.innerHeight / 2); } catch (x) {}
    var nalez = null;
    for (; e && e !== D.body && e !== D.documentElement; e = e.parentElement) {
      var cs = W.getComputedStyle(e);
      if (cs.position === 'fixed') {
        var r = e.getBoundingClientRect();
        if (r.width >= W.innerWidth * 0.5 && r.height >= W.innerHeight * 0.5) nalez = e;
      }
    }
    return nalez || D.body;
  }

  function kandidati() {
    var koren = vrstva();
    if (!koren) return [];
    var vse = koren.getElementsByTagName('*'), v = [], i, el;
    for (i = 0; i < vse.length; i++) {
      el = vse[i];
      if (klikatelny(el) && viditelny(el)) v.push(el);
    }
    // obal (radek s tlacitky, pozadi panelu) pryc, kdyz ma uvnitr vlastni klikatelne prvky; obrovske
    // pozadi (vic nez 70 % obrazovky, ne pole formulare) taky
    var plocha = W.innerWidth * W.innerHeight, ven = [];
    for (i = 0; i < v.length; i++) {
      el = v[i];
      var uvnitr = false;
      for (var j = 0; j < v.length && !uvnitr; j++) if (j !== i && el.contains(v[j])) uvnitr = true;
      if (uvnitr) continue;
      var r = el.getBoundingClientRect();
      if (!interaktivniTag(el) && r.width * r.height > plocha * 0.7) continue;
      ven.push(el);
    }
    return ven;
  }

  function stred(r) { return { x: r.left + r.width / 2, y: r.top + r.height / 2 }; }

  function prvni(v) {
    for (var i = 0; i < v.length; i++) if (v[i].hasAttribute('autofocus')) return v[i];
    var nej = null, s = 1e18;
    for (i = 0; i < v.length; i++) {
      var r = v[i].getBoundingClientRect();
      var mimo = (r.bottom < 0 || r.top > W.innerHeight) ? 1e9 : 0;   // nejdriv co je videt
      var sk = mimo + Math.max(0, r.top) * 4 + Math.max(0, r.left);
      if (sk < s) { s = sk; nej = v[i]; }
    }
    return nej;
  }

  // nejblizsi prvek ve smeru (dx, dy): hlavni vzdalenost + 2x odchylka napric (prekryv = 0)
  function veSmeru(v, odkud, dx, dy) {
    var a = odkud.getBoundingClientRect(), ca = stred(a), nej = null, s = 1e18;
    for (var i = 0; i < v.length; i++) {
      var el = v[i];
      if (el === odkud || el.contains(odkud) || odkud.contains(el)) continue;
      var r = el.getBoundingClientRect(), c = stred(r), hlavni, napric;
      if (dy > 0) { if (c.y <= ca.y + 2 || r.bottom <= a.bottom - 1) continue; hlavni = Math.max(0, r.top - a.bottom); napric = Math.max(0, Math.max(r.left, a.left) - Math.min(r.right, a.right)); }
      else if (dy < 0) { if (c.y >= ca.y - 2 || r.top >= a.top + 1) continue; hlavni = Math.max(0, a.top - r.bottom); napric = Math.max(0, Math.max(r.left, a.left) - Math.min(r.right, a.right)); }
      else if (dx > 0) { if (c.x <= ca.x + 2 || r.right <= a.right - 1) continue; hlavni = Math.max(0, r.left - a.right); napric = Math.max(0, Math.max(r.top, a.top) - Math.min(r.bottom, a.bottom)); }
      else { if (c.x >= ca.x - 2 || r.left >= a.left + 1) continue; hlavni = Math.max(0, a.left - r.right); napric = Math.max(0, Math.max(r.top, a.top) - Math.min(r.bottom, a.bottom)); }
      var sk = hlavni + 2 * napric + 0.05 * (Math.abs(c.x - ca.x) + Math.abs(c.y - ca.y));
      if (sk < s) { s = sk; nej = el; }
    }
    return nej;
  }

  function posunStranku(dx, dy) {
    var koren = vrstva(), el = fokus || koren;
    for (var p = el; p && p !== D.body && p !== D.documentElement; p = p.parentElement) {
      var cs = W.getComputedStyle(p);
      if (dy && /(auto|scroll)/.test(cs.overflowY) && p.scrollHeight > p.clientHeight + 2) {
        var pred = p.scrollTop;
        p.scrollTop += dy * Math.max(60, p.clientHeight * 0.6);
        if (p.scrollTop !== pred) return true;
      }
      if (dx && /(auto|scroll)/.test(cs.overflowX) && p.scrollWidth > p.clientWidth + 2) {
        var predX = p.scrollLeft;
        p.scrollLeft += dx * Math.max(60, p.clientWidth * 0.6);
        if (p.scrollLeft !== predX) return true;
      }
    }
    var y0 = W.pageYOffset, x0 = W.pageXOffset;
    W.scrollBy(dx * W.innerWidth * 0.6, dy * W.innerHeight * 0.6);
    return W.pageYOffset !== y0 || W.pageXOffset !== x0;
  }

  function nastav(el) {
    if (fokus && fokus !== el && fokus.classList) fokus.classList.remove(TRIDA);
    fokus = el;
    if (!el) return;
    styl();
    el.classList.add(TRIDA);
    try {
      var a = D.activeElement;
      if (interaktivniTag(el) && !textovePole(el)) { if (a !== el) el.focus({ preventScroll: true }); }
      else if (a && a !== D.body && a !== el && !textovePole(el)) a.blur();
    } catch (e) {}
    try { el.scrollIntoView({ block: 'nearest', inline: 'nearest' }); } catch (e) { try { el.scrollIntoView(false); } catch (x) {} }
  }

  function textovePole(el) {
    if (!el) return false;
    var t = el.tagName;
    if (t === 'TEXTAREA' || el.isContentEditable) return true;
    if (t !== 'INPUT') return false;
    return !/^(button|submit|reset|checkbox|radio|range|color|file|image)$/i.test(el.type || '');
  }

  function stiskni(el) {
    if (textovePole(el) || el.tagName === 'SELECT') { try { el.focus(); } catch (e) {} if (el.tagName === 'SELECT') { try { el.click(); } catch (e) {} } return; }
    try {
      var r = el.getBoundingClientRect(), o = { bubbles: true, cancelable: true, view: W, clientX: r.left + r.width / 2, clientY: r.top + r.height / 2 };
      el.dispatchEvent(new W.MouseEvent('mousedown', o));
      el.dispatchEvent(new W.MouseEvent('mouseup', o));
    } catch (e) {}
    try { el.click(); } catch (e) {}
  }

  function smer(e) {
    var k = e.key || '', c = e.keyCode || 0;
    if (k === 'ArrowUp' || k === 'Up' || c === 38) return [0, -1];
    if (k === 'ArrowDown' || k === 'Down' || c === 40) return [0, 1];
    if (k === 'ArrowLeft' || k === 'Left' || c === 37) return [-1, 0];
    if (k === 'ArrowRight' || k === 'Right' || c === 39) return [1, 0];
    return null;
  }

  // stranka si klavesu vyresila sama (napr. volba jazyka presune focus na tlacitko) - ramecek jde za focusem
  function prevezmi() {
    var a = D.activeElement;
    if (!aktivni || !a || a === D.body || a === D.documentElement || a === fokus) return;
    if (klikatelny(a) && viditelny(a)) nastav(a);
  }

  function klavesa(e) {
    if (e.altKey || e.ctrlKey || e.metaKey) return;
    if (e.defaultPrevented) { prevezmi(); return; }   // stranka si klavesu vyresila sama
    var s = smer(e), enter = (e.key === 'Enter' || e.keyCode === 13);
    if (!s && !enter) return;
    var akt = D.activeElement;
    if (textovePole(akt)) {
      // v textovem poli sipky vlevo / vpravo hybou kurzorem; nahoru / dolu z pole ven
      if (!s || s[0] !== 0) return;
      try { akt.blur(); } catch (x) {}
      if (!fokus) nastav(akt);
    }
    if (enter) {
      if (!aktivni || !fokus || !D.body.contains(fokus) || !viditelny(fokus)) return;
      e.preventDefault();
      stiskni(fokus);
      return;
    }
    var v = kandidati();
    var platny = fokus && D.body.contains(fokus) && viditelny(fokus) && v.indexOf(fokus) >= 0;
    if (!aktivni || !platny) {
      aktivni = true;
      var p = prvni(v);
      if (p) { e.preventDefault(); nastav(p); }
      else if (posunStranku(s[0], s[1])) e.preventDefault();
      return;
    }
    var dalsi = veSmeru(v, fokus, s[0], s[1]);
    e.preventDefault();
    if (dalsi) nastav(dalsi);
    else posunStranku(s[0], s[1]);
  }

  // az PO strankach (bublani na okne) - co stranka zpracuje sama (preventDefault), nechat
  W.addEventListener('keydown', klavesa, false);
  // dotyk / mys: ramecek pryc (na telefonu se ovlada prstem)
  function dotyk() { if (aktivni && !W.NAP_TV) { aktivni = false; nastav(null); } }
  W.addEventListener('touchstart', dotyk, true);
  W.addEventListener('mousedown', function (e) { if (e.isTrusted) dotyk(); }, true);

  W.NapTvOvladani = {
    /** Appka na TV: ramecek hned (bez cekani na prvni sipku). */
    zapni: function () {
      W.NAP_TV = true;
      aktivni = true;
      if (!fokus || !D.body.contains(fokus) || !viditelny(fokus)) nastav(prvni(kandidati()));
      return fokus ? (fokus.id || fokus.tagName) : '';
    },
    /** B337: stranka sama urci, na cem ma ramecek byt (napr. ODEMKNOUT: SPUSTIT SEGU, ne DO MENU).
     *  Jen na TV (appka zavolala zapni) nebo kdyz uz clovek ovlada sipkami; na telefonu (dotyk) nic. */
    vyber: function (el) {
      if (!W.NAP_TV && !aktivni) return '';
      if (!el || !klikatelny(el) || !viditelny(el)) return '';
      aktivni = true;
      nastav(el);
      return el.id || el.tagName;
    },
    /** Pro testy: co se da vybrat a co je vybrane. */
    kandidati: kandidati,
    fokus: function () { return fokus; }
  };
})(window, document);
