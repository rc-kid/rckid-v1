#pragma once

#include <iostream>

#define LOG(...) std::cout << __VA_ARGS__ << std::endl
#define OK(...) std::cout << __VA_ARGS__ << std::endl;
#define ERROR(...) std::cout << "ERROR: " << __VA_ARGS__ << std::endl;