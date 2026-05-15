#include <QApplication>
#include <QFontDatabase>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    int fontId = QFontDatabase::addApplicationFont(":/assets/fonts/interblack.otf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        app.setFont(QFont(family));
    }
    
    PathProvider::ensureConfigExists();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
