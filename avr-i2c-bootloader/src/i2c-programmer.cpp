#include <iostream>
#include <iomanip>
#include <unordered_map>

#include "platform/platform.h"
#include "utils/utils.h"
#include "utils/intelhex.h"


#include "config.h"

using namespace platform;

static uint8_t i2cAddress = 0x43; 
static bool verbose = true;
static bool trace = true;
static bool verify = true;
static bool dry = false;
static unsigned timeout = 50;
static std::string command;
static std::string hexFile;

/** Prints macro. 
 */
#define INFO(...) std::cout << __VA_ARGS__
#define VERBOSE(...) if (verbose) std::cout << __VA_ARGS__
#define TRACE(...) if (trace) std::cout << __VA_ARGS__
#define PROGRESS_LINE std::cout << (trace ? "\n" : "\x1B[2K\r")

#define VERIFY(...) do { if (verify) { __VA_ARGS__ } } while (false)

#define ERROR(...) throw std::runtime_error{STR(__VA_ARGS__)}


/** List of chips supported by the bootloader. 
 
    Each chip has a MCU name, signature, family identifier, memory size and page size specified.  
*/
#define SUPPORTED_CHIPS \
    CHIP(ATTiny1614, 0x1e9422, TinySeries1, 16384, 128) \
    CHIP(ATTiny1616, 0x1e9421, TinySeries1, 16384, 128) \
    CHIP(ATTiny1617, 0x1e9420, TinySeries1, 16384, 128) \
    CHIP(ATTiny3216, 0x1e9521, TinySeries1, 32768, 128)



/** Displays given uint8_t buffer in hex digits together with size. Useful for printing & comparing byte buffers. 
 */
std::string printBuffer(uint8_t const * buffer, size_t numBytes) {
    std::stringstream ss;
    for (size_t i = 0; i < numBytes; ++i) 
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(buffer[i]);
    ss << " (" << std::dec << numBytes << " bytes)" << std::setfill(' ');
    return ss.str();
}

/** Structure that holds the information about the attached chip. 
 
    This info comes either from the chip itself (such as fuses, clock control and address & last command), or is calculated from the detected part and SUPPORTED_CHIPS configuration. 
*/
struct ChipInfo {
public:
    enum class Family {
        TinySeries1,
    }; // Family

    enum class MCU {
        #define CHIP(ID, ...) ID, 
        SUPPORTED_CHIPS
        #undef CHIP
    }; // Chip

    Family family;
    MCU mcu;
    uint16_t memsize;
    uint16_t pagesize;
    uint16_t progstart;
    uint16_t mappedProgOffset;
    State state;

    ChipInfo(State state):
        state{state} {
        uint32_t signature = state.deviceId0 << 16 | state.deviceId1 << 8 | state.deviceId2;
        switch (signature) {
            #define CHIP(ID, SIG, FAMILY, MEMSIZE, PAGESIZE) case SIG: mcu = MCU::ID; family = Family::FAMILY; memsize = MEMSIZE; pagesize = PAGESIZE; break;
            SUPPORTED_CHIPS
            #undef CHIP
            default:
                ERROR("Unknown device signature");
        }
        switch (family) {
            case Family::TinySeries1:
                progstart = state.fuses[8] * 256;
                mappedProgOffset = 0x8000;
                break;
        }
    }

    friend std::ostream & operator << (std::ostream & s, Family family) {
        switch (family) {
            case Family::TinySeries1:
                s << "TinySeries1";
                break;
        }
        return s;
    }

    friend std::ostream & operator << (std::ostream & s, MCU mcu) {
        switch (mcu) {
            #define CHIP(ID, ...) case MCU::ID: s << # ID; break;
            SUPPORTED_CHIPS
            #undef CHIP
        }
        return s;
    }

