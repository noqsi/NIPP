#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/errno.h>
#include "nipp.h"

static int fd;
static fd_set rfds;	// read
static fd_set efds;	// error

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
 * We have no recovery strategy at present
 */

void nipp_abort_rx( void )
{
	fprintf( stderr, "NIPP: communication problem, no recovery implemented.\n" );
	exit( EXIT_FAILURE );
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


int nipp_send_buffer( nipp_message_t *msg, unsigned bytes )
{
	void *buf = msg;
	unsigned n;
	
	while( bytes > 0 ) {
		n = write( fd, buf, bytes );
		
		if( n < 0 && errno != EINTR ) {
			nipp_errno = NIPP_EIO;
			free( msg );
			return -1;
		}
		
		bytes -= n;
		buf += n;
	}
	
	free( msg );
	return 0;
}

