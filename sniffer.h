#ifndef SNIFFER_H
#define SNIFFER_H

#include <stdint.h>

// Geometry of the sniffer stream the firmware sends on bulk endpoint 1.
//
// Sniff() (Source/logic.c) streams one payload per display cycle: the decode
// union T.LOGIC.data, then 8 more bytes of which the first four are indrx and
// indtx (little endian). The union is bigger than one USB transfer can count (the
// transfer count is 10 bits), so the firmware sends it in parts and *only the last
// part is a short packet*: one bulk read returns the whole cycle, and a read that
// started mid-cycle comes back shorter - which is how a host tells them apart. So
// the payload length identifies the geometry, and neither buffer constant appears
// below: 2 * half + 1 = union and i2c * 5 / 4 + 1 = union.
//
//                  union (payload - 8)                tail (8)
//   unionSize      Serial: RX[half] then TX[half]      indrx indtx ...
//                  I2C:    decoded[i2c] then addr_ack[i2c / 4 + 1]
//
//   XScopes (ATxmega32A4U)  union 1281  half  640  i2c 1024  payload 1289
//                           parts 640 + 649
//   Watch   (ATxmega256A3U) union 2561  half 1280  i2c 2048  payload 2569
//                           parts 640 + 640 + 640 + 649
//
// The firmware packs the address/ack state of decoded I2C byte n into addr_ack[n
// / 4], two bits per byte, MSB of the pair first: bit 7 address, bit 6 ack (see
// the display code in logic.c).

enum {
    SNIFFER_PAYLOAD_XSCOPES = 1289,     // 1281 + 8
    SNIFFER_PAYLOAD_WATCH = 2569        // 2561 + 8
};

// The payload a device family streams, from bcdDevice's low byte (DEVICE_CODE in
// each firmware's hardware.h: 0x10..0x16 XScopes, 0x20 Watch), or 0 when it is
// unknown. One case needs it: 640 + 649 of the Watch's four parts is 1289 bytes,
// exactly an XScopes cycle, so a Watch read that started just before its third
// part would otherwise pass for a complete XScopes payload.
inline int snifferPayloadSizeForDeviceCode(int code) {
    if((code >> 4) == 0x2)
        return SNIFFER_PAYLOAD_WATCH;
    if(code >= 0x01 && (code >> 4) <= 0x1)
        return SNIFFER_PAYLOAD_XSCOPES;
    return 0;
}

struct SnifferLayout {
    int unionSize;     // bytes of T.LOGIC.data; 0 when the payload is not a cycle
    int serialHalf;    // BUFFER_SERIAL: the RX half, the TX half follows it
    int i2cSize;       // BUFFER_I2C: the decoded bytes, addr_ack follows them
    int payloadSize;   // unionSize + the 8 trailing bytes

    bool isValid() const {
        return unionSize > 0;
    }

    // Byte accessors into a payload read into LibUsbDevice::chData.
    static uint8_t at(const uint8_t *data, int offset) {
        return data[offset];
    }
    static uint16_t le16(const uint8_t *data, int offset) {
        return (uint16_t)(data[offset] | (data[offset + 1] << 8));
    }
    uint8_t rx(const uint8_t *data, int n) const {
        return at(data, n);
    }
    uint8_t tx(const uint8_t *data, int n) const {
        return at(data, serialHalf + n);
    }
    uint8_t decoded(const uint8_t *data, int n) const {
        return at(data, n);
    }
    uint8_t addrAck(const uint8_t *data, int n) const {
        return at(data, i2cSize + (n >> 2));
    }
    uint16_t indrx(const uint8_t *data) const {
        return le16(data, unionSize);
    }
    uint16_t indtx(const uint8_t *data) const {
        return le16(data, unionSize + 2);
    }
};

// The layout of a payload of this length, or an invalid one (isValid() false) if
// it is not a complete cycle this firmware can produce: a scope frame, a short
// bulk read, or a sniffer read that started mid-cycle. The sizes are listed,
// because a length alone cannot tell them apart from the parts of a cycle (the
// Watch's short tails are 649, 1289 and 1929 bytes); a device with larger buffers
// adds its payload here. The offsets are still derived from the length.
inline SnifferLayout snifferLayout(int payloadLength) {
    SnifferLayout layout = {0, 0, 0, 0};
    if(payloadLength != SNIFFER_PAYLOAD_XSCOPES && payloadLength != SNIFFER_PAYLOAD_WATCH)
        return layout;
    const int unionSize = payloadLength - 8;    // Sniff()'s 8 trailing bytes
    layout.unionSize = unionSize;
    layout.serialHalf = (unionSize - 1) / 2;
    layout.i2cSize = (unionSize - 1) * 4 / 5;
    layout.payloadSize = payloadLength;
    return layout;
}

#endif // SNIFFER_H
