#include <cstdlib>
#include <iostream>

#include <QApplication>

#include "main_window.h"


int main(int argc, char * argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.showFullScreen();
    return app.exec();
}


