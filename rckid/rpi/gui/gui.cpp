#include "gui.h"

GUI::GUI():
    helpFont_{LoadFont("assets/fonts/Iosevka.ttf")},
    menuFont_{LoadFont("assets/fonts/OpenDyslexic.otf")} {
}


void GUI::draw() {
    BeginDrawing();
    ClearBackground(BLACK);


    drawHeader();
    drawFooter();
    EndDrawing();
}




void GUI::drawHeader() {
    // DrawFPS(0,0);

    // battery voltage

    DrawTextEx(menuFont_, "RCGirl", 100, 160, 64, 1.0, WHITE);
}

void GUI::drawFooter() {
    float x = 0;
    for (FooterItem const & item: footer_) {
        DrawCircle(x + 10, 230, 6, item.color);
        x += 20; 
        Vector2 size = MeasureTextEx(helpFont_, item.text.c_str(), 16, 1.0);
        DrawTextEx(helpFont_, item.text.c_str(), x, 220 + (20 - size.y) / 2 , 16, 1.0, WHITE);
        x += size.x + 5;
    }
}