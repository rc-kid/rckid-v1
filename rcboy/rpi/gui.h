#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPropertyAnimation>
#include <QTimer>
#include <QTime>
#include <vector>

#include "driver.h"
#include "widgets/menu.h"
#include "widgets/page.h"
#include "widgets/carousel.h"
#include "widgets/gauge.h"

#define COLOR_HEADER_DEFAULT QColor{64, 64, 64}


#define COLOR_BATTERY_OK QColor{0, 255, 0}
#define COLOR_BATTERY_WARN QColor{255, 127, 0}
#define COLOR_BATTERY_CRITICAL QColor{255, 0, 0}

/** RCBoy's launcher main window. 
 
    Contains the overlay with header and footer displaying common information and the main page's graphics view for the actual contents. 

    # Volume buttons

    - change the volume (up, down, repeated )
    - when both pressed, enter the power menu
    - ideally the power menu does not change the volume

    - down = set mode
    - down while in mode: execute mode

    # Header

    Information about rcboy's status. In normal mode, only the icons are displayed for a status shorthand. In the power menu details, such as actual battery and volume percentage as well as the WiFi network rcboy is connected to is displayed alongside the icons. 

    # Footer

    The footer always contains information about the various UI actions available under given context. 
 */
class GUI : public QMainWindow {
    Q_OBJECT

    Q_PROPERTY(qreal pageChangeStep READ getPageChangeStep WRITE pageChangeStep)
public:

    static int exec(int argc, char * argv[]);

    static GUI * instance() { return singleton_; }

public slots:

    void navigateBack();
    void navigateTo(Page * page);
    void navigateTo(Menu * menu) { navigateTo(menu, 0); }
    void navigateTo(Menu * menu, size_t index);

private slots:

    void buttonA(bool state) { if (activePage_) activePage_->buttonA(state); }
    void buttonB(bool state) { if (activePage_) activePage_->buttonB(state); }
    void buttonX(bool state) { if (activePage_) activePage_->buttonX(state); }
    void buttonY(bool state) { if (activePage_) activePage_->buttonY(state); }
    void buttonStart(bool state) { if (activePage_) activePage_->buttonStart(state); }
    void buttonSelect(bool state) { if (activePage_) activePage_->buttonSelect(state); }
    void buttonLeft(bool state) { if (activePage_) activePage_->buttonLeft(state); }
    void buttonRight(bool state) { if (activePage_) activePage_->buttonRight(state); }
    void buttonThumb(bool state) { if (activePage_) activePage_->buttonThumb(state); }
    void thumbstick(uint8_t x, uint8_t y) { if (activePage_) activePage_->thumbstick(x, y); }
    void accel(uint8_t x, uint8_t y) { if (activePage_) activePage_->accel(x, y); }
    void dpadUp(bool state) { if (activePage_) activePage_->dpadUp(state); }
    void dpadDown(bool state) {if (activePage_) activePage_->dpadDown(state); }
    void dpadLeft(bool state) {if (activePage_) activePage_->dpadLeft(state); }
    void dpadRight(bool state) {if (activePage_) activePage_->dpadRight(state); }

    /** Volume up and down controls.   

        In normal and power menu settings pressing volume buttons triggers the switch to volume gauge page. In external app mode, they should directly change the volume without displaying anything. 

        When the switch to volume gauge page is underway, we check in the middle if both buttons are pressed and if so instead of volume gauge, the switch happens to the power menu (unless we already are in power menu. )
     */
    void buttonVolumeLeft(bool state);
    void buttonVolumeRight(bool state);

    // TODO also dependning on the volume it might be muted...
    void headphones(bool state);

    void charging(bool state);

    void lowBattery();

    /** Displays the battery capacity icon and charging status.
     */
    void batteryVoltage(uint16_t value);

    void tempAvr(uint16_t value);

    void tempAccel(uint16_t value);

    void pageChangeDone();


private:


    /** Determines the mode in which the GUI runs. 
     */
    enum class Mode {
        Normal, 
        PowerMenuTransition,
        PowerMenu,
        ExternalApp,
        ExternalAppPowerMenu,
    };

    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;


    /** \name Navigation transitions. 
     */
    //@{
    void pushActivePage();

    void setActivePage(Page * page);

    qreal getPageChangeStep() const { return aStep_; }

    void pageChangeStep(qreal x);

    // the active page
    Page * activePage_ = nullptr;
    // the page the gui is switching to, mid transition, the activaPage_ will become this one
    Page * nextPage_ = nullptr;
    // animation for the transition
    QPropertyAnimation * aPage_ = nullptr;

    qreal aStep_;
    //@}
    /** \name Navigation Stack 
     */
    //@{
    // an item for the navigation stack. Can be either a carousel (with menu and index specification), or a generic page
    struct NavigationItem {
        Page * page;
        Menu * menu;
        size_t index;
    }; // NavigationItem

    // stack of the menu items
    std::vector<NavigationItem> navStack_;
    //@}

    void initializeOverlay();

    void repositionOverlay();

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


    /** Mode of the application. */
    Mode mode_ = Mode::Normal;

    /** Main menu. 
     */
    Menu menu_;
    /** Power menu (activated by simultaneous volume up and down press). Power management, wifi and advanced functions. */
    Menu powerMenu_;
    /** Settings menu.
     */
    Menu settingsMenu_;

    Carousel * carousel_ = nullptr;
    Gauge * gauge_ = nullptr;
    Gauge * volumeGauge_ = nullptr;
    Page * debugInfo_ = nullptr;

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





