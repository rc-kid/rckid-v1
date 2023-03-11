static constexpr uint16_t LOW_BATTERY_THRESHOLD = 340;
static constexpr uint16_t CRITICAL_BATTERY_THRESHOLD = 330;
static constexpr uint16_t VUSB_BATTERY_THRESHOLD = 440;

/** Determines whether the thumbstick has button capability. Switch joysticks do, PSP joysticks don't. When disabled, the libevdev gamepad device will not support the BTN_JOYSTICK key. 
*/
#define ENABLE_THUMBSTICK_BUTTON 1
