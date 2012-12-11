#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <string.h>
#include "nipp.h"

static int fd;
static fd_set rfds;	// read
static fd_set efds;	// error

static uint8_t sync_bytes[] = {	// Sync as bytes in bigendian order
	(NIPP_SYNC>>24)&0xff,
	(NIPP_SYNC>>16)&0xff,
	(NIPP_SYNC>>8)&0xff,
	NIPP_SYNC&0xff };

/*
 * Attach NIPP to a particular file or socket.
 */

int nipp_attach( int f )
{
	FD_ZERO( &rfds );
	FD_ZERO( &efds );
	fd = f;
	return 0;
}

/*
 * Free the message buffer, don't send.
 */

void nipp_abort_tx( nipp_message_t *msg )
{
	if( msg ) (void) free( msg );
}

/*
 * This implementation does nothing for nipp_abort_rx().
 * There is no need, as nipp_check_sync() does the work.
 */

void nipp_abort_rx( void )
{
}

/*
 * Allocate a message buffer.
 */
 
nipp_message_t *nipp_outgoing( unsigned length )
{
	nipp_message_t *m = 
		calloc( length + NIPP_HEADER_LENGTH, sizeof(uint8_t));
	
	if( !m ) nipp_errno = NIPP_NOMEM;
	
	return m;
}

unsigned nipp_get_bytes( void *buffer, unsigned bytes, unsigned *timeout )
{ 
	struct timeval tm;
	int n;
	
	if( *timeout != NIPP_FOREVER ) do {
	
		tm.tv_sec = (*timeout)/1000000;
		tm.tv_usec = (*timeout)%1000000;
		
		FD_SET( fd, &rfds );
		FD_SET( fd, &efds );
		
		n = select( fd + 1, &rfds, 0, &efds, &tm );
		
		if( n < 0 && errno != EAGAIN && errno != EINTR) {
			nipp_errno = NIPP_EIO;
			return 0;
		}
		else if( n == 0 ) {
			nipp_errno = NIPP_TIMEOUT;
			return 0;
		}
	} while( n < 0 );
	
	// If we get here, we're either allowed to block or guaranteed not to.
	
	do {
		n = read( fd, buffer, bytes );
	} while( n < 0 && errno == EINTR );
	
	if( n < 0 ) {
		nipp_errno = NIPP_EIO;
		return 0;
	}
		
	return n;
}

/*
 * nipp_find_sync() sets nipp_errno to NIPP_BAD_SYNC if it
 * has to skip data to find sync. It normally returns zero
 * unless some other error (like a timeout) occurs.
 */	

int nipp_find_sync( unsigned *timeout )
{
	uint8_t b[4];	// Buffer to hold sync
	unsigned have = 0, n;
	
	nipp_errno = 0;			// Assume all will be well
	
	for(;;) {
		n = nipp_get_bytes( b + have, 4 - have, timeout );
		if( n == 0 ) return -1;		// Error
		have += n;
		if( have < 4 ) continue;	// need more bytes
		
		if( memcmp( b, sync_bytes, 4 ) == 0 ) return 0;	// Success
		
		nipp_errno = NIPP_BAD_SYNC;			// Bad news
		
		for( have = 3; have > 0; have -= 1 )
			if( b[4-have] == sync_bytes[0] ) break;	// Search
			
		for( n = 0; n < have; n +=1 ) 
			b[n] = b[ 4 - have + n ];		// Shift
	}
}


static int nipp_send_bytes( void *buf, unsigned bytes )
{
	int n;
	
	while( bytes > 0 ) {
		n = write( fd, buf, bytes );
		
		if( n < 0 && errno != EINTR ) {
			nipp_errno = NIPP_EIO;
			return -1;
		}
		
		bytes -= n;
		buf += n;
	}
	
	return 0;
}	


int nipp_send_buffer( nipp_message_t *msg, unsigned bytes )
{
	int n = nipp_send_bytes( msg, bytes );
	
	free( msg );
	
	return n;
}


int nipp_send_sync( void )
{
	return nipp_send_bytes( sync_bytes, 4 );
}
