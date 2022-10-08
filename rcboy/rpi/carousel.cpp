#include "carousel.h"

Carousel::Carousel() {
    setBackgroundBrush(Qt::black);
    setSceneRect(QRectF{0,0,320,192});
    a_ = new QPropertyAnimation{this, "animationStep"};
    a_->setDuration(500);
    a_->setStartValue(0);
    a_->setEndValue(100);
    connect(a_, & QPropertyAnimation::finished, this, & Carousel::animationDone);
    for (size_t i = 0; i < 2; ++i) {
        img_[i] = addPixmap(QPixmap{});
        text_[i] = addSimpleText("");
        text_[i]->setFont(QFont{"OpenDyslexic Nerd Font", 22});
        text_[i]->setBrush(Qt::white);
    }
    Driver * driver = Driver::instance();
    connect(driver, & Driver::dpadLeft, this, & Carousel::dpadLeft, Qt::QueuedConnection);
    connect(driver, & Driver::dpadRight, this, & Carousel::dpadRight, Qt::QueuedConnection);
}