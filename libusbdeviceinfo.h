#ifndef LIBUSBDEVICEINFO_H
#define LIBUSBDEVICEINFO_H


   enum POLICY_TYPE
   {
            SHORT_PACKET_TERMINATE = 1,
            AUTO_CLEAR_STALL,
            PIPE_TRANSFER_TIMEOUT,
            IGNORE_SHORT_PACKETS,
            ALLOW_PARTIAL_READS,
            AUTO_FLUSH,
            RAW_IO
   };

   enum  USB_ENDPOINT_DIRECTION
   {
       USB_ENDPOINT_IN = 0x81,
       USB_ENDPOINT_OUT = 0x01
   };

   enum
   {
       VENDOR_ID = 0x16D0,
       PRODUCT_ID = 0x06F9,
       INTERFACE_NUMBER = 0,
       CLASS_ID = LIBUSB_HOTPLUG_MATCH_ANY

   };

   enum BUFFER_SIZE
   {
       LEN_BULK_IN_BUFFER = 770,                         //(256*3)+2 CH1+CH2+CHD+frame number
       LEN_CONTROL_BUFFER = 64
   };



   enum USBD_PIPE_TYPE
   {
            UsbdPipeTypeControl,
            UsbdPipeTypeIsochronous,
            UsbdPipeTypeBulk,
            UsbdPipeTypeInterrupt
   };

   enum USB_DEVICE_SPEED
   {
            UsbLowSpeed = 1,
            UsbFullSpeed,
            UsbHighSpeed
   };

   struct USB_CONFIGURATION_DESCRIPTOR
   {
            unsigned char bLength;
            unsigned char bDescriptorType;
            unsigned short wTotalLength;
            unsigned char bNumInterfaces;
            unsigned char bConfigurationValue;
            unsigned char iConfiguration;
            unsigned char bmAttributes;
            unsigned char MaxPower;
   };

   struct USB_INTERFACE_DESCRIPTOR
   {
            unsigned char bLength;
            unsigned char bDescriptorType;
            unsigned char bInterfaceNumber;
            unsigned char bAlternateSetting;
            unsigned char bNumEndpoints;
            unsigned char bInterfaceClass;
            unsigned char bInterfaceSubClass;
            unsigned char bInterfaceProtocol;
            unsigned char iInterface;
   };

   struct LIBUSB_PIPE_INFORMATION
   {
            USBD_PIPE_TYPE PipeType;
            unsigned char PipeId;
            unsigned short MaximumPacketSize;
            unsigned char Interval;
   };

   struct LIBUSB_SETUP_PACKET
   {
            unsigned char RequestType;
            unsigned char Request;
            unsigned short Value;
            unsigned short Index;
            unsigned short Length;
   };

#endif // LIBUSBDEVICEINFO_H
