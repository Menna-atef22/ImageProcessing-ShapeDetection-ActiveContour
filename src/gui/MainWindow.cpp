#include "MainWindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMetaObject>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QThreadPool>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <memory>

#include "../common/Draw.h"
#include "../task1/CircleDetector.h"
#include "../task1/EdgeDetector.h"
#include "../task1/EllipseDetector.h"
#include "../task1/LineDetector.h"
#include "../task2/ActiveContour.h"
#include "../task2/ContourAnalysis.h"
#include "ComparePage.h"
#include "Features.h"
#include "ImageView.h"
#include "ParamRow.h"
#include "Theme.h"

namespace {

constexpr double kPi = 3.14159265358979;

// Flip a feature to `true` once its algorithm is implemented: its results then get saved to the library.
// (While false, an empty result - "0 circles" - is shown but not saved, so the Compare tab stays clean.)
bool isReady(const QString& feature) { return feature == "canny" || feature == "lines"; }

void repolish(QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void drawPolyline(RgbImage& img, const std::vector<Point>& pts, Color c, bool closed = true) {
    if (pts.size() < 2) return;
    for (size_t i = 0; i + 1 < pts.size(); ++i) Draw::line(img, pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, c, 2);
    if (closed) Draw::line(img, pts.back().x, pts.back().y, pts.front().x, pts.front().y, c, 2);
    for (const Point& p : pts) Draw::dot(img, p.x, p.y, 2, c);
}

QWidget* withMargins(QWidget* inner, int l = 14, int t = 10, int r = 14, int b = 12) {
    auto* w = new QWidget;
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(l, t, r, b);
    lay->addWidget(inner);
    return w;
}

QIcon monoIcon(const QString& mono, const QColor& color) {
    QPixmap pm(56, 56);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(QRectF(2, 2, 52, 52), 14, 14);
    p.setPen(Qt::white);
    QFont f;
    f.setPixelSize(24);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRectF(0, 0, 56, 56), Qt::AlignCenter, mono);
    return QIcon(pm);
}

QString hashPoints(const std::vector<Point>& pts) {
    QCryptographicHash h(QCryptographicHash::Sha1);
    for (const Point& p : pts) {
        int v[2] = {p.x, p.y};
        h.addData(QByteArray::fromRawData(reinterpret_cast<const char*>(v), int(sizeof(v))));
    }
    return QString::fromLatin1(h.result().toHex().left(12));
}

QJsonArray contourToJson(const std::vector<Point>& pts) {
    QJsonArray a;
    for (const Point& p : pts) a.append(QJsonArray{p.x, p.y});
    return a;
}

std::vector<Point> contourFromJson(const QJsonValue& v) {
    std::vector<Point> out;
    for (const QJsonValue& p : v.toArray()) out.push_back({p.toArray().at(0).toInt(), p.toArray().at(1).toInt()});
    return out;
}

}  // namespace

// =====================================================================
//  Construction
// =====================================================================
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Vision Lab - Image Processing Assignment");
    setAcceptDrops(true);
    m_store = new ResultStore(ResultStore::defaultRoot(), this);
    buildUi();
    loadSettings();
    applyTheme();
    resize(1440, 860);
    setMinimumSize(1080, 660);
    connect(m_store, &ResultStore::changed, this, &MainWindow::refreshLibraryUi);
    connect(m_compare, &ComparePage::restoreRequested, this, &MainWindow::restoreEntry);
    refreshLibraryUi();

    log("Welcome! Open an image (button above, or drag & drop a file) then pick a feature on the left.");
    log(QString("Every result is saved automatically in:  %1").arg(QDir::toNativeSeparators(m_store->root())));
    if (!m_store->entries().isEmpty())
        log(QString("%1 saved results were found - see the Compare tab. Re-running with the same image and settings loads the saved result instantly.")
                .arg(m_store->entries().size()));

    QSettings s;
    QString last = s.value("lastImageHash").toString();
    if (!last.isEmpty() && m_store->imageInfo(last)) loadFromStore(last);
}

MainWindow::~MainWindow() { QThreadPool::globalInstance()->waitForDone(); }

void MainWindow::buildUi() {
    auto* root = new QWidget;
    root->setObjectName("root");
    auto* h = new QHBoxLayout(root);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->addWidget(buildSidebar());
    h->addWidget(buildControls());
    h->addWidget(buildMainArea(), 1);
    setCentralWidget(root);
}

void MainWindow::selectPage(int index) {
    if (index < 0 || index >= m_stack->count()) return;
    m_stack->setCurrentIndex(index);
    if (auto* b = m_navGroup->button(index)) b->setChecked(true);
    if (m_tabs && m_tabs->currentIndex() != 0) m_tabs->setCurrentIndex(0);
    if (index == kSnakePage) updateSnakePreview();
}

void MainWindow::selectTab(int index) {
    if (m_tabs) m_tabs->setCurrentIndex(index);
}

void MainWindow::compareLatest(int n) {
    QStringList ids;
    const auto& list = m_store->entries();
    for (int i = std::max(0, int(list.size()) - n); i < list.size(); ++i) ids << list[i].id;
    m_compare->select(ids);
}

void MainWindow::runCurrent() {
    int i = m_stack->currentIndex();
    if (i >= 0 && i < m_runButtons.size()) m_runButtons[i]->click();
}

