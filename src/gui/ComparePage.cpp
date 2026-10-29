#include "ComparePage.h"

#include <QComboBox>
#include <QFrame>
#include <QHash>
#include <functional>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

#include "Features.h"
#include "ImageView.h"
#include "Theme.h"

namespace {

constexpr int kThumbW = 196, kThumbH = 128, kCardW = 214;

QString chipStyle(const QColor& c) {
    return QString("background: rgba(%1,%2,%3,0.20); color: %4; border-radius: 8px; padding: 1px 8px; font-size: 11px; font-weight: 600;")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.name());
}

// A clickable card (thumbnail + title + parameters).
class Card : public QFrame {
public:
    std::function<void()> onClick;
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && onClick) onClick();
    }
};

void repolish(QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

}  // namespace

ComparePage::ComparePage(ResultStore* store, QWidget* parent) : QWidget(parent), m_store(store) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 8, 0, 0);
    root->setSpacing(10);

    // ---- filter bar ----
    auto* bar = new QHBoxLayout;
    bar->setSpacing(8);
    m_imageCombo = new QComboBox;
    m_imageCombo->setMinimumWidth(170);
    m_featureCombo = new QComboBox;
    m_featureCombo->setMinimumWidth(150);
    m_count = new QLabel;
    m_count->setObjectName("muted");
    m_clearBtn = new QPushButton("Clear selection");
    m_delBtn = new QPushButton("Delete selected");
    auto* open = new QPushButton("Open results folder");
    for (QPushButton* b : {m_clearBtn, m_delBtn, open}) b->setCursor(Qt::PointingHandCursor);
    auto lbl = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setObjectName("muted");
        return l;
    };
    bar->addWidget(lbl("Image"));
    bar->addWidget(m_imageCombo);
    bar->addWidget(lbl("Feature"));
    bar->addWidget(m_featureCombo);
    bar->addWidget(m_count, 1);
    bar->addWidget(m_clearBtn);
    bar->addWidget(m_delBtn);
    bar->addWidget(open);
    root->addLayout(bar);

    // ---- gallery (top) + compare viewer (bottom) ----
    m_galleryHost = new QWidget;
    m_grid = new QGridLayout(m_galleryHost);
    m_grid->setContentsMargins(10, 10, 10, 10);
    m_grid->setSpacing(10);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_galleryScroll = new QScrollArea;
    m_galleryScroll->setObjectName("scroll");
    m_galleryScroll->setWidgetResizable(true);
    m_galleryScroll->setFrameShape(QFrame::NoFrame);
    m_galleryScroll->setWidget(m_galleryHost);

    auto* galleryCard = new QFrame;
    galleryCard->setObjectName("card");
    auto* gl = new QVBoxLayout(galleryCard);
    gl->setContentsMargins(0, 0, 0, 0);
    auto* gt = new QLabel("  Saved results  -  click a card to add it to the comparison");
    gt->setObjectName("cardTitle");
    gt->setContentsMargins(8, 9, 0, 8);
    auto* sep = new QFrame;
    sep->setObjectName("sep");
    gl->addWidget(gt);
    gl->addWidget(sep);
    gl->addWidget(m_galleryScroll, 1);

    m_compareHost = new QWidget;
    m_compareLay = new QHBoxLayout(m_compareHost);
    m_compareLay->setContentsMargins(0, 0, 0, 0);
    m_compareLay->setSpacing(10);
    auto* compareScroll = new QScrollArea;
    compareScroll->setObjectName("scroll");
    compareScroll->setWidgetResizable(true);
    compareScroll->setFrameShape(QFrame::NoFrame);
    compareScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    compareScroll->setWidget(m_compareHost);

    m_split = new QSplitter(Qt::Vertical);
    m_split->setChildrenCollapsible(false);
    m_split->addWidget(galleryCard);
    m_split->addWidget(compareScroll);
    m_split->setStretchFactor(0, 3);
    m_split->setStretchFactor(1, 7);
    m_split->setSizes({300, 560});
    root->addWidget(m_split, 1);

    connect(m_imageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (!m_blockFilter) rebuildGallery();
    });
    connect(m_featureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (!m_blockFilter) rebuildGallery();
    });
    connect(m_clearBtn, &QPushButton::clicked, this, [this] {
        m_selected.clear();
        rebuildGallery();
        rebuildCompare();
    });
    connect(m_delBtn, &QPushButton::clicked, this, &ComparePage::deleteSelected);
    connect(open, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(m_store->root())); });

    refresh();
}

