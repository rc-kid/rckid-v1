#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

#include "driver.h"

/** RCBoy's launcher main window. 
 */
class GUI : public QMainWindow {
    Q_OBJECT
public:
    class Header;
    class Footer;

    static int exec(int argc, char * argv[]);

private:
    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;

    QGraphicsView * headerView_;
    QGraphicsView * pageView_;
    QGraphicsView * footerView_;

    Header * header_;
    Footer * footer_;

}; // GUI

/** GUI Header 
 
    Displays the status bar with information about charging, battery levels, signal strength, volume, etc.

    Time (Alarm) ------------ Speaker/Mute Battery 
 */
class GUI::Header : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Header() {
        setBackgroundBrush(Qt::black);
        //setForegroundPen(Qt::white);
        setSceneRect(QRectF{0,0,320,24});
        addText("Hello world!")->setDefaultTextColor(Qt::white);
    }

protected slots:

    void headphones(bool state);
    void charging(bool state);
    void batteryVoltage(uint16_t value);
}; 

/** GUI Footer
 
    The footer displays hints for current controls such as 
 */
class GUI::Footer : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Footer() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,24});
        addEllipse(QRectF{4,4,16,16}, QPen{Qt::red}, QBrush{Qt::red});
    }
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

    virtual void headphones(bool state) {}
    virtual void charging(bool state) {}
    virtual void lowBattery() {}
    virtual void batteryVoltage(uint16_t value) {}
    virtual void tempAvr(uint16_t value) {}
    virtual void tempAccel(uint16_t value) {}

    
}; 

/** The carousel controller. 
 
    For simplicity this is the single controller used for the UI. Its items consist of an image and a text that can be swapped in/out.

    TODO eventually, add some nice effects, etc., play music and so on. 
 */
class Carousel : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Carousel() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,192});
    }

private:
};


