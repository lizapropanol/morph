#include <QGuiApplication>
#include <QDebug>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    PathProvider::ensureConfigExists();
    qDebug() << "MORPH_START: Loading config from:" << PathProvider::getStyleFilePath();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
