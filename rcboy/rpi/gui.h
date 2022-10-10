#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QTime>

#include "driver.h"
#include "menu.h"

#define COLOR_HEADER_DEFAULT QColor{64, 64, 64}


#define COLOR_BATTERY_OK QColor{0, 255, 0}
#define COLOR_BATTERY_WARN QColor{255, 127, 0}
#define COLOR_BATTERY_CRITICAL QColor{255, 0, 0}





/** RCBoy's launcher main window. 
 
    Contains the overlay with header and footer displaying common information and the main page's graphics view for the actual contents. 

    # Header

    Information about rcboy's status. In normal mode, only the icons are displayed for a status shorthand. In the power menu details, such as actual battery and volume percentage as well as the WiFi network rcboy is connected to is displayed alongside the icons. 

    # Footer

    The footer always contains information about the various UI actions available under given context. 
 */
class GUI : public QMainWindow {
    Q_OBJECT
public:
    class Header;
    class Footer;

    static int exec(int argc, char * argv[]);

    static GUI * instance() { return singleton_; }

#if (defined ARCH_MOCK)
protected:
    /** Emulates the gamepad with keyboard on mock systems for easier development. 
     */
    void keyPressEvent(QKeyEvent *e) override;
#endif


private slots:

    // TODO also dependning on the volume it might be muted...
    void headphones(bool state) {
        if (state)
            volume_->setText("");
        else 
            volume_->setText("");
        volume_->setBrush(COLOR_HEADER_DEFAULT);
    }

    void charging(bool state) {
        batteryVoltage(Driver::instance()->batteryVoltage());
    }

    void lowBattery() {
        // TODO Not sure what to do here
    }

    /** Displays the battery capacity icon and charging status.
     */
    void batteryVoltage(uint16_t value) {
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

    void tempAvr(uint16_t value) {}
    void tempAccel(uint16_t value) {}

private:
    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;

    void initializeOverlay() {
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

    void repositionOverlay() {
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

    QGraphicsView * pageView_;
    QGraphicsView * overlayView_;

    // the overlay scene and its elements
    QGraphicsScene * overlay_;
    bool overlayDetails_ = false;
    QGraphicsSimpleTextItem * battery_;
    QGraphicsSimpleTextItem * batteryPct_;
    QGraphicsSimpleTextItem * wifi_;
    QGraphicsSimpleTextItem * wifiSSID_;
    QGraphicsSimpleTextItem * volume_;
    QGraphicsSimpleTextItem * volumePct_;

    

    //Header * header_;
    //Footer * footer_;

    Menu menu_;
    Menu adminMenu_;

    static GUI * singleton_;

}; // GUI

/** GUI Header that displays the status bar. 
 
    Displays the status bar with information about charging, battery levels, signal strength, volume, etc.

    Time (Alarm) ------------ Speaker/Mute Battery 
 */
class GUI::Header : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Header() {
        setFont(QFont{"Iosevka", 12});
        setBackgroundBrush(Qt::black);
        //setForegroundPen(Qt::white);
        setSceneRect(QRectF{0,0,320,24});
        clock_ = addSimpleText("--:--");
        clock_->setBrush(Qt::gray);
        clock_->setFont(QFont{"Iosevka", 10});
        clock_->setPos(0, 4);
        batt_ = addText("");
        //batt_->setHtml("\uf579<small>99</small>");
        batt_->setDefaultTextColor(Qt::darkGray);
        batt_->setFont(QFont{"Iosevka", 14});
        updateStatusLine();
        timerTimeout();
        timer_ = new QTimer{this};        
        connect(timer_, & QTimer::timeout, this, & Header::timerTimeout);
        timer_->setInterval(1000);
        timer_->start();
        Driver * driver = Driver::instance();

        connect(driver, & Driver::headphonesChanged, this, & Header::headphonesChanged, Qt::QueuedConnection);
        connect(driver, & Driver::chargingChanged, this, & Header::chargingChanged, Qt::QueuedConnection);
        connect(driver, & Driver::batteryVoltageChanged, this, & Header::batteryVoltageChanged, Qt::QueuedConnection);


    }

protected slots:

    void headphonesChanged(bool state);
    void chargingChanged(bool state);
    void batteryVoltageChanged(uint16_t value);

private:

    void timerTimeout() {
        auto t = QTime::currentTime();
        if (t.second() % 2) 
            clock_->setText(t.toString("hh:mm"));
        else
            clock_->setText(t.toString("hh mm"));
    }

    void updateStatusLine();

    QTimer * timer_ = nullptr;
    QGraphicsSimpleTextItem * clock_ = nullptr;
    QGraphicsTextItem * batt_ = nullptr;

    bool headphones_ = false;
    bool charging_ = false;
    size_t batteryPct_ = 55;
    bool displayDetails_ = false;
}; 


/** GUI Footer
 
    The footer displays hints for current controls such as 
 */
class GUI::Footer : public QGraphicsScene {
    Q_OBJECT
    friend class GUI;
public:

    static Footer * instance() { return singleton_; }
private:
    explicit Footer() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,24});
        addEllipse(QRectF{4,4,16,16}, QPen{Qt::red}, QBrush{Qt::red});
        singleton_ = this;
    }

    static Footer * singleton_;

}; 

