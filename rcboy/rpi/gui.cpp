#include <QFrame>
#include <QFontDatabase>
#include <QKeyEvent>

#include <iostream>
#include "gui.h"
#include "debug_info.h"

GUI * GUI::singleton_ = nullptr;
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
    pageView_{new QGraphicsView{this}},
    overlayView_{new QGraphicsView{this}},
    overlay_{new QGraphicsScene{this}} {
    overlayView_->setStyleSheet("background: transparent;");
    singleton_ = this;
    setFixedSize(QSize{320,240});
    overlayView_->setGeometry(QRect{0,0,320,240});
    overlayView_->setFrameStyle(QFrame::NoFrame);
    pageView_->setGeometry(QRect{0,0,320,240});
    pageView_->setFrameStyle(QFrame::NoFrame);
    menu_.addItem(new Menu::Item{"Games", "assets/images/001-game-controller.png"});
    menu_.addItem(new Menu::Item{"Music", "assets/images/003-music.png"});
    menu_.addItem(new Menu::Item{"Videos", "assets/images/005-film-slate.png"});
    menu_.addItem(new Menu::Item{"Walkie-talkie", "assets/images/007-baby-monitor.png"});
    menu_.addItem(new Menu::Item{"Remote", "assets/images/002-rc-car.png"});

    /*
    adminMenu_.addItem(new Menu::Item{"Power Off", "assets/images/011-power-off.png"});
    adminMenu_.addItem(new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"});
    adminMenu_.addItem(new Menu::Item{"Torchlight", "assets/images/004-flashlight.png"});
    adminMenu_.addItem(new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"});
    auto settings = new Menu{};
    settings->addItem(new Menu::Item{"Brightness", "assets/images/009-brightness.png"});
    settings->addItem(new Menu::Item{"Volume", "assets/images/010-high-volume.png"});
    settings->addItem(new Menu::Item{"WiFi", "assets/images/016-wifi.png"});
    settings->addItem(new Menu::Item{"Information", "assets/images/014-info.png"});
    adminMenu_.addItem(new Menu::Item{"Settings", "assets/images/013-settings.png", settings});
    */

    initializeOverlay();
    overlayView_->setScene(overlay_);


    
    //auto c = new DebugInfo();
    auto c = new Carousel(& menu_);
    pageView_->setScene(c);

}

GUI::~GUI() {
    //delete header_;
    //delete footer_;
}

#if (defined ARCH_MOCK)
void GUI::keyPressEvent(QKeyEvent * e) {
    switch (e->key()) {
        case Qt::Key_W:
            emit Driver::instance()->dpadUp(true);
            emit Driver::instance()->dpadUp(false);
            break;
        case Qt::Key_A: 
            emit Driver::instance()->dpadLeft(true);
            emit Driver::instance()->dpadLeft(false);
            break;
        case Qt::Key_S:
            emit Driver::instance()->dpadDown(true);
            emit Driver::instance()->dpadDown(false);
            break;
        case Qt::Key_D: 
            emit Driver::instance()->dpadRight(true);
            emit Driver::instance()->dpadRight(false);
            break;
        case Qt::Key_L:
            emit Driver::instance()->buttonLeft(true);
            emit Driver::instance()->buttonLeft(false);
            break;
        case Qt::Key_R:
            emit Driver::instance()->buttonRight(true);
            emit Driver::instance()->buttonRight(false);
            break;
        case Qt::Key_T:
            emit Driver::instance()->buttonThumb(true);
            emit Driver::instance()->buttonThumb(false);
            break;
        case Qt::Key_B:
            emit Driver::instance()->buttonB(true);
            emit Driver::instance()->buttonB(false);
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