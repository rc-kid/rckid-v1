#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPropertyAnimation>
#include <QTimer>

#include "../driver.h"

#include "menu.h"

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

    void showItem(size_t i);

    void nextItem();

    void prevItem();

    Menu * menu() const { return menu_; }

    void setMenu(Menu * menu) { setMenu(menu, 0); }

    void setMenu(Menu * menu, size_t item);

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
    
    void itemChangeDone();

    void menuChangeDone();

private:

    /** Displays info that element is empty. 
     */
    void showEmpty();

    void selectCurrent() {
        emit selected(i_, (*menu_)[i_]);
    }

    bool busy() const { 
        return aItem_->state() == QAbstractAnimation::State::Running || aMenu_->state() == QAbstractAnimation::State::Running;
    }

    qreal getItemChangeStep() const { return aStep_; }

    void itemChangeStep(qreal x);

    qreal getMenuChangeStep() const { return aStep_; }

    void menuChangeStep(qreal x);

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

