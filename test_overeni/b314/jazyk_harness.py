#!/usr/bin/env python3
"""B314: spolecne veci pro testy jazyka CZ / EN (Chromium + Playwright).

- assety appky pres maly HTTP server (jako WebView: stranky, skripty, obrazky),
- falesne mosty Javy (AHJAZYK, AHINTRO, AHTVWEB ...) - co stranka zavola, vrati rozumnou hodnotu,
- sber VSEHO textu stranky (textove uzly, atributy, titulek, alert / confirm / prompt, texty
  kreslene do canvasu) a hledani zbytku cestiny:
    * ceska pismena s hackem / carkou,
    * ceska slova bez diakritiky (wordfreq: slovo je v cestine bezne a v anglictine skoro ne).
"""
import http.server
import json
import os
import re
import socket
import threading
import unicodedata

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
ASSETS = os.path.join(KOREN, 'app/src/main/assets')

DIAKRITIKA = re.compile('[ěščřžýáíéůúťďňĚŠČŘŽÝÁÍÉŮÚŤĎŇ]')

# jmena a nazvy, ktere v anglictine zustavaji (nejsou to zbytky cestiny)
POVOLENO = {
    'rené', 'frank', 'čeština', 'ceština', 'neil',
}
# zkratky a technicke nazvy - nikdy ne "cesky zbytek"
TECHNICKE = {
    'atari', 'sega', 'ps1', 'vbxe', 'xex', 'atr', 'tbxl', 'cload', 'csave', 'basic', 'turbo', 'run', 'list',
    'kick', 'bass', 'fire', 'start', 'select', 'option', 'reset', 'help', 'menu', 'log', 'net', 'txt',
    'wav', 'mp3', 'bin', 'cue', 'iso', 'chd', 'pbp', 'zip', 'bios', 'dj', 'eq', 'fx', 'bpm', 'n&p', 'tv', 'pc',
    'atarihelp', 'eu', 'cz', 'en', 'ok', 'id', 'ms', 'hz', 'khz', 'kb', 'mb', 'gb', 'fps', 'hd', 'sd', 'lan',
    'wi', 'fi', 'usb', 'gl', 'cpu', 'gpu', 'jpeg', 'jpg', 'png', 'h264', 'mjpeg', 'avd', 'vol', 'rec', 'rew',
    'ff', 'fwd', 'eject', 'play', 'stop', 'pause', 'demo', 'retro', 'universe', 'vision', 'mega', 'drive',
    'playstation', 'nap', 'cdn', 'mix', 'deck', 'cue', 'sync', 'pitch', 'loop', 'sample', 'xl', 'ii', 'iii',
    'genesis', 'sonic', 'doom', 'raw', 'pcm', 'ogg', 'm4a', 'aac', 'flac', 'opus', 'webm', 'fuji', 'esc',
}

_wf = None


def _wordfreq():
    global _wf
    if _wf is None:
        try:
            from wordfreq import zipf_frequency
            _wf = zipf_frequency
        except Exception:
            _wf = False
    return _wf


def bez_diakritiky(s):
    return ''.join(c for c in unicodedata.normalize('NFD', s) if unicodedata.category(c) != 'Mn')


def ceska_slova(text, povolena=()):
    """Seznam podezrelych ceskych slov v textu (prazdny = ok)."""
    nalez = []
    for m in re.finditer(r"[A-Za-zÀ-ž]+(?:['’][A-Za-z]+)?", text):
        w = m.group(0)
        wl = w.lower()
        if wl in POVOLENO or wl in TECHNICKE or wl in povolena:
            continue
        if DIAKRITIKA.search(w):
            nalez.append(w)
            continue
        zf = _wordfreq()
        if not zf or len(wl) < 3:
            continue
        en = zf(wl, 'en')
        cs = max(zf(wl, 'cs'), zf(wl, 'sk'))
        if en < 1.5 and cs >= 2.3:
            nalez.append(w)
    return nalez


def volny_port():
    s = socket.socket()
    s.bind(('127.0.0.1', 0))
    p = s.getsockname()[1]
    s.close()
    return p


