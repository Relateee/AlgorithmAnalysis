# Seam Carving: Analysis and Discussion

## 1. Implementation overview
<!-- Energy function(s), DP, greedy and graph-cut implementations; how horizontal seams are handled. -->

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
