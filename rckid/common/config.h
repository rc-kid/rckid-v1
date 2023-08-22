/** \file Configuration 

  
*/

#define HEART_SIZE_SECONDS 600 
#define HEARTS_LOW_THRESHOLD 600

/** \section Input Controls
 
    
 */

#define LIBEVDEV_GAMEPAD_NAME "rckid-gamepad"

#define BTN_HOME_POWERON_PRESS 400

#define BTN_HOME_POWEROFF_PRESS 800

#define DEFAULT_RUMBLER_STRENGTH 64

#define CHARGING_COLOR Color::RGB(0, 64, 0)

#define ANALOG_BUTTON_THRESHOLD 48

/** Debounce interval for the buttons on RPI side in ticks (10ms). 
 */
#define BTN_DEBOUNCE_DURATION_TICKS 2

/** \section UI 
 */

#define FORCE_FULL_REDRAW false
//#define RENDERING_STATS
//#define RENDERING_STATS_DETAILED
#define RENDERING_STATS_FPS

#define BACKGROUND_OPACITY 0.3
#define GAME_PLAYER_BACKGROUND_OPACITY 128
#define VIDEO_PLAYER_BACKGROUND_OPACITY 128

#define WIDGET_FADE_TIME 250

/** \section Audio 

    Audio volume can be set in range from 0 to 100%, which is recalculated to log/exp scale via the amixer. Min value is always 0 (mute). Max value and the step, as well as the initial power-on value can be set here.  
 */
#define AUDIO_MAX_VOLUME 100
#define AUDIO_DEFAULT_VOLUME 50
#define AUDIO_VOLUME_STEP 10

#define AUDIO_MUSIC_ARTWORK_DIR "/rckid/images/music artwork"

/** \section Device Build Configuration 
 
    The following settings can enable or disable certain features based on the option RCKid hardware being present or not. 
 */

/** Determines whether the thumbstick has button capability. Switch joysticks do, PSP joysticks don't. When disabled, the libevdev gamepad device will not support the BTN_JOYSTICK key. 
*/
#define ENABLE_THUMBSTICK_BUTTON 1

/** \section Comms and timeouts
 */

/** I2C address of the AVR chip. Fully configurable, but this is a nice number. 
 */
#define AVR_I2C_ADDRESS 0x43

/** Size of the receive I2C buffer on the AVR side. The maximum size of any command. It onl has real effect on the AVR where memory is at premium. The Rpi can and will abuse this number.   
 */
#define I2C_BUFFER_SIZE 32


/** The AVR IRQ communication pin on the RPI side. Used by both the rckid and programmer.
 */
#define RPI_PIN_AVR_IRQ 25

/** AVR IRQ comms pin on the AVR side (used by the rckid and the bootloader). */
#define AVR_PIN_AVR_IRQ 10

/** AVR Pin for the backlight control (used by the rckid and the bootloade). */
#define AVR_PIN_BACKLIGHT 9

/** RPi power up timeout in ticks 
 */
#define RPI_POWERUP_TIMEOUT 6000

/** RPi powerdown in ticks
 */
#define RPI_POWERDOWN_TIMEOUT 6000

/** RPi ping timeout.
 */
#define RPI_PING_TIMEOUT 10000

/** \section Power & Voltage 
  
 */

/** When battery voltage reaches this threshold, we enter the PowerDown mode immediately. Must be at least 3V3 for RPi's correct operation. Setting it higher will make the system a bit more stable. When critical battery reached, RCkid won't start until charged above the BATTERY_THRESHOLD_CHARGED voltage. 
 */
#define BATTERY_THRESHOLD_CRITICAL 330

/** When the battery/vcc voltage goes above the battery charged threshold (i.e. either running on USB power or charged enough while sleeping), the critical battery flag is reset allowing RPi to boot. 
 */
#define BATTERY_THRESHOLD_CHARGED 370

/** When battery is below this threshold a low battery warning is displayed. 
 */
#define BATTERY_THRESHOLD_LOW 340

/** When VCC is above this value (set to be greater than that produced by Li-Ion battery, but lower than 5V to account for voltage drop) we can assume that we are running from the USB power. */
#define VCC_THRESHOLD_VUSB 440


/** \section Walkie-Talkie
*/

#define WALKIE_TALKIE_HEARTBEAT_INTERVAL_MIN 300
#define WALKIE_TALKIE_HEARTBEAT_INTERVAL_MAX 500
#define WALKIE_TALKIE_STORE_PTT 