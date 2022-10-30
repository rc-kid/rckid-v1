#include <QFrame>
#include <QFontDatabase>
#include <QKeyEvent>

#include <iostream>
#include "gui.h"
#include "widgets/debug_info.h"
#include "widgets/carousel.h"
#include "widgets/gauge.h"

GUI * GUI::singleton_ = nullptr;

int GUI::exec(int argc, char * argv[]) {
    auto json = json::parse("67");
    QApplication app(argc, argv);
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    QFontDatabase::addApplicationFont("assets/fonts/OpenDyslexic.otf");
    QFontDatabase::addApplicationFont("assets/fonts/Iosevka.ttf");
    GUI w{};
#if (defined ARCH_MOCK)
    w.grabKeyboard();
#endif
    w.show();
    return app.exec();
}

void GUI::navigateBack() {
    if (navStack_.empty())
        return;
    NavigationItem item = navStack_.back();
    navStack_.pop_back();
    // switching menu for menu
    if (activePage_ == carousel_ && item.page == carousel_) {
        carousel_->setMenu(item.menu, item.index);
    } else if (item.page == carousel_) {
        carousel_->setMenu(item.menu, item.index, /* animation */ false);
        setActivePage(carousel_);
    } else {
        setActivePage(item.page);
    }
}

void GUI::navigateTo(Page * page) {
    pushActivePage();
    setActivePage(page);
}

void GUI::navigateTo(Menu * menu, size_t index) {
    pushActivePage();
    if (activePage_ == carousel_) {
        carousel_->setMenu(menu, index);
    } else {
        carousel_->setMenu(menu, index, /* animation */ false);
        setActivePage(carousel_);
    }
}

GUI::GUI(QWidget *parent):
    QMainWindow{parent},
    pageView_{new QGraphicsView{this}},
    overlayView_{new QGraphicsView{this}},
    overlay_{new QGraphicsScene{this}} {

    // page change animation (fade out old, fade in new)
    aPage_ = new QPropertyAnimation{this, "pageChangeStep"};
    aPage_->setDuration(500);
    aPage_->setStartValue(0);
    aPage_->setEndValue(200);

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

    powerMenu_.addItem(new Menu::Item{"Power Off", "assets/images/011-power-off.png"});
    powerMenu_.addItem(new Menu::Item{"Airplane Mode", "assets/images/012-airplane-mode.png"});
    powerMenu_.addItem(new Menu::Item{"Torchlight", "assets/images/004-flashlight.png"});
    powerMenu_.addItem(new Menu::Item{"Baby Monitor", "assets/images/006-baby-crib.png"});
    powerMenu_.addItem(new Menu::Item{"Settings", "assets/images/013-settings.png", [this](Menu::Item const *) {
        navigateTo(& settingsMenu_);
    }});
    powerMenu_.addItem(new Menu::Item{"Debug Info", "assets/images/008-unicorn.png", [this](Menu::Item const *) {
        if (debugInfo_ == nullptr)
            debugInfo_ = new DebugInfo();
        navigateTo(debugInfo_);
    }});
    
    settingsMenu_.addItem(new Menu::Item{"Brightness", "assets/images/009-brightness.png", [this](Menu::Item const *) {
        gauge_->reset("Brightness", Driver::instance()->brightness(), 0, 255, 16);
        connect(gauge_, & Gauge::valueChanged, Driver::instance(), & Driver::setBrightness);
        navigateTo(gauge_);
    }});
    settingsMenu_.addItem(new Menu::Item{"Volume", "assets/images/010-high-volume.png"});
    settingsMenu_.addItem(new Menu::Item{"WiFi", "assets/images/016-wifi.png"});
    settingsMenu_.addItem(new Menu::Item{"Information", "assets/images/014-info.png"});

    initializeOverlay();
    overlayView_->setScene(overlay_);


    
    //auto c = new DebugInfo();
    //auto c = new Carousel(& menu_);


    //pageView_->setScene(c);

    // attach own slots to driver
    Driver * driver = Driver::instance();
    connect(driver, & Driver::buttonA, this, & GUI::buttonA, Qt::QueuedConnection);
    connect(driver, & Driver::buttonB, this, & GUI::buttonB, Qt::QueuedConnection);
    connect(driver, & Driver::buttonX, this, & GUI::buttonX, Qt::QueuedConnection);
    connect(driver, & Driver::buttonY, this, & GUI::buttonY, Qt::QueuedConnection);
    connect(driver, & Driver::buttonStart, this, & GUI::buttonStart, Qt::QueuedConnection);
    connect(driver, & Driver::buttonSelect, this, & GUI::buttonSelect, Qt::QueuedConnection);
    connect(driver, & Driver::buttonLeft, this, & GUI::buttonLeft, Qt::QueuedConnection);
    connect(driver, & Driver::buttonRight, this, & GUI::buttonRight, Qt::QueuedConnection);
    connect(driver, & Driver::buttonVolumeLeft, this, & GUI::buttonVolumeLeft, Qt::QueuedConnection);
    connect(driver, & Driver::buttonVolumeRight, this, & GUI::buttonVolumeRight, Qt::QueuedConnection);
    connect(driver, & Driver::buttonThumb, this, & GUI::buttonThumb, Qt::QueuedConnection);
    connect(driver, & Driver::thumbstick, this, & GUI::thumbstick, Qt::QueuedConnection);
    connect(driver, & Driver::accel, this, & GUI::accel, Qt::QueuedConnection);
    connect(driver, & Driver::dpadUp, this, & GUI::dpadUp, Qt::QueuedConnection);
    connect(driver, & Driver::dpadDown, this, & GUI::dpadDown, Qt::QueuedConnection);
    connect(driver, & Driver::dpadLeft, this, & GUI::dpadLeft, Qt::QueuedConnection);
    connect(driver, & Driver::dpadRight, this, & GUI::dpadRight, Qt::QueuedConnection);
    
    connect(driver, & Driver::headphonesChanged, this, & GUI::headphones, Qt::QueuedConnection);
    connect(driver, & Driver::chargingChanged, this, & GUI::charging, Qt::QueuedConnection);
    connect(driver, & Driver::lowBatteryChanged, this, & GUI::lowBattery, Qt::QueuedConnection);
    connect(driver, & Driver::batteryVoltageChanged, this, & GUI::batteryVoltage, Qt::QueuedConnection);
    connect(driver, & Driver::tempAvrChanged, this, & GUI::tempAvr, Qt::QueuedConnection);
    connect(driver, & Driver::tempAccelChanged, this, & GUI::tempAccel, Qt::QueuedConnection);


    //auto c = new Gauge();
    carousel_ = new Carousel{& menu_};
    connect(carousel_, & Carousel::back, this, & GUI::navigateBack);
    
    gauge_ = new Gauge{};
    connect(gauge_, & Gauge::back, this, & GUI::navigateBack);

    volumeGauge_ = new Gauge{};
    connect(volumeGauge_, & Gauge::back, this, & GUI::navigateBack);
    volumeGauge_->reset("Volume", 50, 0, 100, 5);

    setActivePage(carousel_);
}

