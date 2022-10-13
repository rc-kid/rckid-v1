#include <QGraphicsView>
#include <QGraphicsScene>


#include "page.h"

class Gauge : public Page {
    Q_OBJECT
public:

    Gauge() {
        gauge_ = addRect(0, 50, 320, 75, QPen{QColor{10,10,10}}, QBrush{QColor{10,10,10}});
        gaugeValue_ = addRect(0, 50, 0, 75, QPen{Qt::blue}, QBrush{Qt::blue});
        text_ = addSimpleText("");
        text_->setFont(QFont{"OpenDyslexic Nerd Font", 22});
        text_->setBrush(Qt::white);
        mask_ = addPixmap(QPixmap{"assets/gauge_mask.png"});
        mask_->setPos(0, 45);
        reset("Brightness", 50, 0, 100, 10);
    }

    void reset(std::string const & text, size_t val, size_t min, size_t max, size_t step) {
        text_->setText(text.c_str());
        textWidth_ = text_->boundingRect().width();
        text_->setPos((320 - textWidth_) / 2, 145);
        min_ = min;
        max_ = max;
        step_ = step;
        setValue(val);
    }

    void setValue(size_t value) {
        value_ = value;
        qreal width = (value_ - min_) * 320.0 / (max_ - min_);
        gaugeValue_->setRect(0, 50, width, 75);
    }

    void prev() {
        setValue(step_ > value_ ? 0 : value_ - step_);
    }

    void next() {
        setValue(step_ + value_ > max_ ? max_ : value_ + step_);
    }

protected:

    void buttonLeft(bool state) override { if (state) prev(); }
    void buttonRight(bool state) override { if (state) next(); }
    void dpadLeft(bool state) override { if (state) prev(); }
    void dpadRight(bool state)  override { if (state) next(); }

private:

    size_t min_;
    size_t max_;
    size_t value_;
    size_t step_;

    QGraphicsSimpleTextItem * text_;
    QGraphicsRectItem * gauge_;
    QGraphicsRectItem * gaugeValue_;
    QGraphicsPixmapItem * mask_;
    qreal textWidth_;

}; // Gauge