QWidget* MainWindow::buildSidebar() {
    auto* side = new QWidget;
    side->setObjectName("sidebar");
    side->setFixedWidth(215);
    auto* v = new QVBoxLayout(side);
    v->setContentsMargins(14, 18, 14, 14);
    v->setSpacing(2);

    auto* brand = new QHBoxLayout;
    auto* logo = new QLabel("VL");
    logo->setObjectName("logo");
    auto* names = new QVBoxLayout;
    names->setSpacing(0);
    auto* title = new QLabel("Vision Lab");
    title->setObjectName("appTitle");
    auto* sub = new QLabel("Image Processing");
    sub->setObjectName("appSub");
    names->addWidget(title);
    names->addWidget(sub);
    brand->addWidget(logo);
    brand->addSpacing(6);
    brand->addLayout(names);
    brand->addStretch();
    v->addLayout(brand);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    auto section = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setObjectName("sectionLabel");
        v->addWidget(l);
    };
    auto nav = [&](int idx) {
        const FeatureInfo& f = kFeatures[idx];
        auto* b = new QPushButton(f.title);
        b->setObjectName("nav");
        b->setCheckable(true);
        b->setIcon(monoIcon(f.mono, QColor(f.color)));
        b->setIconSize(QSize(26, 26));
        b->setCursor(Qt::PointingHandCursor);
        m_navGroup->addButton(b, idx);
        if (idx == 0) b->setChecked(true);
        connect(b, &QPushButton::clicked, this, [this, idx] { selectPage(idx); });
        v->addWidget(b);
    };
    section("TASK 1  -  EDGES & SHAPES");
    for (int i = 0; i < 4; ++i) nav(i);
    section("TASK 2  -  CONTOURS");
    for (int i = 4; i < kFeatureCount; ++i) nav(i);

    v->addStretch();
    section("STORAGE");
    m_reuse = new QCheckBox("Reuse saved results");
    m_reuse->setChecked(true);
    m_reuse->setToolTip("Same image + same settings -> load the saved picture instead of computing it again.");
    v->addWidget(m_reuse);
    m_pathLabel = new QLabel;
    m_pathLabel->setObjectName("help");
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setText(QDir::toNativeSeparators(m_store->root()));
    m_pathLabel->setToolTip("All results are saved here");
    v->addWidget(m_pathLabel);
    auto* openFolder = new QPushButton("Open results folder");
    openFolder->setCursor(Qt::PointingHandCursor);
    connect(openFolder, &QPushButton::clicked, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(m_store->root())); });
    v->addWidget(openFolder);
    v->addSpacing(6);
    m_themeBtn = new QPushButton;
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    v->addWidget(m_themeBtn);
    return side;
}

QWidget* MainWindow::buildControls() {
    auto* panel = new QWidget;
    m_controls = panel;
    panel->setObjectName("controls");
    panel->setFixedWidth(370);
    auto* outer = new QVBoxLayout(panel);
    outer->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildCannyPage());
    m_stack->addWidget(buildLinesPage());
    m_stack->addWidget(buildCirclesPage());
    m_stack->addWidget(buildEllipsesPage());
    m_stack->addWidget(buildSnakePage());
    m_stack->addWidget(buildChainPage());

    auto* holder = new QWidget;
    auto* hl = new QVBoxLayout(holder);
    hl->setContentsMargins(16, 18, 12, 18);
    hl->addWidget(m_stack);
    hl->addStretch();

    auto* scroll = new QScrollArea;
    scroll->setObjectName("scroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(holder);
    outer->addWidget(scroll);
    return panel;
}

QFrame* MainWindow::makeCard(const QString& title, QWidget* body, QWidget* headerExtra) {
    auto* card = new QFrame;
    card->setObjectName("card");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* head = new QWidget;
    auto* hl = new QHBoxLayout(head);
    hl->setContentsMargins(14, 10, 10, 8);
    auto* t = new QLabel(title);
    t->setObjectName("cardTitle");
    hl->addWidget(t);
    hl->addStretch();
    if (headerExtra) hl->addWidget(headerExtra);
    v->addWidget(head);

    auto* sep = new QFrame;
    sep->setObjectName("sep");
    v->addWidget(sep);
    v->addWidget(body, 1);
    return card;
}

QWidget* MainWindow::makePage(int fi, const QString& description, bool ready, const QString& requires,
                              const QList<QWidget*>& params, QPushButton* runBtn, QLabel** statOut) {
    const FeatureInfo& f = kFeatures[fi];
    QColor c(f.color);
    auto* page = new QWidget;
    // accent colour of this feature: sliders, checkboxes, run button
    page->setStyleSheet(
        QString("QSlider::sub-page:horizontal { background: %1; } QSlider::handle:horizontal { border: 2px solid %1; }"
                "QCheckBox::indicator:checked { background: %1; border-color: %1; }"
                "QPushButton#primary { background: %1; } QPushButton#primary:hover { background: %2; }"
                "QLineEdit#numBox:focus, QComboBox:focus { border-color: %1; }")
            .arg(c.name(), c.lighter(115).name()));
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    auto* head = new QHBoxLayout;
    auto* mono = new QLabel(f.mono);
    mono->setFixedSize(34, 34);
    mono->setAlignment(Qt::AlignCenter);
    mono->setStyleSheet(QString("background: %1; color: white; border-radius: 9px; font-weight: 800; font-size: 14px;").arg(c.name()));
    auto* t = new QLabel(f.title);
    t->setObjectName("pageTitle");
    auto* badge = new QLabel(ready ? "Ready" : "In development");
    badge->setObjectName(ready ? "badgeReady" : "badgeDev");
    head->addWidget(mono);
    head->addSpacing(4);
    head->addWidget(t);
    head->addStretch();
    head->addWidget(badge);
    v->addLayout(head);

    auto* d = new QLabel(description);
    d->setObjectName("muted");
    d->setWordWrap(true);
    v->addWidget(d);

    if (!requires.isEmpty()) {
        auto* n = new QLabel(requires);
        n->setObjectName("note");
        n->setWordWrap(true);
        v->addWidget(n);
    }

    if (!params.isEmpty()) {
        auto* body = new QWidget;
        auto* bl = new QVBoxLayout(body);
        bl->setContentsMargins(14, 8, 14, 8);
        bl->setSpacing(4);
        for (QWidget* w : params) bl->addWidget(w);
        auto* paramCard = makeCard("Parameters", body);
        paramCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        v->addWidget(paramCard);
    }

    runBtn->setObjectName("primary");
    runBtn->setCursor(Qt::PointingHandCursor);
    m_runButtons.append(runBtn);
    v->addWidget(runBtn);

    v->addStretch();
    return page;
}

// ---------------------------------------------------------------------
//  Parameter registry
// ---------------------------------------------------------------------
ParamRow* MainWindow::addParam(const QString& feature, const QString& name, const QString& label, const QString& help,
                               double lo, double hi, double step, double value, int decimals) {
    QString key = feature + "." + name;
    auto* r = new ParamRow(label, help, lo, hi, step, value, decimals);
    m_params[key] = r;
    m_labels[key] = label;
    m_order[feature] << key;
    return r;
}

QCheckBox* MainWindow::addCheck(const QString& feature, const QString& name, const QString& text, bool checked) {
    QString key = feature + "." + name;
    auto* c = new QCheckBox(text);
    c->setChecked(checked);
    m_checks[key] = c;
    m_labels[key] = text;
    m_order[feature] << key;
    return c;
}

QComboBox* MainWindow::addCombo(const QString& feature, const QString& name, const QString& label, const QStringList& items) {
    QString key = feature + "." + name;
    auto* c = new QComboBox;
    polishCombo(c);
    c->addItems(items);
    installWheelGuard(c);
    m_combos[key] = c;
    m_labels[key] = label;
    m_order[feature] << key;
    return c;
}

