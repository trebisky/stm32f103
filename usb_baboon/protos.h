/* protos.h
 *
 * (c) Tom Trebisky  11-22-2023
 *
 * prototypes and such
 *
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef volatile unsigned int vu32;

#define BIT(x)	(1<<x)

void panic ( char * );

int usb_setup ( char *, int );
int usb_control ( char *, int );
int usb_control_tx ( void );

/* THE END */
