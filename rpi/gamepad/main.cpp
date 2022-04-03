#include <cstdlib>

#include "gamepad.h"

int main(int argc, char * argv[]) {
    gpio::initialize();
    
    Gamepad gamepad;
    gamepad.loop();
    return EXIT_SUCCESS;
}
