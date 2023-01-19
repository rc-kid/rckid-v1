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
    }; // CHipInfo::Family

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
                throw STR("Unknown chip detected, signature: " << std::hex << (unsigned)data[0] << ":" << (unsigned)data[1] << ":" << (unsigned)data[2]);
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
        s << std::setw(20) << "chip" << ":" << info.name << std::endl;
        s << std::setw(20) << "signature" << ":" << std::hex << info.signature << std::endl;
        s << std::setw(20) << "family" << ":" << info.family << std::endl;
        s << std::setw(20) << "page size" << ":" << info.pageSize << std::endl;
        s << std::setw(20) << "program start" << ":" << std::hex << "0x" << info.progStart << std::endl;
        s << std::endl;
        for (auto fuse : info.fuses) {
            s << std::setw(20) << fuse.first << ":" << std::hex << fuse.second << std::endl;
        }
        return s;
    }
}; // ChipInfo

void sendCommand(uint8_t cmd) {
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd);
    cpu::delay_ms(10);
}

void sendCommand(uint8_t cmd, uint8_t arg) {
    uint8_t data[] = { cmd, arg};
    if (! i2c::transmit(I2C_ADDRESS, data, 2, nullptr, 0))
        throw STR("Cannot send command " << (int)cmd << ", arg " << (int)arg);
    cpu::delay_ms(10);
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
    // gpio::output()
    //
    // reset the AVR
    sendCommand(CMD_RESET);
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
    sendCommand(CMD_INFO);
    sendCommand(CMD_CLEAR_INDEX);
    readBuffer(data);
    return ChipInfo{data};
    /*
    sendCommand(CMD_READ_PAGE, 0);
    sendCommand(CMD_CLEAR_INDEX);
    readBuffer(data);
    for (int i = 0; i < 32; ++i)
        std::cout << std::hex << (unsigned)data[i];
    std::cout << std::endl;
    */
}

void writeProgram(ChipInfo const & chip, hex::Program const & p) {
    assert(p.size() % chip.pageSize == 0 && "For simplicity, we expect full pages here");
    for (size_t addr = p.start(), e = p.end(); addr < e; ) {
        // fill in the buffer 
        sendCommand(CMD_CLEAR_INDEX);
        for (int i = 0; i < chip.pageSize / 32; ++i)
            writeBuffer()

    }

}

void verifyProgram(ChipInfo const & chip, hex::Program const & p) {

}

int main(int argc, char * argv[]) {
    try {
        if (argc != 2)
            throw STR("HEX file expected");
        std::cout << "Reading program in " << argv[1] << std::endl;
        hex::Program p{hex::Program::parseFile(argv[1])};
        std::cout << "Found " << p << std::endl;

        // initialize gpio and i2c
        gpio::initialize();
        i2c::initializeMaster();
        // enter the bootloader on AVR
        std::cout << "Detecting chip" << std::endl;
        ChipInfo chip = enterBootloader();
        std::cout << chip << std::endl;
        // flash and verify the program
        std::cout << "Writing program..." << std::endl;
        writeProgram(chip, p);
        std::cout << "Verifying program..." << std::endl;
        verifyProgram(chip, p);
        std::cout << "OK." << std::endl;
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
    return EXIT_FAILURE;
}