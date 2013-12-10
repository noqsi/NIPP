#include "nipp.h"
#include <string.h>

int nipp_errno;

/*
 * Compute the "checksum" (really longitudinal parity).
 * Should be zero for a valid message.
 */

uint8_t nipp_check_message( nipp_message_t *m )
{
	unsigned len = NIPP_LENGTH(m) + NIPP_HDRLEN(m);
	unsigned i;
	uint8_t parity = 0xff;

	if (NIPP_SECHDR(m))
		for( i = 0; i < len; i += 1 )
			parity ^= (*m)[i];
	else
		parity = 0; // no sechdr, no checksum field, all good
	
	return parity;
}


nipp_message_t *nipp_copy_message(nipp_message_t *msg, int length, int function)
{
	return nipp_new_message(
		NIPP_COMMAND(msg), 
		NIPP_ID(msg), 
		NIPP_SEQUENCE(msg),
		length, function);
}

nipp_message_t *nipp_new_message( bool command, unsigned id,
	unsigned sequence, unsigned length, int function )
{
	nipp_message_t *m = nipp_outgoing( length );
	int sechdr;

	if( !m ) return 0;	// errno already set

	if (function == -1)
		sechdr = 0;
	else
		sechdr = 1; // note: must be 1, not some other nonzero value

	if( id > 0x7ff || (sechdr && function > 0x7f) ) {
		nipp_errno = NIPP_INVALID;
		return 0;
	}
	
	(*m)[0] = CCSDS_VERSION | (command<<4) | (sechdr * CCSDS_SECONDARY) | (id>>8);
	(*m)[1] = id;
	(*m)[2] = CCSDS_SEGMENTATION | ((sequence>>8)&0x3f);	// wrap sequence
	(*m)[3] = sequence;
	length += 1;		// Correct for CCSDS conventions
	(*m)[4] = length>>8;
	
	if (sechdr)
	{
		(*m)[5] = length;
		(*m)[6] = CCSDS_SECONDARY_HEADER | function;
	}
	
	// Can't fill in parity byte yet
	
	return m;
}



int nipp_truncate( nipp_message_t *m, unsigned length )
{
	if( length >= NIPP_MAX_LENGTH ) {
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
	
	if (NIPP_SECHDR(m))
	{
		(*m)[7] = 0; // clear it, so the check_message is accurate
		(*m)[7] = nipp_check_message( m );
	}
	
	if( nipp_send_sync() < 0 ) return -1;
	
	return nipp_send_buffer( m, length + NIPP_HDRLEN(m));
}


nipp_message_t *nipp_get_message( unsigned timeout, int expect_sechdr )
{
	static nipp_message_t b;
	static unsigned bytes = 0;
	static int found_sync = 0;
	unsigned c, t;
	int n;
	int hdrlen;
	
	if( !found_sync ) {
		n = nipp_find_sync( &timeout );
		if( n < 0 ) return 0;		// Error looking for sync
		found_sync = 1;
	}

	if (expect_sechdr >= 0)
		hdrlen = (expect_sechdr ? 8 : 6);
	else
	{
		// Get the first byte. We need it to know how long the header is.
		c = nipp_get_bytes( b, 1, &timeout );
		if (c == 0)
			return 0;
		
		bytes ++;
		hdrlen = NIPP_HDRLEN(&b);
	}
	while( bytes < hdrlen ){
		c = nipp_get_bytes( b + bytes, hdrlen - bytes,
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
	
	t += NIPP_HDRLEN(&b);
	
	while( bytes < t ) {
		c = nipp_get_bytes( b + bytes, t - bytes, &timeout );
		if( !c ) return 0;	// timeout or other problem
		bytes += c;
	}
	
	// Set bytes to zero so next call will discard buffer
	
	bytes = found_sync = 0;
	
	return &b;
}


int nipp_default_handler( nipp_message_t *msg )
{
	return 0;	// Stub for now
}

unsigned int
nipp_unpack(uint8_t *m, int offset, int lenval)
{
	int out=0;
	int outlen = 0;
	int byte, toget, offbit, offbyte;

	while (outlen < lenval)
	{
        offbit = (offset + outlen) % 8;
        offbyte = (offset + outlen) / 8;
		byte = m[offbyte];

		if (lenval + offbit - outlen < 8)
			toget = lenval - outlen;
		else
			toget = 8 - offbit;

		offbit = 8 - (offbit + toget);
		out = (out << toget) | ((byte >> offbit) & ((1<<toget)-1));
		outlen += toget;
	}

	return out;
}

void
nipp_pack(uint8_t *m, int offset, int lenval, unsigned int val)
{
	int endoffset, mask, toset, val2set;
	int offbit, offbyte;

    endoffset = offset + lenval;

    while (offset < endoffset)
	{
        offbit = offset % 8;
        offbyte = offset / 8;
        mask=0;

        // compute toset. it's 8-offbit unless that goes beyond endoffset
        toset = 8-offbit;
        if (offset + toset > endoffset)
            toset = endoffset - offset;

        val2set = val >> (endoffset - offset - toset);
        // now recompute offbit based on toset
        offbit = 8 - (offbit + toset);

        if (offbit > 0)
            mask = (1<<offbit) - 1;
        if (toset + offbit != 8)
            mask = mask | (~((1<< (toset + offbit)) - 1));
        m[offbyte] = (m[offbyte] & mask) | ((val2set << offbit) & (~mask & 0xFF));
        // val = val >> toset
        offset = offset + toset;
	}
}
