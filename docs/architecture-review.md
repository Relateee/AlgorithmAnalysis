# Architecture and UI review (Question 1)

## Verdict

The existing C++/CMake structure is appropriate for this assignment. No rewrite is needed.
The original repository was a scaffold: energy calculation, DP and pixel removal were
placeholders, so a working-looking UI did not demonstrate correct seam carving.
Those Question 1 components are now implemented and covered by a dedicated test suite.

## Responsibilities

| Component | Responsibility | Assessment |
|---|---|---|
| `core/Energy` | Grayscale intensities, energy maps, optional forward costs | Shared by finders; handwritten Gradient and Sobel kernels |
| `core/DPSeamFinder` | Minimum cumulative cost and parent backtracking | Fits Question 1b; full and rolling cost tables return the same seam |
| `core/Carver` | Recalculate, find, remove, repeat; transpose for horizontal seams | Fits Question 1c and avoids duplicated horizontal DP |
| `core/SeamFinder` | Common interface and algorithm factory | Appropriate for later comparisons using identical energy and removal code |
| `app` | Input, settings, textures, previews and results | Kept outside the algorithm library; supports the Question 1 demonstration |
| `bench` | Release timings and CSV/image outputs | Useful foundation; restrict to DP until other finders work |
| `tests` | Algorithm and integration checks | Q1 suite is separate from original cross-algorithm checks |

Your teammate's implementation order is valid: energy map, seam removal, then DP.
All three are needed; horizontal seams already have a transpose wrapper, and the repeated
removal loop already recalculates energy. Those wrappers still need tests rather than
being assumed correct from their presence.

## Correctness and UI changes

- Implemented replicated-border central differences and Sobel energy, DP/backtracking,
  and seam removal that preserves every retained pixel, including noncontiguous inputs.
- Validated image sizes, seam counts, seam connectivity and target dimensions before removal.
- Fixed horizontal finding on square images: transposing a shallow alias could edit the original
  matrix in place. Horizontal finding now transposes into a separate matrix.
- Cleared the result texture when loading a new image; previously the old result remained visible.
- Added a normalized energy-map view and scaled images to fit the available panel.
- Disabled the unfinished Greedy, Graph Cut and threading controls. Their core scaffolds
  still need implementation, and direct requests for multi-threaded DP fail explicitly.
- Added a hidden-window UI smoke check that writes inputs, carved results, seam preview
  and a rendered screenshot for inspection.
- Added solution-generator test presets and documented the missing `VCPKG_ROOT` setup.

## Work remaining outside Question 1

- Questions 3/4: greedy, graph construction, Edmonds-Karp/Dinic and residual reachability.
  The original graph-cut TODO must also address border seams: pinning the first and last
  pixel to opposite sides excludes removal of the last column. A one-column input cannot
  be pinned to both sides with a finite cut. Check boundary representation before implementing it.
- Comparisons must use the same objective: the current graph-cut options default to forward
  energy while DP defaults to backward pixel energy. Forward transition costs also need
  deliberate graph modeling; a single horizontal edge cost does not encode all three DP transitions.
- Question 5: collect real measurements and visual comparisons. Rolling costs reduce the cost
  table to two rows but the parent table still takes O(height * width) memory. Threading is pending.
- UI: carving currently runs on the UI thread. A worker with progress/cancellation would improve
  large-image use. Preview shows only the first seam in each direction on the original;
  it does not animate every removed seam or meet the full step-by-step bonus requirement.
- Input assets and report are still scaffolds. The benchmark needs an explicit input image,
  and unfinished algorithm results must not be reported as comparisons.

## Verification

Run `ctest --preset vs2026-release -L q1` after building. The suite checks energy values,
960 combinations of dimensions/seeds/forward/rolling settings against exhaustive enumeration,
retained pixel contents, square-input immutability, both orientations, repeated recalculation,
1x1 targets and invalid requests. The full original suite remains enabled and is expected
to expose the unfinished max-flow and graph-cut implementations.

Run `seam_app.exe --smoke-test results/ui-smoke` to check the UI path and inspect generated PNGs.

Verified locally with Visual Studio 2026 Release:

| Check | Result |
|---|---|
| Configure and build (core, app, benchmark, both test executables) | Passed |
| Q1 suite | All six groups passed, including 960 DP option combinations |
| Original suite | 5/7 passed; max-flow and graph-cut groups fail on their existing placeholders |
| Hidden-window UI check | Passed; rendered screenshot inspected |
| DP benchmark on 64x43 and 96x64 demo inputs, ten removals each | Passed; CSV and resized PNGs generated |

Generated evidence is in `results/ui-smoke/` and `results/q1-smoke/` (ignored by Git).
These small synthetic smoke runs verify operation; they are not the full Question 5 evaluation.

## AI usage record for the final report

AI assistance was used to review the repository against Question 1, implement energy filters,
DP and seam removal, fix UI/state and transpose issues, add verification, and document the work.
The main user prompt asked to check the architecture/UI, verify the teammate's breakdown,
and complete the first question. Adapt this record to include subsequent assistance and the
team's own verification; it is not a complete declaration for the entire project.

Question 4 (added 2026-11-18): AI assistance was used to implement Edmonds-Karp, Dinic and
the residual min-cut search in `MaxFlow.cpp`, build the graph-cut network in
`GraphCutSeamFinder.cpp` as described in the brief, write `tests/src/q4.cpp`, enable Graph Cut
in the viewer, run the DP / greedy / graph-cut measurements and write sections 1 (Graph cut),
3 (graph-cut rows) and 4.2 of the report. Main prompts: "can u write them up" (asking for the
missing Question 4 graph cut and max-flow work after a review of the assignment against the
brief and rubric) and "can u remove all that my assignment doesnt ask for" (which removed
extras beyond the brief and switched to the brief's exact graph construction).
