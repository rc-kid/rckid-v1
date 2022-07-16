#include <cstdlib>
#include <iostream>

#include "platform/platform.h"
#include "comms.h"

constexpr unsigned GPIO_AVR_IRQ = 4;


volatile bool irq = true;

void isrAvrIrq(int gpio, int level, uint32_t tick, void * _) {
    irq = true;
}


int main(int argc, char * argv[]) {
    gpio::initialize();
    i2c::initializeMaster();
    cpu::delay_ms(5);
    gpio::inputPullup(GPIO_AVR_IRQ);
    #if (defined ARCH_RPI)
    gpioSetISRFuncEx(GPIO_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) isrAvrIrq, nullptr);
    #endif
    size_t i = 0;
    std::cout << "Oh hai!" << std::endl;
    while (true) {
        if (irq) {
            irq = false;
            comms::Status status;
            i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, sizeof(comms::Status));
            std::cout << i << ":";
            if (status.avrPowerOn())
                std::cout << " pwrOn";
            if (status.charging())
                std::cout << " chrg";
            if (status.vusb())
                std::cout << " vusb";
            if (status.lowBattery())
                std::cout << " lowBatt";
            if (status.micLoud())
                std::cout << " mic";
            if (status.btnLeftVolume())
                std::cout << " lvol";
            if (status.btnRightVolume())
                std::cout << " rvol";
            if (status.btnJoystick())
                std::cout << " joy:btn";
            std::cout << " joyX:" << (int)status.joyX() << " joyY:" << (int)status.joyY() << " photores:" << (int)status.photores() << std::endl;
            ++i;
        }
    }


    return EXIT_SUCCESS;
}