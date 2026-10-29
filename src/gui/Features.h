#pragma once
#include <QColor>
#include <QString>
#include <QStringList>

// One entry per feature page. Shared by the sidebar, the result library and the compare tab.
struct FeatureInfo {
    const char* id;     // used in file names / settings / the library index
    const char* title;  // shown to the user
    const char* mono;   // 1-2 letter badge
    const char* color;  // accent colour of the feature
};

inline const FeatureInfo kFeatures[] = {
    {"canny", "Canny Edges", "Ca", "#8b5cf6"},
    {"lines", "Hough Lines", "Li", "#06b6d4"},
    {"circles", "Hough Circles", "Ci", "#ec4899"},
    {"ellipses", "Ellipses", "El", "#f59e0b"},
    {"snake", "Active Contour", "Sn", "#22c55e"},
    {"chain", "Chain Code", "Ch", "#f97316"},
};
inline constexpr int kFeatureCount = int(sizeof(kFeatures) / sizeof(kFeatures[0]));

inline const FeatureInfo* featureById(const QString& id) {
    for (const FeatureInfo& f : kFeatures)
        if (id == f.id) return &f;
    return nullptr;
}
inline QString featureTitle(const QString& id) {
    const FeatureInfo* f = featureById(id);
    return f ? QString(f->title) : (id == "original" ? QString("Original") : id);
}
inline QColor featureColor(const QString& id) {
    const FeatureInfo* f = featureById(id);
    return QColor(f ? f->color : "#94a3b8");
}