double MainWindow::P(const QString& key) const { return m_params.value(key)->value(); }
int MainWindow::PI(const QString& key) const { return m_params.value(key)->intValue(); }

QJsonObject MainWindow::valuesFor(const QString& feature) const {
    QJsonObject o;
    if (feature == "chain") {
        o["contour"] = hashPoints(m_snake);
        return o;
    }
    auto add = [&](const QString& f, bool skipView) {
        for (const QString& k : m_order.value(f)) {
            if (skipView && k.endsWith(".view")) continue;
            if (m_params.contains(k)) o[k] = m_params[k]->value();
            else if (m_checks.contains(k)) o[k] = m_checks[k]->isChecked();
            else if (m_combos.contains(k)) o[k] = m_combos[k]->currentIndex();
        }
    };
    add(feature, false);
    if (feature == "lines" || feature == "circles" || feature == "ellipses") add("canny", true);  // they run on the Canny edges
    return o;
}

QStringList MainWindow::paramLines(const QString& feature) const {
    QStringList out;
    auto add = [&](const QString& f, const QString& prefix, bool skipView) {
        for (const QString& k : m_order.value(f)) {
            if (skipView && k.endsWith(".view")) continue;
            QString val;
            if (m_params.contains(k)) val = m_params[k]->valueText();
            else if (m_checks.contains(k)) val = m_checks[k]->isChecked() ? "yes" : "no";
            else if (m_combos.contains(k)) val = m_combos[k]->currentText();
            out << prefix + m_labels[k] + " = " + val;
        }
    };
    add(feature, QString(), false);
    if (feature == "lines" || feature == "circles" || feature == "ellipses") add("canny", "Canny - ", true);
    if (feature == "chain") out << QString("Snake contour points = %1").arg(m_snake.size());
    return out;
}

void MainWindow::applyValues(const QJsonObject& v) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (m_params.contains(it.key())) m_params[it.key()]->setValue(it.value().toDouble());
        else if (m_checks.contains(it.key())) m_checks[it.key()]->setChecked(it.value().toBool());
        else if (m_combos.contains(it.key())) m_combos[it.key()]->setCurrentIndex(it.value().toInt());
    }
}

// ---------------------------------------------------------------------
//  Pages (one per feature, each with its own parameters)
// ---------------------------------------------------------------------
QWidget* MainWindow::buildCannyPage() {
    auto* sigma = addParam("canny", "sigma", "Gaussian sigma",
                           "Blur strength before finding edges. Higher = smoother, less noise, fewer fine edges.", 0.0, 6.0, 0.1, 1.4, 1);
    auto* low = addParam("canny", "low", "Low threshold",
                         "Weak edges below this (fraction of the strongest edge) are discarded.", 0.0, 1.0, 0.01, 0.05, 2);
    auto* high = addParam("canny", "high", "High threshold",
                          "Edges above this are always kept. Weak edges survive only if connected to them.", 0.0, 1.0, 0.01, 0.15, 2);
    auto* view = addCombo("canny", "view", "Show", {"Final edges", "Edges over the original", "Gradient magnitude (Sobel)",
                                                    "After non-maximum suppression", "Blurred image"});
    auto* viewBox = new QWidget;
    auto* vl = new QVBoxLayout(viewBox);
    vl->setContentsMargins(0, 6, 0, 4);
    auto* vlab = new QLabel("Output to show");
    vlab->setObjectName("paramName");
    auto* vh = new QLabel("Pick which stage of the pipeline you want to look at (and save).");
    vh->setObjectName("help");
    vh->setWordWrap(true);
    vl->addWidget(vlab);
    vl->addWidget(view);
    vl->addWidget(vh);
    auto* run = new QPushButton("Run Canny Edge Detector");
    connect(run, &QPushButton::clicked, this, &MainWindow::runCanny);
    QLabel* stat = nullptr;
    return makePage(0, "Finds object boundaries: Gaussian blur, Sobel gradient, non-maximum suppression, then double-threshold hysteresis. Written from scratch.",
                    true, QString(), {sigma, low, high, viewBox}, run, &stat);
}

QWidget* MainWindow::buildLinesPage() {
    auto* vote = addParam("lines", "vote", "Vote threshold", "A line must collect at least this fraction of the best line's votes.", 0.05, 1.0, 0.05, 0.5, 2);
    auto* minv = addParam("lines", "minVotes", "Minimum votes", "Absolute minimum number of edge pixels supporting a line.", 5, 500, 5, 30, 0);
    auto* maxl = addParam("lines", "maxLines", "Max lines", "Keep only the strongest N lines.", 1, 100, 1, 20, 0);
    auto* theta = addParam("lines", "theta", "Angle step (deg)", "Resolution of the angle search. Smaller = more precise but slower.", 0.5, 5.0, 0.5, 1.0, 1);
    auto* acc = addCheck("lines", "acc", "Show Hough space (accumulator) instead", false);
    auto* run = new QPushButton("Detect Lines");
    connect(run, &QPushButton::clicked, this, &MainWindow::runLines);
    QLabel* stat = nullptr;
    return makePage(1, "Every edge pixel votes for all lines passing through it. Peaks in the vote space are the real lines.", true,
                    "Uses the Canny settings from the Canny Edges page (edges are computed automatically).",
                    {vote, minv, maxl, theta, acc}, run, &stat);
}

QWidget* MainWindow::buildCirclesPage() {
    auto* mn = addParam("circles", "minR", "Min radius (px)", "Smallest circle to look for.", 3, 300, 1, 10, 0);
    auto* mx = addParam("circles", "maxR", "Max radius (px)", "Largest circle to look for.", 5, 600, 1, 100, 0);
    auto* votes = addParam("circles", "votes", "Minimum votes", "Edge pixels needed to accept a circle.", 5, 1000, 5, 50, 0);
    auto* maxn = addParam("circles", "maxN", "Max circles", "Keep only the strongest N circles.", 1, 50, 1, 10, 0);
    auto* run = new QPushButton("Detect Circles");
    connect(run, &QPushButton::clicked, this, &MainWindow::runCircles);
    QLabel* stat = nullptr;
    return makePage(2, "Finds circles and reports each centre and radius. Detected circles are drawn on top of the image.", false,
                    "Uses the Canny settings from the Canny Edges page. Algorithm is still being implemented.",
                    {mn, mx, votes, maxn}, run, &stat);
}

