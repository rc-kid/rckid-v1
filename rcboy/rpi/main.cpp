#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include "platform/platform.h"
#include "gamepad.h"

#include "log.h"


void initialize() {
    LOG("  gpio");
    // first initialize the gpio and enable the gamepad driver
    gpio::initialize();
    LOG("  avr");

    LOG("  gamepad");
    Gamepad::initialize();
}



int main(int argc, char * argv[]) {
    // initialize the HW, check peripherals
    LOG("initializing hardware...");
    initialize();

    // initialize the GUI
    LOG("initializing GUI...");
    LOG("  QT");

    while (true) {}
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