    friend std::ostream & operator << (std::ostream & s, ChipInfo const & info) {
        s << std::setw(30) << "signature: " << printBuffer(& info.state.deviceId0, 3) << std::endl;
        s << std::setw(30) << "mode: " << ((info.state.status & 0x7 == 0x7) ? "bootloader" : "app") << std::endl;
        s << std::setw(30) << "mcu: " << info.mcu << std::endl;
        s << std::setw(30) << "family: " << info.family << std::endl;
        s << std::setw(30) << "memory: " << std::dec << info.memsize << std::endl;
        s << std::setw(30) << "pagesize: " << info.pagesize << std::endl;
        s << std::setw(30) << "program start: " << "0x" << std::hex <<info.progstart << std::endl;
        s << std::setw(30) << "mapped program offset: " << "0x" << std::hex <<info.mappedProgOffset << std::endl;
        switch (info.family) {
            case Family::TinySeries1:
                s << std::setw(30) << "FUSE.WDTCFG: " << std::hex << (int)info.state.fuses[0] << std::endl;
                s << std::setw(30) << "FUSE.BODCFG: " << std::hex << (int)info.state.fuses[1] << std::endl;
                s << std::setw(30) << "FUSE.OSCCFG: " << std::hex << (int)info.state.fuses[2] << std::endl;
                s << std::setw(30) << "FUSE.TCD0CFG: " << std::hex << (int)info.state.fuses[4] << std::endl;
                s << std::setw(30) << "FUSE.SYSCFG0: " << std::hex << (int)info.state.fuses[5] << std::endl;
                s << std::setw(30) << "FUSE.SYSCFG1: " << std::hex << (int)info.state.fuses[6] << std::endl;
                s << std::setw(30) << "FUSE.APPEND: " << std::hex << (int)info.state.fuses[7] << std::endl;
                s << std::setw(30) << "FUSE.BOOTEND: " << std::hex << (int)info.state.fuses[8] << std::endl;
                s << std::setw(30) << "FUSE.LOCKBIT: " << std::hex << (int)info.state.fuses[10] << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKCTRLA: " << std::hex << (int)info.state.mclkCtrlA << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKCTRLB: " << std::hex << (int)info.state.mclkCtrlB << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKLOCK: " << std::hex << (int)info.state.mclkLock << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKSTATUS: " << std::hex << (int)info.state.mclkStatus << std::endl;
                break;
        }
        s << std::setw(30) << "address: " << std::hex << info.state.address << std::endl;
        s << std::setw(30) << "nvm last address: " << std::hex << info.state.nvmAddress << std::endl;
        s << std::dec;
        return s;
    }

}; // ChipInfo    

void waitForDevice() {
    if (timeout != 0)
        cpu::delay_ms(timeout);
    size_t i = 0;
    while (gpio::read(PIN_AVR_IRQ) == false) {
        if (++i > 1000) {
            cpu::delay_ms(1);
            if (i > 2000)
                ERROR("Waiting for command timed out");
        }
    };
}

void sendCommand(uint8_t cmd) {
    VERBOSE("  cmd: " << (int)cmd << std::endl);
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        ERROR("Cannot send command" << (int)cmd);
    waitForDevice();        
}

void readBuffer(uint8_t * buffer, size_t numBytes) {
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, buffer, numBytes))
       ERROR("Cannot read buffer of length " << numBytes);
    TRACE("  read: " << printBuffer(buffer, numBytes) << std::endl);
}

void writeBuffer(uint8_t const * buffer, size_t numBytes) {
    uint8_t * data = new uint8_t[numBytes + 1];
    data[0] = CMD_WRITE_BUFFER;
    memcpy(data + 1, buffer, numBytes);
    if (!i2c::transmit(I2C_ADDRESS, data, numBytes + 1, nullptr, 0))
       ERROR("Cannot write buffer of length " << numBytes);
    TRACE("  write: " << printBuffer(buffer, numBytes) << std::endl);
}

/** Sends the reset command. 
 
    Special method since reset should not wait for the AVR_IRQ busy flag (this would lead in an infinite loop when rebooting to the program)
*/
bool resetAvr() {
    VERBOSE("  resetting the avr" << std::endl);
    uint8_t cmd = CMD_RESET;
    return i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0);
}

