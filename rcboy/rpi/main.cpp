#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include "platform/platform.h"
#include "driver.h"

#include "log.h"
#include "gui.h"

#include "utils/json.h"
#include "utils/exec.h"


int main(int argc, char * argv[]) {
    // initialize the driver singleton and peripherals
    Driver * driver = Driver::initialize();
    

    // initialize the GUI
    LOG("initializing GUI...");
    LOG("  QT");

    return GUI::exec(argc, argv);

    QApplication app{argc, argv};




    LOG("  main window");
    QWidget w;
    //MainWindow w;
    w.setFixedSize(QSize{320,240});
    QPushButton * b = new QPushButton{&w};
    b->setText("Hello all");
    b->setGeometry(QRect{-10,-10,50,100});
    // we don't do mouse cursor
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    w.show();


    // though we actually never plan to return really 
    LOG("running...");
    return app.exec();
}