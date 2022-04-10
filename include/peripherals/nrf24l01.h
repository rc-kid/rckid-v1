#pragma once
#include "platform/platform.h"

/** Simplified driver for the NRF24L01 chip and its clones. 
 
     GND VCC
    RXTX CS 
     SCK MOSI
    MISO IRQ

    The big problem with those chips is that there is a lot of fakes and many of the fakes are subtly incompatible with the original, especially in the enhanced shock burst mode, which would be quite handy. To mitigate the simplified driver does not use the features known to be broken across the chips and should therefore work fine with both real and counterfeit ones.

    No dynamic payload. Therefore we can't send data with ACKs.


  - use only enhanced shock burst
  - do not use dynamic payload 

  - these limitations should allow for interoperability with both real and fake nrf modules, test that first though! 
 
 */
class NRF24L01 {
public:
    /** Chip select/device ID for the driver.
     */
    const spi::Device CS;
    /** The RX/TX enable pin to turn the radio on or off. 
     */
    const gpio::Pin RXTX;

    NRF24L01(spi::Device cs, gpio::Pin rxtx):
        CS{cs},
        RXTX{rxtx} {
    }

    /** Initializes the driver and returns true if successful, false is not. 
     */
    bool initialize(char const * rxAddr, char const * txAddr) {
        gpio::output(RXTX);
        gpio::low(RXTX);
        gpio::output(CS);
        gpio::high(CS);
        setTxAddress(rxAddr);
        setRxAddress(rxAddr);
        // get the tx address and verify that it has been set properly
        char txCheck[6];
        txCheck[5] = 0;
        txAddress(txCheck);
        std::cout << txCheck << std::endl;
        if (strncmp(txCheck, txAddr, 5) != 0)
            return false;
        // otherwise continue with the initialization
        return true;
    }

    /** Enters the simple mode on given channel. 
     
        In the simple mode, receiver starts at given channel and receiving address. When a message is received, the IRQ will be set, which can be monitored by the app, otherwise polling can be used. 

        Transmission can be initiated by calling the transmit method, which disables the receiver, enables transciever, sends the 
     
     */
    void simpleMode(uint8_t channel, uint8_t msgSize) {

    }


    void transmit(uint8_t const * buffer) {

    }


    uint8_t receive(uint8_t * buffer) {
        
    }

    /** Enters the enhanced mode. 
     */
    void enhancedMode(uint8_t channel) {

    }

	/** Channel selection. 
	 */

	uint8_t channel() {
		return readRegister(RF_CH & 0x7f);
	}

	void setChannel(uint8_t value) {
		writeRegister(RF_CH, value);
	}
	

    /** Transmittingh Address. 
     
        This is the address to which the radio will transmit. 
     */

	void txAddress(char * addr) {
        begin();
		spi::transfer(READREGISTER + TX_ADDR);
		spi::receive(reinterpret_cast<uint8_t*>(addr), 5);
        end();
	}

	void setTxAddress(const char * addr) {
        begin();
        spi::transfer(WRITEREGISTER | RX_ADDR_P0);
        spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
        end();
        begin();
		spi::transfer(WRITEREGISTER | TX_ADDR);
		spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
        end();
	}

    /** Receiving address.
     */
	void rxAddress(char * addr) {
        begin();
		spi::transfer(READREGISTER | RX_ADDR_P1);
		spi::receive(reinterpret_cast<uint8_t*>(addr), 5);
        end();
	}

	void setRxAddress(const char * addr) {
        begin();
		spi::transfer(WRITEREGISTER | RX_ADDR_P1);
		spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
        end();
	}




private:

    void begin() {
        spi::begin(CS);
        cpu::delay_us(2);
    }

    void end() {
        spi::end(CS);
        cpu::delay_us(2);
    }


    uint8_t readRegister(uint8_t reg) {
		begin();
		spi::transfer(READREGISTER | reg);
		uint8_t result = spi::transfer(0);
		end();
		return result;
	}
	
	void writeRegister(uint8_t reg, uint8_t value) {
        //printf("Writing register %u, value %u\n", reg, value);
        begin();
		spi::transfer(WRITEREGISTER | reg);
		spi::transfer(value);
        end();
	}