void setAddress(uint16_t addr) {
    VERBOSE("  setting address to " << std::hex << addr << std::endl);
    uint8_t cmd[3] = { CMD_SET_ADDRESS, (uint8_t)(addr & 0xff),  (uint8_t)((addr >> 8) & 0xff)};
    if (! i2c::transmit(I2C_ADDRESS, cmd, 3, nullptr, 0))
        ERROR("Cannot set address to " << addr);
    waitForDevice();        
}

uint16_t getAddress(uint8_t & err) {
    sendCommand(CMD_INFO);
    State state;
    readBuffer((uint8_t *) & state, sizeof(State));
    err = state.lastError;
    return state.address;
}

/** Enters the bootloader mode.
 
    This is a bit trickier since we can't HW reset the device, nor can we assume that there is a valid application that will respon to the SW reset case.
 */
void enterBootloader() {
    INFO("Entering bootloader..." << std::endl);
    // set the IRQ pin low to enter bootloader upon device reset
    gpio::output(PIN_AVR_IRQ);
    gpio::low(PIN_AVR_IRQ);
    // attempt to SW reset the device
    resetAvr();
    // wait for th I2C comms to start
    size_t i = 0;
    while (true) {
        cpu::delay_ms(250);
        if (i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
            break;
        if (i++ == 0) 
            VERBOSE(std::endl << "  device not detected, waiting" << std::flush);
        VERBOSE("." << std::flush);
        if (i > 40) 
            ERROR("Unable to connect to device when entering bootloader mode");
    }
    // set the pin to input pullup so that AVR can pull low when busy
    gpio::inputPullup(PIN_AVR_IRQ);
}

ChipInfo getChipInfo() {
    enterBootloader();
    sendCommand(CMD_INFO);
    State state;
    readBuffer((uint8_t *) & state, sizeof(State));
    return ChipInfo{state};
}

void writeProgram(ChipInfo const & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
    INFO("Writing " << std::dec << (end - start) << " bytes in " << (end - start) / info.pagesize << " pages" << std::endl);
    for (uint16_t address = start; address < end; address += info.pagesize) {
        INFO(std::setw(5) << std::hex << address << ": " << std::flush);
        setAddress(address + info.mappedProgOffset);
        VERIFY(
            uint8_t err;
            uint16_t a = getAddress(err);
            if (a != address)
                ERROR("Comms mismatch: Expected address " << address << ", found " << a);
            setAddress(address + info.mappedProgOffset);
        );
        for (size_t i = 0; i < info.pagesize; i += 32) {
            writeBuffer(pgm + address + i, 32);
            INFO("." << std::flush);
        }
        if (!dry)
            sendCommand(CMD_WRITE_PAGE);
        VERIFY(
            uint8_t err;
            uint16_t a = getAddress(err);
            if (info.pagesize != a - address - 0x8000)
                ERROR("Comms mismatch: Expected " << info.pagesize << " bytes written, but " << a - address << " found");
            if (!dry && err != 0) // the error has no meanin in dry mode
                ERROR("Flash write error: " << (int)err);
        );
        PROGRESS_LINE;
    }
}

void verifyProgram(ChipInfo & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
    INFO("Verifying " << std::dec << (end - start) << " bytes in " << (end - start) / info.pagesize << " pages" << std::endl);
    uint8_t buffer[32];
    for (uint16_t address = start; address < end; address += info.pagesize) {
        INFO(std::setw(5) << std::hex << address << ": " << std::flush);
        setAddress(address + info.mappedProgOffset);
        for (size_t i = 0; i < info.pagesize; i += 32) {
            readBuffer(buffer, 32);
            std::string expected = printBuffer(pgm + address + i, 32);
            std::string actual = printBuffer(buffer, 32);
            if (expected != actual) {
                INFO("Error at address block " << (address + i) << ": " << std::endl);
                INFO("  expected: " << expected << std::endl);
                INFO("  actual:   " << actual << std::endl);
                ERROR("Program verification failure");
            }
            INFO("." << std::flush);
        }
        PROGRESS_LINE;
    }
}


/** \name Commands
 */


/** Simply checks if there is an I2C device at the given address. 
 */
void check() {
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        ERROR("Device not detected at I2C address " << I2C_ADDRESS);
    INFO("Device detected at I2C address " << I2C_ADDRESS << std::endl);
}


/** Actually attempts to connect to the device, restarts it into bootloader and displays the information. 
*/
void info(ChipInfo & info) {
    INFO(std::endl << "Device Info:" << std::endl);
    INFO(info << std::endl);
}


void write(ChipInfo & info, bool actuallyWrite) {
    VERBOSE("Reading hex file in " << hexFile << std::endl);
    hex::Program pgm{hex::Program::parseFile(hexFile.c_str())};
    VERBOSE(pgm << std::endl);
    if (info.progstart != pgm.start())
        ERROR("Incompatible program start detected (expected 0x" << std::hex << info.progstart << ", found 0x" << pgm.start() << ")");
    if (info.memsize <= pgm.end())
       ERROR("Program too large (available: " << std::dec << info.memsize << ", required " << pgm.end() << ")");
    VERBOSE("Padding to page size (" << info.pagesize << ")..." << std::endl);
    pgm.padToPage(info.pagesize, 0xff);
    VERBOSE(pgm << std::endl);
    if (actuallyWrite)
        writeProgram(info, pgm.data(), pgm.start(), pgm.end());
    verifyProgram(info, pgm.data(), pgm.start(), pgm.end());
}

void read(ChipInfo & info) {
    uint16_t addr = 0;
    while (addr < 16384) {
        setAddress(addr);
        uint8_t buffer[128];
        readBuffer(buffer, 32);
        readBuffer(buffer + 32, 32);
        readBuffer(buffer + 64, 32);
        readBuffer(buffer + 96, 32);
        addr += 128;
    }
}

void erase(ChipInfo & info) {
    VERBOSE("Erasing all memory after bootloader...");
    size_t memsize = info.memsize - info.progstart;
    std::unique_ptr<uint8_t> mem{new uint8_t[memsize]};
    writeProgram(info, mem.get(), info.progstart, info.memsize);
    verifyProgram(info, mem.get(), info.progstart, info.memsize);
}

void help() {
    std::cout << "I2C Bootloader for AVR ATTiny series 1 chips" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    i2c-programmer COMMAND [ HEX_FILE] { OPTIONS }" << std::endl << std::endl;
    std::cout << "Available commands:" << std::endl << std::endl;
    std::cout << "    help - prints this help" << std::endl;
    std::cout << "    check - checks if the AVR is present" << std::endl;
    std::cout << "    info - resets the AVR to bootloader and displays the chip info" << std::endl;
    std::cout << "    write FILE - flashes the given hex file to chip's memory" << std::endl;
    std::cout << "    read FILE - reads the entire chip memory into given hex file" << std::endl;
    std::cout << "    verify FILE - verifies that the memory contains the given hex file" << std::endl;
    std::cout << "    erase - erases the application memory of the chip" << std::endl;
    std::cout << "Options" << std::endl << std::endl;
    std::cout << "    --verbose - enables verbose oiutput" << std::endl;
    std::cout << "    --timeout - instead of relying on the IRQ pin, waits 10ms after each command" << std::endl;
    std::cout << "    --trace - enables even more verbose outputs (namely all read & written buffers)" << std::endl;
    std::cout << "    --verify - enables more verification checks while writing the programs" << std::endl;
    std::cout << "    --dry - enables dry run, i.e. no pages will be written" << std::endl;
}



void parseArguments(int argc, char * argv[]) {
    try {
        int i = 1;
        if (i > argc)
            throw STR("No command specified!");
        command = argv[i++];
        if (command == "write" || command == "read" || command == "verify") {
            if (i >= argc)
                throw STR("No hex file specified");
            hexFile = argv[i++];
        }
        while (i < argc) {
            std::string arg{argv[i++]};
            if (arg == "--verbose")
                verbose = true;
            else if (arg == "--timeout")
                timeout = 10;
            else if (str::startsWith(arg, "--timeout="))
                timeout = static_cast<unsigned int>(std::atoi(arg.c_str() + 10));
            else if (arg == "--trace")
                trace = true;
            else if (arg == "--verify")
                verify = true;
            else if (arg == "--dry")
                dry = true;
            else if (str::startsWith(arg, "--addr=")) 
                i2cAddress = static_cast<uint8_t>(std::atoi(arg.c_str() + 7));
            else 
                throw STR("Invalid argument " << arg);
        }
    } catch (std::string const & e) {
        std::cout << "ERROR: Invalid command line usage: " << e << std::endl;
        help();
    }
}



int main(int argc, char * argv[]) {
    try {
        // initialize gpio and i2c
        gpio::initialize();
        i2c::initializeMaster();
        parseArguments(argc, argv);
        if (command == "help")
            help();
        else if (command == "check")
            check();
        else {
            ChipInfo ci{getChipInfo()};
            if (command == "info")
                info(ci);
            else if (command == "write")
                write(ci, true);
            else if (command == "read")
                read(ci);
            else if (command == "verify")
                write(ci, false);
            else if (command == "erase")
                erase(ci);
            else
                ERROR("Invalid command " << command);
        }
        return EXIT_SUCCESS;
    } catch (hex::Error & e) {
        std::cout << std::endl << "ERROR in parsing the HEX file: " << e << std::endl;
    } catch (std::exception const & e) {
        std::cout << std::endl << "ERROR: " << e.what() << std::endl;
    } catch (...) {
        std::cout << std::endl << "UNKNOWN ERROR. This should not happen" << std::endl;
    }
    return EXIT_FAILURE;
}




#ifdef HAHA


/** Information about the connected chip we use to verify what & how we are flashing. 
 */
struct ChipInfo {
    enum class Family {
        TinySeries1, 
    }; // ChipInfo::Family

    std::string name;
    Family family;
    uint32_t signature;
    bool bootloader;
    size_t pageSize;
    size_t progStart; 
    std::unordered_map<std::string, uint8_t> fuses;

    /** Returns chip info of the connected chip, or throws an error if not recognized. 
     */
    ChipInfo(uint8_t const * info) {
        signature = (info[0] << 16) + (info[1] << 8) + info[2];
        bootloader = (info[3] == 0);
        switch (signature) {
            case 0x1e9422: 
                name = "ATTiny1614";
                family = Family::TinySeries1;
                break;
            case 0x1e9421:
                name = "ATTiny1616";
                family = Family::TinySeries1;
                break;
            case 0x1e9420:
                name = "ATTiny1617";
                family = Family::TinySeries1;
                break;
            case 0x1e9521:
                name = "ATTiny3216";
                family = Family::TinySeries1;
                break;
            default:
                throw STR("Unknown chip detected, signature: " << std::hex << (unsigned)info[0] << ":" << (unsigned)info[1] << ":" << (unsigned)info[2]);
        }
        switch (family) {
            case Family::TinySeries1:
                setFusesTinySeries1(info);
                break;
        }
    }
private:
    void setFusesTinySeries1(uint8_t const * info) {
        fuses["FUSE.WDTCFG"] = info[4];
        fuses["FUSE.BODCFG"] = info[5];
        fuses["FUSE.OSCCFG"] = info[6];
        fuses["FUSE.TCD0CFG"] = info[8];
        fuses["FUSE.SYSCFG0"] = info[9];
        fuses["FUSE.SYSCFG1"] = info[10];
        fuses["FUSE.APPEND"] = info[11];
        fuses["FUSE.BOOTEND"] = info[12];
        fuses["FUSE.LOCKBIT"] = info[14];
        fuses["CLKCTRL.MCLKCTRLA"] = info[15];
        fuses["CLKCTRL.MCLKCTRLB"] = info[16];
        fuses["CLKCTRL.MCLKLOCK"] = info[17];
        fuses["CLKCTRL.MCLKSTATUS"] = info[18];
        pageSize = (info[19] << 8) + info[20];
        progStart = info[12] * 256;
    }

    friend std::ostream & operator << (std::ostream & s, Family f) {
        switch (f) {
            case Family::TinySeries1: 
                s << "TinySeries1";
                break;
        }
        return s;
    }

    friend std::ostream & operator << (std::ostream & s, ChipInfo const & info) {
        s << std::setw(20) << "chip" << ": " << info.name << std::endl;
        s << std::setw(20) << "signature" << ": " << std::hex << info.signature << std::endl;
        s << std::setw(20) << "family" << ": " << info.family << std::endl;
        s << std::setw(20) << "mode" << ": " << (info.bootloader ? "bootloader" : "app") << std::endl;
        s << std::setw(20) << "page size" << ": " << std::dec << info.pageSize << std::endl;
        s << std::setw(20) << "program start" << ": " << std::hex << "0x" << info.progStart << std::endl;
        s << std::endl;
        for (auto fuse : info.fuses) {
            s << std::setw(20) << fuse.first << ": " << std::hex << (int)fuse.second << std::endl;
        }
        return s;
    }
}; // ChipInfo

/** Sends the reset command. 
 
    Special method since reset should not wait for the AVR_IRQ busy flag (this would lead in an infinite loop when rebooting to the program)
*/
void resetAvr() {
    uint8_t cmd = CMD_RESET;
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        throw STR("Cannot reset AVR");
}

void sendCommand(uint8_t cmd) {
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd);
    if (timeout != 0)
        cpu::delay_ms(timeout);
    while (gpio::read(PIN_AVR_IRQ) == false) {};
}

