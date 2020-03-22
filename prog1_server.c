/* CS 367 Hangman
 * Authors: Michael Albert, Jim Riley
 * Created October 04, 2019
 * Modified October 16, 2019
 * prog1_server.c - code for server program that uses TCP to play hangman
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int checkGuess(char *buf, char *secword, char *word, char *guesses, int wlen);
void addGuess(char *buf, char *guesses);

#define QLEN 10 /* size of request queue */
int visits = 0; /* counts client connections */

/*------------------------------------------------------------------------
* Program: prog1_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) play a game of hangman with the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./prog1_server port word
*
* port - protocol port number to use
* word - hidden word for client to guess
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
  pid_t  cpid;
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	int wlen; /* Length of secret word */
	uint8_t gleft; /* How many remaining guesses */
	char buf[1]; /* buffer for char the client sends */

	if (argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port word\n");
		exit(EXIT_FAILURE);
	}

	if (strlen(argv[2]) > 254) {
		fprintf(stderr,"Error: Word must be less than 255 letters\n");
		exit(EXIT_FAILURE);
	}

	/* Word must consist of lowercase alphabet */
	for (int i = 0; i < strlen(argv[2]); i++) {
		if (argv[2][i] < 97 || argv[2][i] > 122) {
			fprintf(stderr,"Error: Letters must be in the set {a,b,...,z}\n");
			exit(EXIT_FAILURE);
		}
	}

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	wlen =  strlen(argv[2]);
	gleft = wlen;
	char guesses[wlen]; /* Array of previous guesses */
	char secword[wlen]; /* Secret word */
	char word[wlen];
	gleft = wlen;
	/* Initialize data */
	for (int i = 0; i < wlen; i++) {
		guesses[i] = '\0';
		secword[i] = '_';
		word[i] = argv[2][i];
	}

	/* Main server loop - accept and handle requests */
	while (1) {
		alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		/* Start a new process to do the job. */
		signal(SIGCHLD,SIG_IGN);
		cpid = fork();
		if (cpid < 0) {
			/* Fork wasn't successful */
			perror ("fork");
			return -1;
		}
		/* We are the child! */
		if (cpid == 0) {
			close(sd);
			/* game loop */
			while (gleft > 0 && strncmp(word, secword, wlen) != 0) {
				send(sd2,&gleft,sizeof(uint8_t),0);
				send(sd2,secword,sizeof(secword),0);
				recv(sd2,buf,sizeof(uint8_t),0);
				gleft = gleft + checkGuess(buf, secword, argv[2], guesses, wlen);
				addGuess(buf, guesses);
			}
			if (strncmp(word, secword, wlen) == 0) {
				uint8_t x = 255;
				send(sd2,&x,sizeof(uint8_t),0);
				send(sd2,secword,sizeof(secword),0);
			}
			else if (gleft == 0) {
				uint8_t x = 0;
				send(sd2,&x,sizeof(uint8_t),0);
				send(sd2,secword,sizeof(secword),0);
			}
			exit(EXIT_SUCCESS);
			/* End game loop */
		}
		/* We are the parent! */
		close(sd2);
	}
}

/* Checks whether the user input exists in word. Decrements guesses remaining
	 if it does not exist or has previously been guessed. */
int checkGuess(char *buf, char *secword, char *word, char *guesses, int wlen) {
	if (strpbrk(word, buf) != NULL && strpbrk(guesses, buf) == NULL) {
		for (int i = 0; i < wlen; i++) {
			if (buf[0] == word[i]) {
				secword[i] = buf[0];
			}
		}
	}
	else if (strpbrk(word, buf) == NULL || strpbrk(guesses, buf) != NULL) {
		return -1;
	}
	return 0;
}

/* Adds a guess to the list of previously guessed words. */
void addGuess(char *buf, char *guesses) {
	if (strpbrk(guesses, buf) == NULL) {
		int i = 0;
		while (guesses[i] != '\0') {
			i++;
		}
		guesses[i] = buf[0];
	}
	return;
}
