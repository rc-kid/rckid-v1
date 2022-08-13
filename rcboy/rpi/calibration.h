#pragma once

#include <QWidget>
#include <QLabel>

class Calibration : QWidget {
    Q_OBJECT
public:
    explicit Calibration() {

    }

private:
    
    QLabel * thumbX_;
    QLabel * thumbY_;
    QLabel * acdelX_;
    QLabel * accelY_;
    QLabel * accelZ_;

}; // Calibration