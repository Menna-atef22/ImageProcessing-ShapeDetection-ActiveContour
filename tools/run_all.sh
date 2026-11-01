#!/usr/bin/env bash
# Produces every result of Assignment 2 (superimposed detections, snake contours, chain codes, reports)
# for all images in images/input/ using the command-line runner.
#
#   cmake --build build            (builds ImageProcessingApp and ipcli)
#   tools/run_all.sh [path/to/ipcli] [output dir]
set -euo pipefail
cd "$(dirname "$0")/.."
CLI="${1:-build/ipcli}"
OUT="${2:-results/cli_output}"
mkdir -p "$OUT"

T1="images/input/Canny & Line Detection_Test_Images"
T2="images/input/Shapes & Snake_Test_Images"

run() { echo ">> $*"; "$CLI" "$@" > /dev/null; }

# ---- Task 1: Canny + lines + circles + ellipses ("if any") ----
run "$T1/01_hough_lines_test.png"          "$OUT" --lineparams 0.25 100 12
run "$T1/02_canny_grayscale_test.png"      "$OUT" --lineparams 0.2 40 12 --circles 60 140 60 --ellipses 60 160 60
run "$T1/03_canny_color_test.png"          "$OUT" --lineparams 0.2 40 12 --circles 60 140 60 --ellipses 60 160 60
run "$T1/04_hough_grid_test.png"           "$OUT" --lines
run "$T1/05_building_road_lines_test.png"  "$OUT" --lines
run "$T2/06_circles_color_test.png"        "$OUT" --circles 15 100 40 --ellipses 20 120 30
run "$T2/07_ellipses_grayscale_test.png"   "$OUT" --ellipses 20 120 30
run "$T2/08_mixed_shapes_color_test.png"   "$OUT" --lines --circles 20 100 100 --ellipses 20 120 30

# ---- Task 2: greedy snake from a circle around the object, chain code, perimeter, area ----
run "$T2/09_snake_blob_grayscale_test.png"      "$OUT" --snake 160 160 140 --points 80 --iter 400 --balloon 0.3
run "$T2/10_snake_flower_color_test.png"        "$OUT" --snake 170 150 135 --points 80 --iter 400 --balloon 0.3
run "$T2/11_snake_rectangle_grayscale_test.png" "$OUT" --snake 160 160 140 --points 80 --iter 400 --balloon 0.3
echo "Done. Results are in $OUT"
