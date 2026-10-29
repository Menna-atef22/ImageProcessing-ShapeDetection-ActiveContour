#include "Theme.h"

#include <QDir>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPair>
#include <QStandardPaths>

namespace {

// A white tick drawn once into the temp folder (QSS can only show a tick through an image file).
QString tickImage() {
    static QString path;
    if (path.isEmpty()) {
        path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("visionlab_tick.png");
        QImage img(32, 32, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(Qt::white, 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawPolyline(QPolygonF({QPointF(8, 17), QPointF(14, 23), QPointF(24, 10)}));
        p.end();
        img.save(path);
    }
    return path;
}

// Drop-down chevron in the given colour (one small PNG per colour, drawn once into the temp folder).
QString arrowImage(const QString& color) {
    QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                       .filePath("visionlab_arrow_" + QString(color).remove('#') + ".png");
    QImage img(32, 32, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(color), 3.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.drawPolyline(QPolygonF({QPointF(8, 12), QPointF(16, 20), QPointF(24, 12)}));
    p.end();
    img.save(path);
    return path;
}

const char* kTemplate = R"QSS(
* { font-family: "Segoe UI", "Inter", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: @text; }
QMainWindow, QWidget#root { background: @bg; }
QWidget#sidebar { background: @panel; border-right: 1px solid @border; }
QWidget#controls { background: @bg; border-right: 1px solid @border; }
QLabel { background: transparent; }

QFrame#card { background: @card; border: 1px solid @border; border-radius: 12px; }
QFrame#sep  { background: @border; max-height: 1px; min-height: 1px; border: none; }

QLabel#appTitle   { font-size: 19px; font-weight: 700; }
QLabel#appSub     { color: @muted; font-size: 11px; }
QLabel#sectionLabel { color: @muted; font-size: 11px; font-weight: 700; padding: 10px 4px 2px 4px; }
QLabel#pageTitle  { font-size: 18px; font-weight: 700; }
QLabel#cardTitle  { font-weight: 600; }
QLabel#paramName  { font-weight: 600; }
QLabel#muted      { color: @muted; }
QLabel#help       { color: @muted; font-size: 11px; }
QLabel#fileLabel  { color: @muted; }
QLabel#statValue  { font-size: 15px; font-weight: 600; color: @accent; }
QLabel#bigStat    { font-size: 20px; font-weight: 700; color: @accent; }
QLabel#statName   { color: @muted; font-size: 11px; }
QLabel#note       { background: @accentSoft; color: @accent; border-radius: 8px; padding: 7px 10px; }
QLabel#badgeReady { background: @successSoft; color: @success; border-radius: 9px; padding: 2px 10px; font-size: 11px; font-weight: 600; }
QLabel#badgeDev   { background: @warnSoft; color: @warn; border-radius: 9px; padding: 2px 10px; font-size: 11px; font-weight: 600; }

QPushButton { background: @card; border: 1px solid @border; border-radius: 8px; padding: 8px 14px; }
QPushButton:hover { border-color: @accent; }
QPushButton:pressed { background: @accentSoft; }
QPushButton:disabled { color: @muted; }
QPushButton#primary { background: @accent; color: white; border: none; font-weight: 600; padding: 11px 14px; }
QPushButton#primary:hover { background: @accentHover; }
QPushButton#nav { text-align: left; border: none; background: transparent; padding: 10px 12px; border-radius: 8px; }
QPushButton#nav:hover { background: @accentSoft; }
QPushButton#nav:checked { background: @accentSoft; color: @accent; font-weight: 600; }
QPushButton#ghost { background: transparent; border: none; color: @muted; padding: 2px 8px; }
QPushButton#ghost:hover { color: @accent; }

QLineEdit#numBox { background: @inputBg; border: 1px solid @border; border-radius: 4px; padding: 2px 4px; font-size: 13px; font-weight: 600; selection-background-color: @accent; selection-color: white; }
QLineEdit#numBox:focus { border: 1px solid @accent; }
QLineEdit#numBox:hover { border-color: @accent; }

QSlider::groove:horizontal { height: 6px; background: @border; border-radius: 3px; }
QSlider::sub-page:horizontal { background: @accent; border-radius: 3px; }
QSlider::handle:horizontal { background: white; border: 2px solid @accent; width: 14px; height: 14px; margin: -6px 0; border-radius: 8px; }

QCheckBox { spacing: 8px; padding: 4px 0; }
QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px; border: 1px solid @border; background: @inputBg; }
QCheckBox::indicator:checked { background: @accent; border-color: @accent; image: url(@check); }

QPlainTextEdit { background: @inputBg; border: 1px solid @border; border-radius: 8px; padding: 6px;
                 font-family: Consolas, "Cascadia Mono", "Courier New", monospace; font-size: 12px; }

QScrollArea#scroll, QScrollArea#scroll > QWidget > QWidget { background: @bg; border: none; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 0; border: none; }
QScrollBar::handle:vertical { background: @scrollHandle; border-radius: 4px; min-height: 30px; margin: 2px; }
QScrollBar::handle:vertical:hover { background: @accent; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; border: none; background: none; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 0; border: none; }
QScrollBar::handle:horizontal { background: @scrollHandle; border-radius: 4px; min-width: 30px; margin: 2px; }
QScrollBar::handle:horizontal:hover { background: @accent; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; border: none; background: none; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QSplitter::handle { background: transparent; }
QComboBox { background: @inputBg; border: 1px solid @border; border-radius: 8px; padding: 6px 30px 6px 10px; min-height: 20px; combobox-popup: 0; }
QComboBox:hover, QComboBox:focus, QComboBox:on { border-color: @accent; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 28px; border: none; background: transparent; }
QComboBox::down-arrow { image: url(@arrow); width: 12px; height: 12px; }
QComboBox QAbstractItemView { background: @card; border: 1px solid @accent; border-radius: 8px; padding: 4px; outline: none; }
QComboBox QAbstractItemView::item { min-height: 28px; padding: 2px 8px; border-radius: 6px; color: @text; }
QComboBox QAbstractItemView::item:hover { background: @accentSoft; color: @text; }
QComboBox QAbstractItemView::item:selected { background: @accent; color: white; }

QTabWidget::pane { border: none; top: -1px; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: @muted; padding: 9px 20px; margin-right: 4px; font-weight: 600;
               border: none; border-bottom: 3px solid transparent; }
QTabBar::tab:hover { color: @text; }
QTabBar::tab:selected { color: @text; border-bottom: 3px solid @accent; }

QFrame#gcard { background: @card; border: 1px solid @border; border-radius: 10px; }
QFrame#gcard:hover { border-color: @accent; }
QFrame#gcard[sel="true"] { background: @accentSoft; border: 2px solid @accent; }
QLabel#thumb { background: @viewBg; border-radius: 6px; }
QLabel#orderBadge { background: @accent; color: white; border-radius: 9px; min-width: 18px; max-width: 18px;
                    min-height: 18px; max-height: 18px; font-size: 11px; font-weight: 700; qproperty-alignment: AlignCenter; }
QLabel#logo { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6366f1, stop:1 #d946ef); color: white;
              border-radius: 10px; font-size: 15px; font-weight: 800; min-width: 38px; max-width: 38px;
              min-height: 38px; max-height: 38px; qproperty-alignment: AlignCenter; }
QLabel#status { color: @success; font-weight: 600; }
QLabel#status[ok="false"] { color: @warn; }
QTextBrowser#paramBox { background: transparent; border: none; padding: 2px 10px; font-size: 12px; }
QPushButton#danger:hover { border-color: #ef4444; color: #ef4444; }
QProgressBar { background: @border; border: none; border-radius: 2px; max-height: 4px; min-height: 4px; }
QProgressBar::chunk { background: @accent; border-radius: 2px; }
QToolTip { background: @card; color: @text; border: 1px solid @border; padding: 5px; }
)QSS";

struct Palette {
    const char *bg, *panel, *card, *border, *text, *muted, *accent, *accentHover, *accentSoft, *inputBg,
        *success, *successSoft, *warn, *warnSoft, *viewBg, *scrollHandle;
};

const Palette kDark = {"#0f1117", "#151823", "#1b1f2e", "#2a3042", "#e6e8ef", "#8b93a7", "#6366f1",
                       "#7c7ff5", "rgba(99,102,241,0.18)", "#11141d", "#34d399", "rgba(52,211,153,0.15)",
                       "#fbbf24", "rgba(251,191,36,0.15)", "#0b0d13", "#3a4258"};
const Palette kLight = {"#f4f5fa", "#ffffff", "#ffffff", "#e3e6ef", "#1c2033", "#6b7280", "#4f46e5",
                        "#4338ca", "rgba(79,70,229,0.10)", "#f8f9fc", "#059669", "rgba(5,150,105,0.12)",
                        "#b45309", "rgba(217,119,6,0.14)", "#eceef5", "#c3c8d8"};
}  // namespace

