# Image Processing – Assignment 2

C++17 / Qt / CMake. All algorithms implemented **from scratch** (Qt is used only for the window and for reading/writing image files).

* Task 1: Canny edge detector, Hough lines / circles / ellipses (shapes superimposed on the image)
* Task 2: Active contour (greedy snake), chain code, perimeter & area

See **COMMANDS.md** for build & run instructions.

## Status
- [x] Canny edge detector + Hough Lines
- [x] Hough Circles
- [x] Ellipses (Xie & Ji 1D Hough + outline-support check)
- [x] Active Contour (greedy snake)
- [x] Chain code (Freeman 8-direction), perimeter and area (from the polygon *and* from the chain code), GUI integration
- [x] Command-line runner (`ipcli`) + `tools/run_all.sh` that regenerate every result
- [x] Test images for every task (`images/input/`), results in `results/cli_output/`, write-up in **REPORT.md**

## GUI
Web-style light/dark interface (light by default) (indigo/violet theme, a colour per feature).

* **One page per feature** in the left sidebar, each with its own parameters (slider + number box + a short
  explanation), its own Run button and a "Last result" box. Pages marked *In development* are the parts that are
  still stubs.
* **Workspace tab** - original and result side by side (mouse wheel = zoom, drag = pan, double-click = reset;
  both images zoom together) Drag & drop an image onto the window to open it.
  On the "Active Contour" page, click the Original image to set the snake's starting centre.
* **Compare tab** - a gallery of every result ever produced (all images, all features, all parameters).
  Click cards to put them side by side (zoom/pan are linked), filter by image or feature, and press
  **Use these settings** to jump back to a feature page with exactly the parameters of that result.
* **Everything is saved** in `results/` (PNG files + `library.json` with the parameters, stats and log of each run).
  Running the same image with the same settings again just loads the saved picture - nothing is recomputed
  (untick *Reuse saved results* in the sidebar to force a new run). The last image, theme and all parameter values
  are remembered between sessions, and the *Saved images...* list brings back any image you already used.
* Heavy work runs on a background thread, so the window never freezes.

All features are implemented; the `isReady()` flags at the top of `src/gui/MainWindow.cpp` are all `true`, so no
page is marked *In development* any more. The Chain Code page also writes the **complete** chain code to
`results/<image>_..._chaincode.txt` next to the saved result image (the on-screen log only shows the first 400 moves).

## Command line (no window)
`ipcli` runs every algorithm on an image and writes the superimposed results, the chain code and a text report:

```bash
cmake --build build                 # builds ImageProcessingApp and ipcli
tools/run_all.sh                    # all images -> results/cli_output/   (tools/run_all.bat on Windows)
build/ipcli image.png out/ --canny 1.4 0.05 0.15 --lines --circles 15 100 40 --ellipses 20 120 30
build/ipcli image.png out/ --snake 160 160 140 --points 80 --iter 400 --balloon 0.3
```

## Test images
* `images/input/Canny & Line Detection_Test_Images/` - given images (grayscale + colour, lines, a circle, a triangle ...).
* `images/input/Shapes & Snake_Test_Images/` - circles, ellipses, mixed shapes and three snake objects
  (grayscale blob, colour flower, rectangle). Regenerate with `python tools/make_test_images.py`.

See **REPORT.md** for the methods, parameters and measured results.
