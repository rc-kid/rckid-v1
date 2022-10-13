#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QTime>

#include "driver.h"
#include "widgets/menu.h"
#include "widgets/page.h"

#define COLOR_HEADER_DEFAULT QColor{64, 64, 64}


#define COLOR_BATTERY_OK QColor{0, 255, 0}
#define COLOR_BATTERY_WARN QColor{255, 127, 0}
#define COLOR_BATTERY_CRITICAL QColor{255, 0, 0}

/** RCBoy's launcher main window. 
 
    Contains the overlay with header and footer displaying common information and the main page's graphics view for the actual contents. 

    # Header

    Information about rcboy's status. In normal mode, only the icons are displayed for a status shorthand. In the power menu details, such as actual battery and volume percentage as well as the WiFi network rcboy is connected to is displayed alongside the icons. 

    # Footer

    The footer always contains information about the various UI actions available under given context. 
 */
class GUI : public QMainWindow {
    Q_OBJECT
public:
    class Header;
    class Footer;

    static int exec(int argc, char * argv[]);

    static GUI * instance() { return singleton_; }

protected:

    /** Sets active page. 
     
        Active page is displayed on the screen and receives user input events.
     */
    void setActivePage(Page * page);

private slots:

    void buttonA(bool state) { if (activePage_) activePage_->buttonA(state); }
    void buttonB(bool state) { if (activePage_) activePage_->buttonB(state); }
    void buttonX(bool state) { if (activePage_) activePage_->buttonX(state); }
    void buttonY(bool state) { if (activePage_) activePage_->buttonY(state); }
    void buttonStart(bool state) { if (activePage_) activePage_->buttonStart(state); }
    void buttonSelect(bool state) { if (activePage_) activePage_->buttonSelect(state); }
    void buttonLeft(bool state) { if (activePage_) activePage_->buttonLeft(state); }
    void buttonRight(bool state) { if (activePage_) activePage_->buttonRight(state); }
    void buttonVolumeLeft(bool state) {}
    void buttonVolumeRight(bool state) {}
    void buttonThumb(bool state) { if (activePage_) activePage_->buttonThumb(state); }
    void thumbstick(uint8_t x, uint8_t y) { if (activePage_) activePage_->thumbstick(x, y); }
    void accel(uint8_t x, uint8_t y) { if (activePage_) activePage_->accel(x, y); }
    void dpadUp(bool state) { if (activePage_) activePage_->dpadUp(state); }
    void dpadDown(bool state) {if (activePage_) activePage_->dpadDown(state); }
    void dpadLeft(bool state) {if (activePage_) activePage_->dpadLeft(state); }
    void dpadRight(bool state) {if (activePage_) activePage_->dpadRight(state); }

    // TODO also dependning on the volume it might be muted...
    void headphones(bool state);

    void charging(bool state);

    void lowBattery();

    /** Displays the battery capacity icon and charging status.
     */
    void batteryVoltage(uint16_t value);

    void tempAvr(uint16_t value);

    void tempAccel(uint16_t value);

private:
    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;

    void initializeOverlay() {
        overlay_->setSceneRect(QRectF{0,0,320,240});

        auto iconsFont = QFont{"Iosevka", 14};
        auto textFont = QFont{"Iosevka", 10};
        battery_ = overlay_->addSimpleText("", iconsFont);
        wifi_ = overlay_->addSimpleText("直", iconsFont);
        wifi_->setBrush(COLOR_HEADER_DEFAULT);
        volume_ = overlay_->addSimpleText("", iconsFont);
        batteryPct_ = overlay_->addSimpleText("", textFont);
        batteryPct_->setBrush(Qt::darkGray);
        wifiSSID_ = overlay_->addSimpleText("Internet 10", textFont);
        wifiSSID_->setBrush(Qt::darkGray);
        volumePct_ = overlay_->addSimpleText("60", textFont);
        volumePct_->setBrush(Qt::darkGray);
        headphones(true);
        batteryVoltage(390);
        repositionOverlay();
    }

    void repositionOverlay() {
        qreal left = 320;
        if (overlayDetails_) {
            left -= batteryPct_->boundingRect().width();
            batteryPct_->setPos(left, 5);
            left -= 2;
        }
        left -= battery_->boundingRect().width();
        battery_->setPos(left, 0);
        left -= 2;
        if (overlayDetails_) {
            left -= wifiSSID_->boundingRect().width();
            wifiSSID_->setPos(left, 5);
            left -= 2;
        }
        left -= wifi_->boundingRect().width();
        wifi_->setPos(left, 0);
        left -= 2;
        if (overlayDetails_) {
            left -= volumePct_->boundingRect().width();
            volumePct_->setPos(left, 5);
            left -= 2;
        }
        left -= volume_->boundingRect().width();
        volume_->setPos(left, 0);
    }

    QGraphicsView * pageView_;
    QGraphicsView * overlayView_;

    // the overlay scene and its elements
    QGraphicsScene * overlay_;
    bool overlayDetails_ = false;
    QGraphicsSimpleTextItem * battery_;
    QGraphicsSimpleTextItem * batteryPct_;
    QGraphicsSimpleTextItem * wifi_;
    QGraphicsSimpleTextItem * wifiSSID_;
    QGraphicsSimpleTextItem * volume_;
    QGraphicsSimpleTextItem * volumePct_;

    /** Main menu. 
     */
    Menu menu_;
    /** Admin menu (activated by simultaneous volume up and down press). Power management, wifi and advanced functions. */
    Menu adminMenu_;



    /** The active page that is being sent gamepad events. 
     */
    Page * activePage_ = nullptr;

    static GUI * singleton_;

#if (defined ARCH_MOCK)
private:
    /** Emulates the gamepad with keyboard on mock systems for easier development. 
     */
    void keyPressEvent(QKeyEvent *e) override { keyToGamepad(e, true); }
    void keyReleaseEvent(QKeyEvent *e) override { keyToGamepad(e, false); };

    void keyToGamepad(QKeyEvent * e, bool state);
#endif

}; // GUI