QWidget* MainWindow::buildEllipsesPage() {
    auto* mn = addParam("ellipses", "minA", "Min axis (px)", "Smallest semi-axis to accept.", 3, 300, 1, 10, 0);
    auto* mx = addParam("ellipses", "maxA", "Max axis (px)", "Largest semi-axis to accept.", 5, 600, 1, 200, 0);
    auto* votes = addParam("ellipses", "votes", "Minimum votes", "Edge pixels needed to accept an ellipse.", 5, 1000, 5, 30, 0);
    auto* maxn = addParam("ellipses", "maxN", "Max ellipses", "Keep only the strongest N ellipses.", 1, 50, 1, 10, 0);
    auto* run = new QPushButton("Detect Ellipses");
    connect(run, &QPushButton::clicked, this, &MainWindow::runEllipses);
    QLabel* stat = nullptr;
    return makePage(3, "Finds ellipses (centre, axes, orientation) using the method from the assigned paper.", false,
                    "Uses the Canny settings from the Canny Edges page. Algorithm is still being implemented.",
                    {mn, mx, votes, maxn}, run, &stat);
}

QWidget* MainWindow::buildSnakePage() {
    auto* cx = addParam("snake", "cx", "Centre X", "Horizontal centre of the starting circle. Tip: click on the Original image to set it.", 0, 1000, 1, 100, 0);
    auto* cy = addParam("snake", "cy", "Centre Y", "Vertical centre of the starting circle.", 0, 1000, 1, 100, 0);
    auto* r = addParam("snake", "r", "Start radius", "Radius of the starting circle. It should surround the object.", 5, 500, 1, 60, 0);
    auto* n = addParam("snake", "n", "Contour points", "Number of snake points placed on the starting circle.", 8, 200, 1, 60, 0);
    auto* alpha = addParam("snake", "alpha", "Alpha (continuity)", "Keeps points evenly spaced. Higher = more uniform spacing.", 0.0, 5.0, 0.1, 1.0, 1);
    auto* beta = addParam("snake", "beta", "Beta (curvature)", "Penalises sharp corners. Higher = smoother contour.", 0.0, 5.0, 0.1, 1.0, 1);
    auto* gamma = addParam("snake", "gamma", "Gamma (image force)", "Pull towards strong edges. Higher = snake sticks to the object.", 0.0, 5.0, 0.1, 1.2, 1);
    auto* iter = addParam("snake", "iter", "Iterations", "How many times every point is moved.", 10, 1000, 10, 200, 0);
    auto* evo = addCheck("snake", "evolution", "Draw intermediate steps (evolution)", false);
    auto* run = new QPushButton("Evolve Snake");
    connect(run, &QPushButton::clicked, this, &MainWindow::runSnake);
    for (ParamRow* p : {cx, cy, r, n}) p->setOnChange([this] { updateSnakePreview(); });
    QLabel* stat = nullptr;
    return makePage(4, "A contour that starts around the object and shrinks onto its boundary, using the greedy algorithm (internal + external energy).",
                    false, "Cyan = starting contour, yellow = final contour. The result feeds the Chain Code page.",
                    {cx, cy, r, n, alpha, beta, gamma, iter, evo}, run, &stat);
}

QWidget* MainWindow::buildChainPage() {
    auto* run = new QPushButton("Analyze Contour");
    connect(run, &QPushButton::clicked, this, &MainWindow::runChain);

    auto makeStat = [&](const QString& name, QLabel** out) {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        auto* n = new QLabel(name);
        n->setObjectName("statName");
        *out = new QLabel("-");
        (*out)->setObjectName("bigStat");
        l->addWidget(n);
        l->addWidget(*out);
        return w;
    };
    auto* grid = new QWidget;
    auto* gl = new QHBoxLayout(grid);
    gl->setContentsMargins(0, 4, 0, 4);
    gl->addWidget(makeStat("POINTS", &m_chPoints));
    gl->addWidget(makeStat("PERIMETER", &m_chPerimeter));
    gl->addWidget(makeStat("AREA", &m_chArea));

    return makePage(5, "Converts the final snake contour into an 8-direction Freeman chain code and measures the perimeter and the area inside it.",
                    false, "Run the Active Contour page first - this page analyses its final contour.", {grid}, run, nullptr);
}

QWidget* MainWindow::buildWorkspace() {
    auto* area = new QWidget;
    auto* v = new QVBoxLayout(area);
    v->setContentsMargins(0, 10, 0, 0);
    v->setSpacing(12);

    m_origView = new ImageView("Open an image to start\n(or drag & drop a file here)");
    m_resultView = new ImageView("Choose a feature on the left\nand press its Run button");
    connect(m_origView, &ImageView::clicked, this, [this](int x, int y) {
        if (m_stack->currentIndex() != kSnakePage) return;
        m_params["snake.cx"]->setValue(x);
        m_params["snake.cy"]->setValue(y);
        updateSnakePreview();
    });
    connect(m_origView, &ImageView::viewChanged, m_resultView, &ImageView::setView);  // zoom both together
    connect(m_resultView, &ImageView::viewChanged, m_origView, &ImageView::setView);

    auto* titleBox = new QWidget;
    auto* tl = new QHBoxLayout(titleBox);
    tl->setContentsMargins(0, 0, 0, 0);
    m_resultStatus = new QLabel;
    m_resultStatus->setObjectName("status");
    m_resultTitle = new QLabel;
    m_resultTitle->setObjectName("muted");
    tl->addWidget(m_resultStatus);
    tl->addWidget(m_resultTitle);

    auto* split = new QSplitter(Qt::Horizontal);
    split->setChildrenCollapsible(false);
    split->addWidget(makeCard("Original", m_origView));
    split->addWidget(makeCard("Result", m_resultView, titleBox));
    split->setSizes({1, 1});
    v->addWidget(split, 1);

    // The output log is not shown any more; messages that matter appear next to the result title.
    m_log = new QPlainTextEdit(this);
    m_log->setVisible(false);
    return area;
}