class _Ticho(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()


def server(koren=ASSETS):
    """HTTP server nad adresarem assets. Vraci (server, zakladni adresa)."""
    port = volny_port()
    h = lambda *a, **k: _Ticho(*a, directory=koren, **k)
    srv = http.server.ThreadingHTTPServer(('127.0.0.1', port), h)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv, 'http://127.0.0.1:%d/' % port


# falesne mosty Javy: AHJAZYK podle jazyka testu; ostatni - Proxy: kazda metoda existuje
# a vrati hodnotu z NAP_MOCK (nebo '' ). Co stranka zavolala, je v window.__napVolani.
def init_skript(jazyk, mock=None):
    mock = mock or {}
    return '''
(function(){
  var JAZ = %s;
  window.__napVolani = [];
  window.__napDialogy = [];
  window.__napCanvas = {};
  window.NAP_MOCK = %s;
  function hodnota(most, jm, args) {
    window.__napVolani.push(most + '.' + jm);
    var k = most + '.' + jm;
    if (Object.prototype.hasOwnProperty.call(NAP_MOCK, k)) {
      var v = NAP_MOCK[k];
      if (typeof v === 'string' && v.indexOf('fn:') === 0) return (new Function('args', v.slice(3)))(args);
      return v;
    }
    return '';
  }
  function most(jm) {
    return new Proxy({}, {
      get: function (t, p) {
        if (p === 'then' || typeof p === 'symbol') return undefined;
        return function () { return hodnota(jm, String(p), Array.prototype.slice.call(arguments)); };
      },
      has: function () { return true; }
    });
  }
  window.AHJAZYK = {
    kod: function () { return JAZ; },
    nastav: function (k) { window.__napVolani.push('AHJAZYK.nastav:' + k); JAZ = k; return k; }
  };
  ['AHINTRO', 'AHTVWEB', 'AHNATIVE', 'AHRENDER', 'AHSAVE', 'AHPICK', 'AHPS1', 'AHSega', 'AHNET', 'AHATARICPP']
    .forEach(function (m) { window[m] = most(m); });
  // dialogy: zapsat a vratit "OK"
  window.alert = function (m) { __napDialogy.push('alert:' + m); };
  window.confirm = function (m) { __napDialogy.push('confirm:' + m); return true; };
  window.prompt = function (m, d) { __napDialogy.push('prompt:' + m); return d || ''; };
  // canvas: co se opravdu nakreslilo (az po prekladu - obal stranky je vnejsi)
  try {
    var P = CanvasRenderingContext2D.prototype, f = P.fillText, s = P.strokeText;
    P.fillText = function (t) { try { __napCanvas[String(t)] = 1; } catch (e) {} return f.apply(this, arguments); };
    P.strokeText = function (t) { try { __napCanvas[String(t)] = 1; } catch (e) {} return s.apply(this, arguments); };
  } catch (e) {}
})();
''' % (json.dumps(jazyk), json.dumps(mock))


SBER_JS = r'''
() => {
  var out = {texty: [], atributy: [], titulek: document.title, dialogy: (window.__napDialogy || []).slice(),
             canvas: Object.keys(window.__napCanvas || {}), lang: document.documentElement.lang};
  var en = /(^|\s)nap-jazyk-en(\s|$)/.test(document.documentElement.className);
  function preskoc(e) {
    for (; e && e.nodeType === 1; e = e.parentNode) {
      if (/^(SCRIPT|STYLE|NOSCRIPT|TEMPLATE)$/.test(e.nodeName)) return true;
      if (e.hasAttribute('data-nepreklad')) return true;
      // texty ve dvou verzich: vidi se jen verze zvoleneho jazyka
      if (e.hasAttribute(en ? 'data-cs-only' : 'data-en-only')) return true;
    }
    return false;
  }
  var w = document.createTreeWalker(document.body || document.documentElement, NodeFilter.SHOW_TEXT, null, false), n;
  while ((n = w.nextNode())) {
    var t = n.nodeValue.replace(/\s+/g, ' ').trim();
    if (!t || !/[A-Za-zÀ-ɏ]/.test(t)) continue;
    if (preskoc(n.parentNode)) continue;
    out.texty.push(t);
  }
  var els = document.querySelectorAll('[placeholder],[title],[aria-label],[alt],input[type=button],input[type=submit]');
  for (var i = 0; i < els.length; i++) {
    var e = els[i];
    if (preskoc(e)) continue;
    ['placeholder', 'title', 'aria-label', 'alt'].forEach(function (a) {
      if (e.hasAttribute(a) && /[A-Za-z]/.test(e.getAttribute(a))) out.atributy.push(a + '=' + e.getAttribute(a));
    });
    if (e.nodeName === 'INPUT' && /^(button|submit)$/i.test(e.type) && e.value) out.atributy.push('value=' + e.value);
  }
  return out;
}
'''


def zbytky_cestiny(sber, povolene_vety=(), povolena_slova=()):
    """Vsechny texty se zbytky cestiny: [(druh, text, slova)]."""
    vys = []
    vse = [('text', t) for t in sber['texty']] + [('atribut', a) for a in sber['atributy']] + \
          [('titulek', sber['titulek'])] + [('dialog', d) for d in sber['dialogy']] + \
          [('canvas', c) for c in sber['canvas']]
    for druh, t in vse:
        if not t or t in povolene_vety:
            continue
        sl = ceska_slova(t, set(w.lower() for w in povolena_slova))
        if sl:
            vys.append((druh, t, sl))
    return vys
