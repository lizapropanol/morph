#include <QGuiApplication>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    PathProvider::ensureConfigExists();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
