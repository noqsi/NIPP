#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <termios.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include "nipp.h"


static int packet_fd;
static char *tty_name;
static unsigned my_id = 77;	/* Should be an arg */
static unsigned sequence = 0;
static char prompt[] = "LSE> ";	/* Should be arg */
static bool waiting = 0;	/* Indicates wait for command response */
static unsigned command_function = 1;	/* Should be an arg */

static void args( int argc, char *argv[] )	/* should use getopt here */
{
	if( argc != 2 ) {
		fprintf( stderr, "Usage: nipp_terminal /dev/some_device\n" );
		exit( EXIT_FAILURE );
	}
	
	tty_name = argv[1];
}

static void setup_tty( void )
{
	struct termios t;
	
	packet_fd = open( tty_name, O_RDWR );
	if( packet_fd < 0 ) goto error;

	if( tcgetattr( packet_fd, &t ) < 0 ) goto error;
	cfmakeraw( &t );
	if( cfsetspeed( &t, B115200 ) < 0 ) goto error;
	
	if( tcsetattr( packet_fd, TCSANOW, &t ) < 0 ) goto error;
	
	return;
	
error:	perror( tty_name );
	exit( EXIT_FAILURE );	
}
		

static void line_handler( char *line )
{
	unsigned len;
	nipp_message_t *m;
	
	if( !line ) exit( EXIT_SUCCESS );
	
	len = strlen( line );
	m = nipp_new_message( 1, my_id, sequence, len, command_function );
	if( !m ) {
		fprintf( stderr, "nipp_terminal: error %d\n", nipp_errno );
		return;
	}
	
	bcopy( line, NIPP_DATA(m), len );
	nipp_send( m );
	waiting = 1;	/* Can't send more until we get a response */
}


static void print_ascii_packet( nipp_message_t *m )
{
	int len = NIPP_LENGTH(m);
	
	if( fwrite( NIPP_DATA(m), sizeof(uint8_t), len, stdout ) != len ) 
		goto error;
	if( fwrite( "\n", sizeof(uint8_t), 1, stdout ) != 1) 
		goto error;
	return;
	
error:	
	perror( "fwrite" );
	exit( EXIT_FAILURE );
}


static void read_packet( void )
{
	nipp_message_t *m = nipp_get_message( 0 );
	
	if( !m ) {
		if( nipp_errno == NIPP_TIMEOUT )
			return;		/* Don't have a whole packet */
		fprintf( stderr, "nipp: error %d\n", nipp_errno );
		exit( EXIT_FAILURE );
	}
	
	if( NIPP_FUNCTION(m) == command_function ) {	/* command response */
		if( sequence != NIPP_SEQUENCE(m))
			fprintf( stderr, "sequence number mismatch\n" );
		print_ascii_packet( m );
		sequence = (sequence+1)&0x3fff;		/* force 14 bits */
		waiting = 0;
		return;
	}
	
	/* for now, just print anything else */
	
	print_ascii_packet( m );
}
	

static void poll( void )
{
	fd_set rf, ef;
	int n;
	char c;
	
	for(;;) {
		FD_ZERO(&rf);
		if( !waiting ) FD_SET( 0, &rf );
		FD_SET( packet_fd, &rf );
		FD_COPY( &rf, &ef );
		n = select( packet_fd + 1, &rf, 0, &ef, 0 );

		if( n < 0 ) {
			perror( "select" );
			exit( EXIT_FAILURE );
		}
		
		if( FD_ISSET( 0, &rf ) || FD_ISSET( 0, &ef )) 
			rl_callback_read_char();
				
		if( FD_ISSET( packet_fd, &rf ) || FD_ISSET( packet_fd, &ef ))
			read_packet();
	}
}

	
int main( int argc, char *argv[] )
{
	args( argc, argv );
	nipp_attach( packet_fd );
	if( isatty( packet_fd )) setup_tty();
	rl_callback_handler_install( prompt, line_handler );
	poll();
	return EXIT_SUCCESS;
}
