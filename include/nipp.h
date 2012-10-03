#include <stdint.h>
#include <stdbool.h>

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
 
#define NIPP_MAX_LENGTH		640	// Max data bytes for this implementation
#define NIPP_HEADER_LENGTH	8

typedef uint8_t nipp_message_t[ NIPP_HEADER_LENGTH + NIPP_MAX_LENGTH ];

/*
 * Macros to extract message fields. Note that headers are big-endian here.
 */
 
#define NIPP_COMMAND(m) (m[0]&(0x1<<4)!=0)
#define NIPP_ID(m) ((unsigned)(((m[0]<<8)|m[1])&0x7ff))
#define NIPP_SEQUENCE(m) ((unsigned)(((m[2]<<8)|m[3])&0x3fff))
#define NIPP_LENGTH(m) ((unsigned)((m[4]<<8)|m[5])-1)	// Length of data only
#define NIPP_FUNCTION(m) ((unsigned)(m[6]&0x7f))
#define NIPP_DATA(m) ((void *)(m+NIPP_HEADER_LENGTH))

extern nipp_message_t *nipp_new_message( bool command, unsigned id,
	unsigned sequence, unsigned length, unsigned function );

extern int nipp_truncate( nipp_message_t *msg, unsigned length );

extern int nipp_send( nipp_message_t *msg );

extern nipp_message_t *nipp_get_message( unsigned timeout );

extern uint8_t nipp_check_message( nipp_message_t *msg );

extern int nipp_default_handler( nipp_message_t *msg );

extern int nipp_errno;

