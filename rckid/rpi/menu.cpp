#include "window.h"
#include "menu.h"

void Menu::clear() {
    for (Item * i : items_)
        delete i;
    items_.clear();
}

void Menu::onRenderingPaused() {
    for (Item * i : items_)
        i->titleWidth_ = Menu::Item::UNINITIALIZED;
}

void Menu::Item::initialize(Window * window) {
    img_ = LoadTexture(imgFile_.c_str());
    Vector2 fs = MeasureText(window->menuFont(), title_.c_str(), MENU_FONT_SIZE);
    titleWidth_ = fs.x;
}

void WidgetItem::onSelect(Window * window) {
    window->setWidget(widget_);
}

void SubmenuItem::onSelect(Window * window) {
    if (updater_)
        updater_(window, this);
    window->setMenu(& submenu_);
}