void ComparePage::setDark(bool dark) {
    m_dark = dark;
    for (ImageView* v : m_views) v->setColors(Theme::viewBackground(dark), Theme::viewForeground(dark));
}

QList<ComparePage::Item> ComparePage::collect() const {
    QList<Item> out;
    for (const ResultStore::ImageInfo& i : m_store->images()) {
        Item it;
        it.id = "orig:" + i.hash;
        it.imageHash = i.hash;
        it.imageName = i.name;
        it.feature = "original";
        it.title = "Original image";
        it.stats = QString("%1 x %2 px").arg(i.width).arg(i.height);
        it.path = m_store->absPath(i.original);
        it.isOriginal = true;
        out.append(it);
        for (const ResultStore::Entry& e : m_store->entries()) {
            if (e.imageHash != i.hash) continue;
            Item r;
            r.id = e.id;
            r.imageHash = e.imageHash;
            r.imageName = e.imageName;
            r.feature = e.feature;
            r.title = e.title;
            r.stats = e.stats;
            r.time = e.time;
            r.params = e.params;
            r.path = m_store->absPath(e.file);
            out.append(r);
        }
    }
    return out;
}

void ComparePage::refresh() {
    // filters
    m_blockFilter = true;
    QString curImg = m_imageCombo->currentData().toString();
    QString curFeat = m_featureCombo->currentData().toString();
    m_imageCombo->clear();
    m_imageCombo->addItem("All images", QString());
    for (const ResultStore::ImageInfo& i : m_store->images()) m_imageCombo->addItem(i.name, i.hash);
    m_featureCombo->clear();
    m_featureCombo->addItem("All features", QString());
    m_featureCombo->addItem("Original images", "original");
    for (const FeatureInfo& f : kFeatures) m_featureCombo->addItem(f.title, f.id);
    int ii = m_imageCombo->findData(curImg);
    int fi = m_featureCombo->findData(curFeat);
    m_imageCombo->setCurrentIndex(ii < 0 ? 0 : ii);
    m_featureCombo->setCurrentIndex(fi < 0 ? 0 : fi);
    m_blockFilter = false;

    // drop selections that no longer exist
    QStringList valid;
    for (const Item& it : collect()) valid << it.id;
    m_selected.erase(std::remove_if(m_selected.begin(), m_selected.end(), [&](const QString& s) { return !valid.contains(s); }),
                     m_selected.end());
    rebuildGallery();
    rebuildCompare();
}

void ComparePage::showImage(const QString& hash) {
    int i = m_imageCombo->findData(hash);
    if (i >= 0) m_imageCombo->setCurrentIndex(i);
}

void ComparePage::select(const QStringList& ids) {
    m_selected = ids;
    rebuildGallery();
    rebuildCompare();
}

