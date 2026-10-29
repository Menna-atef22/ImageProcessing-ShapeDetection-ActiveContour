#pragma once
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "../common/Image.h"

// Every result the user produces is written to disk (PNG + one JSON index), so nothing has to be
// computed twice: re-running the same feature with the same image + parameters just loads the PNG.
//
//   results/
//     library.json                 index of everything (parameters, stats, log, time)
//     <image name>/original.png    copy of the source image
//     <image name>/<feature>_<id>.png
class ResultStore : public QObject {
    Q_OBJECT
public:
    struct Entry {
        QString id;         // short unique id (hash of the key)
        QString key;        // image hash + feature + parameters
        QString imageHash;
        QString imageName;
        QString feature;    // see Features.h
        QString title;      // e.g. "5 lines"
        QString stats;      // e.g. "5 lines detected - 120 ms"
        QString time;       // ISO date-time
        QString file;       // PNG path relative to the results folder
        QStringList params; // "Gaussian sigma = 1.4" ... (for display)
        QStringList log;    // lines that were written to the output log
        QJsonObject values; // raw parameter values (to restore the settings)
        QJsonObject extra;  // feature specific data (e.g. snake contour)
    };
    struct ImageInfo {
        QString hash, name, folder, original;  // original: relative path of original.png
        int width = 0, height = 0;
    };

    explicit ResultStore(const QString& rootDir, QObject* parent = nullptr);

    QString root() const { return m_root; }
    QString absPath(const QString& rel) const;
    static QString defaultRoot();                    // <project>/results (or next to the exe)
    static QString hashImage(const RgbImage& img);   // content hash (same pixels -> same hash)
    static QString makeKey(const QString& imageHash, const QString& feature, const QJsonObject& values);

    // Saves original.png the first time an image is seen. Returns its hash.
    QString registerImage(const RgbImage& img, const QString& name);
    const ImageInfo* imageInfo(const QString& hash) const;
    bool loadOriginal(const QString& hash, RgbImage& out) const;

    const Entry* find(const QString& key) const;
    const Entry* findById(const QString& id) const;
    bool loadResult(const Entry& e, RgbImage& out) const;
    Entry add(Entry e, const RgbImage& img);  // replaces an older entry with the same key
    void remove(const QStringList& ids);

    const QList<Entry>& entries() const { return m_entries; }
    const QList<ImageInfo>& images() const { return m_images; }

signals:
    void changed();

private:
    void loadIndex();
    void saveIndex() const;
    QString folderFor(const QString& hash, const QString& name);

    QString m_root;
    QList<Entry> m_entries;
    QList<ImageInfo> m_images;
};
