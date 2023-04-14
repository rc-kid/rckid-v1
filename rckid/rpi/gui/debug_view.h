#pragma once

#include "widget.h"
#include "gui.h"

/** A simple class for debugging purposes. 
 
    By default displays the device's inputs. 
 */
class DebugView : public Widget {
public:

    DebugView(GUI * gui): Widget{gui} {}

    bool fullscreen() const { return true; }

protected:

    void draw() {
        // draw rckid's outline
        DrawCircleSector(Vector2{25,40}, 20, 180, 270, 8, DARKGRAY);
        DrawCircleSector(Vector2{25, 215}, 20, 270, 360, 8, DARKGRAY);
        DrawCircleSector(Vector2{125,40}, 20, 90, 180, 8, DARKGRAY);
        DrawCircleSector(Vector2{105, 195}, 40, 0, 90, 16, DARKGRAY);
        DrawRectangle(5, 40, 120, 175, DARKGRAY);
        DrawRectangle(25, 20, 105, 20, DARKGRAY);
        DrawRectangle(25, 215, 85, 20, DARKGRAY);
        DrawRectangle(125, 40, 20, 155, DARKGRAY);
        // draw the display cutout joy & accel values
        DrawRectangle(25, 40, 100, 75, BLACK);
        DrawTextEx(gui()->helpFont(), "Joy", 45, 42, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), "Acc", 85, 42, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), "X", 30, 60, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), "Y", 30, 78, 16, 1.0, DARKGRAY);
        //DrawTextEx(gui->helpFont(), "Z", 30, 96, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), STR((int)joyX_).c_str(), 45, 60, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), STR((int)joyY_).c_str(), 45, 78, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), STR((int)accelX_).c_str(), 85, 60, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), STR((int)accelY_).c_str(), 85, 78, 16, 1.0, WHITE);

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
        // TODO

        DrawTextEx(gui()->helpFont(), "VCC:", 160, 20, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), STR(vcc_).c_str(), 210, 20, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), "VBATT:", 240, 20, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), STR(vbatt_).c_str(), 290, 20, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), "TEMP:", 160, 40, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), STR(temp_).c_str(), 210, 40, 16, 1.0, WHITE);
        DrawTextEx(gui()->helpFont(), "ATEMP:", 240, 40, 16, 1.0, DARKGRAY);
        DrawTextEx(gui()->helpFont(), STR(atemp_).c_str(), 290, 40, 16, 1.0, WHITE);

    }

    void btnA(bool state) override { btnA_ = state; }
    void btnB(bool state) override { btnB_ = state; }
    void btnX(bool state) override { btnX_ = state; }
    void btnY(bool state) override { btnY_ = state; }
    void btnL(bool state) override { btnL_ = state; }
    void btnR(bool state) override { btnR_ = state; }
    void btnSelect(bool state) override {}
    void btnStart(bool state) override {}
    void btnJoy(bool state) override { btnJoy_ = state; }
    void dpadLeft(bool state) override { dpadLeft_ = state; }
    void dpadRight(bool state) override { dpadRight_ = state; }
    void dpadUp(bool state) override { dpadUp_ = state; }
    void dpadDown(bool state) override { dpadDown_ = state; }
    void joy(uint8_t x, uint8_t y) override { joyX_ = x; joyY_ = y; }
    void accel(uint8_t x, uint8_t y) override { accelX_ = x; accelY_ = y;}
    void btnVolUp(bool state) override {}
    void btnVolDown(bool state) override {}
    void btnHome(bool state) override {}

private:
    bool btnA_;
    bool btnB_;
    bool btnX_;
    bool btnY_;
    bool btnL_;
    bool btnR_;
    bool dpadLeft_;
    bool dpadRight_;
    bool dpadUp_;
    bool dpadDown_;
    bool btnJoy_;
    uint8_t joyX_ = 128;
    uint8_t joyY_ = 128;
    uint8_t accelX_ = 10;
    uint8_t accelY_ = 255;

    uint16_t vcc_ = 500;
    uint16_t vbatt_ = 370;
    uint16_t temp_ = 275;
    uint16_t atemp_ = 295;

}; // DebugView