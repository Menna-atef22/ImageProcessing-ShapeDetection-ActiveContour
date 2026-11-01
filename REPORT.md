# Assignment 2 - Shape Detection and Active Contours

Everything is implemented from scratch in C++17 (Qt is used only for the window and for reading / writing image files).

## 1. What was implemented

| Requirement (Assignment 2) | Where | Notes |
|---|---|---|
| Canny edge detector | `src/task1/EdgeDetector.*` | Gaussian blur, Sobel, non-maximum suppression, double threshold + hysteresis |
| Line detection | `src/task1/LineDetector.*` | Hough transform in (rho, theta); peaks with non-maximum suppression; infinite lines clipped to the image |
| Circle detection | `src/task1/CircleDetector.*` | Hough circle transform (one (cx, cy) accumulator per radius), 3D non-maximum suppression, **outline-support check** (>= 80 % of the circle must lie on edge pixels) |
| Ellipse detection | `src/task1/EllipseDetector.*` | Xie & Ji (2002) 1D Hough method (pairs of points = major axis, third point votes for the minor axis), 5D non-maximum suppression, **outline-support check** (>= 85 % of the ellipse must lie on edge pixels) |
| Shapes superimposed on the images | `*_lines.png`, `*_circles.png`, `*_ellipses.png` in `results/cli_output/` | lines green, circles green, ellipses blue, centres red |
| Active contour, greedy algorithm | `src/task2/ActiveContour.*` | Energy = alpha*continuity + beta*curvature + gamma*external (gradient) + balloon; 3x3 search window, points updated one by one, stops when no point moves; corner relaxation |
| Chain code | `src/task2/ContourAnalysis.*` | Freeman 8-direction code (0=E, 1=NE, 2=N, 3=NW, 4=W, 5=SW, 6=S, 7=SE), Bresenham path between consecutive snake points; the code is closed (it returns to its start pixel) |
| Perimeter and area | same | Reported twice: from the polygon (Euclidean length, shoelace area) and from the chain code (even move = 1, odd move = sqrt 2; area by Green's theorem on the pixel path) |

The two additions to the detectors were needed because plain voting also accepts short arcs (for example the curved tip of an ellipse is a
good "circle" for a few dozen edge pixels). Requiring edge support along the whole fitted outline removes these false positives without
changing the true detections.

## 2. Test images

* `images/input/Canny & Line Detection_Test_Images/` (given): 01 lines, 02 grayscale shapes, 03 colour shapes, 04 grid, 05 building and road.
* `images/input/Shapes & Snake_Test_Images/` (added, `tools/make_test_images.py`): 06 circles (colour), 07 ellipses (grayscale),
  08 lines + circle + ellipse (colour), 09 blob (grayscale), 10 flower (colour), 11 rectangle (grayscale).

Both grayscale and colour inputs are used (colour images are converted with 0.299 R + 0.587 G + 0.114 B).

## 3. Task 1 results (Canny sigma = 1.4, low = 0.05, high = 0.15 of the maximum gradient)

| Image | Edge pixels | Lines | Circles | Ellipses | Comment |
|---|---:|---:|---:|---:|---|
| 01 hough lines | 10 916 | 10 | - | - | `--lineparams 0.25 100 12`; all 9 drawn lines found, the thin diagonal gives one near-duplicate |
| 02 canny grayscale | 7 018 | 8 | 1 (centre 465,175, r = 95) | 1 | rectangle sides, triangle sides and the two long lines; circle also fits as a near-circular ellipse |
| 03 canny colour | 8 043 | 9 | 1 (centre 470,180, r = 95) | 1 | same scene in colour |
| 04 hough grid | 15 679 | 12 | - | - | grid lines + the two diagonals |
| 05 building road | 7 604 | 14 | - | - | window grid and road edges |
| 06 circles | 2 403 | - | 4 (r = 28, 36, 51, 56) | 4 | all circles found, including the filled disc |
| 07 ellipses | 2 854 | - | 0 | 3 | exactly the three ellipses (a = 72/85/99, b = 37/40/43) |
| 08 mixed | 3 151 | 2 | 1 (r = 51) | 2 | line, circle and ellipse all found; with minimum votes 100 the weak false circles on the ellipse's pointed ends are rejected |

Edge maps, overlays, reports with all parameters (rho / theta, centres, radii, axes, angles) are in `results/cli_output/`.

## 4. Task 2 results (alpha = 1, beta = 1, gamma = 1.2, balloon = 0.3, 80 points, window 3x3, sigma = 2)

| Image | Start circle | Iterations | Chain code moves | Perimeter polygon / chain code (px) | Area polygon / chain code (px^2) |
|---|---|---:|---:|---|---|
| 09 blob (grayscale) | (160,160) r = 140 | 82 | 513 | 567.35 / 595.43 | 22 906.5 / 22 908.5 |
| 10 flower (colour) | (170,150) r = 135 | 69 | 492 | 540.01 / 575.67 | 20 453.5 / 20 503.0 |
| 11 rectangle (grayscale) | (160,160) r = 140 | 400 (max) | 570 | 579.59 / 582.43 | 22 222.0 / 22 221.0 |

The complete chain codes are in `results/cli_output/*_chaincode.txt` (start pixel, perimeter, area and the code itself).
For the rectangle (140 x 160 px, true area 22 400, perimeter 600) the snake rounds the corners slightly, which explains the small deficit.

Observations
* The chain-code perimeter is longer than the polygon perimeter because the chain code follows the pixel grid (diagonal move = sqrt 2) and the
  polygon cuts across it.
* The polygon area and the chain-code area agree to within 0.3 % (a polygon with vertices that are not on the Bresenham path differs slightly).
* A greedy snake cannot enter deep concavities: on the blob the notch at the bottom is bridged (see `09_*_snake.png`).
* On the rectangle the snake does not fully converge (it keeps flipping corner pixels), so it stops at the iteration limit; the result is stable.

## 5. Reproducing the results

```bash
python tools/make_test_images.py        # optional: regenerates images/input/Shapes & Snake_Test_Images
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
tools/run_all.sh                        # or tools\run_all.bat on Windows; writes results/cli_output/
```
The GUI (`build/ImageProcessingApp`) runs the same algorithms; its Chain Code page also stores the full chain code next to the saved result.

## 6. Notes on how the included results were produced

The numbers and images in `results/cli_output/` were produced by compiling the algorithm sources (`src/common`, `src/task1`, `src/task2`, `src/cli`)
with a small file-I/O replacement instead of Qt (the build environment had no Qt). The algorithms are identical; running `tools/run_all.sh`
on your machine regenerates the same results through the normal Qt build. The Qt GUI code changes made for this submission (page flags,
chain-code export) could not be compiled there, so build once and check that `ImageProcessingApp` starts.
