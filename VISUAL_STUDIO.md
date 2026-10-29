# تشغيل المشروع على Visual Studio 2022

## مرة واحدة بس (التثبيت)
1. **Visual Studio 2022** ← من Visual Studio Installer اختار **Desktop development with C++** ← Install.
2. **Qt**: نزّل Qt Online Installer من qt.io ← سجّل حساب مجاني ← اختار:
   `Qt 6.x ← MSVC 2022 64-bit`  (مش MinGW!) ← ثبّته في المسار الافتراضي `C:\Qt`.

## التشغيل
1. افتح Visual Studio ← **Open a local folder** ← اختار مجلد `ImageProcessingAssignment`.
2. استنى لحد ما تبقى آخر رسالة في تبويب **Output** : `CMake generation finished`.
3. من القائمة اللي فوق (جنب كلمة Debug/Release) اختار **x64 Release (use this one)**.
4. من القائمة **Build ← Build All**.
5. من القائمة اللي جنب زرار ▶ الأخضر اختار **ImageProcessingApp.exe** واضغط ▶.

## استخدام البرنامج
Load Image ← اختار صورة (جرّب `images\test\shapes.png`) ← Canny Edges أو Hough Lines ← Save Current Result.

## لو حصلت مشكلة
- **Could not find Qt**: Qt مش متثبّت في `C:\Qt`. افتح `CMakePresets.json` وضيف جوه `cacheVariables`:
  `"CMAKE_PREFIX_PATH": "C:/Qt/6.8.0/msvc2022_64"` (غيّر الرقم حسب نسختك) ← ثم
  **Project ← Delete Cache and Reconfigure**.
- **أي error تاني**: انسخ نص الخطأ من تبويب Output وابعته.
