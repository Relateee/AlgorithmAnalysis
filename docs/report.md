# Seam Carving: Analysis and Discussion

## 1. Implementation overview
Question 1 is implemented in the UI-independent `seamcore` library. `Energy.cpp` converts
BGR input to grayscale intensity and computes either central-difference energy
`|I(r,c+1)-I(r,c-1)| + |I(r+1,c)-I(r-1,c)|` or the absolute sum of two handwritten
3x3 Sobel responses. Both replicate the nearest pixel at image boundaries.

`DPSeamFinder.cpp` initializes cumulative costs from the first energy row. Each later
pixel adds its energy to the minimum reachable cost in the preceding row (up-left,
up or up-right). A parent table records the chosen predecessor. Backtracking from the
lowest cost in the final row returns one connected pixel per row. Equal costs select
the leftmost predecessor and final column deterministically.

`Carver.cpp` removes each selected pixel by copying the remaining pixels in that row.
It recomputes the energy map and seam after every removal. Horizontal finding transposes
the image and reuses the vertical algorithm; horizontal removal transposes back afterward.
Resizing removes all requested vertical seams, then all requested horizontal seams.
Targets must be positive and no larger than the original. These operations shrink images;
seam insertion is not implemented.

For an H by W image, energy computation, DP and removal each take O(HW) time per seam.
The baseline cost and parent tables each take O(HW) space. Rolling cost rows reduce cost
storage to O(W), with O(HW) parent storage still required for backtracking. Removing k
seams takes at most O(kHW) time, with smaller images in successive iterations. Each
individual backward-energy seam is optimal for the current image; the complete sequence
of removals and the chosen vertical-then-horizontal order are not claimed globally optimal.

The default mode minimizes the requested sum of pixel energies. An optional forward
mode uses the first-row pixel energy and then the CL/CU/CR transition costs documented
in `Energy.h`; it optimizes a different objective. `CarveStats.totalSeamCost` always reports
the sum of backward pixel energies, including when forward mode is enabled.

Validation includes exhaustive enumeration on small maps, both energy filters,
horizontal and vertical seams, exact retained pixels, boundary cases and a controlled
test that verifies energy is recalculated after each removal. See `tests/src/q1.cpp`.
Graph Cut, multi-threading and the remaining analysis below still require work.

### Greedy (Question 3)
`GreedySeamFinder.cpp` starts at the lowest-energy pixel of the top row and, in every
following row, moves to the cheapest of the (up to) three pixels below the current one.
Ties choose the leftmost candidate, as in DP. It keeps no table and never looks ahead, so it
cannot see that a cheap pixel leads into an expensive region. An optional variant
(`tryAllStarts`) runs the same rule from every top-row column and keeps the cheapest seam.
That costs O(HW) like DP but is still not optimal.

`tests/src/q3.cpp` checks the rule step by step, checks DP <= greedy(all starts) <= greedy
on 200 random energy maps, and includes a 3x3 counterexample where greedy pays 100,
greedy(all starts) 15 and DP 10.

## 2. Questions (Item 2)

### 2a(i) The number of seams grows at least exponentially in m
<!-- Hint: from any pixel there are at least 2 valid choices in the next row when n > 1. -->

### 2a(ii) Relating the problem to its subproblems
<!-- Recurrence M(i, j) = e(i, j) + min(...), base case, optimal substructure, answer = min of last row. -->

### 2b Can divide and conquer speed up convolution / gradient computation?
<!-- Direct convolution, separable kernels, FFT-based convolution (a D&C algorithm), when it helps for 3x3. -->

## 3. Time and space complexity
| Algorithm | Time per seam | Time for k seams | Space |
|---|---|---|---|
| Energy map | O(HW) | O(kHW) | O(HW) |
| DP | O(HW) | O(kHW) | O(HW) parents (+ O(W) costs with rolling rows) |
| Greedy | O(H) | O(kH), but O(kHW) overall because energy is recomputed | O(H) for the seam |
| Greedy, all starts | O(HW) | O(kHW) | O(H) |
| Graph cut (Edmonds–Karp) | | | |
| Graph cut (Dinic) | | | |

## 4. Comparison: DP vs Greedy vs Graph Cut
<!-- Seam cost, visual quality (images from results/), running time graphs from results/bench.csv. -->

### 4.1 DP vs Greedy (Question 3)
Measured with the Release build: 100 vertical seams removed, Gradient energy, two Windows
wallpapers scaled to three widths. Raw data and images are in `results/q3/`.

| Image | Size | DP total (ms) | Greedy total (ms) | DP find (ms) | Greedy find (ms) | DP seam cost | Greedy seam cost |
|---|---|---|---|---|---|---|---|
| img100 (waves) | 200x112 | 46.5 | 23.9 | 23.1 | 0.19 | 68,163 | 117,604 |
| img100 (waves) | 400x225 | 179.8 | 81.9 | 86.1 | 0.43 | 63,368 | 116,183 |
| img100 (waves) | 800x450 | 696.8 | 360.0 | 345.4 | 1.44 | 43,251 | 160,274 |
| img102 (sunset) | 200x125 | 48.0 | 28.6 | 24.6 | 0.21 | 58,963 | 112,496 |
| img102 (sunset) | 400x250 | 214.8 | 101.0 | 110.7 | 0.40 | 56,706 | 124,007 |
| img102 (sunset) | 800x500 | 866.7 | 390.6 | 472.0 | 2.35 | 62,771 | 191,015 |

Observations (draft, to be written up):
- Finding a seam is about 120-280x faster with greedy (O(H) vs O(HW)), but the whole carve is only
  about 2x faster, because recomputing the O(HW) energy map after every removal dominates.
- Greedy seams cost 1.7-3.7x more energy, and the gap grows with image size: more rows give
  more chances to be led into an expensive region.
- Visually (`img102-original-dp-greedy.png`), DP keeps the left mountain and the sun intact
  while greedy squashes the mountain.
- Lower energy is not always better-looking: on `img100` DP concentrates seams along the
  curved edges and leaves staircase artifacts, while greedy happens to look smoother.
  Backward energy ignores the new edges created by a removal; forward energy is designed
  to fix exactly this (worth showing as an improvement in section 5).

## 5. Improvements
### 5.1 Do we need the entire DP table?
### 5.2 Does multi-threading help?
### 5.3 Other improvements (local energy updates, forward energy, ...)

## References
- S. Avidan, A. Shamir. *Seam Carving for Content-Aware Image Resizing.* SIGGRAPH 2007.
- M. Rubinstein, A. Shamir, S. Avidan. *Improved Seam Carving for Video Retargeting.* SIGGRAPH 2008.
- Cormen et al. *Introduction to Algorithms*, Problem 15-8 (Image compression by seam carving).
