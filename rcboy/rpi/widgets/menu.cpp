#include "menu.h"

Menu::~Menu() {
    for (auto item : items_)
        delete item;
}

void Menu::addItem(Item * item) {
    items_.push_back(item);
}

