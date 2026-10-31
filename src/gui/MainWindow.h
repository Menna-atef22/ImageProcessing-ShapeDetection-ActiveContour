#pragma once
#include <QJsonObject>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <functional>
#include <vector>

#include "../common/Image.h"
#include "../common/Types.h"
#include "../task1/EdgeDetector.h"
#include "ResultStore.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QFrame;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class ComparePage;
class ImageView;
class ParamRow;
struct FeatureInfo;

// Layout:  [ sidebar: features ] [ controls: parameters of the selected feature ] [ tabs: Workspace | Compare ]
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // also used by --shot / automated checks
    bool loadImage(const QString& path);
    void selectPage(int index);
    void selectTab(int index);
    void runCurrent();
    void compareLatest(int n);  // select the n newest saved results in the Compare tab

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    // what a feature run produces (computed on a worker thread, shown/saved on the UI thread)
    struct Outcome {
        RgbImage image;
        QString title, stat;
        QStringList log;
        QJsonObject extra;
        std::vector<Point> contour;
        bool save = true;
    };

    // ---- UI construction ----
    void buildUi();
    QWidget* buildSidebar();
    QWidget* buildControls();
    QWidget* buildMainArea();
    QWidget* buildWorkspace();
    QFrame* makeCard(const QString& title, QWidget* body, QWidget* headerExtra = nullptr);
    QWidget* makePage(int featureIndex, const QString& description, bool ready, const QString& requires,
                      const QList<QWidget*>& params, QPushButton* runBtn, QLabel** statOut);
    QWidget* buildCannyPage();
    QWidget* buildLinesPage();
    QWidget* buildCirclesPage();
    QWidget* buildEllipsesPage();
    QWidget* buildSnakePage();
    QWidget* buildChainPage();

    // parameter registry: every control is registered under "<feature>.<name>"
    ParamRow* addParam(const QString& feature, const QString& name, const QString& label, const QString& help,
                       double lo, double hi, double step, double value, int decimals);
    QCheckBox* addCheck(const QString& feature, const QString& name, const QString& text, bool checked);
    QComboBox* addCombo(const QString& feature, const QString& name, const QString& label, const QStringList& items);
    double P(const QString& key) const;
    int PI(const QString& key) const;
    QJsonObject valuesFor(const QString& feature) const;   // incl. the Canny settings it depends on
    QStringList paramLines(const QString& feature) const;  // "Gaussian sigma = 1.4" ...
    void applyValues(const QJsonObject& v);

    // ---- actions ----
    void openImageDialog();
    void saveResultAs();
    void toggleTheme();
    void applyTheme();
    void runCanny();
    void runLines();
    void runCircles();
    void runEllipses();
    void runSnake();
    void runChain();
    void updateSnakePreview();
    void launch(const QString& feature, bool ready, std::function<Outcome()> work);
    void finish(const QString& feature, bool ready, ResultStore::Entry base, Outcome o, const QString& error);
    void showSaved(const ResultStore::Entry& e, bool fromCache);
    void restoreEntry(const QString& entryId);
    void setBusy(bool busy, const QString& what = QString());
    void loadSettings();
    void saveSettings() const;
    bool loadFromStore(const QString& hash);
    void setImage(const RgbImage& img, const QString& name, const QString& label, bool fromStore = false);
    void applyImage();  // builds the working image from the source (colour or grayscale)
    void refreshLibraryUi();

    // ---- helpers ----
    bool requireImage();
    const EdgeResult& edgesFor(const CannyParams& p, bool force = false);  // worker thread only
    CannyParams cannyParams() const;
    std::vector<Point> initialContour() const;
    bool polygonMode() const;  // true when the snake starts from the clicked polygon instead of a circle
    void showResult(const RgbImage& img, const QString& title, const QString& status = QString(), bool ok = true);
    void log(const QString& line);
    void logHeader(const QString& title);
    void updateFeatureStats(const QString& feature, const ResultStore::Entry& e);

    // ---- widgets ----
    QStackedWidget* m_stack = nullptr;
    QTabWidget* m_tabs = nullptr;
    QWidget* m_controls = nullptr;
    QButtonGroup* m_navGroup = nullptr;
    ComparePage* m_compare = nullptr;
    ImageView *m_origView = nullptr, *m_resultView = nullptr;
    QLabel *m_resultTitle = nullptr, *m_resultStatus = nullptr, *m_fileLabel = nullptr, *m_pathLabel = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QPushButton* m_themeBtn = nullptr;
    QCheckBox* m_reuse = nullptr;
    QComboBox* m_savedImages = nullptr;
    QCheckBox* m_grayBox = nullptr;
    QProgressBar* m_progress = nullptr;
    QList<QPushButton*> m_runButtons;
    QMap<QString, QLabel*> m_stats;
    QLabel *m_chPoints = nullptr, *m_chPerimeter = nullptr, *m_chArea = nullptr;

    // parameter registry
    QMap<QString, ParamRow*> m_params;
    QMap<QString, QCheckBox*> m_checks;
    QMap<QString, QComboBox*> m_combos;
    QMap<QString, QString> m_labels;
    QMap<QString, QStringList> m_order;  // feature -> keys in display order
    static constexpr int kSnakePage = 4;

    // ---- data ----
    ResultStore* m_store = nullptr;
    bool m_dark = false, m_busy = false;
    RgbImage m_source, m_original, m_result;  // source as loaded / working image (gray or colour) / shown result
    QString m_baseName, m_label;
    bool m_sourceGrayOnly = false;            // source was restored from a saved grayscale copy
    GrayImage m_gray;
    QString m_imageHash, m_imageName;
    EdgeResult m_edges;
    bool m_hasEdges = false;
    QString m_edgeKey;
    std::vector<Point> m_snake;      // last evolved contour (input of the chain-code page)
    std::vector<Point> m_snakePoly;  // points clicked by the user to define the starting contour
};
