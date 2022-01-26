/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
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



const uint LED_PIN = PICO_DEFAULT_LED_PIN;

//ILI9341<DISPLAY_CS, DISPLAY_DC, DISPLAY_FMARK, DISPLAY_WR, DISPLAY_DATA> display_;
ILI9341<ILI9341_SPI<DISPLAY_CS, DISPLAY_DC>> display_;
//NRF24L01 radio_{NRF_CS, NRF_RXTX};



int main() {
    stdio_init_all();
    printf("Initializing...\n");
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio::high(LED_PIN);
    cpu::delay_ms(100);
    gpio::low(LED_PIN);




    i2c::initialize(I2C_SDA, I2C_SCL);
    spi::initialize(SPI_MISO, SPI_MOSI, SPI_SCK);


    gpio::input(NRF_IRQ);

    display_.initialize();

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
