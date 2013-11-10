#ifndef SNIFFER_H
#define SNIFFER_H

class Sniffer
{
    public:

    union
    {
        struct
        {

            uint8_t RX[640];

            uint8_t TX[640];

        } Serial;

        struct
        {

            uint8_t decoded[1024];

            uint8_t addr_ack[257];

        } I2C;

        struct
        {

            uint8_t decoded[1280];

        } All;

    } data;
    volatile uint16_t indrx;    // RX index

    volatile uint16_t indtx;    // TX index

    uint8_t databits;           // UART Data bits

    uint8_t *addr_ack_ptr;      // pointer to address or data (packed bits)

    uint8_t *data_ptr;

    uint8_t addr_ack_pos;       // counter for keeping track of all bits / location for DMA

    uint16_t baud;              // Baud rate

};

#endif // SNIFFER_H
