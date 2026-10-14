#!/usr/bin/env python3
"""B308: Doom na PS1 - kontrola opravy "Enhanced Resolution" v gpu_neon.

Prelozi gpu_doom.c se SKUTECNYM kodem appky (vendor/pcsx_rearmed: gpulib +
gpu_neon) dvakrat - s opravou (soucasny strom) a bez ni (psx_gpu_parse.c
z B307, commit 64e47dd) - pro PC (x86, SSE) a pro telefon (arm64, NEON pres
qemu-aarch64), spusti sceny a porovna:

  1. doom, doom_venku - PSX Doom v mistnosti a venku (nebe): s opravou obraz
               2x pokryva presne to, co puvodni PS1 (strop, podlaha, zdi bez
               der a bez cizich barev); bez opravy diry (chyba z hlaseni)
  2. tenke   - nahodne tenke primitivy "jako Doom": s opravou bez der i bez
               bodu navic proti puvodnimu PS1
  3. nahodne - primitivy ostatnich her (i tenke kusy 3D siti): s opravou
               i bez ni BIT PO BITU STEJNE
  4. arm64 (NEON) = x86 (SSE) bit po bitu (stejny vysledek jako v telefonu)
  5. rychlost: kolik stoji oprava (Doom, 2x)

Pouziti: python3 kontrola_doom.py PRACOVNI_ADRESAR
"""
import os
import re
import shutil
import subprocess
import sys
import time

import numpy as np
from PIL import Image

KOREN = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
V_REL = "app/src/main/cpp/vendor/pcsx_rearmed"
PARSE_REL = V_REL + "/plugins/gpu_neon/psx_gpu/psx_gpu_parse.c"
STARY_COMMIT = "64e47dd"          # B307 - posledni verze pred opravou
HARNESS = os.path.join(KOREN, "test_overeni/b308/gpu_doom.c")

chyb = 0
kontrol = 0


def ok(podminka, text):
    global chyb, kontrol
    kontrol += 1
    if not podminka:
        chyb += 1
    print(("OK     " if podminka else "CHYBA  ") + text, flush=True)


def preloz(strom, vystup, arm):
    cc = "aarch64-linux-gnu-gcc" if arm else "gcc"
    arch = ["-static"] if arm else ["-msse4.2"]
    v = strom
    # stejne prepinace jako appka (CMakeLists.txt: napps1core) - -O2, -ffast-math, SIMD gpu_neon
    cmd = [cc, "-O2", "-w", *arch, "-ffast-math", "-fno-strict-aliasing",
           "-DGPU_NEON", "-DNEON_BUILD", "-DSIMD_BUILD", "-DTEXTURE_CACHE_4BPP", "-DTEXTURE_CACHE_8BPP", "-DNDEBUG",
           "-I" + v, "-I" + v + "/include", "-I" + v + "/plugins/gpu_neon", "-I" + v + "/plugins/gpu_neon/psx_gpu",
           "-o", vystup, HARNESS,
           v + "/plugins/gpulib/gpu.c", v + "/plugins/gpulib/vout_pl.c", v + "/plugins/gpulib/prim.c",
           v + "/plugins/gpu_neon/psx_gpu_if.c", v + "/plugins/gpu_neon/psx_gpu/psx_gpu_simd.c", "-lm"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-3000:])
        raise SystemExit("preklad selhal: " + vystup)


def spust(binar, arm, raw, scena, enh, texadj, snimku=None):
    cmd = (["qemu-aarch64"] if arm else []) + [binar, raw, scena, str(enh), str(texadj)]
    if snimku:
        cmd.append(str(snimku))
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    if r.returncode != 0:
        print(r.stdout, r.stderr)
        raise SystemExit("beh selhal: " + " ".join(cmd))
    with open(raw, "rb") as f:
        hl = np.frombuffer(f.read(16), dtype="<u4")
        assert hl[0] == 0x5250414E, "spatna hlavicka"
        w, h = int(hl[1]), int(hl[2])
        px = np.frombuffer(f.read(w * h * 2), dtype="<u2").reshape(h, w)
    return px, r.stdout