QWidget* MainWindow::buildMainArea() {
    auto* area = new QWidget;
    auto* v = new QVBoxLayout(area);
    v->setContentsMargins(18, 16, 18, 16);
    v->setSpacing(6);

    // toolbar
    auto* bar = new QHBoxLayout;
    auto* open = new QPushButton("Open Image...");
    open->setCursor(Qt::PointingHandCursor);
    m_savedImages = new QComboBox;
    polishCombo(m_savedImages);
    m_savedImages->setToolTip("Images you already worked on - they are kept in the results folder");
    m_savedImages->setMinimumWidth(150);
    auto* save = new QPushButton("Save Result As...");
    save->setCursor(Qt::PointingHandCursor);
    m_fileLabel = new QLabel("No image loaded");
    m_fileLabel->setObjectName("fileLabel");
    connect(open, &QPushButton::clicked, this, &MainWindow::openImageDialog);
    connect(save, &QPushButton::clicked, this, &MainWindow::saveResultAs);
    connect(m_savedImages, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
        if (i <= 0) return;
        loadFromStore(m_savedImages->itemData(i).toString());
        m_savedImages->setCurrentIndex(0);
    });
    bar->addWidget(open);
    bar->addWidget(m_savedImages);
    m_grayBox = new QCheckBox("Grayscale");
    m_grayBox->setToolTip("Work on a grayscale version of the image (results are saved separately for gray and colour)");
    connect(m_grayBox, &QCheckBox::toggled, this, [this](bool on) {
        if (m_busy || m_source.empty()) return;
        if (!on && m_sourceGrayOnly) {  // the colour original may still be in the library
            for (const auto& i : m_store->images()) {
                RgbImage col;
                if (i.name == m_baseName && m_store->loadOriginal(i.hash, col)) {
                    m_source = col;
                    m_sourceGrayOnly = false;
                    break;
                }
            }
        }
        applyImage();
    });
    bar->addSpacing(6);
    bar->addWidget(m_grayBox);
    bar->addSpacing(8);
    bar->addWidget(m_fileLabel, 1);
    bar->addWidget(save);
    v->addLayout(bar);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);
    v->addWidget(m_progress);

    m_compare = new ComparePage(m_store);
    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(buildWorkspace(), "Workspace");
    m_tabs->addTab(m_compare, "Compare");
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int i) {
        m_controls->setVisible(i == 0);  // the parameter panel is only useful on the Workspace tab
        if (i == 1) m_compare->showImage(m_imageHash);
    });
    v->addWidget(m_tabs, 1);
    return area;
}

// =====================================================================
//  Theme / file handling / settings
// =====================================================================
void MainWindow::applyTheme() {
    qApp->setStyleSheet(Theme::stylesheet(m_dark));
    for (ImageView* iv : {m_origView, m_resultView}) iv->setColors(Theme::viewBackground(m_dark), Theme::viewForeground(m_dark));
    m_compare->setDark(m_dark);
    m_themeBtn->setText(m_dark ? "Switch to Light theme" : "Switch to Dark theme");
}

void MainWindow::toggleTheme() {
    m_dark = !m_dark;
    applyTheme();
    m_compare->refresh();
}

void MainWindow::saveSettings() const {
    QSettings s;
    s.setValue("dark", m_dark);
    s.setValue("reuse", m_reuse->isChecked());
    s.setValue("gray", m_grayBox->isChecked());
    s.setValue("lastImageHash", m_imageHash);
    s.setValue("lastPage", m_stack->currentIndex());
    for (auto it = m_params.begin(); it != m_params.end(); ++it) s.setValue("p/" + it.key(), it.value()->value());
    for (auto it = m_checks.begin(); it != m_checks.end(); ++it) s.setValue("p/" + it.key(), it.value()->isChecked());
    for (auto it = m_combos.begin(); it != m_combos.end(); ++it) s.setValue("p/" + it.key(), it.value()->currentIndex());
}

void MainWindow::loadSettings() {
    QSettings s;
    m_dark = s.value("dark", false).toBool();
    m_reuse->setChecked(s.value("reuse", true).toBool());
    m_grayBox->setChecked(s.value("gray", false).toBool());
    for (auto it = m_params.begin(); it != m_params.end(); ++it)
        if (s.contains("p/" + it.key())) it.value()->setValue(s.value("p/" + it.key()).toDouble());
    for (auto it = m_checks.begin(); it != m_checks.end(); ++it)
        if (s.contains("p/" + it.key())) it.value()->setChecked(s.value("p/" + it.key()).toBool());
    for (auto it = m_combos.begin(); it != m_combos.end(); ++it)
        if (s.contains("p/" + it.key())) it.value()->setCurrentIndex(s.value("p/" + it.key()).toInt());
    selectPage(s.value("lastPage", 0).toInt());
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveSettings();
    QMainWindow::closeEvent(e);
}

void MainWindow::openImageDialog() {
    if (m_busy) return;
    QString path = QFileDialog::getOpenFileName(this, "Open image", "images",
                                                "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All files (*)");
    if (!path.isEmpty()) loadImage(path);
}

// True when every pixel has R == G == B (the image is already grayscale, even if stored as RGB).
static bool isNeutralGray(const RgbImage& img) {
    for (size_t i = 0; i + 2 < img.data.size(); i += 3)
        if (img.data[i] != img.data[i + 1] || img.data[i] != img.data[i + 2]) return false;
    return true;
}

void MainWindow::setImage(const RgbImage& img, const QString& name, const QString& label, bool fromStore) {
    QString base = name;
    bool wasGray = false;
    if (base.endsWith(" (gray)")) {
        base.chop(7);
        wasGray = true;
    }
    m_source = img;
    m_baseName = base;
    m_label = label;
    m_sourceGrayOnly = fromStore && wasGray;
    // The image itself decides the grayscale switch (never the previous image's setting):
    // a gray image stays gray, a colour image stays colour. The user can still tick the box manually.
    m_grayBox->blockSignals(true);
    m_grayBox->setChecked(wasGray || isNeutralGray(img));
    m_grayBox->blockSignals(false);
    applyImage();
}

void MainWindow::applyImage() {
    bool gray = m_grayBox->isChecked();
    // only add the " (gray)" suffix when a colour image is really converted (or a saved gray copy is reopened)
    const bool suffix = gray && (m_sourceGrayOnly || !isNeutralGray(m_source));
    const QString name = m_baseName + (suffix ? " (gray)" : "");
    const QString label = m_label;
    m_original = gray ? grayToRgb(toGray(m_source)) : m_source;
    m_gray = toGray(m_original);
    m_hasEdges = false;
    m_snake.clear();
    m_result = RgbImage();
    m_resultView->clear();
    m_resultTitle->clear();
    m_resultStatus->clear();
    m_origView->setImage(m_original);
    m_imageName = name;
    m_imageHash = m_store->registerImage(m_original, name);

    int w = m_original.width, h = m_original.height;
    m_params["snake.cx"]->setRange(0, w - 1);
    m_params["snake.cy"]->setRange(0, h - 1);
    m_params["snake.r"]->setRange(5, std::max(10, std::min(w, h) / 2));
    m_params["snake.cx"]->setValue(w / 2);
    m_params["snake.cy"]->setValue(h / 2);
    m_params["snake.r"]->setValue(std::max(10, std::min(w, h) / 3));
    m_params["circles.maxR"]->setValue(std::min(w, h) / 2);

    m_fileLabel->setText(name + "   -   " + QString::number(w) + " x " + QString::number(h) + " px");
    m_fileLabel->setToolTip(label);
    logHeader("Loaded image");
    log(name + " (" + QString::number(w) + " x " + QString::number(h) + ")");
    int saved = 0;
    for (const auto& e : m_store->entries()) saved += (e.imageHash == m_imageHash);
    if (saved) log(QString("%1 saved result(s) exist for this image - running the same settings again loads them instantly.").arg(saved));
    if (m_stack->currentIndex() == kSnakePage) updateSnakePreview();
}