void sendCommand(uint8_t cmd, uint8_t arg) {
    uint8_t data[] = { cmd, arg};
    if (! i2c::transmit(I2C_ADDRESS, data, 2, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd << ", arg " << (int)arg);
    if (timeout != 0)
        cpu::delay_ms(timeout);
    while (gpio::read(PIN_AVR_IRQ) == false) {};
}

void readBuffer(uint8_t * buffer) {
    if (! i2c::transmit(I2C_ADDRESS, nullptr, 0, buffer, 32))
        {} // throw STR("Cannot read bootloader's buffer");
}

void writeBuffer(uint8_t const * buffer) {
    uint8_t data[33];
    data[0] = CMD_WRITE_BUFFER;
    for (size_t i = 0; i < 32; ++i)
        data[i+1] = buffer[i];
    if (! i2c::transmit(I2C_ADDRESS, data, 33, nullptr, 0))
        throw STR("Cannot write bootloader's buffer");
}

/** Detects whether the AVR chip is present and displays the chip info if all is ok. 
 
    Since the chip can be in both app and bootloader mode, we can't use the sendCommand rountine that waits for the AVR_IRQ to be released and we have to add explicit delays. 
*/
ChipInfo detectAVR() {
    // check that the avr is present
    gpio::inputPullup(PIN_AVR_IRQ);
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        throw STR("Device not detected at I2C address " << I2C_ADDRESS);
    cpu::delay_ms(10);
    uint8_t data[32];
    // get chip info 
    data[0] = CMD_INFO;
    if (! i2c::transmit(I2C_ADDRESS, data, 1, nullptr, 0))
        throw STR("Cannot send command CMD_INFO, code " << CMD_INFO);    
    cpu::delay_ms(1);
    readBuffer(data);
    return ChipInfo{data + 1}; // skip the first byte of the info which is the status byte in RCKid and garbage for compatibility in the bootloader
}

/** Ensures the AVR chip is available, restarts it into a bootloader mode, checks communication and returns the chip info.
 */
ChipInfo enterBootloader() {
    std::cout << "Detecting device..." << std::endl;
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0)) {
        std::cout << "  not found. Waiting for bootloader..." << std::endl;
        gpio::output(PIN_AVR_IRQ);
        gpio::low(PIN_AVR_IRQ);
        cpu::delay_ms(1000);
    }
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        throw STR("Device not detected at I2C address " << I2C_ADDRESS);
    std::cout << "Forcing reset to bootloader state..." << std::endl;
    cpu::delay_ms(10);
    // to enter the bootloader, pull the AVR_IRQ pin low first
    gpio::output(PIN_AVR_IRQ);
    gpio::low(PIN_AVR_IRQ);
    cpu::delay_ms(10);
    // reset the AVR
    resetAvr();
    cpu::delay_ms(200);
    gpio::inputPullup(PIN_AVR_IRQ);
    //cpu::delay_ms(10);
    // verify that we have proper communications available by writing some stuff and then reading it back
    sendCommand(CMD_SET_INDEX, 0);
    char const * text = "ThisIsATest_DoYouHearMe?12345678\0";
    writeBuffer((uint8_t const *)text);
    sendCommand(CMD_SET_INDEX, 0);
    uint8_t data[33];
    readBuffer(data);
    if (strncmp(text, (char const *)data, 32) != 0) {
        data[32] = 0;
        throw STR("IO Error. Device detected but I2C communication not working:\n   Sent:     " << text << "\n" << "   Received: " << data);
    }
    // get chip info 
    sendCommand(CMD_INFO);
    sendCommand(CMD_SET_INDEX, 0);
    readBuffer(data);
    return ChipInfo{data + 1};
}

