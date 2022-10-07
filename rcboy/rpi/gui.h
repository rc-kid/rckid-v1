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
        setBackgroundBrush(Qt::darkGray);
        //setForegroundPen(Qt::white);
        setSceneRect(QRectF{0,0,320,24});
        clock_ = addSimpleText("--:--");
        clock_->setBrush(Qt::white);
        batt_ = addSimpleText("\uf57999");
        batt_->setBrush(Qt::white);
        batt_->setPos(200,0);
        timerTimeout();
        timer_ = new QTimer{this};        
        connect(timer_, & QTimer::timeout, this, & Header::timerTimeout);
        timer_->setInterval(1000);
        timer_->start();
    }

protected slots:

    void headphones(bool state);
    void charging(bool state);
    void batteryVoltage(uint16_t value);

private:

    void timerTimeout() {
        auto t = QTime::currentTime();
        if (t.second() % 2) 
            clock_->setText(t.toString("hh:mm"));
        else
            clock_->setText(t.toString("hh mm"));
    }

    QTimer * timer_ = nullptr;
    QGraphicsSimpleTextItem * clock_ = nullptr;
    QGraphicsSimpleTextItem * batt_ = nullptr;
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

/** The carousel controller. 
 
    For simplicity this is the single controller used for the UI. Its items consist of an image and a text that can be swapped in/out.

    TODO eventually, add some nice effects, etc., play music and so on. 
 */
class Carousel : public QGraphicsScene {
    Q_OBJECT
public:

    /** Element in the carousel. 
     */
    struct Element {
        QString text;
        QPixmap img;

        Element(char const * text, char const * img):
            text{text},
            img{QPixmap{img}.scaled(140, 140, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation)} {
        }
    }; // Carousel::Element

    explicit Carousel() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,192});
        img_ = addPixmap(QPixmap{});
        img_->setPos(90, 0);
        text_ = addSimpleText("");
        text_->setPos(0, 145);
        text_->setFont(QFont{"OpenDyslexic Nerd Font", 22});
        text_->setBrush(Qt::white);
        Driver * driver = Driver::instance();
        connect(driver, & Driver::dpadLeft, this, & Carousel::dpadLeft, Qt::QueuedConnection);
        connect(driver, & Driver::dpadRight, this, & Carousel::dpadRight, Qt::QueuedConnection);
    }

    void addElement(Element e) {
        elements_.push_back(e);
    }

    void setElement(size_t i) {
        i_ = i;
        {
            auto element = elements_[i];
            img_->setPixmap(element.img);
            text_->setText(element.text);
            auto width = text_->boundingRect().width();
            text_->setPos((320 - width) / 2, 145);
        }
    }

    void nextElement() {
        setElement((i_ + 1) % elements_.size());
    }

    void prevElement() {
        setElement((i_ - 1) % elements_.size());
    }

private slots:

    virtual void dpadLeft(bool state) { if (state) prevElement(); }
    virtual void dpadRight(bool state) { if (state) nextElement(); }
    
    // TODO actual select

private:

    QGraphicsPixmapItem * img_ = nullptr;
    QGraphicsSimpleTextItem * text_ = nullptr;

    std::vector<Element> elements_;
    size_t i_;
    
};


