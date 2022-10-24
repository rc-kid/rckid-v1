#include "carousel.h"

Carousel::Carousel(Menu * menu):
    menu_{menu},
    empty_{QPixmap{"assets/images/021-poo.png"}} {
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

    if (menu_ == nullptr || menu_->empty())
        showEmpty();
    else
        showItem(0);
}

void Carousel::nextItem() {
    if (menu_ == nullptr || menu_->empty())
        return;
    auto i = (i_ + 1) % menu_->size();
    auto item = (*menu_)[i];
    img_[1]->setPixmap(item->img());
    text_[1]->setText(item->text());
    textWidth_[1] = text_[1]->boundingRect().width();
    text_[1]->setPos(320 + (320 - textWidth_[1]) / 2, 169);
    img_[1]->setPos(320,36);
    aDir_ = 1;
    aItem_->start();
}

void Carousel::prevItem() {
    if (menu_ == nullptr || menu_->empty())
        return;
    auto i = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
    auto item = (*menu_)[i];
    img_[1]->setPixmap(item->img());
    text_[1]->setText(item->text());
    textWidth_[1] = text_[1]->boundingRect().width();
    text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2, 169);
    img_[1]->setPos(-140,36);
    aDir_ = -1;
    aItem_->start();
}

void Carousel::setMenu(Menu * menu, size_t item, bool animation) {
    if (animation == false) {
        menu_ = menu;
        showItem(item);
    } else {
        if (menu->empty()) {
            img_[1]->setPixmap(empty_);
            text_[1]->setText("Empty...");
        } else {
            img_[1]->setPixmap((*menu)[item]->img());
            text_[1]->setText((*menu)[item]->text());
        }
        // make the new item completely opaque and place it on screen
        img_[1]->setOpacity(0);
        text_[1]->setOpacity(0);
        textWidth_[1] = text_[1]->boundingRect().width();
        text_[1]->setPos((320 - textWidth_[1]) / 2, 169);
        img_[1]->setPos(96,36);
        menu_ = menu;
        i_ = item;
        aMenu_->start();
    }
}

void Carousel::itemChangeDone() {
    std::swap(img_[0], img_[1]);
    std::swap(text_[0], text_[1]);
    std::swap(textWidth_[0], textWidth_[1]);
    if (aDir_ == 1)
        i_ = (i_ + 1) % menu_->size();
    else 
        i_ = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
    // make sure we land at the precise coordinates
    text_[0]->setPos((320 - textWidth_[0]) / 2, 169);
    img_[0]->setPos(96,36);
}

void Carousel::menuChangeDone() {
    // hide the backup items and then reset their opacity
    img_[0]->setPos(320, 0);
    text_[0]->setPos(320, 0);
    img_[0]->setOpacity(1.0);
    text_[0]->setOpacity(1.0);
    std::swap(img_[0], img_[1]);
    std::swap(text_[0], text_[1]);
    std::swap(textWidth_[0], textWidth_[1]);
}

void Carousel::showEmpty() {
    img_[0]->setPixmap(empty_);
    text_[0]->setText("Empty...");
    textWidth_[0] = text_[0]->boundingRect().width();
    text_[0]->setPos((320 - textWidth_[0]) / 2, 169);
    img_[0]->setPos(96,36);
}

void Carousel::showItem(size_t i) {
    if (menu_ == nullptr || menu_->empty())
        return;
    i_ = i;
    auto item = (*menu_)[i];
    img_[0]->setPixmap(item->img());
    text_[0]->setText(item->text());
    textWidth_[0] = text_[0]->boundingRect().width();
    text_[0]->setPos((320 - textWidth_[0]) / 2, 169);
    img_[0]->setPos(96,36);
}

void Carousel::itemChangeStep(qreal x) {
    aStep_ = x;
    if (aDir_ == 1) { // scrolling to the left
        text_[0]->setPos((320 - textWidth_[0]) / 2 - (3.2 * aStep_), 169);
        img_[0]->setPos(96 - 2.3 * aStep_ ,36);
        text_[1]->setPos(3.2 * (100 - aStep_) + (320 - textWidth_[1]) / 2, 169);
        img_[1]->setPos(320 - 2.3 * aStep_, 36);
    } else {
        text_[0]->setPos((320 - textWidth_[0]) / 2 + (3.2 * aStep_), 169);
        img_[0]->setPos(96 + 2.3 * aStep_,36);
        text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2 + 3.2 * aStep_, 169);
        img_[1]->setPos(-140 + 2.3 * aStep_,36);
    }
}

void Carousel::menuChangeStep(qreal x) {
    aStep_ = x;
    if (aStep_ < 100) {
        setOpacity((100 - aStep_) / 100);
    } else {
        text_[1]->setOpacity((aStep_ - 100) / 100);
        img_[1]->setOpacity((aStep_ - 100) / 100);
    }
}