class Page : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Page() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,192});
        // connect the driver
        Driver * driver = Driver::instance();
        connect(driver, & Driver::buttonA, this, & Page::buttonA, Qt::QueuedConnection);
        connect(driver, & Driver::buttonB, this, & Page::buttonB, Qt::QueuedConnection);
        connect(driver, & Driver::buttonX, this, & Page::buttonX, Qt::QueuedConnection);
        connect(driver, & Driver::buttonY, this, & Page::buttonY, Qt::QueuedConnection);
        connect(driver, & Driver::buttonStart, this, & Page::buttonStart, Qt::QueuedConnection);
        connect(driver, & Driver::buttonSelect, this, & Page::buttonSelect, Qt::QueuedConnection);
        connect(driver, & Driver::buttonLeft, this, & Page::buttonLeft, Qt::QueuedConnection);
        connect(driver, & Driver::buttonRight, this, & Page::buttonRight, Qt::QueuedConnection);
        connect(driver, & Driver::buttonVolumeLeft, this, & Page::buttonVolumeLeft, Qt::QueuedConnection);
        connect(driver, & Driver::buttonVolumeRight, this, & Page::buttonVolumeRight, Qt::QueuedConnection);
        connect(driver, & Driver::buttonThumb, this, & Page::buttonThumb, Qt::QueuedConnection);
        connect(driver, & Driver::thumbstick, this, & Page::thumbstick, Qt::QueuedConnection);
        connect(driver, & Driver::accel, this, & Page::accel, Qt::QueuedConnection);
        connect(driver, & Driver::dpadUp, this, & Page::dpadUp, Qt::QueuedConnection);
        connect(driver, & Driver::dpadDown, this, & Page::dpadDown, Qt::QueuedConnection);
        connect(driver, & Driver::dpadLeft, this, & Page::dpadLeft, Qt::QueuedConnection);
        connect(driver, & Driver::dpadRight, this, & Page::dpadRight, Qt::QueuedConnection);
        
        connect(driver, & Driver::headphonesChanged, this, & Page::headphones, Qt::QueuedConnection);
        connect(driver, & Driver::chargingChanged, this, & Page::charging, Qt::QueuedConnection);
        connect(driver, & Driver::lowBatteryChanged, this, & Page::lowBattery, Qt::QueuedConnection);
        connect(driver, & Driver::batteryVoltageChanged, this, & Page::batteryVoltage, Qt::QueuedConnection);
        connect(driver, & Driver::tempAvrChanged, this, & Page::tempAvr, Qt::QueuedConnection);
        connect(driver, & Driver::tempAccelChanged, this, & Page::tempAccel, Qt::QueuedConnection);

    }

protected slots:

    virtual void buttonA(bool state) {}
    virtual void buttonB(bool state) {}
    virtual void buttonX(bool state) {}
    virtual void buttonY(bool state) {}
    virtual void buttonStart(bool state) {}
    virtual void buttonSelect(bool state) {}
    virtual void buttonLeft(bool state) {}
    virtual void buttonRight(bool state) {}
    virtual void buttonVolumeLeft(bool state) {}
    virtual void buttonVolumeRight(bool state) {}
    virtual void buttonThumb(bool state) {}
    virtual void thumbstick(uint8_t x, uint8_t y) {}
    virtual void accel(uint8_t x, uint8_t y) {}
    virtual void dpadUp(bool state) {}
    virtual void dpadDown(bool state) {}
    virtual void dpadLeft(bool state) {}
    virtual void dpadRight(bool state) {}

    virtual void headphones(bool state) {}
    virtual void charging(bool state) {}
    virtual void lowBattery() {}
    virtual void batteryVoltage(uint16_t value) {}
    virtual void tempAvr(uint16_t value) {}
    virtual void tempAccel(uint16_t value) {}

    
}; 