QString Theme::stylesheet(bool dark) {
    const Palette& p = dark ? kDark : kLight;
    QString s = QString::fromUtf8(kTemplate);
    s.replace("@check", tickImage().replace("\\", "/"));
    s.replace("@arrow", arrowImage(p.muted).replace("\\", "/"));
    // longer tokens first so "@accentSoft" is not eaten by "@accent"
    const QList<QPair<QString, QString>> map = {
        {"@accentHover", p.accentHover}, {"@viewBg", p.viewBg}, {"@scrollHandle", p.scrollHandle}, {"@accentSoft", p.accentSoft}, {"@accent", p.accent},
        {"@successSoft", p.successSoft}, {"@success", p.success},       {"@warnSoft", p.warnSoft},
        {"@warn", p.warn},               {"@inputBg", p.inputBg},       {"@border", p.border},
        {"@panel", p.panel},             {"@card", p.card},             {"@bg", p.bg},
        {"@text", p.text},               {"@muted", p.muted}};
    for (const auto& kv : map) s.replace(kv.first, kv.second);
    return s;
}

QColor Theme::viewBackground(bool dark) { return QColor(dark ? "#0b0d13" : "#eceef5"); }
QColor Theme::viewForeground(bool dark) { return QColor(dark ? "#6b7285" : "#8a90a2"); }
