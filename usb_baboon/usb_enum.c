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

static const u8 my_device_desc[] = {
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

// not const because set total size entry
static const u8  my_config_desc[] = {
    // Configuration Descriptor
    0x09,   // bLength: Configuration Descriptor size
    DESC_TYPE_CONFIG,
    0x43,   // wTotalLength: including sub-descriptors
    0x00,   //      "      : MSB of uint16_t
    0x02,   // bNumInterfaces: 2 interface
    0x01,   // bConfigurationValue: Configuration value
    0x00,   // iConfiguration: Index of string descriptor for configuration
    0xC0,   // bmAttributes: self powered
    0x32,   // MaxPower 0 mA
    // To here is 9 bytes and is the first thing we dole out.

    // Interface Descriptor
    0x09,   // bLength: Interface Descriptor size
    // static_cast<uint8_t>(UsbDev::DescriptorType::INTERFACE),
    DESC_TYPE_INTERFACE,	// Interface descriptor type
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

#define ACM_ENDPOINT		2
#define CDC_ENDPOINT_OUT	3
#define CDC_ENDPOINT_IN		1

#define ACM_DATA_SIZE	8
#define CDC_OUT_DATA_SIZE	64
#define CDC_IN_DATA_SIZE	64

#define ENDPOINT_DIR_IN	0x80
#define ENDPOINT_TYPE_BULK	2
#define ENDPOINT_TYPE_INTERRUPT	3

    // Endpoint 2 Descriptor
    0x07,   // bLength: Endpoint Descriptor size
    DESC_TYPE_ENDPOINT,
    ACM_ENDPOINT | ENDPOINT_DIR_IN,	// bEndpointAddress
    ENDPOINT_TYPE_INTERRUPT,		// bmAttributes
    ACM_DATA_SIZE,			// wMaxPacketSize:
    0x00,
    0xFF,   // bInterval:


    // Data class interface descriptor
    0x09,   // bLength: Interface Descriptor size
    DESC_TYPE_INTERFACE,
    0x01,   // bInterfaceNumber: Number of Interface
    0x00,   // bAlternateSetting: Alternate setting
    0x02,   // bNumEndpoints: Two endpoints used
    0x0A,   // bInterfaceClass: CDC
    0x00,   // bInterfaceSubClass:
    0x00,   // bInterfaceProtocol:
    0x00,   // iInterface:

    // Endpoint 3 Descriptor
    0x07,   // bLength: Endpoint Descriptor size
    DESC_TYPE_ENDPOINT,
    CDC_ENDPOINT_OUT,		// bEndpointAddress: (OUT3)
    ENDPOINT_TYPE_BULK,		// bmAttributes: Bulk
    CDC_OUT_DATA_SIZE,		// wMaxPacketSize: 64
    0x00,                                               //    MSB of uint16_t
    0x00,   // bInterval: ignore for Bulk transfer

    // Endpoint 1 Descriptor
    0x07,                               // bLength: Endpoint Descriptor size
    DESC_TYPE_ENDPOINT,
    CDC_ENDPOINT_IN | ENDPOINT_DIR_IN,	//bEndpointAddress
    ENDPOINT_TYPE_BULK,			// bmAttributes: Bulk
    CDC_IN_DATA_SIZE,			// wMaxPacketSize:
    0x00,
    0x00                                // bInterval
};

/* There is a 16 bit language id we need to send.
 * Wireshark recognizes0x0409 as " English (United States)"
 * apparently the language ID codes do not conflict with
 * unicode characters and this just works.
 */
static const u8 my_language_string_desc[] = {
                4,
                DESC_TYPE_STRING,
                0x09,
		0x04
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
static void usb_class ( struct setup * );

static void would_send ( char *, char *, int );

#define RT_RECIPIENT	0x1f
#define RT_TYPE		(0x3<<5)
#define RT_DIR		0x80

static int get_descriptor ( struct setup * );
static int set_address ( struct setup * );
static int set_configuration ( struct setup * );
static int string_send ( int );

/* We get either setup or control packets on endpoint 0
 * This file handles both.
 */

/* setup packet was received
 * unlike much other USB code, I don't code up a tree like
 * hierarchy of handlers, but I "cheat" and test the
 * combination rtype+request word.
 * Most of the enumeration packets are 8006, which is
 * "get descriptor"
 */
int
usb_setup ( char *buf, int count )
{
	struct setup *sp;
	int tag;
	int rv = 0;

	printf ( "Setup packet: %d bytes -- " );
	print_buf ( buf, count );

	/* Just ignore ZLP (zero length packets) */
	if ( count == 0 )
	    return 0;

	sp = (struct setup *) buf;

	if ( sp->rtype == 0x21 ) {
	    usb_class ( sp );
	    return 1;
	}

	tag = sp->rtype << 8 | sp->request;

	switch ( tag ) {
	    case 0x8006:
		rv = get_descriptor ( sp );
		break;
	    case 0x0005:
		rv = set_address ( sp );
		break;
	    case 0x0009:
		rv = set_configuration ( sp );
		break;
	    default:
		break;
	}

	printf ( "%d", rv );
	return rv;
}

#define D_STRING	3

static int
get_descriptor ( struct setup *sp )
{
	int len;
	int value;

	/* Thanks to the idiot USB business of using the 2 byte
	 * value field to hold a 1 byte value and some other index.
	 */
	// value = sp->value >> 8;
	value = sp->value;

	printf ( "\nValue: %04x\n", sp->value );
	switch ( value ) {

	    /* device descriptor */
	    case 1 << 8:
		// printf ( " reply with %d\n", sizeof(my_device_desc) );
		endpoint_send ( 0, my_device_desc, sizeof(my_device_desc) );
		return 1;
		// would_send ( "device descriptor" , my_device_desc, sizeof(my_device_desc) );

	    /* device qualifier */
	    case 6 << 8:
		printf ( "q" );
		endpoint_send_zlp ( 0 );
		return 1;

	    /* configuration */
	    case 2 << 8:
		/* This is interesting.  We have 67 bytes (64+3) to send.
		 * But the game is even more complex.
		 * The first request asks for 9 bytes, so we send the
		 * first 9 bytes as a response.
		 * That contains the 0x43 value for the total length.
		 * The host comes back a second time asking for the
		 * full 67 bytes, but we have to break that into
		 * 2 pieces, the first 64, the second 3.
		 * We let endpoint_send() handle that.
		 */

		/* The host first asks for 9 bytes,
		 * so we truncate what we send accordingly.
		 */
		len = sizeof(my_config_desc);
		if ( len > sp->length )
		    len = sp->length;

		if ( len < 64 ) {
		    // printf ( "Q" );
		    endpoint_send ( 0, my_config_desc, len );
		    return 1;
		}

		//printf ( "%d", len );
		endpoint_send ( 0, my_config_desc, len );
		return 1;

	    /* string - language codes */
	    case D_STRING << 8:
		len = sizeof (my_language_string_desc);
		endpoint_send ( 0, my_language_string_desc, len );
		return 1;
	    case D_STRING << 8 | 1:
	    case D_STRING << 8 | 2:
	    case D_STRING << 8 | 3:
		printf ( "s" );
		// return string_send ( value & 0xff );
		len = string_send ( value & 0xff );
		printf ( "%d", len );
		return len;
	    default:
		break;
	}

	printf ( "?" );
	return 0;
}

struct string_xx {
	u8	length;
	u8	type;
	u16	buf[31];
};

static u8 *my_strings[] = {
    "ACME computers",
    "Stupid ACM port",
    "1234"
};

/* We can handle 3 indexes:
 * 1 - vendor
 * 2 - device
 * 3 - serial number
 */
static int
string_send ( int index )
{
	struct string_xx xx;
	u8 *str;
	int n;
	int i;

	printf ( "Index %d\n", index );
	if ( index < 1 || index > 3 )
	    panic ( "No such string" );

	str = my_strings[index];
	n = strlen ( str );

	if ( n > 31 )
	    panic ( "String too big" );

	xx.length = 2*n;
	xx.type = D_STRING;

	/* 8 bit ascii to 16 bit unicode */
	for ( i=0; i<n; i++ )
	    xx.buf[i] = str[i];

	return 3;
}

/*
 * So far I have seen one thing outside of enumeration.
 * When I start up picocom, I get this:
 * CTR on endpoint 0 8210
 * EPR[0] = EA60
 * Read 8 bytes from EP 0 2122030000000000
 *
 * The setup bit is set in the EPR
 * This is a class interface request.
 * - not a standard request as in chapter 9 of USB 2.0
 *
 * 0x20 is "set line coding"
 * 0x21 is "get line coding"
 * 0x22 is "control line state"
 *
 * Without the ZLP, picocom waits for several seconds.
 * Sending it makes things as they should be.
 *
 * At this point, we just ignore these requests.
 */

static void
usb_class ( struct setup *sp )
{
	// printf ( "USB class/interface request: %02x\n", sp->request );
	endpoint_send_zlp ( 0 );
}

static int
set_address ( struct setup *sp )
{
	return 0;
}

static int
set_configuration ( struct setup *sp )
{
	return 0;
}

/* XXX ----------------------------------- */
/* XXX ----------------------------------- */
/* XXX ----------------------------------- */


/* ----------------------------------------------- */

/* control packet received (control OUT) */
int
usb_control ( char *buf, int count )
{
	printf ( "Control packet: %d bytes -- " );
	print_buf ( buf, count );

	return 0;
}

#ifdef notdef
/* control packet send finished (control IN)
 * (there is no setup IN)
 */
int
usb_control_tx ( void )
{
	return 0;
}
#endif

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
