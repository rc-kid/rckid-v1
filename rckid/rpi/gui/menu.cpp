#include "gui.h"
#include "menu.h"

void Menu::Item::initialize(GUI * gui) {
    img_ = LoadTexture(imgFile_.c_str());
    Vector2 fs = MeasureText(gui->menuFont(), title_.c_str(), MENU_FONT_SIZE);
    titleWidth_ = fs.x;
}

void WidgetItem::onSelect(GUI * gui) {
    gui->setWidget(widget_);
}