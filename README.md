# AlgorithmAnalysis: Seam Carving

Content-aware image resizing implemented three ways: **Dynamic Programming**, **Greedy**, and
**Graph Cut (min-cut / max-flow)**. OpenCV is used only for image I/O and basic Mat operations;
the seam-carving logic is implemented from scratch.

**Current status:** Question 1 (DP, vertical and horizontal seams) and Question 3
(Greedy, including the try-all-start-columns variant) are implemented. Graph Cut and
max-flow are still scaffolds for Question 4, and the viewer disables that choice until it
is implemented. Forward energy and rolling cost rows work; multi-threaded DP remains pending. See [the architecture review](docs/architecture-review.md).

## Layout
| Folder | What |
|---|---|
| `core/` | `seamcore` library: energy, seam finders, max-flow, carver (no UI) |
| `app/` | ImGui + GLFW + OpenGL viewer (`seam_app`) |
| `bench/` | Benchmark CLI that writes CSV for the report (`seam_bench`) |
| `tests/` | Self-contained tests (`seam_tests`) |
| `assets/images/` | Input images (put `sample.jpg` here) |
| `results/` | Benchmark CSVs and output images |
| `docs/report.md` | Analysis and answers |

## Requirements
- Visual Studio 2026 with the **Desktop development with C++** workload (includes CMake, Ninja and vcpkg)
- Dependencies (OpenCV, GLFW, Dear ImGui) are installed automatically by vcpkg from `vcpkg.json`.
  The first configure compiles OpenCV and takes a while.

## Build
**Visual Studio:** File → Open → Folder… → select this folder, pick the `x64 Debug` or
`x64 Release` preset, then Build → Build All. Choose `seam_app.exe` as the startup item.

**Command line** (from a *Developer PowerShell for VS 2026*, with `VCPKG_ROOT` set):
```
cmake --preset x64-release
cmake --build --preset x64-release
ctest --preset x64-release
```

Use the **Release** build for all timing measurements.

If `VCPKG_ROOT` is not set, set it for the current PowerShell session before configuring:
```powershell
$env:VCPKG_ROOT = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg'
```
Adjust that path for your Visual Studio installation. A Developer PowerShell provides
the compiler environment but may not set `VCPKG_ROOT`.

The solution preset also works from an ordinary PowerShell after setting `VCPKG_ROOT`:
```powershell
cmake --preset vs2026
cmake --build --preset vs2026-release
ctest --preset vs2026-release -L q1
```
The Question 1 suite checks 960 DP option combinations against exhaustive search,
Sobel against an independent filter implementation, exact retained pixels, horizontal
seams, repeated energy recalculation, small images and invalid inputs.
Running CTest without `-L q1` also runs the original suite: its max-flow and graph-cut
checks are expected to fail until Questions 3/4 are completed. They remain enabled
so unfinished work is visible.

## Try Question 1

Run `build/vs2026/app/Release/seam_app.exe` (or the equivalent Ninja build path).
Load an image using Browse, a path, or drag-and-drop; the demo image works without assets.
Select Gradient or Sobel and inspect the Energy map tab. Preview seams shows the first
vertical seam in red and horizontal seam in blue on the original. Reduce Width and/or
Height, click Carve, and open Result. Each carve starts from the original image and
removes all requested vertical seams before the horizontal ones. Leave Forward energy
off for the Question 1 minimum-sum-of-pixel-energies objective.

A hidden-window UI check exercises the actual loading, preview, texture, resizing and
rendering code and saves example outputs and a screenshot:
```powershell
build/vs2026/app/Release/seam_app.exe --smoke-test results/ui-smoke
```
It requires an OpenGL-capable desktop. Interactive carving is synchronous, so large
resizes can temporarily stop the window responding. Step-by-step animation and saving
from the viewer remain future enhancements.

## Run the benchmark
```
out/build/x64-release/bench/seam_bench.exe --image path/to/photo.jpg --algos dp --sizes 100,200,400 --seams 50 --save-images
```
Supply an image: `assets/images/sample.jpg` is not bundled. Use `--algos dp,greedy` until
Graph Cut is implemented; its current placeholder produces invalid comparison results.
Run the Question 3 tests with `ctest --preset x64-release -L q3`. In the viewer, pick Greedy
and click Preview seams to see its seams (red/blue) over the DP seams (green), with both costs. For the solution preset, use `build/vs2026/bench/Release/seam_bench.exe`.
