#include <iostream>
#include <iomanip>
#include <unordered_map>

#include "platform/platform.h"
#include "utils/intelhex.h"


#include "config.h"


/** Information about the connected chip we use to verify what & how we are flashing. 
 */
struct ChipInfo {
    enum class Family {
        TinySeries1, 
    }; // ChipInfo::Family

    std::string name;
    Family family;
    uint32_t signature;
    size_t pageSize;
    size_t progStart; 
    std::unordered_map<std::string, uint8_t> fuses;

    /** Returns chip info of the connected chip, or throws an error if not recognized. 
     */
    ChipInfo(uint8_t const * info) {
        signature = (info[0] << 16) + (info[1] << 8) + info[2];
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
        fuses["FUSE.WDTCFG"] = info[3];
        fuses["FUSE.BODCFG"] = info[4];
        fuses["FUSE.OSCCFG"] = info[5];
        fuses["FUSE.TCD0CFG"] = info[6];
        fuses["FUSE.SYSCFG0"] = info[7];
        fuses["FUSE.SYSCFG1"] = info[8];
        fuses["FUSE.APPEND"] = info[9];
        fuses["FUSE.BOOTEND"] = info[10];
        fuses["CLKCTRL.MCLKCTRLA"] = info[11];
        fuses["CLKCTRL.MCLKCTRLB"] = info[12];
        fuses["CLKCTRL.MCLKLOCK"] = info[13];
        fuses["CLKCTRL.MCLKSTATUS"] = info[14];
        pageSize = (info[15] << 8) + info[16];
        progStart = info[10] * pageSize;
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
        s << std::setw(20) << "page size" << ": " << std::dec << info.pageSize << std::endl;
        s << std::setw(20) << "program start" << ": " << std::hex << "0x" << info.progStart << std::endl;
        s << std::endl;
        for (auto fuse : info.fuses) {
            s << std::setw(20) << fuse.first << ": " << std::hex << (int)fuse.second << std::endl;
        }
        return s;
    }
}; // ChipInfo

void sendCommand(uint8_t cmd) {
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd);
    while (gpio::read(PIN_AVR_IRQ) == true) {};
    //cpu::delay_ms(10);
}

void sendCommand(uint8_t cmd, uint8_t arg) {
    uint8_t data[] = { cmd, arg};
    if (! i2c::transmit(I2C_ADDRESS, data, 2, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd << ", arg " << (int)arg);
    while (gpio::read(PIN_AVR_IRQ) == true) {};
    //cpu::delay_ms(10);
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

ChipInfo enterBootloader() {
    // check that the avr is present
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        throw STR("Device not detected at I2C address " << I2C_ADDRESS);
    cpu::delay_ms(10);
    // to enter the bootloader, pull the AVR_IRQ pin low first
    gpio::output(PIN_AVR_IRQ);
    gpio::low(PIN_AVR_IRQ);
    cpu::delay_ms(10);
    // reset the AVR
    sendCommand(CMD_RESET);
    cpu::delay_ms(10);
    gpio::inputPullup(PIN_AVR_IRQ);
    cpu::delay_ms(10);
    // verify that we have proper communications available by writing some stuff and then reading it back
    sendCommand(CMD_CLEAR_INDEX);
    char const * text = "ThisIsATest_DoYouHearMe?12345678\0";
    writeBuffer((uint8_t const *)text);
    sendCommand(CMD_CLEAR_INDEX);
    uint8_t data[33];
    readBuffer(data);
    if (strncmp(text, (char const *)data, 32) != 0) {
        data[32] = 0;
        throw STR("IO Error. Device detected but I2C communication not working:\n   Sent:     " << text << "\n" << "   Received: " << data);
    }
    // get chip info 
    sendCommand(CMD_INFO);
    sendCommand(CMD_CLEAR_INDEX);
    readBuffer(data);
    return ChipInfo{data};
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
        sendCommand(CMD_CLEAR_INDEX);
        std::cout << address << ": " << std::flush;
        for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
            //writeBuffer(p.data() + i);
            i += 32;
            std::cout << "." << std::flush;
        }
        // write the buffer
        //sendCommand(CMD_WRITE_PAGE, address); 
        std::cout << "\x1B[2K\r" << std::flush;
        address += 1;
    }
    std::cout << std::endl;
}

void verifyProgram(ChipInfo const & chip, hex::Program const & p) {
    assert(p.size() % chip.pageSize == 0 && "For simplicity, we expect full pages here");
    std::cout << "Verifying " << p.size() << " bytes in " << p.size() / chip.pageSize << " pages" << std::endl;
    size_t address = p.start() / chip.pageSize;
    uint8_t buffer[32];
    for (size_t i = 0, e = p.size(); i != e; ) {
        sendCommand(CMD_READ_PAGE, address);
        sendCommand(CMD_CLEAR_INDEX);
        std::cout << address << ": " << std::flush;
        for (int pi = 0, pe = chip.pageSize; pi < pe; pi += 32) {
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
        // enter the bootloader on AVR
        std::cout << "Detecting chip" << std::endl;
        ChipInfo chip = enterBootloader();
        std::cout << chip << std::endl;
        if (argc > 1) {
            if (argc < 3)
                throw STR("Invalid number of arguments");
            std::string cmd{argv[1]}; 
            // write HEX_FILE
            if (cmd == "write") {
                std::cout << "Reading program in " << argv[2] << std::endl;
                hex::Program p{hex::Program::parseFile(argv[2])};
                std::cout << "Found " << p << std::endl;
                // flash and verify the program
                p.padToPage(chip.pageSize, 0xff);
                std::cout << "Writing program..." << std::endl;
                writeProgram(chip, p);
                std::cout << "Verifying program..." << std::endl;
                verifyProgram(chip, p);
                std::cout << "OK." << std::endl;
            // read HEX_FILE start bytes
            } else if (cmd == "read") {
                if (argc < 5)
                    throw STR("Invalid number of arguments");
                size_t start = std::atol(argv[3]);
                size_t n = std::atol(argv[4]);
                // TODO
                throw STR("Reading chip memory not supported yet");
            } else {
                throw STR("Invalid command " << cmd);
            }
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
    std::cout << "i2c-programmer" << std::endl;
    std::cout << "    Verified the connection to the chip." << std::endl;
    std::cout << "i2c-programmer write HEX_FILE" << std::endl;
    std::cout << "    Writes the specified HEX file to appropriate regions of the chip." << std::endl;
    return EXIT_FAILURE;
}