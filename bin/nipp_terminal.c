#include <stdlib.h>
#include <unistd.h>
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
static unsigned my_id = 0x50;	/* Should be an arg */
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

static void open_tty( void )
{
	packet_fd = open( tty_name, O_RDWR );
	if( packet_fd >= 0 ) return;
	perror( tty_name );
	exit( EXIT_FAILURE );
}
	

static void setup_tty( void )
{
	struct termios t;
	
	if( tcgetattr( packet_fd, &t ) < 0 ) goto error;
	cfmakeraw( &t );
	t.c_cflag |= CLOCAL;		/* ignore modem signals, should be arg */
	if( cfsetspeed( &t, B115200 ) < 0 ) goto error;
	
	if( tcsetattr( packet_fd, TCSANOW, &t ) < 0 ) goto error;
	
	return;
	
error:	perror( tty_name );
	exit( EXIT_FAILURE );	
}
		

static void print_ascii_packet( nipp_message_t *m )
{
	int len = NIPP_LENGTH(m);
	
	if( fwrite( NIPP_DATA(m), sizeof(uint8_t), len, stdout ) != len ) 
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
	if( len > 0 ) add_history( line );
	waiting = 1;	/* Can't send more until we get a response */
	while( waiting ) read_packet();
}

	

static void poll( void )
{
	fd_set rf, ef;
	int n;
	
	for(;;) {
		FD_ZERO(&rf);
		FD_ZERO(&ef);
		if( !waiting ){
			FD_SET( 0, &rf );
			FD_SET( 0, &ef );
		}
		FD_SET( packet_fd, &rf );
		FD_SET( packet_fd, &ef );
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
	open_tty();
	if( isatty( packet_fd )) setup_tty();
	nipp_attach( packet_fd );
	rl_callback_handler_install( prompt, line_handler );
	poll();
	return EXIT_SUCCESS;
}
