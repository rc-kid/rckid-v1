#include <QFrame>

#include "gui.h"
#include "debug_info.h"

int GUI::exec(int argc, char * argv[]) {
    QApplication app(argc, argv);
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
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
    auto c = new DebugInfo();
    pageView_->setScene(c);

}

GUI::~GUI() {
    delete header_;
    delete footer_;
}


void GUI::Header::headphones(bool stage) {

}

void GUI::Header::charging(bool state) {

}

void GUI::Header::batteryVoltage(uint16_t value) {
    
}