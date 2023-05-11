

#define BTN_HOME_POWERON_PRESS 250

#define BTN_HOME_POWEROFF_PRESS 500

/** RPi power up timeout in ticks 
 */
#define RPI_POWERUP_TIMEOUT 1000
#define RPI_POWERDOWN_TIMEOUT 10000

#define RPI_PING_TIMEOUT 10000

/** I2C address of the AVR chip. Fully configurable, but this is a nice number. 
 */
#define AVR_I2C_ADDRESS 0x43

/** Size of the receive I2C buffer on the AVR side. The maximum size of any command. 
 */
#define I2C_BUFFER_SIZE 32

/** When battery voltage reaches this threshold, we enter the PowerDown mode immediately. Must be at least 3V3 for RPi's correct operation. Setting it higher will make the system a bit more stable. 
 */
#define BATTERY_THRESHOLD_CRITICAL 330

/** When battery is below this threshold a low battery warning is displayed. 
 */
#define BATTERY_THRESHOLD_LOW 340

/** When VCC is above this value (set to be greater than that produced by Li-Ion battery, but lower than 5V to account for voltage drop) we can assume that we are running from the USB power. */
#define VCC_THRESHOLD_VUSB 440






#define AUDIO_MAX_VOLUME 100
#define AUDIO_DEFAULT_VOLUME 50
#define AUDIO_VOLUME_STEP 10







//static constexpr uint16_t LOW_BATTERY_THRESHOLD = 340;
//static constexpr uint16_t CRITICAL_BATTERY_THRESHOLD = 330;
//static constexpr uint16_t VUSB_BATTERY_THRESHOLD = 440;

/** Determines whether the thumbstick has button capability. Switch joysticks do, PSP joysticks don't. When disabled, the libevdev gamepad device will not support the BTN_JOYSTICK key. 
*/
#define ENABLE_THUMBSTICK_BUTTON 1
