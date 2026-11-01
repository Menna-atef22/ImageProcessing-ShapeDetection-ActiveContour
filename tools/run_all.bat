@echo off
rem Produces every result of Assignment 2 for all images in images\input using ipcli.
rem   tools\run_all.bat [path\to\ipcli.exe] [output dir]
cd /d "%~dp0.."
set CLI=%~1
if "%CLI%"=="" set CLI=build\ipcli.exe
set OUT=%~2
if "%OUT%"=="" set OUT=results\cli_output
if not exist "%OUT%" mkdir "%OUT%"
set T1=images\input\Canny & Line Detection_Test_Images
set T2=images\input\Shapes & Snake_Test_Images

"%CLI%" "%T1%\01_hough_lines_test.png"         "%OUT%" --lineparams 0.25 100 12
"%CLI%" "%T1%\02_canny_grayscale_test.png"     "%OUT%" --lineparams 0.2 40 12 --circles 60 140 60 --ellipses 60 160 60
"%CLI%" "%T1%\03_canny_color_test.png"         "%OUT%" --lineparams 0.2 40 12 --circles 60 140 60 --ellipses 60 160 60
"%CLI%" "%T1%\04_hough_grid_test.png"          "%OUT%" --lines
"%CLI%" "%T1%\05_building_road_lines_test.png" "%OUT%" --lines
"%CLI%" "%T2%\06_circles_color_test.png"       "%OUT%" --circles 15 100 40 --ellipses 20 120 30
"%CLI%" "%T2%\07_ellipses_grayscale_test.png"  "%OUT%" --ellipses 20 120 30
"%CLI%" "%T2%\08_mixed_shapes_color_test.png"  "%OUT%" --lines --circles 20 100 100 --ellipses 20 120 30
"%CLI%" "%T2%\09_snake_blob_grayscale_test.png"      "%OUT%" --snake 160 160 140 --points 80 --iter 400 --balloon 0.3
"%CLI%" "%T2%\10_snake_flower_color_test.png"        "%OUT%" --snake 170 150 135 --points 80 --iter 400 --balloon 0.3
"%CLI%" "%T2%\11_snake_rectangle_grayscale_test.png" "%OUT%" --snake 160 160 140 --points 80 --iter 400 --balloon 0.3
echo Done. Results are in %OUT%