void resetToApplication() {
    // reset the AVR
    std::cout << "Resetting AVR..." << std::endl;
    gpio::output(PIN_AVR_IRQ);
    gpio::high(PIN_AVR_IRQ);
    cpu::delay_ms(10);
    resetAvr();
    cpu::delay_ms(200);
    gpio::inputPullup(PIN_AVR_IRQ);
}

void compareBatch(size_t address, size_t size, uint8_t const * expected, uint8_t const * actual) {
    if (std::memcmp(expected, actual, size) == 0)
        return;
    std::cout << "MISMATCH at address: " << std::hex << "0x" << address << ":" << std::endl;
    std::cout << "Expected: ";
    for (size_t i = 0; i < size; ++i) 
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(expected[i]);
    std::cout << std::endl;
    std::cout << "Actual:   ";
    for (size_t i = 0; i < size; ++i) 
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(actual[i]);
    std::cout << std::endl;
    throw STR("Verification mismatch");
}

void writeProgram(ChipInfo const & chip, hex::Program const & p) {
    assert(p.size() % chip.pageSize == 0 && "For simplicity, we expect full pages here");
    if (chip.progStart != p.start())
        throw STR("Incompatible settings detected: Device reports progstart at 0x" << std::hex << chip.progStart << ", program to be flashed from 0x" << p.start());
    if (chip.progStart == 0)
        throw STR("Incompatible settings detected: Device reports progstart at 0. Flashing would override bootloader");
    std::cout << "Writing " << std::dec << p.size() << " bytes in " << p.size() / chip.pageSize << " pages" << std::endl;
    size_t address = p.start() / chip.pageSize;
    for (size_t i = 0, e = p.size(); i != e; ) {
        // fill in the buffer 
        sendCommand(CMD_SET_INDEX, 0);
        std::cout << address << ": " << std::flush;
        for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
            writeBuffer(p.data() + i);
            i += 32;
            std::cout << "." << std::flush;
        }
        // check that we have the proper buffer
        if (verbose) {
            i -= chip.pageSize;
            uint8_t buffer[32];
            for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
                sendCommand(CMD_SET_INDEX, pi);
                readBuffer(buffer);
                compareBatch(address * chip.pageSize + pi, 32, p.data() + i, buffer);
                i += 32;
                std::cout << ":" << std::flush;
            }
        }
        // write the buffer
        sendCommand(CMD_WRITE_PAGE, address); 
        // and finally verify the single page
        if (verbose) {
            i -= chip.pageSize;
            uint8_t buffer[32];
            sendCommand(CMD_READ_PAGE, address);
            for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
                sendCommand(CMD_SET_INDEX, pi);
                readBuffer(buffer);
                compareBatch(address * chip.pageSize + pi, 32, p.data() + i, buffer);
                i += 32;
                std::cout << "?" << std::flush;
            }
        }
        if (verbose)
            std::cout << std::endl;
        else 
            std::cout << "\x1B[2K\r" << std::flush;
        address += 1;
    }
    std::cout << std::endl;
}

