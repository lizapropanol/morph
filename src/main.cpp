#include <QApplication>
#include <QFontDatabase>
#include <QLockFile>
#include <QDir>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QLockFile lockFile(QDir::tempPath() + "/morph.lock");
    if (!lockFile.tryLock(100)) {
        return 0;
    }
    
    int fontId = QFontDatabase::addApplicationFont(":/assets/fonts/interblack.otf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        app.setFont(QFont(family));
    }
    
    Application morph;
    morph.start();
    
    return app.exec();
}
