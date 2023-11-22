/* usb_enum.c
 *
 * (c) Tom Trebisky  11-22-2023
 *
 * This handles USB enumeration.
 *
 * This gets called when there is any activity
 * on endpoint 0 (the control endpoint)
 *
 * We avoid any direct access to USB hardware here in hopes
 *  this will be portable code that can be used on
 *  devices other than the F103 someday.
 */

#include "protos.h"

#define DESC_TYPE_DEVICE	1
#define DESC_TYPE_CONFIG	2
#define DESC_TYPE_STRING	3
#define DESC_TYPE_INTERFACE	4
#define DESC_TYPE_ENDPOINT	5

const u8 my_device_desc[] = {
    0x12,   // bLength
    DESC_TYPE_DEVICE,
    0x00,
    0x02,   // bcdUSB = 2.00
    0x02,   // bDeviceClass: CDC
    0x00,   // bDeviceSubClass
    0x00,   // bDeviceProtocol
    0x40,   // bMaxPacketSize0
    0x83,   // idVendor = 0x0483
    0x04,   //    "     = MSB of uint16_t
    0x40,   // idProduct = 0x5740
    0x57,   //     "     = MSB of uint16_t
    0x00,   // bcdDevice = 2.00
    0x02,   //     "     = MSB of uint16_t
    1,      // Index of string descriptor describing manufacturer
    2,      // Index of string descriptor describing product
    3,      // Index of string descriptor describing device serial number
    0x01    // bNumConfigurations
};

/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */

struct setup {
	u8	rtype;
	u8	request;
	u16	value;
	u16	index;
	u16	length;
};

static int device_request ( struct setup * );
static int descriptor_request ( struct setup * );
static int interface_request ( struct setup * );

static void would_send ( char *, char *, int );

#define RT_RECIPIENT	0x1f
#define RT_TYPE		(0x3<<5)
#define RT_DIR		0x80

/* setup packet was received */
int
usb_setup ( char *buf, int count )
{
	struct setup *sp;
	int type;
	int recip;

	// printf ( "Setup packet: %d bytes -- " );
	// print_buf ( buf, count );

	sp = (struct setup *) buf;
	type = (sp->rtype & RT_TYPE) >> 5;
	recip = sp->rtype & RT_RECIPIENT;

	if ( type == 0 ) {	/* Standard */
	    if ( recip == 0 )	/* Device */
		return device_request ( sp );
	    else if ( recip == 1 ) /* Interface */
		return interface_request ( sp );
	    else if ( recip == 2 ) /* Endpoint */
		;
	    else
		;

	} else if ( type == 1 ) {	/* Class */
	    ;
	}

	// printf ( "Spin in enum for debug\n" );
	// for ( ;; )
	//     ;

	/* Not handled */
	return 0;
}

static int
device_request ( struct setup *sp )
{
	if ( sp->request == 6 )		// Get Descriptor
	    return descriptor_request ( sp );
	return 0;
}

/* Here the 16 bit value field has the descriptor type
 *  in the top 8 bits, and an index in the low 8.
 *  The index is used for strings
 */
static int
descriptor_request ( struct setup *sp )
{
	int what = sp->value >> 8;
	int index = sp->value & 0xff;

	if ( what == 1 ) {	// device
	    endpoint_send ( 0, my_device_desc, sizeof(my_device_desc) );
	    // would_send ( "device descriptor" , my_device_desc, sizeof(my_device_desc) );
	    return 1;
	}

	//if ( what == 2 )	// config
	//if ( what == 3 )	// string

	return 0;
}

static int
interface_request ( struct setup *sp )
{
}

static void
would_send ( char *msg, char *buf, int count )
{
	printf ( "Would send %s ", msg );
	printf ( "%d : ", count );
	print_buf ( buf, count );
}

/* ----------------------------------------------- */

/* control packet received (control OUT) */
int
usb_control ( char *buf, int count )
{
	return 0;
}

/* control packet send finished (control IN)
 * (there is no setup IN)
 */
int
usb_control_tx ( void )
{
}

/* =================================================================================== */
/* =================================================================================== */

/* From papoon usb_dev_cdc_acm.cxx */
// Copyright (C) 2019,2020 Mark R. Rubin

