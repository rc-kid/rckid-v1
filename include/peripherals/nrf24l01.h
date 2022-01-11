#pragma once

#include "platform/gpio.h"
#include "platform/spi.h"

/** A simplified driver for the nRF24l01+ radio chip. 

     GND VCC
    RXTX CS 
     SCK MOSI
    MISO IRQ
 */
template<gpio::Pin CS, gpio::Pin RXTX, gpio::Pin IRQ>
class NRF24L01 {
public:

    enum class Speed : uint8_t {
        kb250 = 0b00100000,
        mb1 = 0b00000000,
        mb2 = 0b00001000,
    }; 

    enum class Power : uint8_t {
        dbm0 = 0b11,
        dbm6 = 0b10,
        dbm12 = 0b01,
        dbm18 = 0b00,
    }; 

	/** Initializes the control pins of the module. 
	 */
    NRF24L01() {
        gpio::output(CS);
        gpio::output(RXTX);
        gpio::input(IRQ);
        spi::setCs(CS, false);
        gpio::low(RXTX);
	}

    void initialize(const char * rxAddr, uint8_t channel, Speed speed = Speed::kb250, Power power = Power::dbm0) {
        // enable interrupt when received, crc 2 bytes, power down
        config_ = MASK_RX_DR | EN_CRC | CRCO;
        w_register(CONFIG, config_);
		w_register(EN_RXADDR, 3); // enable receiver pipes 0 and 1
		w_register(SETUP_AW, 3); // address width set to 5 bytes
        setChannel(channel);
        setRxAddress(rxAddr);
        // enable auto acknowledge
	    w_register(EN_AA, 3); // enable auto ack for data pipe 1
		w_register(FEATURE, EN_DPL | EN_ACK_PAY); // enable dynamic payload length and ack payload features
		w_register(DYNPD, 3); // enable dynamic payloads for data pipe 0 	
		w_register(SETUP_RETR, 0b00111111); // enable auto retransmit after 1000us, 15x max
        // chip reset
        clearStatusFlags();
        flushTX();
        flushRX();
    }

	
	/** Initializes the chip with given RX and TX addresses, payload length and channel (defaults to 2). 
	
	Disables the auto ack feature, sets power to maximum and speed to 2mbps. Enables pipes 0 and 1 (0 for potential ack packages, 1 for general receiver). 
	*/
	void initialize(const char * rxAddr, const char * txAddr, uint8_t payloadLength, uint8_t channel = 2) {
        config_ = 0b00111100; // RX_DR interrupt enabled, CRC 2 bytes, power down
		w_register(CONFIG, config_);
	    //w_register(EN_AA, 0); // disable auto ack
		w_register(EN_RXADDR, 3); // enable receiver pipes 0 and 1
		w_register(SETUP_AW, 3); // address width set to 5 bytes
		//w_register(SETUP_RETR, 0); // disable auto retransmit
		setChannel(channel);
		//w_register(RF_SETUP, 0b00001110); // 2mbps, maximum power
        w_register(RF_SETUP, 0b00100110); // 250kbps, maximum power
        // disable auto acknowledge, dynamic payload, retransmit, etc. 
        w_register(EN_AA, 0);
        w_register(FEATURE, 0);
        w_register(DYNPD, 0);
        w_register(SETUP_RETR, 0);
		clearStatusFlags();
		setRxAddress(rxAddr);
		setTxAddress(txAddr);
		setPayloadLength(payloadLength);
		//w_register(FEATURE, 0); // disable dynamic payload and ack payload features
		//w_register(DYNPD, 0); // disable dynamic payload on all pipes
		flushTX();
		flushRX();
	} 
	
	/** Returns the contents of the config register on the device. This is for debugging purposes only. 
	 */
	uint8_t config() {
		return r_register(CONFIG);
	}
	
	/** Returns the contents of the status register. For debugging purposes only. 
	 */
	uint8_t status() {
		spi::setCs(CS, true);
		uint8_t result = spi::transfer(NOP);
		spi::setCs(CS, false);
		return result;
	}
	
	/** Returns the fifo status register. For debugging purposes only. 
	 */
	uint8_t fifoStatus() {
		return r_register(FIFO_STATUS);
	}
	
	/** Returns the observe TX register. For debugging purposes only. 
	 */
	uint8_t observeTX() {
		return r_register(OBSERVE_TX);
	}
	