QPixmap ComparePage::thumb(const Item& it) {
    QString key = it.path + "|" + it.time;
    auto f = m_thumbCache.find(key);
    if (f != m_thumbCache.end()) return *f;
    QImage img(it.path);
    QPixmap pm = QPixmap::fromImage(img.scaled(kThumbW * 2, kThumbH * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pm.setDevicePixelRatio(2.0);
    m_thumbCache.insert(key, pm);
    return pm;
}

void ComparePage::rebuildGallery() {
    // clear
    while (QLayoutItem* li = m_grid->takeAt(0)) {
        if (QWidget* w = li->widget()) w->deleteLater();
        delete li;
    }
    m_cards.clear();
    m_items.clear();

    QString img = m_imageCombo->currentData().toString();
    QString feat = m_featureCombo->currentData().toString();
    for (const Item& it : collect()) {
        if (!img.isEmpty() && it.imageHash != img) continue;
        if (!feat.isEmpty() && it.feature != feat) continue;
        m_items.append(it);
    }

    for (const Item& it : m_items) {
        auto* card = new Card;
        card->setObjectName("gcard");
        card->setFixedWidth(kCardW);
        card->setCursor(Qt::PointingHandCursor);
        card->setProperty("sel", m_selected.contains(it.id));
        QString id = it.id;
        card->onClick = [this, id] { toggle(id); };

        auto* v = new QVBoxLayout(card);
        v->setContentsMargins(8, 8, 8, 8);
        v->setSpacing(4);
        auto* pic = new QLabel;
        pic->setFixedSize(kThumbW, kThumbH);
        pic->setAlignment(Qt::AlignCenter);
        pic->setObjectName("thumb");
        pic->setPixmap(thumb(it));
        pic->setAttribute(Qt::WA_TransparentForMouseEvents);
        v->addWidget(pic);

        auto* top = new QHBoxLayout;
        auto* chip = new QLabel(featureTitle(it.feature));
        chip->setStyleSheet(chipStyle(featureColor(it.feature)));
        top->addWidget(chip);
        top->addStretch();
        auto* order = new QLabel;
        order->setObjectName("orderBadge");
        int pos = m_selected.indexOf(it.id);
        order->setText(pos >= 0 ? QString::number(pos + 1) : QString());
        order->setVisible(pos >= 0);
        top->addWidget(order);
        v->addLayout(top);

        auto* name = new QLabel(it.imageName);
        name->setObjectName("cardTitle");
        name->setToolTip(it.imageName);
        v->addWidget(name);
        QString small = it.isOriginal ? it.stats : it.title;
        auto* t = new QLabel(small);
        t->setObjectName("help");
        t->setWordWrap(true);
        v->addWidget(t);
        card->setToolTip(it.params.join("\n"));
        m_cards.append(card);
    }
    if (m_items.isEmpty()) {
        auto* empty = new QLabel("Nothing here yet.\nOpen an image and run a feature - every result is saved automatically\nand will show up in this tab.");
        empty->setObjectName("muted");
        empty->setAlignment(Qt::AlignCenter);
        m_grid->addWidget(empty, 0, 0);
        m_cards.append(empty);
    }
    reflow();
    int nResults = 0;
    for (const Item& it : m_items) nResults += it.isOriginal ? 0 : 1;
    m_count->setText(QString("%1 saved results  -  %2 selected").arg(nResults).arg(m_selected.size()));
    m_clearBtn->setEnabled(!m_selected.isEmpty());
    bool anyResult = false;
    for (const QString& s : m_selected) anyResult |= !s.startsWith("orig:");
    m_delBtn->setEnabled(anyResult);
}

void ComparePage::reflow() {
    if (m_cards.isEmpty()) return;
    if (m_items.isEmpty()) return;
    int avail = m_galleryScroll->viewport()->width() - 20;
    int cols = std::max(1, (avail + 10) / (kCardW + 10));
    for (QWidget* c : m_cards) m_grid->removeWidget(c);
    for (int i = 0; i < m_cards.size(); ++i) m_grid->addWidget(m_cards[i], i / cols, i % cols);
}

void ComparePage::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    reflow();
}

void ComparePage::toggle(const QString& id) {
    if (m_selected.contains(id)) m_selected.removeAll(id);
    else m_selected.append(id);
    // update card look in place (no full rebuild -> no flicker, scroll position stays)
    for (int i = 0; i < m_items.size() && i < m_cards.size(); ++i) {
        bool sel = m_selected.contains(m_items[i].id);
        m_cards[i]->setProperty("sel", sel);
        repolish(m_cards[i]);
        if (auto* badge = m_cards[i]->findChild<QLabel*>("orderBadge")) {
            int pos = m_selected.indexOf(m_items[i].id);
            badge->setText(pos >= 0 ? QString::number(pos + 1) : QString());
            badge->setVisible(pos >= 0);
        }
    }
    int nResults = 0;
    for (const Item& it : m_items) nResults += it.isOriginal ? 0 : 1;
    m_count->setText(QString("%1 saved results  -  %2 selected").arg(nResults).arg(m_selected.size()));
    m_clearBtn->setEnabled(!m_selected.isEmpty());
    bool anyResult = false;
    for (const QString& s : m_selected) anyResult |= !s.startsWith("orig:");
    m_delBtn->setEnabled(anyResult);
    rebuildCompare();
}

void ComparePage::rebuildCompare() {
    while (QLayoutItem* li = m_compareLay->takeAt(0)) {
        if (QWidget* w = li->widget()) w->deleteLater();
        delete li;
    }
    m_views.clear();

    if (m_selected.isEmpty()) {
        auto* hint = new QLabel("Select two or more cards above to compare them side by side.\n"
                                "Tip: select the Original image too, then zoom with the mouse wheel - all panels zoom together.");
        hint->setObjectName("muted");
        hint->setAlignment(Qt::AlignCenter);
        m_compareLay->addWidget(hint);
        return;
    }

    QList<Item> all = collect();
    for (const QString& id : m_selected) {
        auto f = std::find_if(all.begin(), all.end(), [&](const Item& i) { return i.id == id; });
        if (f == all.end()) continue;
        const Item it = *f;

        auto* panel = new QFrame;
        panel->setObjectName("card");
        panel->setMinimumWidth(300);
        auto* v = new QVBoxLayout(panel);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);

        auto* head = new QVBoxLayout;
        head->setContentsMargins(12, 9, 8, 8);
        head->setSpacing(4);
        auto* row = new QHBoxLayout;
        auto* chip = new QLabel(featureTitle(it.feature));
        chip->setStyleSheet(chipStyle(featureColor(it.feature)));
        row->addWidget(chip);
        row->addStretch();
        if (!it.isOriginal) {
            auto* use = new QPushButton("Use these settings");
            use->setObjectName("ghost");
            use->setCursor(Qt::PointingHandCursor);
            use->setToolTip("Go back to this feature page with exactly these parameters");
            QString eid = it.id;
            connect(use, &QPushButton::clicked, this, [this, eid] { emit restoreRequested(eid); });
            row->addWidget(use);
        }
        auto* x = new QPushButton("x");
        x->setObjectName("ghost");
        x->setToolTip("Remove from the comparison");
        x->setCursor(Qt::PointingHandCursor);
        QString sid = it.id;
        connect(x, &QPushButton::clicked, this, [this, sid] { toggle(sid); });
        row->addWidget(x);
        head->addLayout(row);
        auto* name = new QLabel(it.imageName + (it.isOriginal ? "" : "  -  " + it.title));
        name->setObjectName("cardTitle");
        name->setWordWrap(true);
        head->addWidget(name);
        v->addLayout(head);
        auto* sep = new QFrame;
        sep->setObjectName("sep");
        v->addWidget(sep);

        auto* view = new ImageView("");
        RgbImage rgb;
        if (ImageIO::loadRgb(it.path.toStdString(), rgb)) view->setImage(rgb);
        view->setColors(Theme::viewBackground(m_dark), Theme::viewForeground(m_dark));
        v->addWidget(view, 1);
        m_views.append(view);

        auto* info = new QTextBrowser;
        info->setFrameShape(QFrame::NoFrame);
        info->setFixedHeight(it.isOriginal ? 40 : 110);
        info->setObjectName("paramBox");
        QString html = "<div style='color:" + QString(m_dark ? "#8b93a7" : "#6b7280") + "'>" + it.stats.toHtmlEscaped() + "</div>";
        if (!it.params.isEmpty()) {
            QStringList esc;
            for (const QString& p : it.params) esc << p.toHtmlEscaped();
            html += "<div>" + esc.join("<br>") + "</div>";
        }
        info->setHtml(html);
        v->addWidget(info);

        m_compareLay->addWidget(panel, 1);
    }

    // link zoom / pan of all panels
    for (ImageView* a : m_views)
        for (ImageView* b : m_views)
            if (a != b) connect(a, &ImageView::viewChanged, b, &ImageView::setView);
}

void ComparePage::deleteSelected() {
    QStringList ids;
    for (const QString& s : m_selected)
        if (!s.startsWith("orig:")) ids << s;
    if (ids.isEmpty()) return;
    auto r = QMessageBox::question(this, "Delete results",
                                   QString("Delete %1 saved result(s) and their image files from disk?").arg(ids.size()));
    if (r != QMessageBox::Yes) return;
    m_selected.erase(std::remove_if(m_selected.begin(), m_selected.end(), [&](const QString& s) { return ids.contains(s); }),
                     m_selected.end());
    m_store->remove(ids);  // emits changed() -> refresh()
}
