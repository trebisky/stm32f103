/* papoon.hxx
 * Introduced by Tom Trebisky  11-4-2023
 *
 * The original Papoon setup did all this CONFIGURATION stuff
 # in the Makefile via the following long list of compiler options.
 *
 * I decided that I would rather clean up the build messages
 * and moved it all into this file.
 */

extern "C" void printf ( const char *, ... );

// SEND_B4_RECV    ?= 64
// SYNC_LEN     ?= 4
// REPORT_EVERY    ?= 10000
// ASYNC           ?= -D
// DEBUG           ?= -U

// CONFIGURATION = -DUP_MAX_PACKET_SIZE=64                 \
//                 -DDOWN_MAX_PACKET_SIZE=64               \
//                 -DSEND_B4_RECV_MAX=$(SEND_B4_RECV)      \
//                 -DUSB_RANDOMTEST_UP_SEED=0x5f443bba     \
//                 -DUSB_RANDOMTEST_DOWN_SEED=0x684053d8   \
//                 -DUSB_RANDOMTEST_LENGTH_SEED=0x769bc5e6 \
//                 -DSLAVE_LENGTH_SEED=0x4b6420e0          \
//                 -DUSB_RANDOMTEST_SYNC_LENGTH=$(SYNC_LEN)\
//                 -DHISTOGRAM_LENGTH=8                    \
//                 -DREPORT_EVERY=$(REPORT_EVERY)          \
//                 $(ASYNC)RANDOMTEST_LIBUSB_ASYNC         \
//                 $(DEBUG)DEBUG

#define UP_MAX_PACKET_SIZE	64
#define DOWN_MAX_PACKET_SIZE	64
#define SEND_B4_RECV_MAX	64

#define USB_RANDOMTEST_UP_SEED		0x5f443bba
#define USB_RANDOMTEST_DOWN_SEED	0x684053d8
#define USB_RANDOMTEST_LENGTH_SEED	0x769bc5e6

#define SLAVE_LENGTH_SEED		0x4b6420e0
#define USB_RANDOMTEST_SYNC_LENGTH	4

#define HISTOGRAM_LENGTH		8
#define REPORT_EVERY			1000

#define RANDOMTEST_LIBUSB_ASYNC
// #define DEBUG

// THE END
