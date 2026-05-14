#include <QApplication>
#include <QFontDatabase>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    int fontId = QFontDatabase::addApplicationFont(":/assets/fonts/NimbusSans.otf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont nimbus(family);
        app.setFont(nimbus);
    }
    QFontDatabase::addApplicationFont(":/assets/fonts/NimbusSans-Bold.otf");
    
    PathProvider::ensureConfigExists();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
