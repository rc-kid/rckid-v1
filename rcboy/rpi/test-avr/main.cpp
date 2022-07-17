#include <cstdlib>
#include <iostream>

#include "platform/platform.h"
#include "comms.h"

constexpr unsigned GPIO_AVR_IRQ = 4;


volatile bool irq = true;

void isrAvrIrq(int gpio, int level, uint32_t tick, void * _) {
    irq = true;
}




void displayStatus() {
    size_t i = 0;
    while (true) {
        if (irq) {
            irq = false;
            comms::Status statuses[10];
            comms::Status & status = statuses[0];
            //i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, sizeof(comms::Status))) ;
            i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, (uint8_t*)& status, 33) ;
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
}


void testAudio() {
    uint8_t buffer[8100];
    {
        msg::StartAudioRecording start;
        i2c::transmit(comms::AVR_I2C_ADDRESS, (uint8_t*)&start, sizeof(start), nullptr, 0);
    }
    size_t packets = 0;
    size_t i = 0;
    while (i < 8000) {
        if (irq) {
            irq = false;
            i2c::transmit(comms::AVR_I2C_ADDRESS, nullptr, 0, buffer + i, 32);
            i += 32;
            ++packets;
        }
    }
    {
        msg::StopAudioRecording stop;
        i2c::transmit(comms::AVR_I2C_ADDRESS, (uint8_t*)&stop, sizeof(stop), nullptr, 0);
    }
    std::cout << "Received " << i << " bytes in " << packets << " packets" << std::endl;
    size_t missing = 0;
    uint8_t expect = 0;
    for (int x = 0; x < i; ++x) {
        while (expect != buffer[x]) {
            ++expect;
            ++missing;
        }
        ++expect;
    }
    std::cout << "Missing values: " << missing << std::endl;
}

int main(int argc, char * argv[]) {
    gpio::initialize();
    i2c::initializeMaster();
    cpu::delay_ms(5);
    gpio::inputPullup(GPIO_AVR_IRQ);
    #if (defined ARCH_RPI)
    gpioSetISRFuncEx(GPIO_AVR_IRQ, FALLING_EDGE, 0,  (gpioISRFuncEx_t) isrAvrIrq, nullptr);
    #endif
    std::cout << "Oh hai!" << std::endl;
    if (argc == 1)
        displayStatus();
    else if (strcmp(argv[1], "audio") == 0)
        testAudio();
    return EXIT_SUCCESS;
}