	/** Enables the auto acknowledgement feature. 
	
	Also enables the dynamic payload length and ack payload features so that ack packages can return non critical data. 
	 */
    /*
	void enableAutoAck() {
	    w_register(EN_AA, 3); // enable auto ack for data pipe 1
		w_register(FEATURE, EN_DPL | EN_ACK_PAY); // enable dynamic payload length and ack payload features
		w_register(DYNPD, 3); // enable dynamic payloads for data pipe 0 	
		w_register(SETUP_RETR, 0b00111111); // enable auto retransmit after 1000us, 15x max
	}
    */
	
	/** Disables the automatic acknowledgement features. 
	 */
    /*
	void disableAutoAck() {
	    w_register(EN_AA, 0); // disable auto ack
		w_register(FEATURE, 0); // disable dynamic payload and ack payload features
		w_register(DYNPD, 0); // disable dynamic payload on all pipes
		w_register(SETUP_RETR, 0); // disable auto retransmit
	}
    */

	/** Flushes the trasmitter's buffer. 
	 */
	void flushTX() {
		spi::setCs(CS, true);
		spi::transfer(FLUSH_TX);
		spi::setCs(CS, false);
	}
	
	/** Flushes the receiver's buffer. 
	 */
	void flushRX() {
		spi::setCs(CS, true);
		spi::transfer(FLUSH_RX);
		spi::setCs(CS, false);
	}

	/** Sets the channel. 
	 */
	void setChannel(uint8_t value) {
		w_register(RF_CH, value);
	}
	
	/** Returns the current channel. 
	 */
	uint8_t channel() {
		return r_register(RF_CH & 0x7f);
	}
	
	/** Sets the payload length. 
	 */
	void setPayloadLength(uint8_t payloadLength) {
		payloadLength_ = payloadLength;
		w_register(RX_PW_P0, payloadLength);
		w_register(RX_PW_P1, payloadLength);
	}
	
	/** Returns the payload length. 
	 */
	uint8_t payloadLength() {
		return payloadLength_;
	}
	
	/** Sets the receiver's address. The address must be 5 bytes long. 
	 */
	void setRxAddress(const char * addr) {
		spi::setCs(CS, true);
		spi::transfer(W_REGISTER + RX_ADDR_P1);
		send(reinterpret_cast<uint8_t const*>(addr), 5);
        spi::setCs(CS, false);
	}
	
	/** Reads the receiver's address into the given buffer. The buffer must be at least 5 bytes long. 
	 */
	void rxAddress(char * addr) {
        spi::setCs(CS, true);
		spi::transfer(R_REGISTER + RX_ADDR_P1);
		receive(reinterpret_cast<uint8_t*>(addr), 5);
        spi::setCs(CS, false);
	}
	
	/** Sets the transmitter's address (package target). The address must be 5 bytes long. 
	 */
	void setTxAddress(const char * addr) {
        spi::setCs(CS, true);
		spi::transfer(W_REGISTER + RX_ADDR_P0);
		send(reinterpret_cast<uint8_t const*>(addr), 5);
        spi::setCs(CS, false);
        spi::setCs(CS, true);
		spi::transfer(W_REGISTER + TX_ADDR);
		send(reinterpret_cast<uint8_t const*>(addr), 5);
        spi::setCs(CS, false);
	}
	
	/** Reads the transmitter's address into the given buffer. The buffer must be at least 5 bytes long. 
	 */
	void txAddress(char * addr) {
        spi::setCs(CS, true);
		spi::transfer(R_REGISTER + TX_ADDR);
		receive(reinterpret_cast<uint8_t*>(addr), 5);
        spi::setCs(CS, false);
	}

	/** Enters power down mode. 
	 */	
	void powerDown() {
        gpio::low(RXTX);
		config_ &= ~PWR_UP;
		w_register(CONFIG, config_);
	}

	/** Enters the standby mode. 
	 */
	void standby() {
		gpio::low(RXTX);
        config_ |= PWR_UP | PRIM_RX;
		w_register(CONFIG, config_);
	}
	
	/** Enables the receiver. Use standby() to disable the receiver. 
	 */
	void enableReceiver() {
        config_ |= PWR_UP | PRIM_RX;
		w_register(CONFIG, config_);
		gpio::high(RXTX);
	}
	
