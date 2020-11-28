/* CS 367 Hangman
 * Authors: Michael Albert, Jim Riley
 * Created October 04, 2019
 * Modified October 11, 2019
 * prog1_client.c - code for client program that uses TCP to play hangman
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: prog1_client
*
* Purpose: allocate a socket, connect to a server, take user input,
* and display an ongoing game of hangman.
*
* Syntax: ./prog1_client server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
	uint8_t guessint; /* buffer for guesses left from the server */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	/* Repeatedly read data from socket and write to user's screen. */
	recv(sd, &guessint, sizeof(uint8_t), 0);
	char buf[guessint];
	n = recv(sd, buf, sizeof(buf), 0);
	buf[n] = '\0';
	while (guessint != 0 && guessint != 255) {
		/* Game is ongoing */
		printf("Board: %s (%d guesses left)\n", buf, guessint);
		printf("Enter guess: ");
		int in = getchar();
		char input = (char)in;
		while ((getchar()) != '\n'); /* Remove excess input */
		send(sd,&input,sizeof(uint8_t),0);
		recv(sd, &guessint, sizeof(uint8_t), 0);
		n = recv(sd, buf, sizeof(buf), 0);
		buf[n] = '\0';
	}
	/* Game is lost */
	if (guessint == 0) {
		printf("Board: %s\n", buf);
		printf("You lost\n");
	}
	/* Game is won */
	else if (guessint == 255) {
		printf("Board: %s\n", buf);
		printf("You won\n");
	}
	close(sd);
	exit(EXIT_SUCCESS);
}
