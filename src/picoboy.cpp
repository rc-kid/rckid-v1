/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <inttypes.h>
#include <pico/stdlib.h>

#include "peripherals/nrf24l01.h"

#include "rp2040/i2c.h"
#include "ili9341.h"

//#include <pico/binary_info.h>
//#include <hardware/i2c.h>

#define ACCELEROMETER_ADDR 0x68
#define I2C_SDA 4
#define I2C_SCL 5

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
    // initialize the peripherals
    display_.initialize();
    printf("HW Initialization done");
    cpu::delay_ms(50); // delay 100ms so that the voltages across the system can settle
    display_.initializeDisplay(DisplayRotation::Left);
    uint16_t color = 0;
    while (true) {
        color = 0b0000000011111000; // red max
        display_.fill(Rect::WH(320,240), reinterpret_cast<uint8_t*>(& color), 2);
        sleep_ms(15);
        color = 0b1110000000000111; // green max
        display_.fill(Rect::WH(320,240), reinterpret_cast<uint8_t*>(& color), 2);
        sleep_ms(15);
        color = 0b0001111100000000; // blue max
        display_.fill(Rect::WH(320,240), reinterpret_cast<uint8_t*>(& color), 2);
        sleep_ms(15);
    }

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