#ifdef notdef
uint8_t UsbDev::_CONFIG_DESC[] = {  // not const because set total size entry
    // Configuration Descriptor
    0x09,   // bLength: Configuration Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::CONFIGURATION),
    0,      // wTotalLength: including sub-descriptors; will be set a runtime
    0x00,   //      "      : MSB of uint16_t
    0x02,   // bNumInterfaces: 2 interface
    0x01,   // bConfigurationValue: Configuration value
    0x00,   // iConfiguration: Index of string descriptor for configuration
    0xC0,   // bmAttributes: self powered
    0x32,   // MaxPower 0 mA

    // Interface Descriptor
    0x09,   // bLength: Interface Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::INTERFACE),
    // Interface descriptor type
    0x00,   // bInterfaceNumber: Number of Interface
    0x00,   // bAlternateSetting: Alternate setting
    0x01,   // bNumEndpoints: One endpoints used
    0x02,   // bInterfaceClass: Communication Interface Class
    0x02,   // bInterfaceSubClass: Abstract Control Model
    0x01,   // bInterfaceProtocol: Common AT commands
    0x00,   // iInterface:

    // Header Functional Descriptor
    0x05,   // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x00,   // bDescriptorSubtype: Header Func Desc
    0x10,   // bcdCDC: spec release number
    0x01,

    // Call Management Functional Descriptor
    0x05,   // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x01,   // bDescriptorSubtype: Call Management Func Desc
    0x00,   // bmCapabilities: D0+D1
    0x01,   // bDataInterface: 1

    // ACM Functional Descriptor
    0x04,   // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x02,   // bDescriptorSubtype: Abstract Control Management desc
    0x02,   // bmCapabilities

    // Union Functional Descriptor
    0x05,   // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x06,   // bDescriptorSubtype: Union func desc
    0x00,   // bMasterInterface: Communication class interface
    0x01,   // bSlaveInterface0: Data Class Interface

    // Endpoint 2 Descriptor
    0x07,   // bLength: Endpoint Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::ENDPOINT),
    UsbDevCdcAcm::ACM_ENDPOINT | UsbDev::ENDPOINT_DIR_IN,   // bEndpointAddress
    static_cast<uint8_t>(UsbDev::EndpointType::INTERRUPT),  // bmAttributes
    UsbDevCdcAcm::ACM_DATA_SIZE,                            // wMaxPacketSize:
    0x00,
    0xFF,   // bInterval:


    // Data class interface descriptor
    0x09,   // bLength: Interface Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::INTERFACE),
    0x01,   // bInterfaceNumber: Number of Interface
    0x00,   // bAlternateSetting: Alternate setting
    0x02,   // bNumEndpoints: Two endpoints used
    0x0A,   // bInterfaceClass: CDC
    0x00,   // bInterfaceSubClass:
    0x00,   // bInterfaceProtocol:
    0x00,   // iInterface:

    // Endpoint 3 Descriptor
    0x07,   // bLength: Endpoint Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::ENDPOINT),
    UsbDevCdcAcm::CDC_ENDPOINT_OUT, // bEndpointAddress: (OUT3)
    static_cast<uint8_t>(UsbDev::EndpointType::BULK),   // bmAttributes: Bulk
    UsbDevCdcAcm::CDC_OUT_DATA_SIZE,                    // wMaxPacketSize: 64
    0x00,                                               //    MSB of uint16_t
    0x00,   // bInterval: ignore for Bulk transfer

    // Endpoint 1 Descriptor
    0x07,                               // bLength: Endpoint Descriptor size
    static_cast<uint8_t>(UsbDev::DescriptorType::ENDPOINT),
    UsbDevCdcAcm::CDC_ENDPOINT_IN | UsbDev::ENDPOINT_DIR_IN,//bEndpointAddress
    static_cast<uint8_t>(UsbDev::EndpointType::BULK),  // bmAttributes: Bulk
    UsbDevCdcAcm::CDC_IN_DATA_SIZE,     // wMaxPacketSize:
    0x00,
    0x00                                // bInterval
};

#ifdef notdef
const uint8_t   UsbDevCdcAcm::_device_string_desc[] = {
                46,
                static_cast<uint8_t>(UsbDev::DescriptorType::STRING),
                'S', 0, 'T', 0, 'M', 0, '3', 0,
                '2', 0, ' ', 0, 'V', 0, 'i', 0,
                'r', 0, 't', 0, 'u', 0, 'a', 0,
                'l', 0, ' ', 0, 'C', 0, 'O', 0,
                'M', 0, ' ', 0, 'P', 0, 'o', 0,
                'r', 0, 't', 0                };     // "STM32 Virtual COM Port"
#endif

const uint8_t   UsbDevCdcAcm::_device_string_desc[] = {
                46,
                static_cast<uint8_t>(UsbDev::DescriptorType::STRING),
                'U', 0, 'n', 0, 'c', 0, 'l', 0,
                'e', 0, ' ', 0, 'J', 0, 'o', 0,
                'e', 0, '\'', 0, 's', 0, ' ', 0,
                's', 0, 'e', 0, 'r', 0, 'i', 0,
                'a', 0, 'l', 0, ' ', 0, 'I', 0,
                'O', 0, ' ', 0                };     // Uncle Joe's serial IO

/* TJT - the above is pretty awful to deal with
 * why not set up a method to move a plain old C string
 * into a "structure" of this sort?
 * STM32 Virtual COM Port is 22 bytes.
 * double this (to 44), add 2 and we get the 46 value.
 */
#endif

/* THE END */