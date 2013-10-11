#include <stdint.h>
#include <stdbool.h>

/*
 * Sync word separating packets
 */

#define NIPP_SYNC	0xaec7cd20

/*
 * Constants for benefit of CCSDS
 */
 
#define CCSDS_VERSION		(0x0<<5)	// byte 0
#define CCSDS_SECONDARY 	(0x1<<3)	// byte 0
#define CCSDS_SEGMENTATION	(0x3<<6)	// byte 2
#define CCSDS_SECONDARY_HEADER	(0x0<<7)	// byte 6

/*
 * In this implementation, a message is an array of bytes
 */
 
#define NIPP_MAX_LENGTH		1024	// Max data bytes for this implementation
#define NIPP_HEADER_LENGTH	8

typedef uint8_t nipp_message_t[ NIPP_HEADER_LENGTH + NIPP_MAX_LENGTH ];

/*
 * Macros to extract message fields. Note that headers are big-endian here.
 */

#define NIPP_VERSION(m) (((*m)[0] >> 5) & 0x7) 
#define NIPP_COMMAND(m) (((*m)[0]&(0x1<<4))!=0)
#define NIPP_SECHDR(m) (((*m)[0] >> 3) & 1)
#define NIPP_ID(m) ((unsigned)((((*m)[0]<<8)|(*m)[1])&0x7ff))
#define NIPP_SEQUENCEFLAG(m) ((unsigned)((((*m)[2]>>6) & 0x3)))
#define NIPP_SEQUENCE(m) ((unsigned)((((*m)[2]<<8)|(*m)[3])&0x3fff))
#define NIPP_LENGTH(m) ((unsigned)(((*m)[4]<<8)|(*m)[5])-1)	// Length of data only
#define NIPP_SECHDR2(m) (((*m)[6] >> 7) & 0x1)
#define NIPP_FUNCTION(m) ((unsigned)((*m)[6]&0x7f))
#define NIPP_CHECKSUM(m) ((*m)[7])
#define NIPP_DATA(m) (((*m)+NIPP_HEADER_LENGTH))

/*
 * And some macros for storing message fields.
 */
#define WR_NIPP_COMMAND(m, v)  (((*m)[0] = ((*m)[0] & 0xEF) | ((v) << 4)
#define WR_NIPP_ID(m, v) { (*m)[0] = ((*m)[0] & 0xF8) | (((v) >> 8) & 0x7); (*m)[1] = ((v) & 0xFF); }
#define WR_NIPP_SEQUENCE(m, v) { (*m)[0] = ((*m)[2] & 0xC0) | (((v) >> 8) & 0x3F); (*m)[3] = ((v) & 0xFF); }
#define WR_NIPP_LENGTH(m, v) { (*m)[4] = (((v)+1) >> 8); (*m)[5] = (((v)+1) & 0xFF); }
#define WR_NIPP_FUNCTION(m, v) (*m)[6] = ((*m)[6] & 0x80) | ((v) & 0x7f)

/*
 * Primary API from nipp.c
 */

extern nipp_message_t *nipp_copy_message(nipp_message_t *msg, int length, int function);
extern nipp_message_t *nipp_new_message( bool command, unsigned id,
	unsigned sequence, unsigned length, unsigned function );

extern int nipp_truncate( nipp_message_t *msg, unsigned length );

extern int nipp_send( nipp_message_t *msg );

extern nipp_message_t *nipp_get_message( unsigned timeout );

extern uint8_t nipp_check_message( nipp_message_t *msg );

extern int nipp_default_handler( nipp_message_t *msg );

extern int nipp_errno;

/*
 * Lower level functions that the host interface code or embedded application
 * must provide..
 */

extern void nipp_abort_tx( nipp_message_t *msg );

extern void nipp_abort_rx( void );

extern nipp_message_t *nipp_outgoing( unsigned length );

extern unsigned nipp_get_bytes( void *buffer, unsigned bytes, unsigned *timeout );

extern int nipp_send_buffer( nipp_message_t *msg, unsigned bytes );

extern int nipp_find_sync( unsigned *timeout );

extern int nipp_send_sync( void );

/*
 * Host interface code may provide a function to attach NIPP to a particular
 * file or socket.
 */

extern int nipp_attach( int fd );

/*
 * Error codes
 */

#define NIPP_INVALID 1
#define NIPP_TOO_LONG 2
#define NIPP_TIMEOUT 3
#define NIPP_NOMEM 4
#define NIPP_EIO 5
#define NIPP_BAD_SYNC 6

/*
 * Wait forever for message.
 */

#define NIPP_FOREVER ((unsigned)-1)
