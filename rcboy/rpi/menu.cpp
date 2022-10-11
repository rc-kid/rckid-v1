#include "menu.h"

Menu::~Menu() {
    for (auto item : items_)
        delete item;
}

void Menu::addItem(Item * item) {
    items_.push_back(item);
}


Carousel::Carousel(Menu * menu):
    menu_{menu},
    empty_{QPixmap{"assets/images/021-poo.png"}} {
    setBackgroundBrush(Qt::black);
    setSceneRect(QRectF{0,0,320,192});
    aItem_ = new QPropertyAnimation{this, "itemChangeStep"};
    aItem_->setDuration(500);
    aItem_->setStartValue(0);
    aItem_->setEndValue(100);
    aItem_->setEasingCurve(QEasingCurve::InOutQuad);    
    connect(aItem_, & QPropertyAnimation::finished, this, & Carousel::itemChangeDone);
    aMenu_ = new QPropertyAnimation{this, "menuChangeStep"};
    aMenu_->setDuration(500);
    aMenu_->setStartValue(0);
    aMenu_->setEndValue(200);
    connect(aMenu_, & QPropertyAnimation::finished, this, & Carousel::menuChangeDone);
    for (size_t i = 0; i < 2; ++i) {
        img_[i] = addPixmap(QPixmap{});
        text_[i] = addSimpleText("");
        text_[i]->setFont(QFont{"OpenDyslexic Nerd Font", 22});
        text_[i]->setBrush(Qt::white);
    }
    Driver * driver = Driver::instance();
    connect(driver, & Driver::buttonLeft, this, & Carousel::dpadLeft, Qt::QueuedConnection);
    connect(driver, & Driver::buttonRight, this, & Carousel::dpadRight, Qt::QueuedConnection);
    connect(driver, & Driver::buttonStart, this, & Carousel::buttonStart, Qt::QueuedConnection);
    connect(driver, & Driver::buttonB, this, & Carousel::buttonB, Qt::QueuedConnection);

    connect(driver, & Driver::dpadUp, this, & Carousel::buttonB, Qt::QueuedConnection);
    connect(driver, & Driver::dpadLeft, this, & Carousel::dpadLeft, Qt::QueuedConnection);
    connect(driver, & Driver::dpadRight, this, & Carousel::dpadRight, Qt::QueuedConnection);
    connect(driver, & Driver::dpadDown, this, & Carousel::buttonStart, Qt::QueuedConnection);
    connect(driver, & Driver::buttonThumb, this, & Carousel::buttonStart, Qt::QueuedConnection);
    if (menu_ == nullptr || menu_->empty())
        showEmpty();
    else
        showItem(0);
}