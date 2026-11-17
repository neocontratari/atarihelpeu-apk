#!/usr/bin/env python3
"""B342: TLACITKO OTOCENI OBRAZOVKY na strankach Sega a PS1 + radek OBRAZOVKA v OPTIONS (Chromium, skutecne stranky).

Rene: "nech tu apku na vysku, pridelej tlacitko u kazde emu i zakladni obrazovky - sirka vyska a je to vyresene".
Falesny most AHOBRAZ (jako Java MainActivity.AHObrazovka) zapisuje volani.

  S1-S6) Sega a PS1 na vysku (telefon 412x846), na sirku (846x412), TV cela (1280x720) a TV pruh 9:16 (405x720):
         tlacitko je videt, cele na obrazovce, neprekryva zadne jine viditelne tlacitko stranky (na sirku vlevo
         vedle kolecka), ikona telefonu ukazuje cil (na vysku lezi, na sirku stoji), popisek co stisk udela,
         klik -> AHOBRAZ.prepni() (dvojity klik = jedno otoceni), na sirku i dotykem pres dotykovy engine stranky,
  S7) otoceni za chodu (resize) - popisek a ikona se prehodi,
  S8) uprava rozlozeni tlacitek (lista nahore) - otoceni schovane,
  S9) anglicky - popisek SCREEN TO LANDSCAPE / PORTRAIT, zadna cestina,
  S10) bez mostu AHOBRAZ (stranka mimo appku) - tlacitko schovane,
  O1-O4) OPTIONS -> OBRAZOVKA: telefon 3 tlacitka (aktivni podle Javy), klik -> AHOBRAZ.nastav(...), TV jen 2
         (NA VYSKU = pruh), anglicky SCREEN / PORTRAIT / LANDSCAPE / AUTO-ROTATE; bez mostu radek schovany,
  J) zadna chyba JavaScriptu.
Pouziti (z test_overeni/b342): python3 test_b342_stranky.py [PRAC]   -> VYSTUP_test_b342_stranky.txt (+ obrazky v PRAC)
"""
import json
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b342')
os.makedirs(PRAC, exist_ok=True)

MOST = r'''
window.__obraz = {volani: [], tv: %s, menu: %s};
window.AHOBRAZ = {
  stav: function () {
    var en = window.AHJAZYK && AHJAZYK.kod() === 'en';
    var p = __obraz.tv ? (__obraz.menu === 'vyska' ? 'na výšku - pruh jako telefon uprostřed televize' : 'celá obrazovka televize')
                       : (__obraz.menu === 'auto' ? 'podle telefonu (otáčení telefonu)' : (__obraz.menu === 'vyska' ? 'na výšku' : 'na šířku'));
    if (en) p = __obraz.menu === 'auto' ? 'follows the phone (auto-rotate)' : (__obraz.menu === 'vyska' ? 'portrait' : 'landscape');
    return JSON.stringify({skupina: 'menu', stav: __obraz.menu, menu: __obraz.menu, tv: __obraz.tv,
                           naSirku: innerWidth > innerHeight, popis: p});
  },
  prepni: function () { __obraz.volani.push('prepni'); },
  nastav: function (co) { __obraz.volani.push('nastav:' + co); __obraz.menu = co; }
};
'''

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


VIEW = {'tel_vyska': (412, 846, True), 'tel_sirka': (846, 412, True), 'tv': (1280, 720, False), 'tv_pruh': (405, 720, False)}


def otevri(b, zaklad, stranka, vid, chyby, jazyk='cs', most=True, tv=False, menu='auto'):
    w, h, dotyk = VIEW[vid]
    ctx = b.new_context(viewport={'width': w, 'height': h}, has_touch=dotyk, is_mobile=dotyk,
                        device_scale_factor=2 if dotyk else 1)
    ctx.add_init_script(H.init_skript(jazyk, {}))
    if most:
        ctx.add_init_script(MOST % (json.dumps(tv), json.dumps(menu)))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(stranka + ': ' + str(e).split('\n')[0][:200]))
    pg.goto(zaklad + stranka)
    pg.wait_for_timeout(1200)
    return ctx, pg


