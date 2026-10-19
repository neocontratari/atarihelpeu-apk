import re,os,sys,glob
R='/home/claude/atarihelpeu-apk/app/src/main'
java=open(R+'/java/eu/atarihelp/emu10/MainActivity.java',encoding='utf-8').read()
# rozhrani -> trida
reg=dict(re.findall(r'addJavascriptInterface\(new (\w+)\(\), "(\w+)"\)',java))
iface={v:k for k,v in reg.items()}
# metody tridy: najit "public class X {" a vsechny @JavascriptInterface metody uvnitr (do dalsi "public class" na stejne urovni)
def metody(cls):
    m=re.search(r'\n    public class '+cls+r' \{',java)
    if not m: return None
    i=m.end(); hloubka=1; j=i
    while hloubka>0 and j<len(java):
        c=java[j]
        if c=='{': hloubka+=1
        elif c=='}': hloubka-=1
        j+=1
    telo=java[i:j]
    return set(re.findall(r'@JavascriptInterface\s*(?:public\s+)?(?:[\w\[\]<>]+\s+)+(\w+)\s*\(',telo))
volani={}
soubory=[f for f in glob.glob(R+'/assets/**/*.html',recursive=True)]+[f for f in glob.glob(R+'/assets/**/*.js',recursive=True)]
for f in soubory:
    t=open(f,encoding='utf-8',errors='replace').read()
    for a,b in re.findall(r'\b(AH[A-Za-z0-9]+)\.(\w+)\s*\(',t):
        volani.setdefault((a,b),set()).add(os.path.relpath(f,R+'/assets'))
for a,b in re.findall(r'\b(AH[A-Za-z0-9]+)\.(\w+)\s*\(',java):
    volani.setdefault((a,b),set()).add('MainActivity.java (HTML v Jave)')
chyb=0; ok=0
for (a,b),kde in sorted(volani.items()):
    if a not in iface: continue
    ms=metody(iface[a])
    if ms is None: print('TRIDA CHYBI',a,iface[a]); chyb+=1; continue
    if b in ms: ok+=1
    else:
        print('CHYBI METODA %s.%s (trida %s) volano z %s'%(a,b,iface[a],', '.join(sorted(kde)))); chyb+=1
print('rozhrani:',', '.join('%s=%s'%(k,v) for k,v in sorted(iface.items())))
print('volani mostu z JS: %d v poradku, %d chybi'%(ok,chyb))
