#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QTime>

#include "driver.h"

/** RCBoy's launcher main window. 
 */
class GUI : public QMainWindow {
    Q_OBJECT
public:
    class Header;
    class Footer;

    static int exec(int argc, char * argv[]);

#if (defined ARCH_MOCK)
protected:
    /** Emulates the gamepad with keyboard on mock systems for easier development. 
     */
    void keyPressEvent(QKeyEvent *e) override;
#endif

private:
    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;

    QGraphicsView * headerView_;
    QGraphicsView * pageView_;
    QGraphicsView * footerView_;

    Header * header_;
    Footer * footer_;

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



