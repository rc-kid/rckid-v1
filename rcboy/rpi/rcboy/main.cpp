#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include <QGamepadManager>


#include "main_window.h"


int main(int argc, char * argv[]) {
    // start main window in 320x240 size which is the display
    QApplication app(argc, argv);

    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    std::cout << "Found " << gamepads.size() << " connected gamepads" << std::endl;
    for (auto i : gamepads) {
        std::cout << "Gamepad " << i << ": " << QGamepadManager::instance()->gamepadName(i).toStdString() << std::endl;
    }
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
    return app.exec();
}