    /** Local cache of the config register.
     */
    uint8_t config_;

    // commands
	
    static constexpr uint8_t READREGISTER = 0;
	static constexpr uint8_t WRITEREGISTER = 0b00100000;
	static constexpr uint8_t R_RX_PAYLOAD = 0b01100001;
	static constexpr uint8_t W_TX_PAYLOAD = 0b10100000;
	static constexpr uint8_t FLUSH_TX = 0b11100001;
	static constexpr uint8_t FLUSH_RX = 0b11100010;
	static constexpr uint8_t REUSE_TX_PL = 0b11100011;
	static constexpr uint8_t R_RX_PL_WID = 0b01100000;
	static constexpr uint8_t W_ACK_PAYLOAD = 0b10101000;
	static constexpr uint8_t W_TX_PAYLOAD_NO_ACK = 0b10110000;
	static constexpr uint8_t NOP = 0b11111111;
	
	// registers
	
	static constexpr uint8_t CONFIG = 0;
	static constexpr uint8_t EN_AA = 1;
	static constexpr uint8_t EN_RXADDR = 2;
	static constexpr uint8_t SETUP_AW = 3;
	static constexpr uint8_t SETUP_RETR = 4;
	static constexpr uint8_t RF_CH = 5;
	static constexpr uint8_t RF_SETUP = 6;
	static constexpr uint8_t STATUS = 7;
	static constexpr uint8_t OBSERVE_TX = 8;
	static constexpr uint8_t RPD = 9;
	static constexpr uint8_t RX_ADDR_P0 = 0x0a; // 5 bytes
	static constexpr uint8_t RX_ADDR_P1 = 0x0b; // 5 bytes
	static constexpr uint8_t RX_ADDR_P2 = 0x0c;
	static constexpr uint8_t RX_ADDR_P3 = 0x0d;
	static constexpr uint8_t RX_ADDR_P4 = 0x0e;
	static constexpr uint8_t RX_ADDR_P5 = 0x0f;
	static constexpr uint8_t TX_ADDR = 0x10; // 5 bytes
	static constexpr uint8_t RX_PW_P0 = 0x11;
	static constexpr uint8_t RX_PW_P1 = 0x12;
	static constexpr uint8_t RX_PW_P2 = 0x13;
	static constexpr uint8_t RX_PW_P3 = 0x14;
	static constexpr uint8_t RX_PW_P4 = 0x15;
	static constexpr uint8_t RX_PW_P5 = 0x16;
	static constexpr uint8_t FIFO_STATUS = 0x17;
	static constexpr uint8_t DYNPD = 0x1c;
	static constexpr uint8_t FEATURE = 0x1d;
	
    // config register values
	static constexpr uint8_t CONFIG_MASK_RX_DR = 1 << 6;
	static constexpr uint8_t CONFIG_MASK_TX_DS = 1 << 5;
	static constexpr uint8_t CONFIG_MASK_MAX_RT = 1 << 4;
	static constexpr uint8_t CONFIG_EN_CRC = 1 << 3;
	static constexpr uint8_t CONFIG_CRCO = 1 << 2;
	static constexpr uint8_t CONFIG_PWR_UP = 1 << 1;
	static constexpr uint8_t CONFIG_PRIM_RX = 1 << 0;
	
    // status register values
	static constexpr uint8_t STATUS_RX_DR = 1 << 6;
	static constexpr uint8_t STATUS_TX_DS = 1 << 5;
	static constexpr uint8_t STATUS_MAX_RT = 1 << 4;
	static constexpr uint8_t STATUS_TX_FULL = 1 << 0;
	
    // features
	static constexpr uint8_t EN_DPL = 1 << 2;
	static constexpr uint8_t EN_ACK_PAY = 1 << 1;
    static constexpr uint8_t EN_DYN_ACK = 1 << 0;

    // fifo status register values
    static constexpr uint8_t TX_REUSE = 1 << 6;
    static constexpr uint8_t TX_FULL = 1 << 5;
    static constexpr uint8_t TX_EMPTY = 1 << 4;
    static constexpr uint8_t RX_FULL = 1 << 1;
    static constexpr uint8_t RX_EMPTY = 1 << 0;

};