	/** Clears the status flags for interrupt events. 
	 */
	void clearStatusFlags() {
		w_register(STATUS, 0b01111110); // clear interrupt flags
	}
	
	/** Loads the received package into the given buffer. The buffer must be at least as long as is the payload length.
	
	This method is supposed to be called only when RX queue is not empty.  
	 */
	void receive(uint8_t * buffer) {
	    spi::setCs(CS, true);
		spi::transfer(R_RX_PAYLOAD);
		receive(buffer, payloadLength_);
		spi::setCs(CS, false);
	}

    void receiveAndCheck(uint8_t * buffer) {
        receive(buffer);
        if (fifoStatus() || RX_EMPTY)
            w_register(STATUS, STATUS_RX_DR);
    }
	
	/** Transmits the given payload. 
	
	Does not wait for the transmission to succeed. 
	 */
	void transmit(uint8_t const * payload) {
		gpio::low(RXTX); // disable power to allow switching, or sending a pulse
		// set mode to standby, enable transmitter
		config_ &= ~PRIM_RX;
		config_ |= PWR_UP;
		w_register(CONFIG, config_);
        // transfer the payload
	    spi::setCs(CS, true);
		spi::transfer(W_TX_PAYLOAD);
		send(payload, payloadLength_);
		spi::setCs(CS, false);
		// send RXTX pulse to initiate the transmission, datasheet requires 10us delay
		gpio::high(RXTX);
		cpu::delay_us(20);
		gpio::low(RXTX);
	}
	
	/** Sends given payload as next ack's payload. 
	
	 */
	void transmitAckPayload(uint8_t const * payload) {
		spi::setCs(CS, true);
		spi::transfer(W_ACK_PAYLOAD | 1);
		send(payload, payloadLength_);
        spi::setCs(CS, false);
	}
	
private:

    void send(uint8_t const * buffer, uint8_t length) {
		while (length-- > 0) 
		    spi::transfer(*(buffer++));
	}
	
	void receive(uint8_t * buffer, uint8_t length) {
		while (length-- > 0) 
		    *(buffer++) = spi::transfer(0);
	}

    uint8_t r_register(uint8_t reg) {
		spi::setCs(CS, true);
		spi::transfer(R_REGISTER + reg);
		uint8_t result = spi::transfer(0);
		spi::setCs(CS, false);
		//serial << "READ " << reg << " : " << result << endl;
		return result;
	}
	
	void w_register(uint8_t reg, uint8_t value) {
        spi::setCs(CS, true);
		spi::transfer(W_REGISTER + reg);
		spi::transfer(value);
		//serial << "WRITE " << reg << " : " << value << endl;
        spi::setCs(CS, false);
	}
	
    // commands
	
    static constexpr uint8_t R_REGISTER = 0;
	static constexpr uint8_t W_REGISTER = 0b00100000;
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
	static constexpr uint8_t MASK_RX_DR = 1 << 6;
	static constexpr uint8_t MASK_TX_DR = 1 << 5;
	static constexpr uint8_t MASK_RT_DR = 1 << 4;
	static constexpr uint8_t EN_CRC = 1 << 3;
	static constexpr uint8_t CRCO = 1 << 2;
	static constexpr uint8_t PWR_UP = 1 << 1;
	static constexpr uint8_t PRIM_RX = 1 << 0;
	
    // status register values
	static constexpr uint8_t STATUS_RX_DR = 1 << 6;
	static constexpr uint8_t STATUS_TX_DS = 1 << 5;
	static constexpr uint8_t STATUS_MAX_RT = 1 << 4;
	static constexpr uint8_t STATUS_TX_FULL = 1 << 0;
	
	static constexpr uint8_t EN_DPL = 1 << 2;
	static constexpr uint8_t EN_ACK_PAY = 1 << 1;

    // fifo status register values
    static constexpr uint8_t TX_REUSE = 1 << 6;
    static constexpr uint8_t TX_FULL = 1 << 5;
    static constexpr uint8_t TX_EMPTY = 1 << 4;
    static constexpr uint8_t RX_FULL = 1 << 1;
    static constexpr uint8_t RX_EMPTY = 1 << 0;

    
	
	
	uint8_t payloadLength_;
	
	uint8_t config_;
	
};




