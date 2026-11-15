#!/usr/bin/env python3
"""WordPress wptexturize() v Pythonu (podle wp-includes/formatting.php, WordPress 6.x).

PROC: WordPress pousti obsah KAZDE stranky pres filtr the_content -> wptexturize (i blok Vlastni HTML
v Gutenbergu; wpautop jen v klasickem editoru). wptexturize rozseka text na "znacky" a text:
znacka = od znaku "<" po nejblizsi ">" - i kdyz je to porovnani v JavaScriptu (i < n ... x > 0).
V KAZDE takove "znacce" nahradi kazdy & za &#038; - i uvnitr <script> (skript chrani jen pred
upravou textu mezi znackami). Z "if (img && !img...)" se tak stane "if (img &#038;&#038; !img...)"
-> chyba syntaxe -> cely skript stranky nebezi (odpocet 00:00:00:00, obrazky bez src). Stalo se v B322
15. 11. 2026 na atarihelp.eu/?page_id=1130 (B321 && mezi < a > nemela, proto bezela).

Verne: rozdeleni na znacky / komentare / shortcody, & -> &#038; ve znackach, zasobnik znacek bez
upravy (pre, code, kbd, style, script, tt). Zjednodusene: uprava viditelneho textu mimo znacky
(uvozovky, pomlcky, ...) - na beh skriptu nema vliv.
"""
import re

NO_TEXTURIZE_TAGS = ('pre', 'code', 'kbd', 'style', 'script', 'tt')
NO_TEXTURIZE_SHORTCODES = ('code',)
SHORTCODY_JADRA = ('embed', 'wp_caption', 'caption', 'gallery', 'playlist', 'audio', 'video')

# _get_wptexturize_split_regex(): komentar <!-- ... --> nebo "<" az po nejblizsi ">" (bez ">" az do konce)
_KOMENTAR = r'!(?:-(?!->)[^\-]*+)*+(?:-->)?'
_HTML = r'<(?:(?=!--)' + _KOMENTAR + r'|(?!!--)[^>]*>?)'
_ENTITA = re.compile(r'&(?!#(?:\d+|x[a-f0-9]+);|[a-z1-4]{1,8};)', re.I)


def _shortcode_regex(jmena):
    t = '(?:' + '|'.join(re.escape(j) for j in jmena) + r')(?=[\s\]/])'
    return r'\[[/\[]?' + t + r'(?:[^\[\]<>]+|<[^\[\]>]*>)*+\]\]?'


def _php_substr(s, start, delka):
    if start >= len(s):
        return ''
    if delka < 0:
        konec = len(s) + delka
        return s[start:konec] if konec > start else ''
    return s[start:start + delka]


def _pushpop(text, zasobnik, vypnute):
    """_wptexturize_pushpop_element()"""
    if len(text) > 1 and text[1] != '/':
        oteviraci, posun = True, 1
    elif not zasobnik:
        return
    else:
        oteviraci, posun = False, 2
    mezera = text.find(' ')
    delka = -1 if mezera == -1 else mezera - posun
    znacka = _php_substr(text, posun, delka)
    if znacka in vypnute:
        if oteviraci:
            zasobnik.append(znacka)
        elif zasobnik and zasobnik[-1] == znacka:
            zasobnik.pop()


def _text(t):
    """zjednodusena uprava viditelneho textu (na beh skriptu nema vliv)"""
    for a, b in (('...', '&#8230;'), ('``', '&#8220;'), ("''", '&#8221;'), (' (tm)', ' &#8482;')):
        t = t.replace(a, b)
    if "'" in t:
        t = re.sub(r"(^|[\s(\[{\"-])'", r'\1&#8216;', t)
        t = t.replace("'", '&#8217;')
    if '"' in t:
        t = re.sub(r'(^|[\s(\[{-])"(?=\S)', r'\1&#8220;', t)
        t = t.replace('"', '&#8221;')
    if '-' in t:
        t = t.replace('---', '&#8212;')
        t = re.sub(r'(^|\s)--(?=$|\s)', r'\1&#8212;', t)
        t = re.sub(r'(?<!xn)--', '&#8211;', t)
        t = re.sub(r'(^|\s)-(?=$|\s)', r'\1&#8211;', t)
    t = re.sub(r'\b(\d[\d.,]*)x(\d[\d.,]*)\b', r'\1&#215;\2', t)
    return _ENTITA.sub('&#038;', t)


def wptexturize(text, shortcody=SHORTCODY_JADRA):
    if not text:
        return text
    nalezene = set(re.findall(r'\[/?([^<>&/\[\]\x00-\x20=]++)', text)) & set(shortcody)
    sc = _shortcode_regex(sorted(nalezene)) if nalezene else ''
    rozdel = re.compile('(' + _HTML + ('|' + sc if sc else '') + ')')
    casti = [c for c in rozdel.split(text) if c != '']
    zas_znacky, zas_sc = [], []
    for i, c in enumerate(casti):
        if c[0] == '<':
            if c.startswith('<!--'):
                continue
            c = _ENTITA.sub('&#038;', c)          # i uvnitr <script>!
            _pushpop(c, zas_znacky, NO_TEXTURIZE_TAGS)
        elif c.strip() == '':
            continue
        elif c[0] == '[' and sc and re.fullmatch(sc, c):
            if not c.startswith('[[') and not c.endswith(']]'):
                _pushpop(c, zas_sc, NO_TEXTURIZE_SHORTCODES)
            continue
        elif not zas_sc and not zas_znacky:
            c = _text(c)
        casti[i] = c
    return ''.join(casti)


if __name__ == '__main__':
    import sys
    sys.stdout.write(wptexturize(open(sys.argv[1], encoding='utf-8').read()))