bool MainWindow::loadImage(const QString& path) {
    if (m_busy) return false;
    RgbImage img;
    if (!ImageIO::loadRgb(path.toStdString(), img)) {
        log("ERROR: could not load image: " + path);
        return false;
    }
    setImage(img, QFileInfo(path).fileName(), path);
    return true;
}

bool MainWindow::loadFromStore(const QString& hash) {
    if (m_busy) return false;
    const ResultStore::ImageInfo* info = m_store->imageInfo(hash);
    RgbImage img;
    if (!info || !m_store->loadOriginal(hash, img)) return false;
    setImage(img, info->name, m_store->absPath(info->original), true);
    return true;
}

void MainWindow::saveResultAs() {
    if (m_result.empty()) {
        log("Nothing to save yet - run a feature first.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Save result as", "result.png", "PNG (*.png);;JPEG (*.jpg)");
    if (path.isEmpty()) return;
    if (ImageIO::saveRgb(path.toStdString(), m_result)) log("Exported: " + path);
    else log("ERROR: could not save: " + path);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    const auto urls = e->mimeData()->urls();
    if (!urls.isEmpty()) loadImage(urls.first().toLocalFile());
}

void MainWindow::refreshLibraryUi() {
    m_compare->refresh();
    m_tabs->setTabText(1, QString("Compare  (%1)").arg(m_store->entries().size()));
    m_savedImages->clear();
    m_savedImages->addItem(m_store->images().isEmpty() ? "No saved images yet" : "Saved images...");
    for (const auto& i : m_store->images()) m_savedImages->addItem(QString("%1  (%2x%3)").arg(i.name).arg(i.width).arg(i.height), i.hash);
}

// =====================================================================
//  Helpers
// =====================================================================
bool MainWindow::requireImage() {
    if (!m_original.empty()) return true;
    log("Please open an image first (Open Image... button).");
    return false;
}

void MainWindow::showResult(const RgbImage& img, const QString& title, const QString& status, bool ok) {
    m_result = img;
    m_resultView->setImage(img);
    m_resultTitle->setText(title);
    m_resultStatus->setText(status);
    m_resultStatus->setProperty("ok", ok);
    repolish(m_resultStatus);
}

void MainWindow::log(const QString& line) {
    m_log->appendPlainText(line);
    static const char* important[] = {"ERROR", "Please open", "No contour", "Nothing to save"};
    for (const char* k : important)
        if (line.startsWith(k) && m_resultStatus) {
            m_resultStatus->setText(line);
            m_resultStatus->setProperty("ok", false);
            repolish(m_resultStatus);
        }
}

void MainWindow::logHeader(const QString& title) { m_log->appendPlainText("\n== " + title + " =="); }

CannyParams MainWindow::cannyParams() const {
    CannyParams p;
    p.sigma = P("canny.sigma");
    p.lowRatio = P("canny.low");
    p.highRatio = P("canny.high");
    return p;
}

// Runs on a worker thread; only one job runs at a time (buttons are disabled meanwhile).
const EdgeResult& MainWindow::edgesFor(const CannyParams& p, bool force) {
    QString key = QString("%1|%2|%3").arg(p.sigma).arg(p.lowRatio).arg(p.highRatio);
    if (force || !m_hasEdges || key != m_edgeKey) {
        m_edges = EdgeDetector::canny(m_gray, p);
        m_hasEdges = true;
        m_edgeKey = key;
    }
    return m_edges;
}

void MainWindow::setBusy(bool busy, const QString& what) {
    m_busy = busy;
    for (QPushButton* b : m_runButtons) b->setEnabled(!busy);
    m_savedImages->setEnabled(!busy);
    m_grayBox->setEnabled(!busy);
    m_progress->setVisible(busy);
    if (busy) {
        m_resultStatus->setText("Running...");
        m_resultStatus->setProperty("ok", false);
        repolish(m_resultStatus);
        m_resultTitle->setText(what);
        QApplication::setOverrideCursor(Qt::BusyCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
}

// =====================================================================
//  Running a feature:  cache lookup -> worker thread -> show + save
// =====================================================================
void MainWindow::launch(const QString& feature, bool ready, std::function<Outcome()> work) {
    if (m_busy || !requireImage()) return;
    if (feature == "chain" && m_snake.empty()) {
        log("No contour yet - run 'Active Contour' first.");
        return;
    }

    ResultStore::Entry base;
    base.imageHash = m_imageHash;
    base.imageName = m_imageName;
    base.feature = feature;
    base.values = valuesFor(feature);
    base.params = paramLines(feature);
    base.key = ResultStore::makeKey(m_imageHash, feature, base.values);

    if (m_reuse->isChecked()) {
        if (const ResultStore::Entry* e = m_store->find(base.key)) {
            showSaved(*e, true);
            return;
        }
    }

    setBusy(true, featureTitle(feature));
    auto job = std::make_shared<std::function<Outcome()>>(std::move(work));
    QThreadPool::globalInstance()->start([this, job, base, feature, ready] {
        auto out = std::make_shared<Outcome>();
        QString err;
        try {
            *out = (*job)();
        } catch (const std::exception& ex) {
            err = QString::fromUtf8(ex.what());
        } catch (...) {
            err = "unknown error";
        }
        QMetaObject::invokeMethod(
            this, [this, feature, ready, base, out, err] { finish(feature, ready, base, *out, err); }, Qt::QueuedConnection);
    });
}

void MainWindow::finish(const QString& feature, bool ready, ResultStore::Entry base, Outcome o, const QString& error) {
    setBusy(false);
    logHeader(featureTitle(feature));
    if (!error.isEmpty()) {
        log("ERROR: " + error);
        m_resultStatus->setText("Failed");
        return;
    }
    for (const QString& l : o.log) log(l);

    if (feature == "snake") {
        m_snake = o.contour;
        o.extra["contour"] = contourToJson(o.contour);
    }
    base.title = o.title;
    base.stats = o.stat;
    base.log = o.log;
    base.extra = o.extra;

    if (o.save) {
        ResultStore::Entry saved = m_store->add(base, o.image);
        showResult(o.image, o.title, "Saved", true);
        log("Saved to: " + QDir::toNativeSeparators(m_store->absPath(saved.file)));
        updateFeatureStats(feature, saved);
    } else {
        showResult(o.image, o.title, "Not saved (empty result)", false);
        updateFeatureStats(feature, base);
    }
    saveSettings();
}

void MainWindow::updateFeatureStats(const QString& feature, const ResultStore::Entry& e) {
    if (m_stats.contains(feature)) m_stats[feature]->setText(e.stats.isEmpty() ? "-" : e.stats);
    if (feature == "chain") {
        m_chPoints->setText(QString::number(e.extra["points"].toInt()));
        m_chPerimeter->setText(QString::number(e.extra["perimeter"].toDouble(), 'f', 1));
        m_chArea->setText(QString::number(e.extra["area"].toDouble(), 'f', 1));
    }
}

void MainWindow::showSaved(const ResultStore::Entry& e, bool fromCache) {
    RgbImage img;
    if (!m_store->loadResult(e, img)) {
        log("ERROR: saved file is missing: " + e.file);
        return;
    }
    showResult(img, e.title, fromCache ? "Loaded from saved results" : "Saved result", true);
    logHeader(featureTitle(e.feature));
    for (const QString& l : e.log) log(l);
    log(fromCache ? "(Loaded from saved results - nothing was recomputed. Untick 'Reuse saved results' to force a new run.)"
                  : "(Showing a saved result.)");
    if (e.feature == "snake") m_snake = contourFromJson(e.extra["contour"]);
    updateFeatureStats(e.feature, e);
}

void MainWindow::restoreEntry(const QString& id) {
    const ResultStore::Entry* e = m_store->findById(id);
    if (!e || m_busy) return;
    ResultStore::Entry entry = *e;  // setImage() may add to the store
    if (m_imageHash != entry.imageHash && !loadFromStore(entry.imageHash)) {
        log("ERROR: the original image of this result is missing.");
        return;
    }
    applyValues(entry.values);
    int idx = 0;
    for (int i = 0; i < kFeatureCount; ++i)
        if (entry.feature == kFeatures[i].id) idx = i;
    selectPage(idx);
    showSaved(entry, false);
}

// =====================================================================
//  Feature runners (the lambdas run on a worker thread - no UI access inside!)
// =====================================================================
void MainWindow::runCanny() {
    CannyParams cp = cannyParams();
    int view = m_combos["canny.view"]->currentIndex();
    launch("canny", true, [this, cp, view] {
        Outcome o;
        QElapsedTimer t;
        t.start();
        const EdgeResult& r = edgesFor(cp, true);
        qint64 ms = t.elapsed();
        long long count = std::count_if(r.edges.data.begin(), r.edges.data.end(), [](float v) { return v > 0.f; });
        static const char* names[] = {"Final edges", "Edges over the original", "Gradient magnitude", "After non-max suppression", "Blurred image"};
        switch (view) {
            case 1: o.image = EdgeDetector::overlayEdges(m_original, r.edges); break;
            case 2: o.image = grayToRgb(normalizeTo255(r.magnitude)); break;
            case 3: o.image = grayToRgb(normalizeTo255(r.suppressed)); break;
            case 4: o.image = grayToRgb(r.blurred); break;
            default: o.image = EdgeDetector::edgesToRgb(r); break;
        }
        o.title = names[std::clamp(view, 0, 4)];
        o.stat = QString("%1 edge pixels  -  %2 ms").arg(count).arg(ms);
        o.log << QString("sigma=%1  low=%2  high=%3").arg(cp.sigma).arg(cp.lowRatio).arg(cp.highRatio)
              << QString("%1 edge pixels found in %2 ms").arg(count).arg(ms);
        return o;
    });
}

void MainWindow::runLines() {
    CannyParams cp = cannyParams();
    HoughLineParams hp;
    hp.voteRatio = P("lines.vote");
    hp.minVotes = PI("lines.minVotes");
    hp.maxLines = PI("lines.maxLines");
    hp.thetaStepDeg = P("lines.theta");
    bool showAcc = m_checks["lines.acc"]->isChecked();
    launch("lines", true, [this, cp, hp, showAcc] {
        Outcome o;
        QElapsedTimer t;
        t.start();
        const EdgeResult& e = edgesFor(cp);
        HoughLineResult r = LineDetector::detect(e.edges, hp);
        qint64 ms = t.elapsed();
        if (showAcc) {
            o.image = grayToRgb(r.accumulator);
            o.title = "Hough space (angle x distance)";
        } else {
            o.image = LineDetector::drawLines(m_original, r.lines);
            o.title = QString("%1 lines").arg(r.lines.size());
        }
        o.stat = QString("%1 lines detected  -  %2 ms").arg(r.lines.size()).arg(ms);
        o.log << QString("vote ratio=%1  min votes=%2  max lines=%3  angle step=%4 deg")
                     .arg(hp.voteRatio).arg(hp.minVotes).arg(hp.maxLines).arg(hp.thetaStepDeg);
        int i = 1;
        for (const Line& l : r.lines)
            o.log << QString("  #%1  rho=%2  theta=%3 deg  votes=%4")
                         .arg(i++).arg(l.rho, 0, 'f', 1).arg(l.theta * 180.0 / kPi, 0, 'f', 1).arg(l.votes);
        o.log << QString("%1 lines in %2 ms").arg(r.lines.size()).arg(ms);
        o.save = true;
        return o;
    });
}

void MainWindow::runCircles() {
    CannyParams cp = cannyParams();
    HoughCircleParams hp;
    hp.minRadius = PI("circles.minR");
    hp.maxRadius = PI("circles.maxR");
    hp.minVotes = PI("circles.votes");
    hp.maxCircles = PI("circles.maxN");
    launch("circles", isReady("circles"), [this, cp, hp] {
        Outcome o;
        QElapsedTimer t;
        t.start();
        const EdgeResult& e = edgesFor(cp);
        std::vector<Circle> circles = CircleDetector::detect(e.edges, hp);
        qint64 ms = t.elapsed();
        o.image = CircleDetector::drawCircles(m_original, circles);
        o.title = QString("%1 circles").arg(circles.size());
        o.stat = QString("%1 circles detected  -  %2 ms").arg(circles.size()).arg(ms);
        int i = 1;
        for (const Circle& c : circles)
            o.log << QString("  #%1  centre=(%2, %3)  radius=%4  votes=%5").arg(i++).arg(c.cx).arg(c.cy).arg(c.r).arg(c.votes);
        o.log << QString("%1 circles in %2 ms").arg(circles.size()).arg(ms);
        if (circles.empty()) o.log << "(No circles found - or the circle detector is not implemented yet.)";
        o.save = isReady("circles") || !circles.empty();
        return o;
    });
}

void MainWindow::runEllipses() {
    CannyParams cp = cannyParams();
    EllipseParams ep;
    ep.minAxis = PI("ellipses.minA");
    ep.maxAxis = PI("ellipses.maxA");
    ep.minVotes = PI("ellipses.votes");
    ep.maxEllipses = PI("ellipses.maxN");
    launch("ellipses", isReady("ellipses"), [this, cp, ep] {
        Outcome o;
        QElapsedTimer t;
        t.start();
        const EdgeResult& e = edgesFor(cp);
        std::vector<Ellipse> list = EllipseDetector::detect(e.edges, ep);
        qint64 ms = t.elapsed();
        o.image = EllipseDetector::drawEllipses(m_original, list);
        o.title = QString("%1 ellipses").arg(list.size());
        o.stat = QString("%1 ellipses detected  -  %2 ms").arg(list.size()).arg(ms);
        int i = 1;
        for (const Ellipse& el : list)
            o.log << QString("  #%1  centre=(%2, %3)  a=%4  b=%5  angle=%6 deg")
                         .arg(i++).arg(el.cx, 0, 'f', 1).arg(el.cy, 0, 'f', 1).arg(el.a, 0, 'f', 1).arg(el.b, 0, 'f', 1)
                         .arg(el.angle * 180.0 / kPi, 0, 'f', 1);
        o.log << QString("%1 ellipses in %2 ms").arg(list.size()).arg(ms);
        if (list.empty()) o.log << "(No ellipses found - or the ellipse detector is not implemented yet.)";
        o.save = isReady("ellipses") || !list.empty();
        return o;
    });
}

std::vector<Point> MainWindow::initialContour() const {
    std::vector<Point> pts;
    int n = PI("snake.n");
    double cx = P("snake.cx"), cy = P("snake.cy"), r = P("snake.r");
    for (int i = 0; i < n; ++i) {
        double a = 2.0 * kPi * i / n;
        pts.push_back({int(std::lround(cx + r * std::cos(a))), int(std::lround(cy + r * std::sin(a)))});
    }
    return pts;
}

void MainWindow::updateSnakePreview() {
    if (m_original.empty() || m_busy || m_stack->currentIndex() != kSnakePage) return;
    RgbImage out = m_original;
    drawPolyline(out, initialContour(), {0, 220, 255});
    showResult(out, "Starting contour (preview - not saved)", "", true);
}

void MainWindow::runSnake() {
    SnakeParams sp;
    sp.alpha = P("snake.alpha");
    sp.beta = P("snake.beta");
    sp.gamma = P("snake.gamma");
    sp.iterations = PI("snake.iter");
    std::vector<Point> init = initialContour();
    bool evolution = m_checks["snake.evolution"]->isChecked();
    launch("snake", isReady("snake"), [this, sp, init, evolution] {
        Outcome o;
        QElapsedTimer t;
        t.start();
        std::vector<std::vector<Point>> history;
        std::vector<Point> finalC = ActiveContour::evolve(m_gray, init, sp, &history);
        qint64 ms = t.elapsed();

        RgbImage out = m_original;
        if (evolution && history.size() > 2) {
            size_t step = std::max<size_t>(1, history.size() / 8);
            for (size_t i = step; i < history.size(); i += step) drawPolyline(out, history[i], {255, 150, 40});
        }
        drawPolyline(out, init, {0, 220, 255});
        o.image = ActiveContour::drawContour(out, finalC);
        o.title = "Snake: cyan = start, yellow = final";
        o.stat = QString("%1 contour points  -  %2 iterations  -  %3 ms").arg(finalC.size()).arg(sp.iterations).arg(ms);
        o.log << QString("alpha=%1  beta=%2  gamma=%3  iterations=%4  points=%5")
                     .arg(sp.alpha).arg(sp.beta).arg(sp.gamma).arg(sp.iterations).arg(init.size())
              << QString("Finished in %1 ms. Go to 'Chain Code' to analyse the final contour.").arg(ms);
        bool moved = !(finalC == init);
        if (!moved) o.log << "(Contour did not move - or the snake algorithm is not implemented yet.)";
        o.contour = finalC;
        o.save = isReady("snake") || moved;
        return o;
    });
}

void MainWindow::runChain() {
    std::vector<Point> contour = m_snake;
    launch("chain", isReady("chain"), [this, contour] {
        Outcome o;
        ContourMeasurements m = ContourAnalysis::analyze(contour);
        RgbImage out = m_original;
        drawPolyline(out, contour, {255, 230, 0});
        if (!contour.empty()) Draw::dot(out, contour[0].x, contour[0].y, 4, {255, 0, 0});  // chain-code start point
        o.image = out;
        o.title = "Final contour (red dot = chain-code start)";
        o.stat = QString("%1 points  -  perimeter %2  -  area %3").arg(m.numPoints).arg(m.perimeter, 0, 'f', 1).arg(m.area, 0, 'f', 1);
        o.log << QString("Points    : %1").arg(m.numPoints) << QString("Perimeter : %1 px").arg(m.perimeter, 0, 'f', 2)
              << QString("Area      : %1 px^2").arg(m.area, 0, 'f', 2);
        QString code;
        for (int c : m.chainCode) code += QString::number(c);
        QString shown = code.size() > 400 ? code.left(400) + "..." : code;
        o.log << "Chain code (" + QString::number(m.chainCode.size()) + " moves): " + (shown.isEmpty() ? "-" : shown);
        bool empty = m.chainCode.empty() && m.perimeter == 0.0;
        if (empty) o.log << "(Empty result - the chain-code module is not implemented yet.)";
        o.extra["points"] = m.numPoints;
        o.extra["perimeter"] = m.perimeter;
        o.extra["area"] = m.area;
        o.extra["chainCode"] = code;
        o.save = isReady("chain") || !empty;
        return o;
    });
}
