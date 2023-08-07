#pragma once

#include "widget.h"
#include "window.h"

/** A simple class for debugging purposes. 
 
    By default displays the device's inputs. 
 */
class DebugView : public Widget {
public:

    bool fullscreen() const { return true; }

protected:

    void onFocus() override {
        rckid().setGamepadActive(true);
    }

    void onBlur() override {
        rckid().setGamepadActive(false);
    }

    void draw() override{
        comms::ExtendedState state{rckid().extendedState()};
        Canvas & c = window().canvas();
        BeginBlendMode(BLEND_ADD_COLORS);
        // now draw the displayed information
        c.drawText(160, 20, "VCC:", DARKGRAY);
        c.drawText(210, 20, STR(state.einfo.vcc()), WHITE);
        c.drawText(240, 20, "VBATT:", DARKGRAY);
        c.drawText(290, 20, STR(state.einfo.vBatt()), WHITE);
        c.drawText(160, 40, "TEMP:", DARKGRAY);
        c.drawText(210, 40, STR(state.einfo.temp()), WHITE);
        c.drawText(240, 40, "ATEMP:", DARKGRAY);
        c.drawText(290, 40, STR(rckid().accelTemp()), WHITE);
        c.drawText(160, 60, "CHRG:", DARKGRAY);
        c.drawText(210, 60, state.einfo.charging() ? "1" : "0", WHITE);
        c.drawText(160, 80, "UP:", DARKGRAY);
        c.drawText(210, 80, STR(state.uptime), WHITE);

        //DrawTextEx(window().helpFont(), "VCC:", 160, 20, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR(rckid().vcc()).c_str(), 210, 20, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), "VBATT:", 240, 20, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR(rckid().vBatt()).c_str(), 290, 20, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), "TEMP:", 160, 40, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR(rckid().avrTemp()).c_str(), 210, 40, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), "ATEMP:", 240, 40, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR(rckid().accelTemp()).c_str(), 290, 40, 16, 1.0, WHITE);

        //DrawTextEx(window().helpFont(), "CHRG:", 160, 60, 16, 1, DARKGRAY);
        //DrawTextEx(window().helpFont(), rckid().charging() ? "1" : "0", 210, 60, 16, 1.0, WHITE);

        //DrawTextEx(window().helpFont(), "UP:", 160, 80, 16, 1, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR(rckid().avrUptime()).c_str(), 210, 80, 16, 1.0, WHITE);
        EndBlendMode();

        BeginBlendMode(BLEND_ALPHA);
        // draw rckid's outline
        DrawCircleSector(Vector2{25,40}, 20, 180, 270, 8, DARKGRAY);
        DrawCircleSector(Vector2{25, 215}, 20, 270, 360, 8, DARKGRAY);
        DrawCircleSector(Vector2{125,40}, 20, 90, 180, 8, DARKGRAY);
        DrawCircleSector(Vector2{105, 195}, 40, 0, 90, 16, DARKGRAY);
        DrawRectangle(5, 40, 120, 175, DARKGRAY);
        DrawRectangle(25, 20, 105, 20, DARKGRAY);
        DrawRectangle(25, 215, 85, 20, DARKGRAY);
        DrawRectangle(125, 40, 20, 155, DARKGRAY);
        // draw the display cutout joy & accel
        DrawRectangle(25, 40, 100, 75, BLACK);

        // draw the ABXY buttons
        DrawCircle(125, 155, 10, btnA_ ? YELLOW : BLACK);
        DrawCircle(105, 175, 10, btnB_ ? RED : BLACK);
        DrawCircle(105, 135, 10, btnX_ ? BLUE : BLACK);
        DrawCircle(85, 155, 10, btnY_ ? GREEN : BLACK);
        // dpad
        DrawRectangle(20, 135, 12, 12, dpadLeft_ ? RED : BLACK);
        DrawTriangle(Vector2{32,135}, Vector2{32, 147}, Vector2{38, 141}, dpadLeft_ ? RED : BLACK);
        DrawRectangle(44, 135, 12, 12, dpadRight_ ? RED : BLACK);
        DrawTriangle(Vector2{38, 141}, Vector2{44, 147}, Vector2{44, 135}, dpadRight_ ? RED : BLACK);
        DrawRectangle(32, 123, 12, 12, dpadUp_ ? RED : BLACK);
        DrawTriangle(Vector2{38, 141}, Vector2{44, 135}, Vector2{32, 135}, dpadUp_ ? RED : BLACK);
        DrawRectangle(32, 147, 12, 12, dpadDown_ ? RED : BLACK);
        DrawTriangle(Vector2{38, 141}, Vector2{32, 147}, Vector2{44, 147}, dpadDown_ ? RED : BLACK);
        // thumbstick
        DrawCircle(38, 185, 18, btnJoy_ ? RED : BLACK);
        DrawCircleV(Vector2{20 + joyX_ * 36.0f / 255, 167 + joyY_ * 36.0f / 255}, 2, WHITE);
        // home, start and select
        DrawCircle(75, 210, 5, btnHome_ ? RED : BLACK);
        DrawRectangle(55, 205, 6, 10, btnSelect_ ? RED : BLACK);
        DrawRectangle(90, 207, 10, 6, btnStart_ ? RED : BLACK);
        // left and right buttons
        DrawRectangle(47, 220, 3, 15, btnL_ ? RED : BLACK);
        DrawRectangle(103, 220, 3, 15, btnR_ ? RED : BLACK);
        
        EndBlendMode();

        c.setFg(DARKGRAY);
        c.drawText(45, 42, "Joy");
        c.drawText(85, 42, "Acc");
        c.drawText(30, 60, "X");
        c.drawText(30, 78, "Y");
        c.setFg(WHITE);
        c.drawText(45, 60, STR((int)joyX_));
        c.drawText(45, 78, STR((int)joyY_));
        c.drawText(85, 60, STR((int)accelX_));
        c.drawText(85, 78, STR((int)accelY_));

        //DrawTextEx(window().helpFont(), "Joy", 45, 42, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), "Acc", 85, 42, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), "X", 30, 60, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), "Y", 30, 78, 16, 1.0, DARKGRAY);
        //DrawTextEx(window->helpFont(), "Z", 30, 96, 16, 1.0, DARKGRAY);
        //DrawTextEx(window().helpFont(), STR((int)joyX_).c_str(), 45, 60, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), STR((int)joyY_).c_str(), 45, 78, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), STR((int)accelX_).c_str(), 85, 60, 16, 1.0, WHITE);
        //DrawTextEx(window().helpFont(), STR((int)accelY_).c_str(), 85, 78, 16, 1.0, WHITE);

    }

