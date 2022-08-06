#include <cstdlib>
#include <iostream>

#include <QApplication>

#include "main_window.h"


int main(int argc, char * argv[]) {
    // start main window in 320x240 size which is the display
    QApplication app(argc, argv);
    MainWindow w;
    w.setFixedSize(QSize{320,240});
    // we don't do mouse cursor
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    w.show();
    return app.exec();
}


