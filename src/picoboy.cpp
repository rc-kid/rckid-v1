
#include <stdio.h>
#include <inttypes.h>
#include <pico/stdlib.h>

#include "platform/utils.h"

#include "peripherals/nrf24l01.h"
#include "peripherals/mpu6050.h"


#include "graphics/ili9341.h"
#include "graphics/fonts/FreeMono12pt7b.h"

#include "audio/audio.h"

#include "comms.h"


/** Pinout
 
           TX -- 0         VBUS --
           RX -- 1         VSYS --
              -- GND        GND --
          SDA -- 2        3V3EN --
          SCL -- 3    3V3 (out) --
      DISP_CS -- 4     ADC_VREF --
      DISP_DC -- 5      28/ADC2 -- I2S_DATA
              -- GND   GND/AGND --
   DISP_FMARK -- 6      27/ADC1 -- I2S_LRCLK
      DISP_WR -- 7      26/ADC0 -- I2S_BCLK
      DISP_D7 -- 8          RUN --
      DISP_D6 -- 9           22 -- ????
              -- GND        GND --
      DISP_D5 -- 10          21 -- ????
      DISP_D4 -- 11          20 -- ????
      DISP_D3 -- 12          19 -- MOSI
      DISP_D2 -- 13          18 -- SCK
              -- GND        GND --
      DISP_D1 -- 14          17 -- ????
      DISP_D0 -- 15          16 -- MISO


      7 pins left, 9 if UART disabled

      AVR_IRQ 
      SD_CARD_CS
      RADIO_CS
      I2S_
      I2S_
      I2S_
      PDM
 */


#define I2S_DATA 28
#define I2S_LRCLK 27
#define I2S_BCLK 26

#define ACCELEROMETER_ADDR 0x68
#define I2C_SDA 2
#define I2C_SCL 3

#define SPI_MISO 16
#define SPI_MOSI 19
#define SPI_SCK 18

#define NRF_CS 17
#define NRF_RXTX 20
#define NRF_IRQ 21

#define DISPLAY_CS 4
#define DISPLAY_DC 5
#define DISPLAY_FMARK 6
#define DISPLAY_WR 7
#define DISPLAY_DATA 8



constexpr unsigned LED_PIN = PICO_DEFAULT_LED_PIN;

//ILI9341<DISPLAY_CS, DISPLAY_DC, DISPLAY_FMARK, DISPLAY_WR, DISPLAY_DATA> display_;

//ILI9341<ILI9341_SPI<DISPLAY_CS, DISPLAY_DC>> display_;
ILI9341<ILI9341_8080<DISPLAY_CS, DISPLAY_DC, DISPLAY_WR, DISPLAY_FMARK, DISPLAY_DATA>, DISPLAY_FMARK> display_; 
//NRF24L01 radio_{NRF_CS, NRF_RXTX};

Audio audio_;



int main() {
    // Initialize the GPIO
    gpio::initialize();
    // initialize the onboard led and turn on for the initialization phase
    gpio::output(LED_PIN);
    gpio::high(LED_PIN);
    // initialize HW interfaces
    spi::initialize(SPI_MISO, SPI_MOSI, SPI_SCK);
    i2c::initialize(I2C_SDA, I2C_SCL);
    printf("Interfaces initialized\n");
    audio_.initialize(pio1, I2S_DATA, I2S_BCLK);
    printf("Audio initialized");
    // initialize the peripherals
    display_.initialize();
    printf("HW Initialization done");
    cpu::delay_ms(50); // delay 100ms so that the voltages across the system can settle
    display_.initializeDisplay(DisplayRotation::Raw);
    Canvas canvas{320,240};
    canvas.fill(Pixel::White());
    display_.fill(Rect::WH(240, 320), canvas.buffer(), canvas.bufferSize());
    printf("Display cleared");

    State state;
    i2c::transmit(AVR_I2C_ADDRESS, nullptr, 0, reinterpret_cast<uint8_t*>(& state), sizeof(state));
    printf("State:\n");
    printf("    vcc: : %u", state.vcc());
    printf("    temp : %i", state.temp());

    Pixel color;
    int i = 0;
    int inc = 1;
    canvas.setFont(FreeMono12pt7b);
    while (true) {
        canvas.fill(Pixel::Black());
        canvas.fill(Rect::XYWH(i,i,60,10), Pixel::White());
        canvas.write(0,18, "H_|_Hello World!", 1);
        canvas.write(0,18 + 24, "TWO IS TOO AWESOME", 1);
        display_.fill(Rect::WH(240,320), canvas.buffer(), canvas.bufferSize());
        i = i + inc;
        if (i >= 230)
            inc = -1;
        else if (i == 0)
            inc = 1;
    }
    /*

    //uint16_t color = 0x001f;
    auto start = get_absolute_time();
    display_.fill(Rect::WH(320,240), reinterpret_cast<uint8_t*>(& color), 2);
    auto end = get_absolute_time();
    // 40630 for "classic"
    // 12395 for "pio", 10 cycles
    // 11166 for "pio", 9 cycles
    // 9937 for "pio", 8 cycles
    printf("Refresh: %" PRId64 "[us]\n", absolute_time_diff_us(start, end));
    color = 0;
    display_.fill(Rect::XYWH(10, 10, 10, 10), reinterpret_cast<uint8_t*>(& color), 2);

    printf("Initialization done");
    gpio::low(LED_PIN);

    while(true) {}
    */

    //mpu6050_reset();

    /*

    int16_t acceleration[3], gyro[3], temp;

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(50);
        //printf("On");
        gpio_put(LED_PIN, 0);
        //printf(" and Off\n");
        sleep_ms(50);
        mpu6050_read_raw(acceleration, gyro, &temp);

        // These are the raw numbers from the chip, so will need tweaking to be really useful.
        // See the datasheet for more information
        printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
        //printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
        // Temperature is simple so use the datasheet calculation to get deg C.
        // Note this is chip temperature.
        //printf("Temp. = %f\n", (temp / 340.0) + 36.53);        
    }
    */
}


/*
void nrftest() {
    radio_.standby();
    cpu::delay_ms(10);
    radio_.initialize("TEST1", "TEST2", 80, NRF24L01::Speed::k250, NRF24L01::Power::dbm0);
    char buf[6];
    buf[5] = 0;
    radio_.rxAddress(buf);
    printf("%s\n", buf);
    //radio_.enableAutoAck(false);
    cpu::delay_ms(10);
    unsigned x = 0;
    printf("feature: %u\n", radio_.features());

    uint16_t i = 0;
    uint8_t buffer[32];
    uint16_t valid = 0;
    uint16_t retransmits = 0;
    while (true) {
        radio_.clearIRQ();
        for (int j = 0; j < 32; ++j)
            buffer[j] = i & 0xff;
        gpio::high(LED_PIN);
        radio_.transmit(buffer, 32);
        gpio::low(LED_PIN);
        //cpu::delay_ms(3);
        cpu::delay_ms(50);
        auto status = radio_.status();
        if (status.maxRetransmits()) {
            ++retransmits;
            radio_.flushTx();
        }
        if (status.dataSent()) 
            ++valid;
        if (i % 100 == 0) {
            i = 0;
            printf("%u: %u (ok), %u (timeout) (status: %u, config: %u, channel: %u)\n", x, valid, retransmits, status.raw, radio_.config().raw, radio_.channel());
            //char buf[6];
            //buf[5] = 0;
            //radio_.txAddress(buf);
            //printf("%s\n", buf);
            valid = 0;
            retransmits = 0;
            ++x;
        }
        ++i;
    }
}
*/

//40960 blink
