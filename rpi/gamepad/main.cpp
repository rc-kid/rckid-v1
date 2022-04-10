#include <cstdlib>

#include "gamepad.h"

#include "peripherals/nrf24l01.h"


int main(int argc, char * argv[]) {

    gpio::initialize();
    spi::initialize();

    NRF24L01 nrf{16,5};
    if (! nrf.initialize("ABCDE", "ABCDE"))
        std::cout << "Failed to initialize NRF";

/*

    Gamepad gamepad;
    gamepad.loop();
*/
    return EXIT_SUCCESS;
}
