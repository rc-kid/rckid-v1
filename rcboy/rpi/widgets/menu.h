#pragma once

#include <cstdlib>
#include <vector>
#include <functional>
#include <QString>
#include <QPixmap>

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

    using SelectEvent = std::function<void(Item const *)>;

    Item(std::string const & text, std::string const & img):
        text_{QString{text.c_str()}},
        imgSource_{img} { 
    }

    Item(std::string const & text, std::string const & img, SelectEvent onSelect):
        text_{QString{text.c_str()}},
        imgSource_{img},
        onSelect_{onSelect} { 
    }

    QString const & text() const {
        return text_;
    }

    QPixmap const & img() const {
        if (img_ == nullptr)
            img_ = new QPixmap{imgSource_.c_str()};
        return * img_;
    }

    SelectEvent const & onSelect() const {
        return onSelect_;
    }

    ~Item() {
        delete img_;
    }

protected:
    Item() = default;

    QString text_;
    std::string imgSource_; 
    SelectEvent onSelect_;
    mutable QPixmap * img_ = nullptr;
}; // Menu::Item


