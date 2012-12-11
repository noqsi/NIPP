/*
 * Simple filter to convert ascii to nipp packets.
 *
 * Derived from HETE-2's ascii2ipp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


#include "nipp.h"

#define WORD 32	/* size in chars of a single keyword or datum */

static char word[ WORD ], nextword[ WORD ];
int array[ NIPP_MAX_LENGTH ];

static void getword()
{
	(void) scanf( " %31s", nextword );
	if( ferror( stdin )) {
		perror( "ascii2nipp" );
		exit( 1 );
	}
}

static void getentry()
{
	int *p = array;
	int *e = array + NIPP_MAX_LENGTH;
	
	strcpy( word, nextword );	/* get keyword */
	bzero(( char *) array, sizeof array );
	
	for(;;) {
		char *form;
		char *cp = nextword;
		int value;
		
		getword();
		if( feof( stdin )) break;
		
		if( !isdigit( *cp ) 
			&& '-' != *cp 
			&& '+' != *cp ) break;	/* not arg, keyword */
		
		if( *cp != '0' ) form = "%d";		/* decimal */
		else if( tolower( cp[ 1 ] ) == 'x' ) {	/* hex */
			cp += 2;	/* skip the 0x */
			form = "%x";
		}
		else form = "%o";	/* octal */
		
		(void) sscanf( cp, form, &value );
		
		if( p < e ) *p++ = value;
	}
}


int main(int argc, char *argv[] )
{
	bool command = 0;
	unsigned id = 0;
	unsigned sequence = 0;
	unsigned length = 0;
	unsigned function = 0;

	if (argc > 1)			/* no args */
	{
		fprintf (stderr, "Usage:  ascii2nipp < asciifile > nippfile (no options)\n");
		exit(0);
	}

	/*
	 * Attach to standard output (fd=1)
	 */

	nipp_attach( 1 );
	
	getword();	/* prime the pump */
	
	while( !feof( stdin )) {

		getentry();
				
		if( 0 == strcmp( word, "command" ))command = true;
		else if( 0 == strcmp( word, "id" )) id = array[0];
		else if( 0 == strcmp( word, "sequence" )) sequence = array[0];
		else if( 0 == strcmp( word, "length" )) {
		        length = array[0];

			if( length > NIPP_MAX_LENGTH ) {
				fprintf( stderr,
					"ascii2nipp: length too big: %d\n",
					length );
				exit( 1 );
			}
		}
		else if( 0 == strcmp( word, "function" )) function = array[0];
		else if( 0 == strcmp( word, "data" )) {
			int i;
			
			/* At this point, we have the complete message:
			 * get a buffer and send it.
			 */
			
			nipp_message_t *m = nipp_new_message( command, id,
       				sequence++, length, function );
			
			if( !m ) {
				fprintf( stderr, "nipp_new_message: error %d\n", 
					nipp_errno );
				exit( EXIT_FAILURE );
			}
;
			for( i = 0; i < length; i += 1 )
				((uint8_t *)NIPP_DATA(m))[i] = array[ i ];
		
			if( nipp_send( m )) {
				fprintf( stderr, "nipp_send: error %d\n", 
					nipp_errno );
				exit( EXIT_FAILURE );
			}
		}
		else {
			fprintf( stderr, 
				"ascii2nipp: unknown keyword: %s\n", word );
			exit( 1 );
		}
	}
	
	exit( 0 );
}
