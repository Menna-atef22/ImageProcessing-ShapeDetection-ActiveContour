#pragma once
#include <QColor>
#include <QHash>
#include <QPixmap>
#include <QStringList>
#include <QWidget>

#include "ResultStore.h"

class QComboBox;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QSplitter;
class ImageView;

// "Compare" tab: gallery of everything saved on disk (original + every result), filters,
// and a side-by-side viewer (zoom / pan are linked between the panels).
class ComparePage : public QWidget {
    Q_OBJECT
public:
    explicit ComparePage(ResultStore* store, QWidget* parent = nullptr);

    void refresh();                          // rebuild after the library changed
    void setDark(bool dark);
    void showImage(const QString& imageHash);  // filter on one image
    void select(const QStringList& ids);       // make these the compared items

signals:
    void restoreRequested(const QString& entryId);  // "use these settings" was pressed

protected:
    void resizeEvent(QResizeEvent*) override;

private:
    struct Item {
        QString id;  // entry id, or "orig:<hash>" for an original image
        QString imageHash, imageName, feature, title, stats, time;
        QStringList params;
        QString path;  // absolute png path
        bool isOriginal = false;
    };
    QList<Item> collect() const;
    void rebuildGallery();
    void rebuildCompare();
    void reflow();
    void toggle(const QString& id);
    void deleteSelected();
    QPixmap thumb(const Item& it);

    ResultStore* m_store;
    bool m_dark = true;
    QComboBox *m_imageCombo, *m_featureCombo;
    QLabel* m_count;
    QPushButton *m_clearBtn, *m_delBtn;
    QScrollArea* m_galleryScroll;
    QWidget* m_galleryHost;
    QGridLayout* m_grid;
    QWidget* m_compareHost;
    QHBoxLayout* m_compareLay;
    QSplitter* m_split;

    QList<Item> m_items;       // everything matching the filters (gallery order)
    QList<QWidget*> m_cards;   // parallel to m_items
    QStringList m_selected;    // ids, in the order the user clicked
    QList<ImageView*> m_views;
    QHash<QString, QPixmap> m_thumbCache;
    bool m_blockFilter = false;
};
