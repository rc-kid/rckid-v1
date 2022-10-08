#include <QFrame>
#include <QFontDatabase>
#include <QKeyEvent>

#include <iostream>
#include "gui.h"
#include "carousel.h"
#include "debug_info.h"

GUI::Footer * GUI::Footer::singleton_ = nullptr;

int GUI::exec(int argc, char * argv[]) {
    auto json = json::parse("67");
    QApplication app(argc, argv);
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    QFontDatabase::addApplicationFont("assets/fonts/OpenDyslexic.otf");
    QFontDatabase::addApplicationFont("assets/fonts/Iosevka.ttf");
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
    c->addElement(Carousel::Element{"Games", "assets/images/gamepad.png"});
    c->addElement(Carousel::Element{"Music", "assets/images/music.png"});
    c->addElement(Carousel::Element{"Movies", "assets/images/video.png"});
    c->addElement(Carousel::Element{"Walkie-Talkie", "assets/images/walkie-talkie.png"});
    c->addElement(Carousel::Element{"Remote", "assets/images/rc-car.png"});
    c->addElement(Carousel::Element{"Torchlight", "assets/images/torch.png"});
    c->addElement(Carousel::Element{"Baby Monitor", "assets/images/baby-monitor.png"});
    c->addElement(Carousel::Element{"Settings", "assets/images/settings.png"});
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


void GUI::Header::headphonesChanged(bool state) {
    headphones_ = state;
    updateStatusLine();
}

void GUI::Header::chargingChanged(bool state) {
    charging_ = state;
    updateStatusLine();
}

void GUI::Header::batteryVoltageChanged(uint16_t value) {
    batteryPct_ = (value - 330);
    updateStatusLine();
}

void GUI::Header::updateStatusLine() {
    auto ss = std::stringstream{};
    // volume
    ss << "<span style='color:white'></span>&nbsp;";
    ss << "<span style='color:white'></span>&nbsp;";
    ss << "<span style='color:gray'>婢</span>&nbsp;";
    if (displayDetails_)
        ss << "<small>4</small>&nbsp;";
    // wifi - signal strength and WiFi name
    ss << "<span style='color:blue'>直</span> ";
    //ss << "<span>睊</span>";
    if (displayDetails_)
        ss << "<small>Internet 10 </small>";
    // battery - charging, or how much is left + pct
    if (charging_)
        ss << "<span style='color:white'></span>";
    else if (batteryPct_ > 90)
        ss << "<span style='color:green'></span>";
    else if (batteryPct_ > 70)
        ss << "<span style='color:green'></span>";
    else if (batteryPct_ > 50)
        ss << "<span style='color:green'></span>";
    else if (batteryPct_ > 20)
        ss << "<span style='color:orange'></span>";
    else 
        ss << "<span style='color:red'></span>";
    if (displayDetails_)
        ss << " <small>&nbsp;" << batteryPct_ << "</small>";
    batt_->setHtml(ss.str().c_str());
    batt_->setPos(320 - batt_->boundingRect().width(), -4);
}