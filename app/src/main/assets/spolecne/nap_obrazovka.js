/* nap_obrazovka.js
 * ====================================================================
 *  B342: TLACITKO OTOCENI OBRAZOVKY (NA VYSKU / NA SIRKU) na strankach emulatoru (Sega, PS1).
 *
 *  Rene: "nech tu apku na vysku, pridelej tlacitko u kazde emu i zakladni obrazovky - sirka
 *  vyska a je to vyresene".
 *
 *  Kazdy prvek <button data-nap-otoc> dostane:
 *   - ikonu: telefon a dve zahnute sipky (stejna jako v hlavnim menu a u Atari). Telefon v ikone
 *     ukazuje, KAM se obrazovka otoci: na vysku lezi (stisk = na sirku), na sirku stoji,
 *   - popisek (title, aria-label) cesky / anglicky (NapJazyk),
 *   - klik -> AHOBRAZ.prepni(): Java otoci telefon, na TV prepne pruh 9:16 uprostred / celou
 *     obrazovku a zapamatuje si volbu pro tuto konzoli (NapObrazovka).
 *  Vzhled a poloha je v CSS kazde stranky (trida .napOtoc). Psano v ES5 (TV prohlizece).
 * ====================================================================
 */
(function (W, D) {
  'use strict';
  if (W.NapObrazovka) return;

  var IKONA = '<svg class="napOtocIkona" viewBox="-12 -12 24 24" aria-hidden="true" focusable="false">'
    + '<g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round">'
    + '<path d="M-6.07 2.83A6.7 6.7 0 0 1-1.73-6.47"/>'
    + '<path d="M6.07-2.83A6.7 6.7 0 0 1 1.73 6.47"/>'
    + '</g>'
    + '<path fill="currentColor" d="M-0.18-6.88L-3.01-7.79L-2.19-4.69ZM0.18 6.88L3.01 7.79L2.19 4.69Z"/>'
    + '<g class="napOtocTelefon">'
    + '<rect x="-2.1" y="-3.6" width="4.2" height="7.2" rx="0.9" fill="none" stroke="currentColor" stroke-width="1.4"/>'
    + '<circle cx="0" cy="2.4" r="0.55" fill="currentColor"/>'
    + '</g></svg>';

  var tlacitka = [];
  var nedavno = 0;

  function t(cs, en) {
    try { if (W.NapJazyk && W.NapJazyk.t) return W.NapJazyk.t(cs, en); } catch (e) {}
    return cs;
  }
  function naSirku() {
    return (W.innerWidth || D.documentElement.clientWidth || 0) > (W.innerHeight || D.documentElement.clientHeight || 0);
  }
  function popis() {
    return naSirku() ? t('OBRAZOVKA NA VÝŠKU', 'SCREEN TO PORTRAIT') : t('OBRAZOVKA NA ŠÍŘKU', 'SCREEN TO LANDSCAPE');
  }

  function obnov() {
    var s = naSirku(), p = popis();
    for (var i = 0; i < tlacitka.length; i++) {
      var b = tlacitka[i];
      b.setAttribute('title', p);
      b.setAttribute('aria-label', p);
      if (s) b.className = b.className.replace(/(^|\s)napOtocNaVysku(?=\s|$)/g, '');
      else if (!/(^|\s)napOtocNaVysku(\s|$)/.test(b.className)) b.className += ' napOtocNaVysku';
      var tel = b.querySelector('.napOtocTelefon');
      if (tel) { if (s) tel.removeAttribute('transform'); else tel.setAttribute('transform', 'rotate(-90)'); }
    }
  }

  function ted() { return Date.now ? Date.now() : new Date().getTime(); }

  // B356: KLEPNUTI HNED PO OTOCENI SE ZAHODI. Prst, ktery otacel, lezi po otoceni na jinem tlacitku stranky
  // (Rene, PS1 19. 11.: 21:33:21 otoceni NA VYSKU -> 21:33:22 SAVE GAME 2x - tlacitko otoceni na sirku je
  // fyzicky tam, kde je na vysku SAVE GAME - a menu BIOSu prepsalo rychlou pozici Final Fantasy VIII).
  // 1,2 s po otoceni (tlacitko i otoceni telefonu) se klik (tlacitka a odkazy stranky) nebere. Dotykove
  // ovladani (touchstart / touchend) beze zmeny - hra se ovlada dal.
  var OTOCENI_KLID_MS = 1200;
  var otoceno = 0;
  var sirkaPred = null;
  function oznacOtoceni() { otoceno = ted(); }
  function hlidejKlik(e) {
    if (!otoceno || ted() - otoceno > OTOCENI_KLID_MS) return;
    var t = e && e.target;
    while (t && t !== D) {                        // tlacitko otoceni samo: dvojity klik resi prepni()
      if (t.getAttribute && t.getAttribute('data-nap-otoc') !== null) return;
      t = t.parentNode;
    }
    try { e.preventDefault(); e.stopPropagation(); if (e.stopImmediatePropagation) e.stopImmediatePropagation(); } catch (er) {}
    try { if (typeof W.jsLog === 'function') W.jsLog('B356 klepnuti ' + (ted() - otoceno) + ' ms po otoceni obrazovky zahozeno'); } catch (er2) {}
  }
  function zmenaVelikosti() {
    var s = naSirku();
    if (sirkaPred !== null && s !== sirkaPred) oznacOtoceni();
    sirkaPred = s;
  }

  function prepni(e) {
    if (e) { try { e.preventDefault(); e.stopPropagation(); } catch (er) {} }
    var t0 = ted();
    if (t0 - nedavno < 700) return;            // dvojity klik / klik + dotyk = jedno otoceni
    nedavno = t0;
    oznacOtoceni();                            // B356
    try {
      if (W.AHOBRAZ && W.AHOBRAZ.prepni) { W.AHOBRAZ.prepni(); return; }
    } catch (er2) {}
  }

  function zapoj(b) {
    if (!b || b.getAttribute('data-nap-otoc-zapojeno')) return;
    // bez mostu do Javy (stranka mimo appku) by tlacitko nic nedelalo - schovat
    if (!W.AHOBRAZ) { b.style.display = 'none'; return; }
    b.setAttribute('data-nap-otoc-zapojeno', '1');
    if (!b.querySelector('svg')) b.innerHTML = IKONA;
    b.addEventListener('click', prepni, true);
    tlacitka.push(b);
  }

  function najdi() {
    var l = D.querySelectorAll('[data-nap-otoc]');
    for (var i = 0; i < l.length; i++) zapoj(l[i]);
    obnov();
  }

  W.addEventListener('click', hlidejKlik, true);   // B356: zachyt na okne = drive nez tlacitka stranky
  sirkaPred = naSirku();
  W.addEventListener('resize', function () { zmenaVelikosti(); obnov(); });
  W.addEventListener('orientationchange', function () { oznacOtoceni(); setTimeout(obnov, 120); });
  if (D.readyState === 'loading') D.addEventListener('DOMContentLoaded', najdi);
  else najdi();
  try { if (W.NapJazyk && W.NapJazyk.priZmene) W.NapJazyk.priZmene(obnov); } catch (e) {}

  W.NapObrazovka = { prepni: prepni, obnov: obnov, popis: popis, najdi: najdi,
    oznacOtoceni: oznacOtoceni, klidPoOtoceni: function () { return otoceno > 0 && ted() - otoceno <= OTOCENI_KLID_MS; } };
})(window, document);
