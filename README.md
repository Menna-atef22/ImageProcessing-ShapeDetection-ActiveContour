# Image Processing – Assignment 2

C++17 / Qt / CMake. All algorithms implemented **from scratch** (Qt is used only for the window and for reading/writing image files).

* Task 1: Canny edge detector, Hough lines / circles / ellipses
* Task 2: Active contour (greedy snake), chain code, perimeter & area

See **COMMANDS.md** for build & run instructions.

## Status
- [x] Canny edge detector + Hough Lines
- [x] Circles + Ellipses
- [x] Active Contour (greedy snake)
- [x] Chain code, area / perimeter (Chain Code page: snake result or built-in known test shapes with PASS/FAIL check)

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

When you implement a stub (circles / ellipses / snake / chain code) flip its flag in `isReady()` at the top of
`src/gui/MainWindow.cpp` - until then an empty result is shown but not saved to the library.
