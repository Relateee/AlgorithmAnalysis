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
Greedy, Graph Cut, multi-threading and the remaining analysis below still require work.

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
| Energy map | | | |
| DP | | | |
| Greedy | | | |
| Graph cut (Edmonds–Karp) | | | |
| Graph cut (Dinic) | | | |

## 4. Comparison: DP vs Greedy vs Graph Cut
<!-- Seam cost, visual quality (images from results/), running time graphs from results/bench.csv. -->

## 5. Improvements
### 5.1 Do we need the entire DP table?
### 5.2 Does multi-threading help?
### 5.3 Other improvements (local energy updates, forward energy, ...)

## References
- S. Avidan, A. Shamir. *Seam Carving for Content-Aware Image Resizing.* SIGGRAPH 2007.
- M. Rubinstein, A. Shamir, S. Avidan. *Improved Seam Carving for Video Retargeting.* SIGGRAPH 2008.
- Cormen et al. *Introduction to Algorithms*, Problem 15-8 (Image compression by seam carving).
