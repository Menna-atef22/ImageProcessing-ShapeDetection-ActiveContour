#include "ResultStore.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace {
QString sanitize(const QString& s) {
    QString out;
    for (QChar c : s) out += (c.isLetterOrNumber() || c == '-' || c == '_') ? c : QChar('_');
    return out.isEmpty() ? QString("image") : out;
}
}  // namespace

QString ResultStore::defaultRoot() {
    // Walk up from the executable until we find the project (CMakeLists.txt) -> <project>/results.
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (d.exists("CMakeLists.txt") && d.exists("src")) return d.filePath("results");
        if (!d.cdUp()) break;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath("results");
}

ResultStore::ResultStore(const QString& rootDir, QObject* parent) : QObject(parent), m_root(rootDir) {
    QDir().mkpath(m_root);
    loadIndex();
}

QString ResultStore::absPath(const QString& rel) const { return QDir(m_root).filePath(rel); }

QString ResultStore::hashImage(const RgbImage& img) {
    QCryptographicHash h(QCryptographicHash::Sha1);
    int dims[2] = {img.width, img.height};
    h.addData(QByteArray::fromRawData(reinterpret_cast<const char*>(dims), int(sizeof(dims))));
    h.addData(QByteArray::fromRawData(reinterpret_cast<const char*>(img.data.data()), int(img.data.size())));
    return QString::fromLatin1(h.result().toHex().left(12));
}

QString ResultStore::makeKey(const QString& imageHash, const QString& feature, const QJsonObject& values) {
    return imageHash + "|" + feature + "|" + QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact));
}

QString ResultStore::folderFor(const QString& hash, const QString& name) {
    for (const ImageInfo& i : m_images)
        if (i.hash == hash) return i.folder;
    QString base = sanitize(QFileInfo(name).completeBaseName());
    QString folder = base;
    auto used = [&](const QString& f) {
        for (const ImageInfo& i : m_images)
            if (i.folder == f) return true;
        return false;
    };
    if (used(folder)) folder = base + "_" + hash.left(6);
    return folder;
}

QString ResultStore::registerImage(const RgbImage& img, const QString& name) {
    QString hash = hashImage(img);
    if (imageInfo(hash)) return hash;
    ImageInfo info;
    info.hash = hash;
    info.name = QFileInfo(name).fileName();
    info.folder = folderFor(hash, name);
    info.original = info.folder + "/original.png";
    info.width = img.width;
    info.height = img.height;
    QDir().mkpath(absPath(info.folder));
    ImageIO::saveRgb(absPath(info.original).toStdString(), img);
    m_images.append(info);
    saveIndex();
    emit changed();
    return hash;
}

const ResultStore::ImageInfo* ResultStore::imageInfo(const QString& hash) const {
    for (const ImageInfo& i : m_images)
        if (i.hash == hash) return &i;
    return nullptr;
}

bool ResultStore::loadOriginal(const QString& hash, RgbImage& out) const {
    const ImageInfo* i = imageInfo(hash);
    return i && ImageIO::loadRgb(absPath(i->original).toStdString(), out);
}

const ResultStore::Entry* ResultStore::find(const QString& key) const {
    for (const Entry& e : m_entries)
        if (e.key == key && QFile::exists(absPath(e.file))) return &e;
    return nullptr;
}

const ResultStore::Entry* ResultStore::findById(const QString& id) const {
    for (const Entry& e : m_entries)
        if (e.id == id) return &e;
    return nullptr;
}

bool ResultStore::loadResult(const Entry& e, RgbImage& out) const {
    return ImageIO::loadRgb(absPath(e.file).toStdString(), out);
}

ResultStore::Entry ResultStore::add(Entry e, const RgbImage& img) {
    const ImageInfo* info = imageInfo(e.imageHash);
    QString folder = info ? info->folder : sanitize(e.imageName);
    e.id = QString::fromLatin1(QCryptographicHash::hash(e.key.toUtf8(), QCryptographicHash::Sha1).toHex().left(8));
    e.file = folder + "/" + e.feature + "_" + e.id + ".png";
    e.time = QDateTime::currentDateTime().toString(Qt::ISODate);
    QDir().mkpath(absPath(folder));
    ImageIO::saveRgb(absPath(e.file).toStdString(), img);
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].key == e.key) {
            m_entries.removeAt(i);
            break;
        }
    m_entries.append(e);
    saveIndex();
    emit changed();
    return e;
}

void ResultStore::remove(const QStringList& ids) {
    for (const QString& id : ids)
        for (int i = 0; i < m_entries.size(); ++i)
            if (m_entries[i].id == id) {
                QFile::remove(absPath(m_entries[i].file));
                m_entries.removeAt(i);
                break;
            }
    saveIndex();
    emit changed();
}

// ---------------------------------------------------------------------------------------
void ResultStore::saveIndex() const {
    QJsonArray imgs, ents;
    for (const ImageInfo& i : m_images)
        imgs.append(QJsonObject{{"hash", i.hash}, {"name", i.name}, {"folder", i.folder}, {"original", i.original},
                                {"width", i.width}, {"height", i.height}});
    for (const Entry& e : m_entries)
        ents.append(QJsonObject{{"id", e.id}, {"key", e.key}, {"imageHash", e.imageHash}, {"imageName", e.imageName},
                                {"feature", e.feature}, {"title", e.title}, {"stats", e.stats}, {"time", e.time},
                                {"file", e.file}, {"params", QJsonArray::fromStringList(e.params)},
                                {"log", QJsonArray::fromStringList(e.log)}, {"values", e.values}, {"extra", e.extra}});
    QSaveFile f(absPath("library.json"));
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(QJsonObject{{"version", 1}, {"images", imgs}, {"entries", ents}}).toJson(QJsonDocument::Indented));
    f.commit();
}

void ResultStore::loadIndex() {
    QFile f(absPath("library.json"));
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    auto strs = [](const QJsonValue& v) {
        QStringList l;
        for (const QJsonValue& x : v.toArray()) l << x.toString();
        return l;
    };
    for (const QJsonValue& v : root["images"].toArray()) {
        QJsonObject o = v.toObject();
        ImageInfo i{o["hash"].toString(), o["name"].toString(), o["folder"].toString(), o["original"].toString(),
                    o["width"].toInt(), o["height"].toInt()};
        if (QFile::exists(absPath(i.original))) m_images.append(i);
    }
    for (const QJsonValue& v : root["entries"].toArray()) {
        QJsonObject o = v.toObject();
        Entry e;
        e.id = o["id"].toString();
        e.key = o["key"].toString();
        e.imageHash = o["imageHash"].toString();
        e.imageName = o["imageName"].toString();
        e.feature = o["feature"].toString();
        e.title = o["title"].toString();
        e.stats = o["stats"].toString();
        e.time = o["time"].toString();
        e.file = o["file"].toString();
        e.params = strs(o["params"]);
        e.log = strs(o["log"]);
        e.values = o["values"].toObject();
        e.extra = o["extra"].toObject();
        if (QFile::exists(absPath(e.file))) m_entries.append(e);  // silently drop files the user deleted
    }
}
