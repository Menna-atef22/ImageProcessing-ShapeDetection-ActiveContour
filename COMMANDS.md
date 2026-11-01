# الأوامر (Build & Run)

المشروع: **C++17 + Qt (Widgets) + CMake**. مفيش OpenCV — كل الخوارزميات مكتوبة from scratch.

---

## 0) المتطلبات (مرة واحدة)

| الأداة | المطلوب |
|---|---|
| Compiler | g++ ≥ 9 / MSVC 2019+ / MinGW |
| CMake | ≥ 3.16 |
| Qt | Qt 6 (أو Qt 5) – مكوّن **Widgets** |

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev
```

### Windows (الأسهل: Qt Creator)
1. نزّل **Qt Online Installer** من qt.io واختار: `Qt 6.x → MinGW 64-bit` + `Qt Creator` + `CMake` + `Ninja`.
2. افتح Qt Creator → `File > Open File or Project` → اختار `CMakeLists.txt`.
3. اختار Kit (MinGW) → Configure Project → اختار **Release** → زرار ▶ Run.

### macOS
```bash
brew install cmake qt
```

---

## 1) Build (Linux / macOS / Git-Bash)

```bash
cd ImageProcessingAssignment
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
```

لو Qt مش بيتلاقي:
```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64      # Linux/macOS
```

### Build على Windows (Command Prompt - MinGW)
افتح **Qt 6.x (MinGW) command prompt** من Start Menu، وبعدين:
```bat
cd ImageProcessingAssignment
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
```
لو Qt مش بيتلاقي:
```bat
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\mingw_64
```

---

## 2) Run

### أ) الواجهة (GUI)
```bash
./build/ImageProcessingApp            # Linux / macOS
build\ImageProcessingApp.exe          # Windows
```
بعدها: **Open Image → اختار الـ feature من الـ sidebar → عدّل الـ parameters → Run**.
كل نتيجة بتتسيف لوحدها في مجلد `results/` (جنب `CMakeLists.txt`) وبتظهر في تاب **Compare** عشان تقارن بينها.
لو شغّلت نفس الصورة بنفس الـ parameters تاني، البرنامج بيحمّل النتيجة المتسيفة فوراً من غير ما يحسبها من جديد.

> على Windows لو ظهر خطأ DLL مش موجودة، شغّل `windeployqt build\ImageProcessingApp.exe` مرة واحدة
> (أو شغّل البرنامج من Qt Creator).

---

## 3) الـ Parameters

| Parameter | معناه | لو عايز |
|---|---|---|
| `sigma` | قوة الـ Gaussian blur | زوّده (2–3) للصور الـ noisy |
| `lowRatio` / `highRatio` | عتبات Canny كنسبة من أقوى gradient | قلّلهم لو edges ناقصة، زوّدهم لو edges كتير |
| `voteRatio` | الخط لازم ياخد ≥ النسبة دي من أعلى votes | زوّده → خطوط أقل وأقوى |
| `maxLines` | أقصى عدد خطوط | |

---

## 4) Git workflow للفريق

```bash
git init
git add .
git commit -m "Initial structure + Canny + Hough Lines"
git branch -M main
git remote add origin <repo-url>
git push -u origin main

# كل واحد يشتغل على branch لوحده
git checkout -b feature/circles-ellipses
git checkout -b feature/snake
git checkout -b feature/chaincode-gui
```
كل واحد بيعدّل **ملفاته بس** → مفيش merge conflicts.

---

## 5) الملفات حسب الجزء

| الجزء | الملفات | الحالة |
|---|---|---|
| Canny + Hough Lines | `task1/EdgeDetector.*`, `task1/LineDetector.*` | ✅ جاهز |
| Circles + Ellipses | `task1/CircleDetector.*`, `task1/EllipseDetector.*` | ⏳ stub (TODO) |
| Active Contour | `task2/ActiveContour.*` | ⏳ stub (TODO) |
| Chain Code + GUI | `task2/ContourAnalysis.*`, `gui/MainWindow.*` | ⏳ stub + GUI مبدئي |
| مشترك | `common/Types.h`, `Image.*`, `Draw.*` | ✅ لا تعدّلوا من غير اتفاق |

باقي الأجزاء لقوا في الـ stubs نفس الـ interface جاهز، وبياخدوا `GrayImage edges` من
`EdgeDetector::canny(...).edges` ويرسموا النتيجة بـ `Draw::*`.


---

## Command-line runner (ipcli) - English

`ipcli` is built together with the GUI (`cmake --build build`). It needs no window:

```bash
tools/run_all.sh                       # every image -> results/cli_output/  (Windows: tools\run_all.bat)
build/ipcli <image> <outDir> [options]
  --canny sigma low high               # default 1.4 0.05 0.15
  --lines | --lineparams ratio minVotes maxLines
  --circles minR maxR minVotes
  --ellipses minA maxA minVotes
  --snake cx cy r  [--points n --alpha a --beta b --gamma g --balloon k --iter n --window w --sigma s]
```
Outputs per image: `*_canny_edges.png`, `*_canny_overlay.png`, `*_lines.png`, `*_circles.png`, `*_ellipses.png`,
`*_snake.png`, `*_chaincode.txt` (start pixel, full chain code, perimeter, area) and `*_report.txt`.
