#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include "platform/platform.h"
#include "driver.h"

#include "log.h"
#include "gui.h"

#include "utils/json.h"
#include "utils/exec.h"


int main(int argc, char * argv[]) {

    //auto json = json::parse("{ \"foo\" : \"bar\" }");
    //std::cout << json["foo"] << std::endl;



    //return EXIT_SUCCESS;
    // initialize the driver singleton and peripherals
    Driver * driver = Driver::initialize();
    

    // initialize the GUI
    LOG("initializing GUI...");
    LOG("  QT");

    return GUI::exec(argc, argv);
}