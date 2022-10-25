#pragma once

#include "widgets/page.h"
#include "gui.h"
#include "driver.h"

#include <iostream>


class DebugInfo : public Page {
    Q_OBJECT
public:
    explicit DebugInfo() {
        a_ = newText("A", 5, 24);
        b_ = newText("B", 20, 24);
        x_ = newText("X", 35, 24);
        y_ = newText("Y", 50, 24); 
        select_ = newText("SEL", 65, 24);
        start_ = newText("START", 95, 24);
        l_ = newText("L", 140, 24);
        r_ = newText("R", 155, 24);
        lVol_ = newText("L_VOL", 170, 24);
        rVol_ = newText("R_VOL", 215, 24);
        thumbBtn_ = newText("THUMB", 260, 24);
        dpadUp_ = newText("UP", 5, 40);
        dpadDown_ = newText("DOWN", 25, 40);
        dpadLeft_ = newText("LEFT", 70, 40);
        dpadRight_ = newText("RIGHT", 105, 40);
        newText("Thumbstick:", 0, 64);
        newText("X:", 5, 80);
        newText("Y:", 5, 96);
        thumbX_ = newText("255", 25, 80);
        thumbY_ = newText("0", 25, 96);
        thumbX_->setBrush(Qt::white);
        thumbY_->setBrush(Qt::white);

        newText("Accel:", 160, 64);
        newText("X:", 165, 80);
        newText("Y:", 165, 96);
        accelX_ = newText("255", 185, 80);
        accelY_ = newText("0", 185, 96);
        accelX_->setBrush(Qt::white);
        accelY_->setBrush(Qt::white);
        addLine(50, 130, 150, 130, QPen{Qt::darkGray});
        addLine(100, 80, 100, 180, QPen{Qt::darkGray});
        thumb_ = addEllipse(100 - 3, 130 - 3, 6,6,QPen{Qt::white});

        addLine(210, 130, 310, 130, QPen{Qt::darkGray});
        addLine(260, 80, 260, 180, QPen{Qt::darkGray});
        accel_ = addEllipse(260 - 3, 130 - 3, 6,6,QPen{Qt::white});
        newText("VBatt:", 0, 184);
        vBatt_ = newText("0", 5, 199);
        newText("Vcc:", 50, 184);
        vcc_ = newText("0", 55, 199);
        newText("tAvr:", 100, 184);
        tempAvr_ = newText("0", 105, 199);
        newText("tAccel:", 150, 184);
        tempAccel_ = newText("0", 155, 199);
        newText("audio:", 200, 184);
        audio_ = newText("0", 205, 199);
        newText("chrg:", 250, 184);
        chrg_ = newText("0", 255, 199);

        Driver * driver = Driver::instance();
        connect(driver, & Driver::headphonesChanged, this, & DebugInfo::headphones, Qt::QueuedConnection);
        connect(driver, & Driver::chargingChanged, this, & DebugInfo::charging, Qt::QueuedConnection);
        connect(driver, & Driver::batteryVoltageChanged, this, & DebugInfo::batteryVoltage, Qt::QueuedConnection);
        connect(driver, & Driver::vccVoltageChanged, this, & DebugInfo::vccVoltage, Qt::QueuedConnection);
        connect(driver, & Driver::tempAvrChanged, this, & DebugInfo::tempAvr, Qt::QueuedConnection);
        connect(driver, & Driver::tempAccelChanged, this, & DebugInfo::tempAccel, Qt::QueuedConnection);

    }

protected:

