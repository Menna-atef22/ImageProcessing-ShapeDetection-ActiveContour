#pragma once
#include <QColor>
#include <QString>

// Web-like look: one stylesheet, two palettes (dark / light).
namespace Theme {
QString stylesheet(bool dark);
QColor viewBackground(bool dark);  // background behind images
QColor viewForeground(bool dark);  // placeholder text colour
}  // namespace Theme
