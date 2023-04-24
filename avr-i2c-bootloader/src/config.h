#define I2C_ADDRESS 0x43

#define BOOTLOADER_SIZE 0x200

#define PIN_AVR_IRQ 25

#define CMD_RESERVED 0x00
#define CMD_RESET 0x01
#define CMD_INFO 0x02
#define CMD_WRITE_BUFFER 0x03
#define CMD_WRITE_PAGE 0x04
#define CMD_SET_ADDRESS 0x05

struct State {
    uint8_t status; 
    uint8_t deviceId0;
    uint8_t deviceId1;
    uint8_t deviceId2;
    uint8_t fuses[11];
    uint8_t mclkCtrlA;
    uint8_t mclkCtrlB;
    uint8_t mclkLock;
    uint8_t mclkStatus;
    uint16_t address;
    uint16_t nvmAddress;
    uint8_t lastError;
    //uint16_t pgmmemstart;
    //uint16_t pagesize;
} __attribute__((packed));

static_assert(sizeof(State) == 24, "Invalid state size.");
