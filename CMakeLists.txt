cmake_minimum_required(VERSION 3.16)
project(ImageProcessingAssignment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)   # Hough/Canny are MUCH faster in Release
endif()


# ---- Windows: auto-detect Qt installed by the Qt Online Installer (C:/Qt/<ver>/msvc*_64) ----
if(WIN32 AND NOT CMAKE_PREFIX_PATH AND NOT DEFINED ENV{QTDIR})
  file(GLOB _QT_CANDIDATES "C:/Qt/6.*/msvc*_64" "C:/Qt/5.*/msvc*_64")
  list(SORT _QT_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
  if(_QT_CANDIDATES)
    list(GET _QT_CANDIDATES 0 _QT_BEST)
    set(CMAKE_PREFIX_PATH "${_QT_BEST}")
    message(STATUS "Auto-detected Qt at: ${_QT_BEST}")
  endif()
endif()

# Qt6 preferred, fall back to Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ---- core library: all algorithms (common + task1 + task2) ----
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
     src/common/*.cpp src/common/*.h
     src/task1/*.cpp  src/task1/*.h
     src/task2/*.cpp  src/task2/*.h)
add_library(ipcore STATIC ${CORE_SOURCES})
target_include_directories(ipcore PUBLIC src)
target_link_libraries(ipcore PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)  # QImage only for file IO

# ---- GUI application ----
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS src/gui/*.cpp src/gui/*.h)
add_executable(ImageProcessingApp src/main.cpp ${GUI_SOURCES})
target_link_libraries(ImageProcessingApp PRIVATE ipcore)
if(WIN32)
  set_target_properties(ImageProcessingApp PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ---- Windows: copy the Qt DLLs next to the exe after every build ----
if(WIN32)
  find_program(WINDEPLOYQT NAMES windeployqt windeployqt6
               HINTS "${CMAKE_PREFIX_PATH}/bin" "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin")
  if(WINDEPLOYQT)
    add_custom_command(TARGET ImageProcessingApp POST_BUILD
      COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:ImageProcessingApp>")
  else()
    message(WARNING "windeployqt not found - run it manually if the exe complains about missing DLLs")
  endif()
endif()
