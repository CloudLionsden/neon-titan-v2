# Picks the presets that actually SOUND different.
# Input: CSV from "TitanRender feat" (index,category,subtype,feature...) rendered in pool mode.
# Output: Source/PresetKeep.h with the pool indices to keep.
#
# Each sound's fingerprint (24-band tone colour + brightness, movement, noisiness, attack, sustain,
# modulation rate/depth, stereo width) is normalised, then a candidate is kept only if it is further
# than a distance threshold from every sound already kept in the same category.
import csv, sys, random
import numpy as np

CLASSICS = 12
rows = [r for r in csv.reader(open(sys.argv[1])) if int(r[0]) >= CLASSICS]
target = int(sys.argv[2]) if len(sys.argv) > 2 else 2300
idx = np.array([int(r[0]) - CLASSICS for r in rows])
cat = [r[1] for r in rows]
sub = [r[2] for r in rows]
X = np.array([[float(v) for v in r[3:]] for r in rows])

mu, sd = X.mean(0), X.std(0) + 1e-6
Z = np.clip((X - mu) / sd, -4, 4)
w = np.ones(Z.shape[1]); w[24:] = 1.5          # tone-colour bands vs. behaviour features, roughly equal say
Z = Z * w

# Interleave recipes so every style gets an early chance (fixed seed = same result every time)
rng = random.Random(1234)
by_sub = {}
for i, s in enumerate(sub): by_sub.setdefault((cat[i], s), []).append(i)
for v in by_sub.values(): rng.shuffle(v)
order, k = [], 0
while any(k < len(v) for v in by_sub.values()):
    for v in by_sub.values():
        if k < len(v): order.append(v[k])
    k += 1

def greedy(T):
    # Compare each candidate with the kept sounds in its OWN category
    kept, K = [], {}
    for i in order:
        c = cat[i]
        if c not in K or np.min(np.sum((K[c] - Z[i]) ** 2, axis=1)) > T * T:
            kept.append(i); K[c] = Z[i][None, :] if c not in K else np.vstack([K[c], Z[i]])
    return kept

lo, hi = 0.5, 12.0
for _ in range(18):
    mid = (lo + hi) / 2
    n = len(greedy(mid))
    if n > target: lo = mid
    else: hi = mid
T = (lo + hi) / 2
kept = sorted(greedy(T), key=lambda i: idx[i])

# Report
nn = []
for i in range(0, len(Z), 7):
    d = np.sum((Z - Z[i]) ** 2, axis=1); d[i] = 1e18; nn.append(np.sqrt(d.min()))
print(f"pool {len(rows)}  kept {len(kept)}  threshold {T:.2f}  (typical nearest-neighbour distance in the pool: {np.median(nn):.2f})")
from collections import Counter
cc = Counter(cat[i] for i in kept); pc = Counter(cat)
for c in pc: print(f"  {c:10s} kept {cc[c]:4d} of {pc[c]:4d}")
sc = Counter((cat[i], sub[i]) for i in kept)
low = [f"{c}/{s}:{sc[(c,s)]}" for (c, s) in by_sub if sc[(c, s)] < 8]
if low: print("  styles with fewer than 8 kept (they mostly sounded alike):", ", ".join(low))

with open("Source/PresetKeep.h", "w") as f:
    f.write("#pragma once\n// Indices into the candidate pool that passed the audio de-duplication (Tools/dedupe.py).\n")
    f.write(f"// {len(kept)} kept of {len(rows)} candidates; distance threshold {T:.2f}.\n")
    f.write(f"static constexpr int kPresetKeepCount = {len(kept)};\n")
    f.write("static constexpr int kPresetKeep[] = {" + ",".join(str(int(idx[i])) for i in kept) + "};\n")
