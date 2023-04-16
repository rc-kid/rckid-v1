#include <cassert>
#include <sstream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#define UNREACHABLE throw std::runtime_error{STR("Unreachable code triggered at " << __FILE__ << ":" << __LINE__)};
#define UNIMPLEMENTED throw std::runtime_error{STR("Unimplemented code triggered at " << __FILE__ << ":" << __LINE__)};

#define ASSERT(...) assert(__VA_ARGS__)
