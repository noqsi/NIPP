/*
 * Simple filter to print nipp messages.
 * Derived from HETE-2's ipp2ascii
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


#include "nipp.h"

void print_msg( nipp_message_t *m )
{
	int i, len = NIPP_LENGTH(m);
	char *err = "";
	
	if( NIPP_COMMAND(m)) printf("command\n");
	printf( "id\t\t%d\n", NIPP_ID(m));
	printf( "sequence\t%d\n", NIPP_SEQUENCE(m));
	if( len > NIPP_MAX_LENGTH ) err = "*** Bad Length ***";
	printf( "length\t\t%d\t%s\n", len, err );
	err = "";
	printf( "function\t%d\n", NIPP_FUNCTION(m));
	if( nipp_check_message(m)) printf( "*** Bad parity check ***\n");
	printf( "data" );
	for( i = 0; i < len; i += 1 ) {
		if( i % 10 ) putchar( '\t' );
		else putchar( '\n' );
		printf( "0x%.2x", NIPP_DATA(m)[i] );
	}
	
	printf( "\n\n" );
}




int main(int argc, char *argv[])
{
	nipp_message_t *m;
	
	if (argc > 1)		/* no args in this version */
	{
		fprintf (stderr, "Usage:  nipp2ascii < ippfile > asciifile (no options)\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Attach NIPP to standard input (fd=0)
	 */
	
	(void) nipp_attach( 0 );
	
	while( (m = nipp_get_message( NIPP_FOREVER )) ) print_msg( m );
				
	if( nipp_errno == 0 ) exit( EXIT_SUCCESS );
	
	fprintf( stderr, "nipp2ascii: error %d\n", nipp_errno );
	
	exit( EXIT_FAILURE );
}
