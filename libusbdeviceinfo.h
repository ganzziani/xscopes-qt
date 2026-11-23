#ifndef LIBUSBDEVICEINFO_H
#define LIBUSBDEVICEINFO_H

#define ZERO_POS 128

enum  USB_ENDPOINT_DIRECTION {
    USB_ENDPOINT_IN = 0x81,
    USB_ENDPOINT_OUT = 0x01
};

enum {
    VENDOR_ID = 0x16D0,
    PRODUCT_ID = 0x06F9,
    INTERFACE_NUMBER = 0,
    CLASS_ID = LIBUSB_HOTPLUG_MATCH_ANY

};

enum BUFFER_SIZE {
    // The largest payload the firmware streams on endpoint 1: a scope frame is
    // (256*3)+2 = 770 (CH1+CH2+CHD+frame number), the sniffer's is 1289 on the
    // XScopes and 2569 on the Oscilloscope Watch (sniffer.h). Every payload ends
    // on a short packet, so a longer transfer only sets a ceiling.
    LEN_BULK_IN_BUFFER = 2570,
    LEN_CONTROL_BUFFER = 64,
    // 'm' reply: 4 byte meter value, then (current firmware) the two gains, Srate
    // and fsrate stamped with it, then the two ACHANNEL structs {offset, max, min,
    // vpp, f} (Source/main.h) and T.SCOPE.adjusting: 19 bytes. Older firmware: 18
    // (value, channels, Srate, adjusting, gains) or 14 (value, channels); the first
    // Watch firmware only the meter value. Short reads are normal: use what the
    // reply carries.
    METER_VALUE_SIZE = 4,
    METER_CHANNELS_SIZE = 14,
    METER_REPLY_SIZE_OLD = 18,
    METER_REPLY_SIZE = 19
};
#endif // LIBUSBDEVICEINFO_H
