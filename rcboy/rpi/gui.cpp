#include <QFrame>
#include <QFontDatabase>
#include <QKeyEvent>

#include <iostream>
#include "gui.h"
#include "debug_info.h"

int GUI::exec(int argc, char * argv[]) {
    QApplication app(argc, argv);
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    int id = QFontDatabase::addApplicationFont(":/fonts/OpenDyslexic.otf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);    
    std::cout << family.toStdString() << std::endl;
    QFontDatabase::addApplicationFont("qrc:/fonts/OpenDyslexic.otf");           
    GUI w{};
    w.show();
    return app.exec();
}

GUI::GUI(QWidget *parent):
    QMainWindow{parent},
    headerView_{new QGraphicsView{this}},
    pageView_{new QGraphicsView{this}},
    footerView_{new QGraphicsView{this}},
    header_{new Header{}},
    footer_{new Footer{}} {

    setFixedSize(QSize{320,240});
    headerView_->setGeometry(QRect{0,0,320,24});
    headerView_->setFrameStyle(QFrame::NoFrame);
    pageView_->setGeometry(QRect{0,24,320,192});
    pageView_->setFrameStyle(QFrame::NoFrame);
    footerView_->setGeometry(QRect{0,216,320,24});
    footerView_->setFrameStyle(QFrame::NoFrame);
    headerView_->setScene(header_);
    footerView_->setScene(footer_);
    //auto c = new DebugInfo();
    auto c = new Carousel();
    c->addElement(Carousel::Element{"Games", ":/images/gamepad.png"});
    c->addElement(Carousel::Element{"Music", ":/images/music.png"});
    c->addElement(Carousel::Element{"Movies", ":/images/video.png"});
    c->addElement(Carousel::Element{"Walkie-Talkie", ":/images/walkie-talkie.png"});
    c->addElement(Carousel::Element{"Remote", ":/images/rc-car.png"});
    c->addElement(Carousel::Element{"Torchlight", ":/images/torch.png"});
    c->addElement(Carousel::Element{"Baby Monitor", ":/images/baby-monitor.png"});
    c->addElement(Carousel::Element{"Settings", ":/images/settings.png"});
    c->setElement(0);
    pageView_->setScene(c);

}

GUI::~GUI() {
    delete header_;
    delete footer_;
}

#if (defined ARCH_MOCK)
void GUI::keyPressEvent(QKeyEvent * e) {
    switch (e->key()) {
        case Qt::Key_A: 
            emit Driver::instance()->dpadLeft(true);
            emit Driver::instance()->dpadLeft(false);
            break;
        case Qt::Key_D: 
            emit Driver::instance()->dpadRight(true);
            emit Driver::instance()->dpadRight(false);
            break;
    }
}
#endif


void GUI::Header::headphones(bool stage) {

}

void GUI::Header::charging(bool state) {

}

void GUI::Header::batteryVoltage(uint16_t value) {
    
}