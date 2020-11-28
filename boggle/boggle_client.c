/* CS 367 Boggle
 * Authors: Michael Albert, Jim Riley
 * Created October 16, 2019
 * Modified Novemeber 02, 2019
 * prog2_client.c - code for client program to play boggle
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

void printboard(char *board, uint8_t boardlen);
void Send(int sd, void *msg, int msglen, int flag);
void Recv(int sd, void *msg, int msglen, int flag);

/*------------------------------------------------------------------------
* Program: prog1_client
*
* Purpose: allocate a socket, connect to a server, take user input,
* and display an ongoing game of boggle.
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
	char playernum; /* player number */
	uint8_t boardlen; /* board size */
	uint8_t roundtime; /* seconds per round */

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
	Recv(sd, &playernum, sizeof(char), MSG_WAITALL);
	Recv(sd, &boardlen, sizeof(uint8_t), MSG_WAITALL);
	Recv(sd, &roundtime, sizeof(uint8_t), MSG_WAITALL);
	if (playernum == '1') {
		printf("You are Player %c... the game will begin when Player 2 joins...\n", playernum);
	}
	else {
		printf("You are Player %c...\n", playernum);
	}
	printf("Board size: %u\n", boardlen);
	printf("Seconds per turn: %u\n", roundtime);
	uint8_t p1score, p2score, turnend, roundnum, guesslen; /* game logic vars */
	char pturn; /* 'Y' if active, 'N' if inactive */
	char board[boardlen]; /* board to print */
	char guess[boardlen]; /* user input to send to sever */
	Recv(sd, &p1score, sizeof(uint8_t), MSG_WAITALL);
	Recv(sd, &p2score, sizeof(uint8_t), MSG_WAITALL);

	/* Loop until one player wins */
	while (p1score < 3 && p2score < 3) {
		turnend = 1;
		printf("Score is %d-%d\n", p1score, p2score);
		Recv(sd, &roundnum, sizeof(uint8_t), MSG_WAITALL);
		printf("Round %u...\n", roundnum);
		Recv(sd, board, sizeof(uint8_t)*boardlen, 0);
		printf("Board:");
		printboard(board, boardlen);
		while (turnend) {
			Recv(sd, &pturn, sizeof(char), 0);

			/* We are the active player */
			if (pturn == 'Y') {
				printf("Your turn, enter word: ");
				fflush(stdout);
				guesslen = read(STDIN_FILENO, guess, boardlen+1);
				while (guess[0] == '\n') {
					guesslen = read(STDIN_FILENO, guess, boardlen+1);
				}
				guesslen--; /* ignore newline */

				/* check for timeout */
				recv(sd, &turnend, sizeof(uint8_t), MSG_PEEK | MSG_DONTWAIT);
				if (turnend != 0) {
					Send(sd, &guesslen, sizeof(uint8_t),0);
					Send(sd, &guess, sizeof(uint8_t)*guesslen,0);
				}

				/* Figure out if word was valid or not */
				Recv(sd, &turnend, sizeof(uint8_t), MSG_WAITALL);
				if (turnend == 1) {
					printf("Valid word!\n");
				}
				else {
					printf("Invalid word!\n");
				}
			}

			/* We are the inactive player */
			else {
				printf("Please wait for opponent to enter word...\n");
				Recv(sd, &turnend, sizeof(uint8_t), MSG_WAITALL);

				/* Opponent entered valid word */
				if (turnend) {
					uint8_t word[turnend];
					Recv(sd, word, sizeof(uint8_t)*turnend, 0);
					word[turnend] = '\0';
					printf("Opponent entered \"%s\"\n", word);
				}

				/* Opponent entered invalid word */
				else {
					printf("Opponent lost the round!\n");
				}
			}
		}
		Recv(sd, &p1score, sizeof(uint8_t), MSG_WAITALL);
		Recv(sd, &p2score, sizeof(uint8_t), MSG_WAITALL);
	}

	/* Determine winner/loser */
	if ((p1score == 3 && playernum == '1') || (p2score == 3 && playernum == '2')) {
		printf("You won!\n");
	}
	else {
		printf("You lost!\n");
	}
	close(sd);
	exit(EXIT_SUCCESS);
}

/* Prints out a board of chars with spaces between them */
void printboard(char *board, uint8_t boardlen) {
	for (int i = 0; i < boardlen; i++) {
		printf(" %c", board[i]);
	}
	printf("\n");
	return;
}

/* Wrapper for send to minimize repeat code */
void Send(int sd, void *msg, int msglen, int flag) {
	int ret = send(sd,msg,msglen,flag);

	/* Socket closed */
	if (ret <= 0) {
		close(sd);
		exit(EXIT_FAILURE);
	}
	return;
}

/* Wrapper for recv to minimize repeat code */
void Recv(int sd, void *msg, int msglen, int flag) {
	int ret = recv(sd,msg,msglen,flag);

	/* Socket closed */
	if (ret <= 0) {
		close(sd);
		exit(EXIT_FAILURE);
	}
	return;
}
