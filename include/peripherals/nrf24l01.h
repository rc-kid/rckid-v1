#pragma once

#include "platform/gpio.h"
#include "platform/spi.h"

/** A simplified driver for the nRF24l01+ radio chip. 

     GND VCC
    RXTX CS 
     SCK MOSI
    MISO IRQ
 */
template<gpio::Pin CS, gpio::Pin RXTX>
class NRF24L01 {
public:

    struct Config {
        bool disableDataReadyIRQ() const { return raw & CONFIG_MASK_RX_DR; }
        bool disableDataSentIRQ() const { return raw & CONFIG_MASK_TX_DS; }
        bool disableMaxRetransmitsIRQ() const { return raw & CONFIG_MASK_MAX_RT; }
        uint8_t crcSize() const {
            if (raw | CONFIG_EN_CRC)
                return (raw | CONFIG_CRCO) ? 2 : 1;
            else
                return 0;
        }
        bool powerUp() const { return raw & CONFIG_PWR_UP; }
        bool transmitReady() const { return raw & CONFIG_PRIM_RX == 0; }
        bool receiveReady() const { return raw & CONFIG_PRIM_RX; }
        uint8_t const raw;
    }; 

    struct Status {
        bool dataReady() const { return raw & STATUS_RX_DR; }
        bool dataSent() const { return raw & STATUS_TX_DS; }
        bool maxRetransmits() const { return raw & STATUS_MAX_RT; }
        bool txFull() const { return raw & STATUS_TX_FULL; }
        bool rxEmpty() const { return dataReadyPipe() == 7; }
        /** Returns the data pipe from which the data is ready. 
         
            0..5 are valid pipe values, 7 is returned when rx pipe is empty. 
         */
        uint8_t dataReadyPipe() const { return (raw > 1) & 7; }

        uint8_t const raw;
    }; // NRF24L01::Status

