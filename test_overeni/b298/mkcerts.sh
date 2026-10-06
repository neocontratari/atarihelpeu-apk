#!/bin/sh
# B298: testovaci certifikaty: koren -> mezilehly -> list "atarihelp.eu" s AIA
# (adresa mezilehleho na http://127.0.0.1:$1/inter.der) + cizi list (jina CA)
set -e
P=${1:-18765}
D=${2:-certs}
mkdir -p $D && cd $D
openssl req -x509 -newkey rsa:2048 -nodes -keyout root.key -out root.pem -days 3650 -subj "/CN=Test Root B298" \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign" 2>/dev/null
openssl req -newkey rsa:2048 -nodes -keyout inter.key -out inter.csr -subj "/CN=Test Inter R11" 2>/dev/null
printf "basicConstraints=critical,CA:TRUE,pathlen:0\nkeyUsage=critical,keyCertSign,cRLSign\n" > inter.ext
openssl x509 -req -in inter.csr -CA root.pem -CAkey root.key -CAcreateserial -out inter.pem -days 3650 -extfile inter.ext 2>/dev/null
openssl x509 -in inter.pem -outform DER -out inter.der
openssl req -newkey rsa:2048 -nodes -keyout leaf.key -out leaf.csr -subj "/CN=atarihelp.eu" 2>/dev/null
printf "basicConstraints=CA:FALSE\nkeyUsage=critical,digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\nsubjectAltName=DNS:atarihelp.eu\nauthorityInfoAccess=caIssuers;URI:http://127.0.0.1:$P/inter.der\n" > leaf.ext
openssl x509 -req -in leaf.csr -CA inter.pem -CAkey inter.key -CAcreateserial -out leaf.pem -days 365 -extfile leaf.ext 2>/dev/null
# cizi CA (neni v duvere) se stejnym AIA
openssl req -x509 -newkey rsa:2048 -nodes -keyout evil.key -out evil.pem -days 3650 -subj "/CN=Test Inter R11" \
  -addext "basicConstraints=critical,CA:TRUE" 2>/dev/null
openssl req -newkey rsa:2048 -nodes -keyout eleaf.key -out eleaf.csr -subj "/CN=atarihelp.eu" 2>/dev/null
openssl x509 -req -in eleaf.csr -CA evil.pem -CAkey evil.key -CAcreateserial -out eleaf.pem -days 365 -extfile leaf.ext 2>/dev/null
echo hotovo
