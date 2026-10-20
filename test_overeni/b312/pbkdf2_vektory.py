#!/usr/bin/env python3
# B312: vektory PBKDF2-HMAC-SHA256 z Pythonu (hashlib) pro JVM test NapServis.pbkdf2
# radek: heslo_hex sul_hex iteraci delka otisk_hex
#   python3 pbkdf2_vektory.py VYSTUP.txt
import hashlib
import random
import sys

r = random.Random(312)
radky = []
pevne = [(b"password", b"salt", 1, 32), (b"password", b"salt", 2, 32), (b"password", b"salt", 4096, 32),
         (b"passwordPASSWORDpassword", b"saltSALTsaltSALTsaltSALTsaltSALTsalt", 4096, 40),
         (b"pass\x00word", b"sa\x00lt", 4096, 16)]
for h, s, c, d in pevne:
    radky.append((h, s, c, d))
for i in range(40):
    h = bytes(r.randrange(33, 127) for _ in range(r.randrange(6, 33)))
    s = bytes(r.randrange(0, 256) for _ in range(r.choice([8, 16, 32])))
    c = r.choice([1, 2, 3, 10, 100, 1000, 5000])
    d = r.choice([16, 32, 33, 64, 65])
    radky.append((h, s, c, d))
with open(sys.argv[1], "w") as f:
    for h, s, c, d in radky:
        f.write("%s %s %d %d %s\n" % (h.hex(), s.hex(), c, d, hashlib.pbkdf2_hmac("sha256", h, s, c, d).hex()))
# ukazkovy kod (NENI skutecny servisni kod): sul + otisk pro test kodSedi
s = bytes(r.randrange(0, 256) for _ in range(16))
with open(sys.argv[1], "a") as f:
    f.write("UKAZKA TESTKOD234 %s 1000 %s\n" % (s.hex(), hashlib.pbkdf2_hmac("sha256", b"TESTKOD234", s, 1000, 32).hex()))
print("vektoru:", len(radky) + 1)
