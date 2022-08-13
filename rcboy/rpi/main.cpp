#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include "platform/platform.h"
#include "gamepad.h"

#include <QGamepadManager>



int main(int argc, char * argv[]) {
    // first initialize the gpio and enable the gamepad driver
    gpio::initialize();

    Gamepad::initialize();
    std::cout << "gamepad initialized" << std::endl;

    //cpu::delay_ms(1000);

    // then start the gui application
    QApplication app(argc, argv);
    std::cout << "qt initialized" << std::endl;

    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    std::cout << "Found " << gamepads.size() << " connected gamepads" << std::endl;
    for (auto i : gamepads) {
        std::cout << "Gamepad " << i << ": " << QGamepadManager::instance()->gamepadName(i).toStdString() << std::endl;
    }

    while (true) {}




    /*
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
    */


    // though we actually never plan to return really 
    return app.exec();
}