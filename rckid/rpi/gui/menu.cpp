#include "gui.h"
#include "menu.h"

void Menu::clear() {
    for (Item * i : items_)
        delete i;
    items_.clear();
}

void Menu::Item::initialize(GUI * gui) {
    img_ = LoadTexture(imgFile_.c_str());
    Vector2 fs = MeasureText(gui->menuFont(), title_.c_str(), MENU_FONT_SIZE);
    titleWidth_ = fs.x;
}

void WidgetItem::onSelect(GUI * gui) {
    gui->setWidget(widget_);
}

void SubmenuItem::onSelect(GUI * gui) {
    if (updater_)
        updater_(gui, this);
    gui->setMenu(& submenu_);
}