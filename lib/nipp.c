#include "nipp.h"
#include <string.h>

int nipp_errno;

/*
 * Compute the "checksum" (really longitudinal parity).
 * Should be zero for a valid message.
 */

uint8_t nipp_check_message( nipp_message_t *m )
{
	unsigned len = NIPP_LENGTH(m) + NIPP_HEADER_LENGTH;
	unsigned i;
	uint8_t parity = 0xff;
	
	for( i = 0; i < len; i += 1 ) parity ^= (*m)[i];
	
	return parity;
}



nipp_message_t *nipp_new_message( bool command, unsigned id,
	unsigned sequence, unsigned length, unsigned function )
{
	nipp_message_t *m = nipp_outgoing( length );
	
	if( !m ) return 0;	// errno already set
	
	if( id > 0x7ff || function > 0x7f ) {
		nipp_errno = NIPP_INVALID;
		return 0;
	}
	
	(*m)[0] = CCSDS_VERSION | (command<<4) | CCSDS_SECONDARY | (id>>8);
	(*m)[1] = id;
	(*m)[2] = CCSDS_SEGMENTATION | ((sequence>>8)&0x3f);	// wrap sequence
	(*m)[3] = sequence;
	length += 1;		// Correct for CCSDS conventions
	(*m)[4] = length>>8;
	(*m)[5] = length;
	(*m)[6] = CCSDS_SECONDARY_HEADER | function;
	
	// Can't fill in parity byte yet
	
	return m;
}



int nipp_truncate( nipp_message_t *m, unsigned length )
{
	if( length > NIPP_MAX_LENGTH ) {
		nipp_errno = NIPP_TOO_LONG;
		return -1;
	}
	
	length += 1;		// Correct for CCSDS conventions
	(*m)[4] = length>>8;
	(*m)[5] = length;
	
	return 0;
}



int nipp_send( nipp_message_t *m )
{
	unsigned length = NIPP_LENGTH(m);
	
	if( length > NIPP_MAX_LENGTH ) {
		nipp_errno = NIPP_TOO_LONG;
		nipp_abort_tx( m );
		return -1;
	}
	
	(*m)[7] = nipp_check_message( m );
	
	if( nipp_send_sync() < 0 ) return -1;
	
	return nipp_send_buffer( m, length + NIPP_HEADER_LENGTH );
}


nipp_message_t *nipp_get_message( unsigned timeout )
{
	static nipp_message_t b;
	static unsigned bytes = 0;
	unsigned c, t;
	int n;
	
	n = nipp_find_sync( &timeout );
	if( n < 0 ) return 0;		// Error looking for sync
	
	while( bytes < NIPP_HEADER_LENGTH ){
		c = nipp_get_bytes( b + bytes, NIPP_HEADER_LENGTH - bytes,
			&timeout );
		if( !c ) return 0;	// timeout or other problem
		bytes += c;
	}
	
	// OK, have a header, now get data
	
	t = NIPP_LENGTH( &b );
	
	if( t > NIPP_MAX_LENGTH ) {
		nipp_errno = NIPP_TOO_LONG;
		nipp_abort_rx();
		bytes = 0;
		return 0;
	}
	
	t += NIPP_HEADER_LENGTH;
	
	while( bytes < t ) {
		c = nipp_get_bytes( b + bytes, t - bytes, &timeout );
		if( !c ) return 0;	// timeout or other problem
		bytes += c;
	}
	
	// Set bytes to zero so next call will discard buffer
	
	bytes = 0;
	
	return &b;
}


int nipp_default_handler( nipp_message_t *msg )
{
	return 0;	// Stub for now
}
