#pragma once

#include <QGraphicsScene>

class Page : public QGraphicsScene {
    Q_OBJECT
protected:
    friend class GUI;

    Page() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,192});
    }

    virtual void setOpacity(qreal value) {}

    virtual void onFocus() {}
    virtual void onBlur() {}

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

}; // Page