# viditelna tlacitka stranky (krome otoceni) a jejich obdelniky
OBDELNIKY = r'''
(id) => {
  function vid(el) {
    var cs = getComputedStyle(el);
    if (cs.display === 'none' || cs.visibility === 'hidden' || parseFloat(cs.opacity) === 0) return false;
    var r = el.getBoundingClientRect();
    return r.width > 2 && r.height > 2;
  }
  var t = document.getElementById(id);
  var r = t.getBoundingClientRect();
  var tel = t.querySelector('.napOtocTelefon');
  var out = {vid: vid(t), r: [r.left, r.top, r.right, r.bottom], vw: innerWidth, vh: innerHeight,
             title: t.getAttribute('title'), aria: t.getAttribute('aria-label'),
             tel: tel ? (tel.getAttribute('transform') || '') : null, svg: !!t.querySelector('svg'), jine: []};
  var l = document.querySelectorAll('main .hit, main button, main a');
  for (var i = 0; i < l.length; i++) {
    var e = l[i];
    // obraz PS1 (#psMonitor) je na sirku cela plocha pod tlacitky - neni to tlacitko
    if (e === t || t.contains(e) || !vid(e) || e.id === 'psMonitor') continue;
    var q = e.getBoundingClientRect();
    var px = Math.min(r.right, q.right) - Math.max(r.left, q.left), py = Math.min(r.bottom, q.bottom) - Math.max(r.top, q.top);
    if (px > 1 && py > 1) out.jine.push((e.id || e.className || e.tagName) + ' ' + Math.round(px) + 'x' + Math.round(py));
  }
  return out;
}
'''