    void buttonA(bool state) override { updateButton(a_, state); }
    void buttonB(bool state) override { updateButton(b_, state); }
    void buttonX(bool state) override { updateButton(x_, state); }
    void buttonY(bool state) override { updateButton(y_, state); }
    void buttonStart(bool state) override { updateButton(start_, state); }
    void buttonSelect(bool state) override { updateButton(select_, state); }
    void buttonLeft(bool state) override { updateButton(l_, state); }
    void buttonRight(bool state) override { updateButton(r_, state); }
    void buttonVolumeLeft(bool state) override { updateButton(lVol_, state); }
    void buttonVolumeRight(bool state) override { updateButton(rVol_, state); }
    void buttonThumb(bool state) override { updateButton(thumbBtn_, state); }
    void thumbstick(uint8_t x, uint8_t y) override { updatePoint(thumb_, x, y); }
    void accel(uint8_t x, uint8_t y) override { updatePoint(accel_, x, y); }
    void dpadUp(bool state) override { updateButton(dpadUp_, state); }
    void dpadDown(bool state) override { updateButton(dpadDown_, state); }
    void dpadLeft(bool state) override { updateButton(dpadLeft_, state); }
    void dpadRight(bool state) override { updateButton(dpadRight_, state); }

protected slots:

    void headphones(bool state) { updateValue(audio_, state ? "hp" : "spkr"); }
    void charging(bool state) { updateValue(chrg_, state); }
    
    void vccVoltage(uint16_t value) { updateValue(vcc_, value); }

    void batteryVoltage(uint16_t value) { updateValue(vBatt_, value); }
    void tempAvr(uint16_t value) { updateValue(tempAvr_, value); }
    void tempAccel(uint16_t value) { updateValue(tempAccel_, value); }

private:

    QGraphicsSimpleTextItem * newText(char const * text, qreal x, qreal y) {
        auto i = addSimpleText(text);
        i->setPos(x, y);
        i->setBrush(QBrush{Qt::darkGray});
        return i;
    }

    void updateButton(QGraphicsSimpleTextItem * text, bool state) {
        text->setBrush(state ? Qt::white : Qt::darkGray);
    }

    void updateValue(QGraphicsSimpleTextItem * text, unsigned value) {
        updateValue(text, QString::number(value, 10));
    }

    void updateValue(QGraphicsSimpleTextItem * text, QString value) {
        text->setBrush(Qt::white);
        text->setText(value);
    }

    void updatePoint(QGraphicsEllipseItem * p, uint8_t x, uint8_t y) {
        qreal xx = (p == thumb_ ? 50 : 210) - 3 + static_cast<qreal>(x) / 255 * 100;
        qreal yy = 80 - 3 + static_cast<qreal>(y) / 255 * 100;
        p->setRect(xx, yy, 6, 6);
        if (p == thumb_) {
            thumbX_->setText(QString::number(static_cast<uint>(x), 10));
            thumbY_->setText(QString::number(static_cast<uint>(y), 10));
        } else {
            accelX_->setText(QString::number(static_cast<uint>(x), 10));
            accelY_->setText(QString::number(static_cast<uint>(y), 10));
        }
    }

    QGraphicsSimpleTextItem * a_;
    QGraphicsSimpleTextItem * b_;
    QGraphicsSimpleTextItem * x_;
    QGraphicsSimpleTextItem * y_;
    QGraphicsSimpleTextItem * select_;
    QGraphicsSimpleTextItem * start_;
    QGraphicsSimpleTextItem * l_;
    QGraphicsSimpleTextItem * r_;
    QGraphicsSimpleTextItem * lVol_;
    QGraphicsSimpleTextItem * rVol_;
    QGraphicsSimpleTextItem * thumbBtn_;
    QGraphicsSimpleTextItem * thumbX_;
    QGraphicsSimpleTextItem * thumbY_;
    QGraphicsSimpleTextItem * accelX_;
    QGraphicsSimpleTextItem * accelY_;
    QGraphicsSimpleTextItem * dpadUp_;
    QGraphicsSimpleTextItem * dpadDown_;
    QGraphicsSimpleTextItem * dpadLeft_;
    QGraphicsSimpleTextItem * dpadRight_;
    QGraphicsEllipseItem * thumb_;
    QGraphicsEllipseItem * accel_;
    QGraphicsSimpleTextItem * vBatt_;
    QGraphicsSimpleTextItem * vcc_;
    QGraphicsSimpleTextItem * tempAvr_;
    QGraphicsSimpleTextItem * tempAccel_;
    QGraphicsSimpleTextItem * audio_;
    QGraphicsSimpleTextItem * chrg_;
}; // Calibration