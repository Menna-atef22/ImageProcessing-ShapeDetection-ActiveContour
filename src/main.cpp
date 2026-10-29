#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");  // same look on every OS (the stylesheet does the rest)
    QApplication::setApplicationName("VisionLab");
    QApplication::setOrganizationName("ImageProcessingAssignment");

    // Optional: --shot out.png [--image file] [--page N] [--tab N]  -> saves a screenshot and exits
    // (used for documentation / automated checks; normal users just double-click the app)
    QCommandLineParser cli;
    QCommandLineOption shot("shot", "Save a screenshot and quit.", "file");
    QCommandLineOption image("image", "Image to open.", "file");
    QCommandLineOption page("page", "Feature page index.", "n", "0");
    QCommandLineOption tab("tab", "Main tab (0 workspace, 1 compare).", "n", "0");
    QCommandLineOption run("run", "Press the Run button of the selected page.");
    QCommandLineOption cmp("compare", "Select the N newest results in the Compare tab.", "n");
    cli.addOptions({shot, image, page, tab, run, cmp});
    cli.process(app);

    MainWindow w;
    w.show();
    if (cli.isSet(image)) w.loadImage(cli.value(image));
    w.selectPage(cli.value(page).toInt());
    w.selectTab(cli.value(tab).toInt());
    if (cli.isSet(run)) w.runCurrent();
    if (cli.isSet(cmp)) w.compareLatest(cli.value(cmp).toInt());
    if (cli.isSet(shot)) {
        QString out = cli.value(shot);
        QTimer::singleShot(600, [&w, out, &app] {
            w.grab().save(out);
            app.quit();
        });
    }
    return app.exec();
}
