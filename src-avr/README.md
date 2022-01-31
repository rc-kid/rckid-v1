# User Input and Power Management

Both are handled by an ATTiny chip that communicates with RP2040 via I2C bus and an IRQ pin. The chip operates at the voltage directly from the battery / charger so that it can monitor battery level and charging. 