def main():
    srv, zaklad = H.server()
    chyby = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        for jm, stranka, tid, kolecko in (('SEGA', 'emu_sega/index.html', 'btnSegaOtoc', 'btnSegaSettings'),
                                          ('PS1', 'emu_ps1/index.html', 'btnPs1Otoc', 'btnSettings')):
            for vid in ('tel_vyska', 'tel_sirka', 'tv', 'tv_pruh'):
                ctx, pg = otevri(b, zaklad, stranka, vid, chyby, tv=vid.startswith('tv'))
                o = pg.evaluate(OBDELNIKY, tid)
                na_sirku = o['vw'] > o['vh']
                r = o['r']
                cele = r[0] >= 0 and r[1] >= 0 and r[2] <= o['vw'] + 0.5 and r[3] <= o['vh'] + 0.5
                velikost = min(r[2] - r[0], r[3] - r[1])
                k = pg.evaluate("(id) => { var r = document.getElementById(id).getBoundingClientRect(); return [r.left, r.top, r.right, r.bottom]; }", kolecko)
                # aspon tak velke jako kolecko vedle (na sirku stejna pilulka), na vysku aspon 40 px
                minimum = (min(k[2] - k[0], k[3] - k[1]) - 1) if na_sirku else 40
                vedle = (not na_sirku) or (r[2] <= k[0] + 0.5 and abs((r[1] + r[3]) / 2 - (k[1] + k[3]) / 2) < 3)
                ikona = (o['tel'] == '') if na_sirku else (o['tel'] == 'rotate(-90)')
                popis = 'OBRAZOVKA NA VÝŠKU' if na_sirku else 'OBRAZOVKA NA ŠÍŘKU'
                pg.screenshot(path=os.path.join(PRAC, 'b342_%s_%s.png' % (jm.lower(), vid)))
                # klik (a hned druhy) -> jedno prepni
                if VIEW[vid][2]:
                    pg.tap('#' + tid)
                    pg.wait_for_timeout(80)
                    pg.tap('#' + tid)
                else:
                    pg.click('#' + tid)
                    pg.wait_for_timeout(80)
                    pg.click('#' + tid)
                pg.wait_for_timeout(250)
                vol = pg.evaluate('() => __obraz.volani.slice()')
                pg.wait_for_timeout(700)
                pg.click('#' + tid) if not VIEW[vid][2] else pg.tap('#' + tid)
                pg.wait_for_timeout(200)
                vol2 = pg.evaluate('() => __obraz.volani.slice()')
                over(o['vid'] and o['svg'] and cele and velikost >= minimum and not o['jine'] and vedle and ikona
                     and o['title'] == popis and o['aria'] == popis and vol == ['prepni'] and vol2 == ['prepni', 'prepni'],
                     '%s %s %dx%d: tlacitko %dx%d (min. %d) na [%d,%d], %s, ikona %s, popisek "%s", klik 2x rychle -> %s, pak dalsi -> %d'
                     % (jm, vid, o['vw'], o['vh'], r[2] - r[0], r[3] - r[1], minimum, r[0], r[1],
                        'vlevo vedle kolecka' if na_sirku else 'na vysku nahore', 'stoji' if o['tel'] == '' else 'lezi',
                        o['title'], vol, len(vol2))
                     + ('' if not o['jine'] else ' PREKRYVA: %s' % o['jine'])
                     + ('' if cele else ' MIMO OBRAZOVKU') + ('' if vedle else ' NE VEDLE KOLECKA %s' % k))
                ctx.close()

            # S7: otoceni za chodu (telefon se otoci) - popisek a ikona se prehodi
            ctx, pg = otevri(b, zaklad, stranka, 'tel_vyska', chyby)
            a1 = pg.evaluate("(id) => document.getElementById(id).getAttribute('title')", tid)
            pg.set_viewport_size({'width': 846, 'height': 412})
            pg.wait_for_timeout(400)
            a2 = pg.evaluate("(id) => [document.getElementById(id).getAttribute('title'), document.getElementById(id).querySelector('.napOtocTelefon').getAttribute('transform')]", tid)
            over(a1 == 'OBRAZOVKA NA ŠÍŘKU' and a2[0] == 'OBRAZOVKA NA VÝŠKU' and not a2[1],
                 '%s: otoceni za chodu - popisek "%s" -> "%s", telefon v ikone se postavil' % (jm, a1, a2[0]))
            # S8: uprava rozlozeni (lista nahore) - otoceni schovane
            bar = 'segaLayoutEditBar' if jm == 'SEGA' else 'layoutEditBar'
            v1 = pg.evaluate("(a) => { document.getElementById(a[0]).classList.add('show'); return getComputedStyle(document.getElementById(a[1])).display; }", [bar, tid])
            v2 = pg.evaluate("(a) => { document.getElementById(a[0]).classList.remove('show'); return getComputedStyle(document.getElementById(a[1])).display; }", [bar, tid])
            over(v1 == 'none' and v2 != 'none', '%s: pri uprave rozlozeni tlacitek otoceni schovane (%s), po HOTOVO zpet (%s)' % (jm, v1, v2))
            ctx.close()
            # S9: anglicky
            ctx, pg = otevri(b, zaklad, stranka, 'tel_vyska', chyby, jazyk='en')
            e1 = pg.evaluate("(id) => [document.getElementById(id).getAttribute('title'), document.getElementById(id).getAttribute('aria-label')]", tid)
            pg.set_viewport_size({'width': 846, 'height': 412})
            pg.wait_for_timeout(400)
            e2 = pg.evaluate("(id) => document.getElementById(id).getAttribute('title')", tid)
            ctx.close()
            over(e1 == ['SCREEN TO LANDSCAPE', 'SCREEN TO LANDSCAPE'] and e2 == 'SCREEN TO PORTRAIT',
                 '%s anglicky: "%s" / na sirku "%s"' % (jm, e1[0], e2))
            # S10: bez mostu
            ctx, pg = otevri(b, zaklad, stranka, 'tel_vyska', chyby, most=False)
            d = pg.evaluate("(id) => getComputedStyle(document.getElementById(id)).display", tid)
            ctx.close()
            over(d == 'none', '%s bez mostu AHOBRAZ (stranka mimo appku): tlacitko schovane (%s)' % (jm, d))

        # na sirku pres dotykovy engine Segy (dotyk mimo ovladani -> klik projde)
        ctx, pg = otevri(b, zaklad, 'emu_sega/index.html', 'tel_sirka', chyby)
        r = pg.evaluate("() => { var r = document.getElementById('btnSegaOtoc').getBoundingClientRect(); return [r.left + r.width / 2, r.top + r.height / 2]; }")
        pg.touchscreen.tap(r[0], r[1])
        pg.wait_for_timeout(300)
        vol = pg.evaluate('() => __obraz.volani.slice()')
        # tmave pozadi (jako hra pod strankou) - obrazek pro oko
        pg.evaluate("() => { document.documentElement.style.background = '#123'; document.body.style.background = 'linear-gradient(#245,#012)'; }")
        pg.screenshot(path=os.path.join(PRAC, 'b342_sega_sirka_tmave.png'))
        ctx.close()
        over(vol == ['prepni'], 'SEGA na sirku: dotyk prstem (dotykovy engine stranky) -> %s' % vol)

        # ---- OPTIONS -> OBRAZOVKA ----
        def options(jazyk='cs', tv=False, menu='auto', most=True, vid='tel_vyska'):
            ctx, pg = otevri(b, zaklad, 'index.html', vid, chyby, jazyk=jazyk, tv=tv, menu=menu, most=most)
            pg.evaluate('() => window.napOtevriNastaveni()')
            pg.wait_for_timeout(300)
            return ctx, pg
        STAV = r'''() => {
          function vid(id) { var e = document.getElementById(id); return !!e && getComputedStyle(e).display !== 'none'; }
          var t = {};
          ['stObrNadpis', 'stObraz', 'stObrHint', 'stObrVyska', 'stObrSirka', 'stObrAuto'].forEach(function (id) { t[id] = vid(id); });
          t.aktivni = Array.prototype.filter.call(document.querySelectorAll('#stObraz button'), function (b) { return b.classList.contains('active'); }).map(function (b) { return b.id; });
          t.texty = Array.prototype.map.call(document.querySelectorAll('#stObrNadpis, #stObraz button'), function (b) { return b.textContent.trim(); });
          t.hint = document.getElementById('stObrHint').textContent;
          var j = document.getElementById('stJazyk').getBoundingClientRect(), o = document.getElementById('stObraz').getBoundingClientRect();
          t.podJazykem = o.top > j.bottom && o.top - j.bottom < 80;
          return t;
        }'''
        ctx, pg = options()
        t = pg.evaluate(STAV)
        pg.click('#stObrSirka')
        pg.wait_for_timeout(400)
        t2 = pg.evaluate(STAV)
        vol = pg.evaluate('() => __obraz.volani.slice()')
        pg.screenshot(path=os.path.join(PRAC, 'b342_options_telefon.png'))
        pg.click('#stObrAuto')
        pg.wait_for_timeout(400)
        vol2 = pg.evaluate('() => __obraz.volani.slice()')
        ctx.close()
        over(t['stObraz'] and t['stObrAuto'] and t['aktivni'] == ['stObrAuto'] and t['podJazykem']
             and t['texty'] == ['OBRAZOVKA', 'NA VÝŠKU', 'NA ŠÍŘKU', 'PODLE TELEFONU'] and 'podle telefonu' in t['hint']
             and t2['aktivni'] == ['stObrSirka'] and vol == ['nastav:sirka'] and vol2 == ['nastav:sirka', 'nastav:auto'],
             'O1) OPTIONS telefon: radek OBRAZOVKA hned pod jazykem, %s, aktivni %s; NA ŠÍŘKU -> %s (aktivni %s), PODLE TELEFONU -> %s'
             % (t['texty'], t['aktivni'], vol, t2['aktivni'], vol2[-1:]))
        ctx, pg = options(tv=True, menu='auto', vid='tv')
        t = pg.evaluate(STAV)
        pg.screenshot(path=os.path.join(PRAC, 'b342_options_tv.png'))
        ctx.close()
        over(t['stObraz'] and not t['stObrAuto'] and t['aktivni'] == ['stObrSirka'] and 'pruhu uprostřed televize' in t['hint'],
             'O2) OPTIONS na TV: jen NA VÝŠKU / NA ŠÍŘKU (AUTO = cela obrazovka -> aktivni %s), napoveda o pruhu' % t['aktivni'])
        ctx, pg = options(jazyk='en')
        t = pg.evaluate(STAV)
        sber = pg.evaluate(H.SBER_JS)
        ctx.close()
        zb = [x for x in H.zbytky_cestiny(sber) if 'OBRAZ' in x.upper() or 'VÝŠ' in x.upper() or 'ŠÍŘ' in x.upper() or 'TELEFON' in x.upper()]
        over(t['texty'] == ['SCREEN', 'PORTRAIT', 'LANDSCAPE', 'AUTO-ROTATE'] and 'Main menu' in t['hint'] and not zb,
             'O3) OPTIONS anglicky: %s, napoveda "%s..."%s' % (t['texty'], t['hint'][:40], '' if not zb else ' ZBYTKY: %s' % zb))
        ctx, pg = options(most=False)
        t = pg.evaluate(STAV)
        ctx.close()
        over(not t['stObrNadpis'] and not t['stObraz'] and not t['stObrHint'], 'O4) bez mostu AHOBRAZ (starsi appka / PC): radek OBRAZOVKA schovany')

        over(not chyby, 'J) zadna chyba JavaScriptu%s' % ('' if not chyby else ': ' + '; '.join(chyby[:3])))
        b.close()
    srv.shutdown()
    vystup.append('VYSLEDEK: %d testu, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_b342_stranky.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