GUI::~GUI() {
    //delete header_;
    //delete footer_;
}

void GUI::setActivePage(Page * page) {
    if (! activePage_) {
        pageView_->setScene(page);
        activePage_ = page;
        page->onFocus();
    } else {
        nextPage_ = page;
        aPage_->start();
    }
}

void GUI::pushActivePage() {
    if (activePage_ == carousel_) {
        navStack_.push_back(NavigationItem{ carousel_, carousel_->menu(), carousel_->index()});
    } else {
        navStack_.push_back(NavigationItem{ activePage_, nullptr, 0});
    }
}

void GUI::pageChangeStep(qreal x) {
    aStep_ = x;
    if (aStep_ < 100) {
        activePage_->setOpacity((100 - aStep_) / 100);
    } else {
        if (activePage_ != nextPage_) {
            // if mid-animation we have both volumes pressed, enter the power menu instead
            if (nextPage_ == volumeGauge_) {
                Driver * d = Driver::instance();
                if (d->volumeLeft() && d->volumeRight()) {
                    carousel_->setMenu(& powerMenu_, 0, /* animation */ false);
                    nextPage_ = carousel_;
                }
            }
            activePage_->onBlur();
            activePage_ = nextPage_;
            activePage_->setOpacity(0);
            pageView_->setScene(activePage_);
            activePage_->onFocus();
        }
        activePage_->setOpacity((aStep_ - 100) / 100);
    }
}

void GUI::buttonVolumeLeft(bool state) {
    if (activePage_ != volumeGauge_)
        navigateTo(volumeGauge_);
    else
        activePage_->buttonLeft(state); 
}

void GUI::buttonVolumeRight(bool state) {
    if (activePage_ != volumeGauge_)
        navigateTo(volumeGauge_);
    else 
        activePage_->buttonRight(state);
}

void GUI::headphones(bool state) {
    if (state)
        volume_->setText("");
    else 
        volume_->setText("");
    volume_->setBrush(COLOR_HEADER_DEFAULT);
}

void GUI::charging(bool state) {
    batteryVoltage(Driver::instance()->batteryVoltage());
}

void GUI::lowBattery() {
    // TODO Not sure what to do here
}

