#pragma once

#include "gui.h"
#include "driver.h"

#include <iostream>

class DebugInfo : public Page {
    Q_OBJECT
public:
    explicit DebugInfo() {
        a_ = newText("A", 5, 0);
        b_ = newText("B", 20, 0);
        x_ = newText("X", 35, 0);
        y_ = newText("Y", 50, 0); 
        select_ = newText("SEL", 65, 0);
        start_ = newText("START", 95, 0);
        l_ = newText("L", 140, 0);
        r_ = newText("R", 155, 0);
        lVol_ = newText("L_VOL", 170, 0);
        rVol_ = newText("R_VOL", 215, 0);
        thumbBtn_ = newText("THUMB", 260, 0);
        dpadUp_ = newText("UP", 5, 16);
        dpadDown_ = newText("DOWN", 25, 16);
        dpadLeft_ = newText("LEFT", 70, 16);
        dpadRight_ = newText("RIGHT", 105, 16);
        newText("Thumbstick:", 0, 40);
        newText("X:", 5, 56);
        newText("Y:", 5, 72);
        thumbX_ = newText("255", 25, 56);
        thumbY_ = newText("0", 25, 72);
        thumbX_->setBrush(Qt::white);
        thumbY_->setBrush(Qt::white);

        newText("Accel:", 160, 40);
        newText("X:", 165, 56);
        newText("Y:", 165, 72);
        accelX_ = newText("255", 185, 56);
        accelY_ = newText("0", 185, 72);
        accelX_->setBrush(Qt::white);
        accelY_->setBrush(Qt::white);
        addLine(50, 106, 150, 106, QPen{Qt::darkGray});
        addLine(100, 56, 100, 156, QPen{Qt::darkGray});
        thumb_ = addEllipse(100 - 3, 106 - 3, 6,6,QPen{Qt::white});

        addLine(210, 106, 310, 106, QPen{Qt::darkGray});
        addLine(260, 56, 260, 156, QPen{Qt::darkGray});
        accel_ = addEllipse(260 - 3, 106 - 3, 6,6,QPen{Qt::white});
        newText("VBatt:", 0, 160);
        vBatt_ = newText("0", 5, 175);
        newText("Vcc:", 50, 160);
        vcc_ = newText("0", 55, 175);
        newText("tAvr:", 100, 160);
        tempAvr_ = newText("0", 105, 175);
        newText("tAccel:", 150, 160);
        tempAccel_ = newText("0", 155, 175);
    }

protected slots:

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

    void batteryVoltage(uint16_t value) override { updateValue(vBatt_, value); }
    void tempAvr(uint16_t value) override { updateValue(tempAvr_, value); }
    void tempAccel(uint16_t value) override { updateValue(tempAccel_, value); }

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
        text->setBrush(Qt::white);
        text->setText(QString::number(value, 10));
    }

    void updatePoint(QGraphicsEllipseItem * p, uint8_t x, uint8_t y) {
        qreal xx = (p == thumb_ ? 50 : 210) - 3 + static_cast<qreal>(x) / 255 * 100;
        qreal yy = 56 - 3 + static_cast<qreal>(y) / 255 * 100;
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
}; // Calibration