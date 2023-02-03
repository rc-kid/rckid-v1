//#include "raylib.h"

#include "platform/platform.h"

#include "rcboy.h"

/** Main RCBoy app. 
 
    TODO start main loop and do some nice and efficient event loop for the main thread

 */
int main(int argc, char * argv[]) {
    // initialize raylib
    //InitWindow(320, 240, "RCBoy");
    RCBoy * rcboy = RCBoy::initialize();
    while (true) {
        cpu::delay_ms(100);
    }
}