/** Displays the battery capacity icon and charging status.
 */
void GUI::batteryVoltage(uint16_t value) {
    bool charging = Driver::instance()->charging() | true;
    uint8_t pct = (value < 330) ? 0 : (value - 330) * 100 / 90;
    if (pct > 90) {
        battery_->setText(charging ? "" : "");
        battery_->setBrush(overlayDetails_ ? COLOR_BATTERY_OK : COLOR_HEADER_DEFAULT);
    } else if (pct > 75) {
        battery_->setText(charging ? "" : "");
        battery_->setBrush(overlayDetails_ ? COLOR_BATTERY_OK : COLOR_HEADER_DEFAULT);
    } else if (pct > 50) {
        battery_->setText(charging ? "" : "");
        battery_->setBrush(overlayDetails_ ? COLOR_BATTERY_OK : COLOR_HEADER_DEFAULT);
    } else if (pct > 25) {
        battery_->setText(charging ? "" : "");
        battery_->setBrush(overlayDetails_ ? COLOR_BATTERY_WARN : COLOR_HEADER_DEFAULT);

        battery_->setBrush(QColor{0xff, 0x7f, 0});
    } else {
        battery_->setText(charging ? "" : "");
        battery_->setBrush(COLOR_BATTERY_CRITICAL);
    }
    batteryPct_->setText(QString::number(pct)); 
}

void GUI::tempAvr(uint16_t value) {

}

void GUI::tempAccel(uint16_t value) {

}

void GUI::initializeOverlay() {
    overlay_->setSceneRect(QRectF{0,0,320,240});

    auto iconsFont = QFont{"Iosevka", 14};
    auto textFont = QFont{"Iosevka", 10};
    battery_ = overlay_->addSimpleText("", iconsFont);
    wifi_ = overlay_->addSimpleText("直", iconsFont);
    wifi_->setBrush(COLOR_HEADER_DEFAULT);
    volume_ = overlay_->addSimpleText("", iconsFont);
    batteryPct_ = overlay_->addSimpleText("", textFont);
    batteryPct_->setBrush(Qt::darkGray);
    wifiSSID_ = overlay_->addSimpleText("Internet 10", textFont);
    wifiSSID_->setBrush(Qt::darkGray);
    volumePct_ = overlay_->addSimpleText("60", textFont);
    volumePct_->setBrush(Qt::darkGray);
    headphones(true);
    batteryVoltage(390);
    repositionOverlay();
}

void GUI::repositionOverlay() {
    qreal left = 320;
    if (overlayDetails_) {
        left -= batteryPct_->boundingRect().width();
        batteryPct_->setPos(left, 5);
        left -= 2;
    }
    left -= battery_->boundingRect().width();
    battery_->setPos(left, 0);
    left -= 2;
    if (overlayDetails_) {
        left -= wifiSSID_->boundingRect().width();
        wifiSSID_->setPos(left, 5);
        left -= 2;
    }
    left -= wifi_->boundingRect().width();
    wifi_->setPos(left, 0);
    left -= 2;
    if (overlayDetails_) {
        left -= volumePct_->boundingRect().width();
        volumePct_->setPos(left, 5);
        left -= 2;
    }
    left -= volume_->boundingRect().width();
    volume_->setPos(left, 0);
}



#if (defined ARCH_MOCK)
void GUI::keyToGamepad(QKeyEvent * e, bool state) {
    switch (e->key()) {
        case Qt::Key_Up:
            emit Driver::instance()->dpadUp(state);
            break;
        case Qt::Key_Left:
            emit Driver::instance()->dpadLeft(state);
            break;
        case Qt::Key_Down:
            emit Driver::instance()->dpadDown(state);
            break;
        case Qt::Key_Right:
            emit Driver::instance()->dpadRight(state);
            break;
        case Qt::Key_L:
            emit Driver::instance()->buttonLeft(state);
            break;
        case Qt::Key_R:
            emit Driver::instance()->buttonRight(state);
            break;
        case Qt::Key_T:
            emit Driver::instance()->buttonThumb(state);
            break;
        case Qt::Key_A: 
            emit Driver::instance()->buttonA(state);
            break;
        case Qt::Key_B:
            emit Driver::instance()->buttonB(state);
            break;
        case Qt::Key_X: 
            emit Driver::instance()->buttonX(state);
            break;
        case Qt::Key_Y:
            emit Driver::instance()->buttonY(state);
            break;
        case Qt::Key_9:
            emit Driver::instance()->buttonVolumeLeft(state);
            break;
        case Qt::Key_0:
            emit Driver::instance()->buttonVolumeRight(state);
            break;
    }
}
#endif
