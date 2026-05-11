#include <QApplication>
#include "core/Application.h"
#include "utils/PathProvider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    PathProvider::ensureConfigExists();
    
    Application morph;
    morph.start();
    
    return app.exec();
}