def png(px, jmeno):
    r = (px & 31).astype(np.uint8) << 3
    g = ((px >> 5) & 31).astype(np.uint8) << 3
    b = ((px >> 10) & 31).astype(np.uint8) << 3
    Image.fromarray(np.dstack([r, g, b])).save(jmeno)


def pokryti(px):
    return (px & 0x7FFF) != 0


def zvetsi(nat):
    return np.repeat(np.repeat(nat, 2, axis=0), 2, axis=1)


def diry(nat, enh, maska=None):
    """Puvodni PS1 (nat, 256x240) zvetseny 2x proti obrazu 2x (enh, 512x480).
    Vraci (diry %, navic %) - diry = PS1 tam kreslil a 2x obraz je cerny."""
    n2 = pokryti(zvetsi(nat))
    e = pokryti(enh)
    if maska is not None:
        n2 = n2 & maska
        e = e & maska
    zaklad = max(int(n2.sum()), 1)
    return 100.0 * float((n2 & ~e).sum()) / zaklad, 100.0 * float((e & ~n2).sum()) / zaklad


def spatne_barvy(nat, enh, maska):
    """% bodu 2x obrazu, jejichz barva se lisi od bodu PS1 o vic nez 2 stupne
    (z 31) v nektere slozce. Trochu jich je vzdy (2x obraz cte texturu jemneji),
    diry a prosvitajici pozadi jich pridaji hodne."""
    n2 = zvetsi(nat)
    d = np.maximum.reduce([np.abs(((n2 >> s) & 31).astype(int) - ((enh >> s) & 31).astype(int)) for s in (0, 5, 10)])
    return 100.0 * float(((d > 2) & maska).sum()) / max(int(maska.sum()), 1)


def oblasti_doom(venku):
    """Masky oblasti obrazu 512x480 podle sceny doom (souradnice PS1 x 2)."""
    yy, xx = np.mgrid[0:480, 0:512]
    ny, nx = yy // 2, xx // 2
    zed = ((nx >= 96) & (nx < 160) & (ny >= 60) & (ny < 141)) | ((nx >= 10) & (nx < 60) & (ny >= 40) & (ny < 151)) \
        | ((nx >= 200) & (nx < 250) & (ny >= 30) & (ny < 161))
    postavy = ((nx >= 110) & (nx < 150) & (ny >= 95) & (ny < 160)) | ((nx >= 30) & (nx < 70) & (ny >= 80) & (ny < 150))
    jine = zed | postavy
    o = {
        "STROP": (ny < 100) & ~jine,
        "PODLAHA": (ny >= 100) & (ny < 200) & ~jine,
        "ZDI": zed & ~postavy,
        "POSTAVY": postavy,
        "STAVOVY RADEK": ny >= 200,
    }
    if venku:
        o["STROP"] = (ny < 100) & (nx < 150) & ~jine
        o["NEBE"] = (ny < 28) & (nx >= 180) & ~jine
    return o


