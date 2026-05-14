#include <QApplication>
#include <QFontDatabase>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QFontDatabase::addApplicationFont(":/assets/fonts/Rubik.ttf");
    QFontDatabase::addApplicationFont(":/assets/fonts/Rubik-Italic.ttf");
    
    PathProvider::ensureConfigExists();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