    struct TxStats {
        uint8_t lostPackets() const { return raw >> 4; }
        uint8_t lastRetransmissions() const { return raw & 0xf; }
        uint8_t const raw;
    }; // NRF24L01::TxStats

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
        spi::setCs(CS, false);
        gpio::low(RXTX);
	}

    void initialize(const char * rxAddr, const char * txAddr,  uint8_t channel = 76, Speed speed = Speed::kb250, Power power = Power::dbm0, uint8_t payloadSize = 32) {
        // set the desired speed and output power
        w_register(RF_SETUP, static_cast<uint8_t>(power) | static_cast<uint8_t>(speed));
        // set the channel and tx and rx addresses
        setChannel(channel);
        setTxAddress(txAddr);
        setRxAddress(rxAddr);
        // enable auto ack and enhanced shock-burst
        enableAutoAck();
        // enable read pipes 0 and 1
        w_register(EN_RXADDR, 3);
        // set the payload size
        setPayloadSize(payloadSize);
        // reset the chip's status
        w_register(STATUS, STATUS_MAX_RT | STATUS_RX_DR | STATUS_TX_DS);
        // flush the tx and rx fifos
        flushTx();
        flushRx();
        // set configuration to powered down, ready to receive, crc to 2 bytes
        // all events will appear on the interrupt pin
        config_ = CONFIG_CRCO | CONFIG_EN_CRC;
        w_register(CONFIG, config_);
    }

    /** Sets static payload size for all receiving pipes. 
     */
    void setPayloadSize(uint8_t value) {
        for (uint8_t i = 0; i < 6; ++i)
            w_register(RX_PW_P0 + i, value);        
    }

	/** Enables or disables the auto acknowledgement feature. 
	
    	Also enables the dynamic payload length and ack payload features so that ack packages can return non critical data. 
	 */
    void enableAutoAck(bool enable = true) {
        if (enable) {
            // enable automatic acknowledge on all input pipes
            w_register(EN_AA, 0x3f);
            // enables the enhanced shock-burst features, dynamic payload size and transmit of packages without ACKs
            w_register(FEATURE, EN_DPL | EN_ACK_PAY | EN_DYN_ACK);
            // disable dynamic payloads on all input pipes except pipe 0 used for ack payloads
            w_register(DYNPD, 1);
            // auto retransmit count to 15, auto retransmit delay to 1500us, which is the minimum for the worst case of 32bytes long payload and 250kbps speed
            w_register(SETUP_RETR, 0x5f);
        } else {
            w_register(EN_AA, 0); // disable auto ack
            w_register(FEATURE, 0); // disable dynamic payload and ack payload features
            w_register(DYNPD, 0); // disable dynamic payload on all pipes
            w_register(SETUP_RETR, 0); // disable auto retransmit
        }
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


	/** Clears the status flags for interrupt events. 
	 */
	void clearIRQ() {
		w_register(STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT); // clear interrupt flags
	}

	/** Flushes the trasmitter's buffer. 
	 */
	void flushTx() {
		spi::setCs(CS, true);
		spi::transfer(FLUSH_TX);
		spi::setCs(CS, false);
	}
	
	/** Flushes the receiver's buffer. 
	 */
	void flushRx() {
		spi::setCs(CS, true);
		spi::transfer(FLUSH_RX);
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

	/** Sets the transmitter's address (package target). The address must be 5 bytes long. 
     
        The address is also set 
	 */
	void setTxAddress(const char * addr) {
        spi::setCs(CS, true);
        spi::transfer(W_REGISTER + RX_ADDR_P0);
        spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
        spi::setCs(CS, false);
        spi::setCs(CS, true);
		spi::transfer(W_REGISTER + TX_ADDR);
		spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
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

	/** Sets the receiver's address. The address must be 5 bytes long. 

        For now only pipe 1 is supported as pipe 1 can have a full address specified, the other pipes differ only it the last byte. Also, as of now, I can't imagine a situation where I would like to receive on two addresses. 
	 */
	void setRxAddress(const char * addr) {
		spi::setCs(CS, true);
		spi::transfer(W_REGISTER + RX_ADDR_P1);
		spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
        spi::setCs(CS, false);
	}

    /** Enters the power down mode. 
     */
    void powerDown() {
        gpio::low(RXTX);
		config_ &= ~CONFIG_PWR_UP;
		w_register(CONFIG, config_);
    }

	/** Enters the standby mode
	 */
    void standby() {
		gpio::low(RXTX);
        config_ |= CONFIG_PWR_UP | CONFIG_PRIM_RX;
 		w_register(CONFIG, config_);
        cpu::delay_ms(3); // startup time by the datasheet is 1.5ms
    }

    /** Starts the receiver. 
     
        Also enables the received message IRQ. To stop the receiver, call either standby() or powerDown(). 
     */
    void startReceiver() {
        config_ |= (CONFIG_PWR_UP | CONFIG_PRIM_RX);
		w_register(CONFIG, config_);
		gpio::high(RXTX);
    }

    /** Receives a single message from the TX FIFO.
     
        The caller must make sure that the message is of the specified size, otherwise the protocol will stop working. 
     */
    void receive(uint8_t * buffer, uint8_t size) {
	    spi::setCs(CS, true);
		spi::transfer(R_RX_PAYLOAD);
		receive(buffer, size);
		spi::setCs(CS, false);
    }

    /** Downloads a variable length message returning its length. 
     
        Returns 0 if there is no new message. 

        TODO check that I can do this in a single cs
     */
    uint8_t receive(uint8_t * buffer) {
        spi::setCs(CS, true);
        Status status{spi::transfer(R_RX_PL_WID)};
        uint8_t len = 0;
        if (status) {
            len = spi::transfer(0);
            spi::transfer(R_RX_PAYLOAD);
            spi::receive(buffer, len);
        }
        spi::setCs(CS, false);
        return len;
    }

    void transmit(uint8_t * buffer, uint8_t size) {
		gpio::low(RXTX); // disable power to allow switching, or sending a pulse
        config_ &= CONFIG_PRIM_RX;
        config_ |= CONFIG_PWR_UP; 
        w_register(CONFIG, config_);
        // transmit the payload
	    spi::setCs(CS, true);
		spi::transfer(W_TX_PAYLOAD);
		spi::send(buffer, size);
		spi::setCs(CS, false);
        // send RXTX pulse to initiate the transmission, datasheet requires 10 us delay
		gpio::high(RXTX);
		cpu::delay_us(15); // some margin to 10us required by the datasheet
		gpio::low(RXTX);
    }

    /** Transmits the given payload not requiring an ACK from the receiver. 
     
        Disables all interrupts from NRF but if enableIrq is true, enables the transmit finished interrupt (active low). 
     */
    void transmitNoAck(uint8_t * buffer, uint8_t size) {
		gpio::low(RXTX); // disable power to allow switching, or sending a pulse
        config_ &= CONFIG_PRIM_RX;
        config_ |= CONFIG_PWR_UP; 
        w_register(CONFIG, config_);
        // transmit the payload
	    spi::setCs(CS, true);
		spi::transfer(W_TX_PAYLOAD_NO_ACK);
		spi::send(buffer, size);
		spi::setCs(CS, false);
        // send RXTX pulse to initiate the transmission, datasheet requires 10 us delay
		gpio::high(RXTX);
		cpu::delay_us(15); // some margin
		gpio::low(RXTX);
    }

	/** Sends given payload as next ack's payload. 
	
	 */
	void transmitAckPayload(uint8_t const * payload, uint8_t size) {
		spi::setCs(CS, true);
		spi::transfer(W_ACK_PAYLOAD | 1); // for pipe 1, since we do not use other pipes
		spi::send(payload, size);
        spi::setCs(CS, false);
	}

	/** Returns the contents of the config register on the device. This is for debugging purposes only. 
	 */
	Config config() {
		return Config{r_register(CONFIG)};
	}
	
	/** Returns the contents of the status register. For debugging purposes only. 
	 */
	Status status() {
		spi::setCs(CS, true);
        Status result{spi::transfer(NOP)};
		spi::setCs(CS, false);
		return result;
	}
	
	/** Returns the observe TX register. For debugging purposes only. 
	 */
	TxStats observeTX() {
		return TxStats{r_register(OBSERVE_TX)};
	}


	/** Returns the fifo status register. For debugging purposes only. 
	 */
	uint8_t fifoStatus() {
		return r_register(FIFO_STATUS);
	}


	
private:

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
	
	static constexpr uint8_t EN_DPL = 1 << 2;
	static constexpr uint8_t EN_ACK_PAY = 1 << 1;
    static constexpr uint8_t EN_DYN_ACK = 1 << 0;

    // fifo status register values
    static constexpr uint8_t TX_REUSE = 1 << 6;
    static constexpr uint8_t TX_FULL = 1 << 5;
    static constexpr uint8_t TX_EMPTY = 1 << 4;
    static constexpr uint8_t RX_FULL = 1 << 1;
    static constexpr uint8_t RX_EMPTY = 1 << 0;
	
	uint8_t config_;
	
};




