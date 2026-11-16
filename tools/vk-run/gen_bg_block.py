#!/usr/bin/env python3
"""Builds the 384-byte constant block of glass_background_sdf from the values a real Mac sets
(captures/<ver>/<scene>.txt, written by tools/glass-probe), following what
CA::OGL::GlassBackgroundFilter::render stores (docs/APPLE-DECISIONS.md).  BSD-2-Clause.
usage: gen_bg_block.py CAPTURE.txt OUT.bin [W H halfW halfH radius]"""
import re, struct, sys
import numpy as np
cap, out = sys.argv[1], sys.argv[2]
W, H = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (512, 512)
hx, hy, rad = (float(x) for x in sys.argv[5:8]) if len(sys.argv) > 7 else (170, 100, 60)
K = {}; on = False
for l in open(cap):
    if re.match(r'\s*filters: glassBackground\s*$', l): on = True; continue
    if on:
        m = re.match(r'\s+input(\w+) = (.*)$', l.rstrip('\n'))
        if not m: break
        K[m.group(1)] = m.group(2)
def f(k, d=0.0):
    v = K.get(k); 
    if v is None or v == 'nil': return d
    try: return float(v)
    except ValueError: return d
def color(k):
    v = K.get(k)
    if not v or v == 'nil': return (0, 0, 0, 0)
    n = re.search(r'\(\s*([-\d.e ]+)\)\s*$', v); return tuple(float(x) for x in n.group(1).split()) if n else (0, 0, 0, 0)
def ycc_matrix(white, black, sat, fill):   # set_ycc_composite: rgb -> ycc (BT.709), Y' = (w-b)Y + b, CbCr' = sat about 0.5, back to rgb, * (1-fill.a) + fill
    R2Y = np.array([[.2126, .7152, .0722], [-.1146, -.3854, .5], [.5, -.4542, -.0458]]); o1 = np.array([0, .5, .5])
    Y2R = np.array([[1, 0, 1.5748], [1, -.1873, -.4681], [1, 1.8556, 0]]); o2 = np.array([-.7874, .3277, -.9278])
    D = np.diag([white - black, sat, sat]); od = np.array([black, .5 - .5 * sat, .5 - .5 * sat])
    M = Y2R @ D @ R2Y; t = Y2R @ (D @ o1 + od) + o2
    fa = fill[3]; return M * (1 - fa), t * (1 - fa) + np.array(fill[:3])
def halves(M, t):
    return b''.join(struct.pack('<4e', *M[r], t[r]) for r in range(3))
s = 1.6   # scale * alpha * 1.6 of the CPU code, with scale = alpha = 1
bleed_fn = f('BleedBlurRadius', 100) * 0.5 if f('BleedOpacity', 1) > 0 else 0.0
shadow_fn = f('ShadowBlurRadius', 25) * 0.5 if f('ShadowOpacity', 1) > 0 else 0.0
def inv(x): return 1.0 / x if x else float('inf')
b = bytearray(384)
def put(off, *v): struct.pack_into('<%df' % len(v), b, off, *v)
put(0, hx, hy, 4.0, 0.0); put(16, 1, 0, 0, 1); put(32, 0, 0, rad, 0)           # shape (kind 4 = continuous corners)
put(48, 1.0 / W, 0, 0, 1.0 / H)                                                   # layer matrix in uv units
put(64, f('InnerRefractionAmount'), inv(f('InnerRefractionHeight')))
put(72, f('OuterRefractionAmount'), inv(f('OuterRefractionHeight')))
put(80, f('RefractionDistance0'), f('RefractionDistance1'))
put(88, f('BlurRadius') * s, bleed_fn * s)
put(96, f('BleedAmount'), inv(f('BleedHeight')))
put(104, f('ShadowAmount'), inv(f('ShadowHeight')))
sx, sy = re.findall(r'-?[\d.]+', K.get('ShadowOffset', '{0, 0}'))[-2:] if 'ShadowOffset' in K else (0, 0)
put(112, -float(sx) / W, -float(sy) / H)                                          # shadow offset in uv units
put(120, shadow_fn * s, inv(f('ShadowRadius')))
b[128:152] = halves(*ycc_matrix(f('FaceColorMatrixWhite', 1), f('FaceColorMatrixBlack'), f('FaceColorMatrixSaturation', 1), color('FaceColorMatrixFillColor')))
b[152:176] = halves(*ycc_matrix(f('BleedColorMatrixWhite', 1), f('BleedColorMatrixBlack'), f('BleedColorMatrixSaturation', 1), color('BleedColorMatrixFillColor')))
b[176:200] = halves(*ycc_matrix(f('ShadowColorMatrixWhite', 1), f('ShadowColorMatrixBlack'), f('ShadowColorMatrixSaturation', 1), color('ShadowColorMatrixFillColor')))
put(200, f('ShadowVibrancyContribution'), color('ShadowColorMatrixFillColor')[3])
o = [f('BlurOpacity%d' % i) for i in range(4)]
put(208, o[0], o[0] - o[1], o[1] - o[2], o[2] - o[3])
put(224, f('BlurDistance0'), f('BlurDistance1'), f('BlurDistance2'), f('BlurDistance3'))
put(240, f('BleedDistance0'), f('BleedDistance1'))
put(248, min(1.0, max(0.0, f('BleedOpacity'))), min(1.0, max(0.0, f('FaceOpacity'))))
dk = f('BleedDarkenBlend') != 0
put(256, 1.0 if dk else -1.0, 0.0 if dk else 1.0)
put(264, f('ShadowDistanceOffset'), min(1.0, max(0.0, f('ShadowOpacity'))))
put(272, f('RefractionOpacity'), 0.0)
put(280, f('SDRGradientDistance0'), f('SDRGradientDistance1'))
put(288, f('Clamp'), f('ClampPreserveHue'), f('SDRHoldingToneWhite', 1), 1.0)
put(304, 1.0)
open(out, 'wb').write(bytes(b)); print('wrote', out, len(b), 'bytes')
