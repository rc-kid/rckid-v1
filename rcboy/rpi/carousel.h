#pragma once

#include <iostream>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPropertyAnimation>
#include <QTimer>


#include "driver.h"

/** The carousel controller. 
 
    For simplicity this is the single controller used for the UI. Its items consist of an image and a text that can be swapped in/out.

    TODO eventually, add some nice effects, etc., play music and so on. 
 */
class Carousel : public QGraphicsScene {
    Q_OBJECT

    Q_PROPERTY(qreal animationStep READ getAnimationStep WRITE animationStep)
public:

    /** Element in the carousel. 
     */
    struct Element {
        QString text;
        QPixmap img;

        Element(char const * text, char const * img):
            text{text},
            img{QPixmap{img}.scaled(140, 140, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation)} {
        }
    }; // Carousel::Element

    explicit Carousel();

    void addElement(Element e) {
        elements_.push_back(e);
    }

    void setElement(size_t i) {
        i_ = i;
        auto element = elements_[i];
        img_[0]->setPixmap(element.img);
        text_[0]->setText(element.text);
        textWidth_[0] = text_[0]->boundingRect().width();
        text_[0]->setPos((320 - textWidth_[0]) / 2, 145);
        img_[0]->setPos(90,0);
    }

    void nextElement() {
        auto i = (i_ + 1) % elements_.size();
        auto element = elements_[i];
        img_[1]->setPixmap(element.img);
        text_[1]->setText(element.text);
        textWidth_[1] = text_[1]->boundingRect().width();
        text_[1]->setPos(320 + (320 - textWidth_[1]) / 2, 145);
        img_[1]->setPos(320,0);
        aDir_ = 1;
        a_->start();
    }

    void prevElement() {
        auto i = (i_ - 1) % elements_.size();
        auto element = elements_[i];
        img_[1]->setPixmap(element.img);
        text_[1]->setText(element.text);
        textWidth_[1] = text_[1]->boundingRect().width();
        text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2, 145);
        img_[1]->setPos(-140,0);
        aDir_ = -1;
        a_->start();
    }


private slots:

    void dpadLeft(bool state) { 
        if (state && a_->state() != QAbstractAnimation::State::Running) 
            prevElement();
    }
    
    void dpadRight(bool state) { 
        if (state && a_->state() != QAbstractAnimation::State::Running) 
            nextElement(); 
    }
    
    // TODO actual select

    void animationDone() {
        std::swap(img_[0], img_[1]);
        std::swap(text_[0], text_[1]);
        std::swap(textWidth_[0], textWidth_[1]);
        i_ = (i_ + aDir_) % elements_.size();
    }

private:

    qreal getAnimationStep() const {
        return aStep_;
    }

    void animationStep(qreal x) {
        aStep_ = x;
        if (aDir_ == 1) { // scrolling to the left
            text_[0]->setPos((320 - textWidth_[0]) / 2 - (3.2 * aStep_), 145);
            img_[0]->setPos(90 - 2.3 * aStep_ ,0);
            text_[1]->setPos(3.2 * (100 - aStep_) + (320 - textWidth_[1]) / 2, 145);
            img_[1]->setPos(320 - 2.3 * aStep_, 0);
        } else {
            text_[0]->setPos((320 - textWidth_[0]) / 2 + (3.2 * aStep_), 145);
            img_[0]->setPos(90 + 2.3 * aStep_,0);
            text_[1]->setPos(-320 + (320 - textWidth_[1]) / 2 + 3.2 * aStep_, 145);
            img_[1]->setPos(-140 + 2.3 * aStep_,0);
        }
    }

    QPropertyAnimation * a_ = nullptr;
    qreal aStep_ = 0;
    int aDir_ = 0;

    QGraphicsPixmapItem * img_[2];
    QGraphicsSimpleTextItem * text_[2];
    qreal textWidth_[2];

    std::vector<Element> elements_;
    size_t i_;

    
};
