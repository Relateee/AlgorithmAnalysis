# AlgorithmAnalysis: Seam Carving

Content-aware image resizing implemented three ways: **Dynamic Programming**, **Greedy**, and
**Graph Cut (min-cut / max-flow)**. OpenCV is used only for image I/O and basic Mat operations;
the seam-carving logic is implemented from scratch.

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

**Command line** (from a *Developer PowerShell for VS 2026*, which sets `VCPKG_ROOT`):
```
cmake --preset x64-release
cmake --build --preset x64-release
ctest --preset x64-release
```

Use the **Release** build for all timing measurements.

## Run the benchmark
```
out/build/x64-release/bench/seam_bench.exe --sizes 100,200,400 --seams 50 --save-images
```