    void btnA(bool state) override { btnA_ = state; }
    void btnB(bool state) override { btnB_ = state; Widget::btnB(state); }
    void btnX(bool state) override { btnX_ = state; }
    void btnY(bool state) override { btnY_ = state; }
    void btnL(bool state) override { btnL_ = state; }
    void btnR(bool state) override { btnR_ = state; }
    void btnSelect(bool state) override { btnSelect_ = state; }
    void btnStart(bool state) override { btnStart_ = state; }
    void btnJoy(bool state) override { btnJoy_ = state; }
    void dpadLeft(bool state) override { dpadLeft_ = state; }
    void dpadRight(bool state) override { dpadRight_ = state; }
    void dpadUp(bool state) override { dpadUp_ = state; }
    void dpadDown(bool state) override { dpadDown_ = state; }
    void joy(uint8_t x, uint8_t y) override { joyX_ = x; joyY_ = y; }
    void accel(uint8_t x, uint8_t y) override { accelX_ = x; accelY_ = y;}
    void btnHome(bool state) override { btnHome_ = state; }

private:
    bool btnA_ = false;
    bool btnB_ = false;
    bool btnX_ = false;
    bool btnY_ = false;
    bool btnL_ = false;
    bool btnR_ = false;
    bool dpadLeft_ = false;
    bool dpadRight_ = false;
    bool dpadUp_ = false;
    bool dpadDown_ = false;
    bool btnJoy_ = false;
    bool btnSelect_ = false;
    bool btnStart_ = false;
    bool btnHome_ = false;
    uint8_t joyX_ = 128;
    uint8_t joyY_ = 128;
    uint8_t accelX_ = 10;
    uint8_t accelY_ = 255;
}; // DebugView