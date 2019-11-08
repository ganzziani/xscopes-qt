#ifndef SNIFFER_H
#define SNIFFER_H

struct Serial {
    uint8_t RX[640];
    uint8_t TX[640];
    uint8_t dummy;
};

struct I2C {
    uint8_t decoded[1024];
    uint8_t addr_ack[257];
};

struct All {
    uint8_t decoded[1280];
    uint8_t dummy;
};

union Data {
    Serial serial;
    I2C i2c;
    All all;
};

struct Sniffer {
    Data data;
    uint16_t indrx;    // RX index
    uint16_t indtx;    // TX index
    uint8_t databits;           // UART Data bits
    uint8_t *addr_ack_ptr;      // pointer to address or data (packed bits)
    uint8_t *data_ptr;
    uint8_t addr_ack_pos;       // counter for keeping track of all bits / location for DMA
    uint16_t baud;              // Baud rate
};

#endif // SNIFFER_H