void verifyProgram(ChipInfo const & chip, hex::Program const & p) {
    assert(p.size() % chip.pageSize == 0 && "For simplicity, we expect full pages here");
    std::cout << "Verifying " << p.size() << " bytes in " << p.size() / chip.pageSize << " pages" << std::endl;
    size_t address = p.start() / chip.pageSize;
    address = 0;
    uint8_t buffer[32];
    for (size_t i = 0, e = p.size(); i != e; ) {
        sendCommand(CMD_READ_PAGE, address);
        std::cout << address << ": " << std::flush;
        for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
            sendCommand(CMD_SET_INDEX, pi);
            readBuffer(buffer);
            compareBatch(address * chip.pageSize + pi, 32, p.data() + i, buffer);
            i += 32;
            std::cout << "." << std::flush;
        }
        std::cout << "\x1B[2K\r" << std::flush;
        address += 1;
    }
}

int main(int argc, char * argv[]) {
    try {
        // initialize gpio and i2c
        gpio::initialize();
        i2c::initializeMaster();
        // determine what command to run
        if (argc < 2)
            throw STR("Invalid number of arguments");
        std::string cmd{argv[1]}; 
        // write HEX_FILE
        if (cmd == "write") {
            if (argc < 3)
                throw STR("Invalid number of arguments");
            ChipInfo chip = enterBootloader(); 
            std::cout << chip << std::endl;
            std::cout << "Reading program in " << argv[2] << std::endl;
            hex::Program p{hex::Program::parseFile(argv[2])};
            std::cout << "Found " << p << std::endl;
            // flash and verify the program
            p.padToPage(chip.pageSize, 0xff);
            std::cout << "Writing program..." << std::endl;
            writeProgram(chip, p);
            std::cout << "Verifying program..." << std::endl;
            verifyProgram(chip, p);
            resetToApplication();
        // read HEX_FILE start bytes
        } else if (cmd == "read") {
            if (argc < 5)
                throw STR("Invalid number of arguments");
            ChipInfo chip = enterBootloader(); 
            std::cout << chip << std::endl;
            size_t start = std::atol(argv[3]);
            size_t n = std::atol(argv[4]);
            // TODO
            throw STR("Reading chip memory not supported yet");
        } else if (cmd == "check") {
            ChipInfo chip = detectAVR(); 
            std::cout << chip << std::endl;
        } else if (cmd == "reset") {
            resetToApplication();
        } else {
            throw STR("Invalid command " << cmd);
        }
        return EXIT_SUCCESS;
    } catch (hex::Error const & e) {
        std::cout << "ERROR in parsing the HEX file: " << e << std::endl;
    } catch (std::exception const & e) {
        std::cout << "ERROR: " << e.what() << std::endl;
    } catch (std::string const & e) {
        std::cout << "ERROR: " << e << std::endl;
    } catch (...) {
        std::cout << "ERROR: unknown error occured" << std::endl;
    }
    std::cout << "Usage:" << std::endl << std::endl;
    std::cout << "i2c-programmer check" << std::endl;
    std::cout << "    Verified the connection to the chip." << std::endl;
    std::cout << "i2c-programmer write HEX_FILE" << std::endl;
    std::cout << "    Writes the specified HEX file to appropriate regions of the chip." << std::endl;
    return EXIT_FAILURE;
}

#endif