def main():
    prac = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "prac_b308")
    os.makedirs(prac, exist_ok=True)

    # strom bez opravy: kopie vendor/pcsx_rearmed + psx_gpu_parse.c z B307
    stary = os.path.join(prac, "strom_b307")
    if os.path.exists(stary):
        shutil.rmtree(stary)
    shutil.copytree(os.path.join(KOREN, V_REL), stary)
    obsah = subprocess.run(["git", "-C", KOREN, "show", STARY_COMMIT + ":" + PARSE_REL],
                           capture_output=True, check=True).stdout
    with open(os.path.join(stary, "plugins/gpu_neon/psx_gpu/psx_gpu_parse.c"), "wb") as f:
        f.write(obsah)
    novy = os.path.join(KOREN, V_REL)
    with open(os.path.join(novy, "plugins/gpu_neon/psx_gpu/psx_gpu_parse.c"), "rb") as f:
        ok(b"do_thin_triangle_enhanced" in f.read() and b"do_thin_triangle_enhanced" not in obsah,
           "oprava je v soucasnem psx_gpu_parse.c, v B307 (%s) neni" % STARY_COMMIT)

    binary = {}
    for jm, strom in (("novy", novy), ("stary", stary)):
        for arm in (False, True):
            b = os.path.join(prac, "gpu_doom_%s_%s" % (jm, "arm64" if arm else "x86"))
            preloz(strom, b, arm)
            binary[(jm, arm)] = b
    print("prelozeno: x86 (SSE) a arm64 (NEON, qemu-aarch64), s opravou i bez ni", flush=True)

    vysl = {}

    def obraz(jm, arm, scena, enh, texadj=1):
        k = (jm, arm, scena, enh, texadj)
        if k not in vysl:
            raw = os.path.join(prac, "%s_%s_%s_enh%d_adj%d.raw" % (scena, jm, "arm64" if arm else "x86", enh, texadj))
            vysl[k] = spust(binary[(jm, arm)], arm, raw, scena, enh, texadj)
        return vysl[k][0]

    # ---- 1. PSX Doom ----
    for scena, venku in (("doom", False), ("doom_venku", True)):
        print("\n== 1. PSX Doom %s ==" % ("venku: nebe, strop jen cast, podlaha, zdi, postavy, stavovy radek" if venku
                                         else "v mistnosti: strop, podlaha, zdi, postavy, stavovy radek"))
        nat = obraz("novy", False, scena, 0)
        ok(nat.shape == (240, 256), "puvodni PS1 (bez Enhanced): obraz %dx%d" % (nat.shape[1], nat.shape[0]))
        ok(np.array_equal(nat, obraz("stary", False, scena, 0)), "bez Enhanced: s opravou i bez ni bit po bitu stejne")
        png(nat, os.path.join(prac, "%s_ps1.png" % scena))
        for adj in (1, 0):
            st = obraz("stary", False, scena, 1, adj)
            nv = obraz("novy", False, scena, 1, adj)
            ok(st.shape == (480, 512) and nv.shape == (480, 512), "Enhanced (tex_adj %d): obraz 512x480" % adj)
            png(st, os.path.join(prac, "%s_2x_bez_opravy_adj%d.png" % (scena, adj)))
            png(nv, os.path.join(prac, "%s_2x_s_opravou_adj%d.png" % (scena, adj)))
            for nazev, m in oblasti_doom(venku).items():
                d_st, _ = diry(nat, st, m)
                d_nv, n_nv = diry(nat, nv, m)
                b_st, b_nv = spatne_barvy(nat, st, m), spatne_barvy(nat, nv, m)
                print("   tex_adj %d  %-14s BEZ opravy: diry %5.1f %%, jina barva nez PS1 %5.1f %%   "
                      "S opravou: diry %4.2f %%, navic %4.2f %%, jina barva %4.1f %%"
                      % (adj, nazev, d_st, b_st, d_nv, n_nv, b_nv))
                if nazev in ("STROP", "PODLAHA", "ZDI"):
                    ok(b_st > 15.0, "bez opravy (B307) %s ve 2x rozbity: %.1f %% bodu jinak nez PS1 (to videl Rene)"
                       % (nazev, b_st))
                    ok(b_nv < 10.0 and b_nv * 2.5 < b_st, "s opravou %s ve 2x jako PS1: jen %.1f %% bodu jinak "
                       "(jemnejsi textura ve 2x)" % (nazev, b_nv))
                if not venku and nazev in ("STROP", "PODLAHA"):
                    ok(d_st > 15.0, "bez opravy %s v mistnosti: %.1f %% cerne diry" % (nazev, d_st))
                ok(d_nv < 0.5 and n_nv < 0.5, "s opravou %s bez der (%.2f %%) a bez bodu navic (%.2f %%)"
                   % (nazev, d_nv, n_nv))
                if nazev in ("STAVOVY RADEK", "NEBE"):
                    ok(b_nv < 0.5 and b_st < 0.5, "%s (sprity) ve 2x stejne jako PS1 s opravou i bez ni" % nazev)

    # ---- 2. tenke primitivy jako v Doomu ----
    print("\n== 2. Tenke primitivy jako v Doomu (radky, sloupce, spicky, ctyruhelniky vysoke 1 bod) ==")
    nat = obraz("novy", False, "tenke", 0)
    ok(np.array_equal(nat, obraz("stary", False, "tenke", 0)), "bez Enhanced: s opravou i bez ni bit po bitu stejne")
    for adj in (1, 0):
        st = obraz("stary", False, "tenke", 1, adj)
        nv = obraz("novy", False, "tenke", 1, adj)
        d_st, n_st = diry(nat, st)
        d_nv, n_nv = diry(nat, nv)
        print("   tex_adj %d: BEZ opravy diry %.1f %%, navic %.1f %%;  S opravou diry %.2f %%, navic %.2f %%"
              % (adj, d_st, n_st, d_nv, n_nv))
        ok(d_st > 10.0, "tex_adj %d: bez opravy tenke primitivy derave (%.1f %%)" % (adj, d_st))
        ok(d_nv < 0.5 and n_nv < 0.5, "tex_adj %d: s opravou pokryti jako puvodni PS1 (diry %.2f %%, navic %.2f %%)"
           % (adj, d_nv, n_nv))
        if adj == 1:
            png(st, os.path.join(prac, "tenke_2x_bez_opravy.png"))
            png(nv, os.path.join(prac, "tenke_2x_s_opravou.png"))
            png(nat, os.path.join(prac, "tenke_ps1.png"))

    # ---- 3. ostatni hry beze zmeny ----
    print("\n== 3. Primitivy ostatnich her (normalni i tenke kusy 3D siti) ==")
    obraz("novy", False, "nahodne", 1)
    m = re.search(r"tenkych_jinych=(\d+)", vysl[("novy", False, "nahodne", 1, 1)][1])
    tenkych = int(m.group(1)) if m else 0
    ok(tenkych > 500, "ve scene je %d tenkych trojuhelniku jinych her (textura napric meni / bez textury)" % tenkych)
    for enh in (0, 1):
        for adj in ((1, 0) if enh else (1,)):
            st = obraz("stary", False, "nahodne", enh, adj)
            nv = obraz("novy", False, "nahodne", enh, adj)
            ok(np.array_equal(st, nv), "enh %d tex_adj %d: s opravou i bez ni BIT PO BITU STEJNE (%dx%d, %d bodu kresby)"
               % (enh, adj, nv.shape[1], nv.shape[0], int(pokryti(nv).sum())))

    # ---- 4. telefon (arm64 NEON) = PC ----
    print("\n== 4. arm64 (NEON - jako v telefonu, pres qemu-aarch64) ==")
    for scena in ("doom", "doom_venku", "tenke", "nahodne"):
        for jm in ("novy", "stary"):
            for enh in (0, 1):
                a = obraz(jm, True, scena, enh)
                x = obraz(jm, False, scena, enh)
                ok(np.array_equal(a, x), "%s, %s, enh %d: arm64 = x86 bit po bitu" %
                   (scena, "s opravou" if jm == "novy" else "bez opravy", enh))

    # ---- 5. rychlost ----
    print("\n== 5. Rychlost (x86, Doom v mistnosti, 2x, 600 snimku) ==")
    casy = {}
    for jm in ("stary", "novy"):
        nejlepsi = 1e9
        for _ in range(3):
            t0 = time.perf_counter()
            spust(binary[(jm, False)], False, os.path.join(prac, "rychlost_%s.raw" % jm), "doom", 1, 1, 600)
            nejlepsi = min(nejlepsi, time.perf_counter() - t0)
        casy[jm] = nejlepsi
    print("   bez opravy %.2f ms/snimek, s opravou %.2f ms/snimek (vcetne puvodniho 1x obrazu)"
          % (casy["stary"] / 600 * 1000, casy["novy"] / 600 * 1000))
    ok(casy["novy"] < casy["stary"] * 1.6, "oprava stoji %+.0f %% casu kresleni Doomu (snimek pod 1/60 s i tak)"
       % ((casy["novy"] / casy["stary"] - 1) * 100))

    print("\nVYSLEDEK: %d kontrol, %d CHYB" % (kontrol, chyb))
    return 1 if chyb else 0


if __name__ == "__main__":
    sys.exit(main())
