#pragma once

#include <iostream>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPropertyAnimation>
#include <QTimer>


#include "driver.h"




class Menu {
public:
    class Item;

    Menu() = default;

    ~Menu();

    bool empty() const { return items_.empty(); }

    size_t size() const { return items_.size(); }

    Item const * operator[](size_t index) const { return items_[index]; }

    void addItem(Item * item);

private:

    Menu * parent_ = nullptr;
    std::vector<Item *> items_;

}; // Menu

class Menu::Item {
    friend class Menu;
public:

    Item(std::string const & text, std::string const & img):
        text_{QString{text.c_str()}},
        imgSource_{img} { 
    }

    QString const & text() const {
        return text_;
    }

    QPixmap const & img() const {
        if (img_ == nullptr)
            img_ = new QPixmap{imgSource_.c_str()};
        return * img_;
    }

    ~Item() {
        delete img_;
    }

protected:
    Item() = default;

    QString text_;
    std::string imgSource_; 
    mutable QPixmap * img_ = nullptr;
}; // Menu::Item


/** The carousel controller. 
 
    For simplicity this is the single controller used for the UI. Its items consist of an image and a text that can be swapped in/out.

    TODO eventually, add some nice effects, etc., play music and so on. 
 */
class Carousel : public QGraphicsScene {
    Q_OBJECT

    Q_PROPERTY(qreal itemChangeStep READ getItemChangeStep WRITE itemChangeStep)
    Q_PROPERTY(qreal menuChangeStep READ getMenuChangeStep WRITE menuChangeStep)
public:

    explicit Carousel(Menu * menu);

    void showItem(size_t i) {
        if (menu_ == nullptr || menu_->empty())
            return;
        i_ = i;
        auto item = (*menu_)[i];
        img_[0]->setPixmap(item->img());
        text_[0]->setText(item->text());
        textWidth_[0] = text_[0]->boundingRect().width();
        text_[0]->setPos((320 - textWidth_[0]) / 2, 145);
        img_[0]->setPos(96,12);
    }

    void nextItem() {
        if (menu_ == nullptr || menu_->empty())
            return;
        auto i = (i_ + 1) % menu_->size();
        auto item = (*menu_)[i];
        img_[1]->setPixmap(item->img());
        text_[1]->setText(item->text());
        textWidth_[1] = text_[1]->boundingRect().width();
        text_[1]->setPos(320 + (320 - textWidth_[1]) / 2, 145);
        img_[1]->setPos(320,12);
        aDir_ = 1;
        aItem_->start();
    }

    void prevItem() {
        if (menu_ == nullptr || menu_->empty())
            return;
        auto i = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
        auto item = (*menu_)[i];
        img_[1]->setPixmap(item->img());
        text_[1]->setText(item->text());
        textWidth_[1] = text_[1]->boundingRect().width();
        text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2, 145);
        img_[1]->setPos(-140,12);
        aDir_ = -1;
        aItem_->start();
    }

    Menu * menu() const { return menu_; }

    void setMenu(Menu * menu) { setMenu(menu, 0); }

    void setMenu(Menu * menu, size_t item) {
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
        text_[1]->setPos((320 - textWidth_[1]) / 2, 145);
        img_[1]->setPos(96,12);
        menu_ = menu;
        i_ = item;
        aMenu_->start();
    }

signals:

    /** Triggered when a */
    void selected(size_t index, Menu::Item const * item);

    void back();

private slots:

    void dpadLeft(bool state) { 
        if (state && ! busy()) 
            prevItem();
    }
    
    void dpadRight(bool state) { 
        if (state && ! busy()) 
            nextItem(); 
    }

    void buttonStart(bool state) {
        if (state && ! busy()) 
            selectCurrent();
    }

    void buttonB(bool state) {
        if (state && ! busy()) 
            emit back();
    }
    
    void itemChangeDone() {
        std::swap(img_[0], img_[1]);
        std::swap(text_[0], text_[1]);
        std::swap(textWidth_[0], textWidth_[1]);
        if (aDir_ == 1)
            i_ = (i_ + 1) % menu_->size();
        else 
            i_ = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
    }

    void menuChangeDone() {
        // hide the backup items and then reset their opacity
        img_[0]->setPos(320, 240);
        text_[0]->setPos(320, 240);
        img_[0]->setOpacity(1.0);
        text_[0]->setOpacity(1.0);
        std::swap(img_[0], img_[1]);
        std::swap(text_[0], text_[1]);
        std::swap(textWidth_[0], textWidth_[1]);
    }

private:

    /** Displays info that element is empty. 
     */
    void showEmpty() {
        img_[0]->setPixmap(empty_);
        text_[0]->setText("Empty...");
        textWidth_[0] = text_[0]->boundingRect().width();
        text_[0]->setPos((320 - textWidth_[0]) / 2, 145);
        img_[0]->setPos(96,12);
    }

    void selectCurrent() {
        emit selected(i_, (*menu_)[i_]);
    }


    bool busy() const { 
        return aItem_->state() == QAbstractAnimation::State::Running || aMenu_->state() == QAbstractAnimation::State::Running;
    }

    qreal getItemChangeStep() const { return aStep_; }

    void itemChangeStep(qreal x) {
        aStep_ = x;
        if (aDir_ == 1) { // scrolling to the left
            text_[0]->setPos((320 - textWidth_[0]) / 2 - (3.2 * aStep_), 145);
            img_[0]->setPos(96 - 2.3 * aStep_ ,12);
            text_[1]->setPos(3.2 * (100 - aStep_) + (320 - textWidth_[1]) / 2, 145);
            img_[1]->setPos(320 - 2.3 * aStep_, 12);
        } else {
            text_[0]->setPos((320 - textWidth_[0]) / 2 + (3.2 * aStep_), 145);
            img_[0]->setPos(96 + 2.3 * aStep_,12);
            text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2 + 3.2 * aStep_, 145);
            img_[1]->setPos(-140 + 2.3 * aStep_,12);
        }
    }

    qreal getMenuChangeStep() const { return aStep_; }

    void menuChangeStep(qreal x) {
        aStep_ = x;
        if (aStep_ < 100) {
            text_[0]->setOpacity((100 - aStep_) / 100);
            img_[0]->setOpacity((100 - aStep_) / 100);
        } else {
            text_[1]->setOpacity((aStep_ - 100) / 100);
            img_[1]->setOpacity((aStep_ - 100) / 100);
        }
    }

    QPropertyAnimation * aItem_ = nullptr;
    QPropertyAnimation * aMenu_ = nullptr;
    qreal aStep_ = 0;
    int aDir_ = 0;

    QGraphicsPixmapItem * img_[2];
    QGraphicsSimpleTextItem * text_[2];
    qreal textWidth_[2];
    QPixmap empty_;

    Menu * menu_;
    size_t i_;
    
};
