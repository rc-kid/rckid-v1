#pragma once

#include "gui.h"
#include "driver.h"

#include <iostream>

class Calibration : public Page {
    Q_OBJECT
public:
    explicit Calibration() {
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



}; // Calibration