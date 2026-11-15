#!/usr/bin/env python3
"""WordPress wpautop() v Pythonu (podle wp-includes/formatting.php, WordPress 6.x) - jen to, co se tyka
stranky 1130: odstavce z prazdnych radku, <br /> z jednotlivych radku, ochrana novych radku ve znackach
(wp_replace_in_html_tags) a ve <script>/<style>. Neni tu <pre>, <option>, <object>, <audio>/<video>.

PROC: WordPress v klasickem editoru prozene obsah stranky pres wpautop (blok Vlastni HTML v Gutenbergu ne).
Prazdny radek UVNITR <script> muze skript rozdelit na dva "odstavce" a vlozit do nej </p> ... <p>
-> chyba JavaScriptu. POZOR: B322 15. 11. nerozbil wpautop (test: B322 i B321 jim projdou), ale
wptexturize (& -> &#038; mezi "<" a ">", viz wptexturize.py). Stranka 1130 proto nema prazdne radky
ani "<" a "&" ve skriptech - hlida test_web_wordpress.py.
"""
import re

ALLBLOCKS = (r'(?:table|thead|tfoot|caption|col|colgroup|tbody|tr|td|th|div|dl|dd|dt|ul|ol|li|pre|form|map|area|'
             r'blockquote|address|style|p|h[1-6]|hr|fieldset|legend|section|article|aside|hgroup|header|footer|nav|'
             r'figure|figcaption|details|menu|summary)')

# get_html_split_regex(): komentar, CDATA, nebo "<" az po nejblizsi ">" (i kdyz je to porovnani v JavaScriptu)
HTML_SPLIT = re.compile(r'(<(?:!--.*?(?:-->|$)|!\[CDATA\[.*?(?:\]\]>|$)|[^>]*>?))', re.S)


def wp_replace_in_html_tags(text, hledat, nahradit):
    casti = HTML_SPLIT.split(text)
    for i in range(1, len(casti), 2):
        if hledat in casti[i]:
            casti[i] = casti[i].replace(hledat, nahradit)
    return ''.join(casti)


def wpautop(text, br=True):
    if text.strip() == '':
        return ''
    text = text + '\n'
    text = re.sub(r'<br\s*/?>\s*<br\s*/?>', '\n\n', text)
    text = re.sub(r'(<' + ALLBLOCKS + r'[\s/>])', r'\n\n\1', text)
    text = re.sub(r'(</' + ALLBLOCKS + r'>)', r'\1\n\n', text)
    text = re.sub(r'(<hr\s*?/?>)', r'\1\n\n', text)
    text = text.replace('\r\n', '\n').replace('\r', '\n')
    text = wp_replace_in_html_tags(text, '\n', ' <!-- wpnl --> ')
    text = re.sub(r'\n\n+', '\n\n', text)
    odstavce = [x for x in re.split(r'\n\s*\n', text) if x != '']
    text = ''.join('<p>' + x.strip('\n') + '</p>\n' for x in odstavce)
    text = re.sub(r'<p>\s*</p>', '', text)
    text = re.sub(r'<p>([^<]+)</(div|address|form)>', r'<p>\1</p></\2>', text)
    text = re.sub(r'<p>\s*(</?' + ALLBLOCKS + r'[^>]*>)\s*</p>', r'\1', text)
    text = re.sub(r'<p>(<li.+?)</p>', r'\1', text)
    text = re.sub(r'<p><blockquote([^>]*)>', r'<blockquote\1><p>', text, flags=re.I)
    text = text.replace('</blockquote></p>', '</p></blockquote>')
    text = re.sub(r'<p>\s*(</?' + ALLBLOCKS + r'[^>]*>)', r'\1', text)
    text = re.sub(r'(</?' + ALLBLOCKS + r'[^>]*>)\s*</p>', r'\1', text)
    if br:
        text = re.sub(r'<(script|style|svg|math).*?</\1>', lambda m: m.group(0).replace('\n', '<WPPreserveNewline />'), text, flags=re.S)
        text = text.replace('<br>', '<br />').replace('<br/>', '<br />')
        text = re.sub(r'(?<!<br />)\s*\n', '<br />\n', text)
        text = text.replace('<WPPreserveNewline />', '\n')
    text = re.sub(r'(</?' + ALLBLOCKS + r'[^>]*>)\s*<br />', r'\1', text)
    text = re.sub(r'<br />(\s*</?(?:p|li|div|dl|dd|dt|th|pre|td|ul|ol)[^>]*>)', r'\1', text)
    text = re.sub(r'\n</p>$', '</p>', text)
    if '<!-- wpnl -->' in text:
        text = text.replace(' <!-- wpnl --> ', '\n').replace('<!-- wpnl -->', '\n')
    return text


if __name__ == '__main__':
    import sys
    sys.stdout.write(wpautop(open(sys.argv[1], encoding='utf-8').read()))
