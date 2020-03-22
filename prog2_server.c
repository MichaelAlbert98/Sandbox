/* CS 367 Boggle
 * Authors: Michael Albert, Jim Riley
 * Created October 16, 2019
 * Modified Novemeber 02, 2019
 * prog2_server.c - code for server program that uses TCP to play boggle
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "trie.h"

void roundloop(int sd2, int sd3, uint8_t *board);
bool turnloop(int p1, int p2, uint8_t *board, struct TrieNode *pastguesses);
void generateboard(uint8_t *board, uint8_t boardlen);
int checkguess(uint8_t *word, uint8_t wordlen, uint8_t *board, uint8_t boardlen, struct TrieNode *pastguesses);
void Send(int p1, int p2, void *msg, int msglen, int flag);
bool Recv(int p1, int p2, void *msg, int msglen, int flag);

#define QLEN 10 /* size of request queue */
int visits = 0; /* counts client connections */
uint8_t boardlen, roundnum, roundtime, p1score, p2score; /* game logic vars */
bool timeout;
struct TrieNode *dictionary;
char vowels[5] = {'a', 'e', 'i', 'o', 'u'}; /* vowels */
char yes[1] = {'Y'};
char no[1] = {'N'};

/*------------------------------------------------------------------------
* Program: prog2_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for two clients to connect
* (2) begin a game of boggle with the client(s)
* 	(2.2) close the connection when one player wins/disconnects
* (3) go back to step (1)
*
* Syntax: ./prog2_server port board seconds dictionary
*
* port - protocol port number to use
* board - one byte unsigned integer for size of game board
* seconds - one byte unsigned integer for seconds per turn
* dictionary - path to dictionary of valid words
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad1; /* structure to hold client's address */
	struct sockaddr_in cad2; /* structure to hold client's address */
  pid_t  cpid;
	int sd, sd2, sd3; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	char playernum = '1'; /* player number */

	if (argc != 5) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port board_size, seconds_per_round, word_dictionary\n");
		exit(EXIT_FAILURE);
	}

	if (atoi(argv[2]) > 255 || atoi(argv[2]) < 1) {
		fprintf(stderr,"Error: Board must be between 0-255 letters\n");
		exit(EXIT_FAILURE);
	}

	if (atoi(argv[3]) > 255 || atoi(argv[3]) < 1) {
		fprintf(stderr,"Error: Time limit must be between 0-255 seconds\n");
		exit(EXIT_FAILURE);
	}

	/* Read in word dictionary into trie */
	dictionary = getNode();
	char const* const fileName = argv[4];
  FILE* file;
	if ((file = fopen(fileName, "r")) == NULL) {
		fprintf(stderr,"Error: File not found\n");
		exit(EXIT_FAILURE);
	}
  char line[255];
  while (fgets(line, sizeof(line), file)) {
		line[strcspn(line, "\r\n")] = 0;
		insert(dictionary, line);
  }
  fclose(file);
	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	boardlen = (uint8_t)atoi(argv[2]); /* convert argument to uint */
	roundtime = (uint8_t)atoi(argv[3]); /* convert argument to uint */
	uint8_t board[boardlen]; /* declare game board */
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

	/* Set timeout value for recv */
	struct timeval tv;
	tv.tv_sec = roundtime;
	tv.tv_usec = 0;
	setsockopt(sd, SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(struct timeval));

	/* Main server loop - accept and handle requests */
	while (1) {

		/* Accept first client's request */
		alen = sizeof(cad1);
		while ((sd2=accept(sd, (struct sockaddr *)&cad1, &alen)) < 0) {
			if (errno != EAGAIN) {
				fprintf(stderr, "Error: Accept failed\n");
				exit(EXIT_FAILURE);
			}
		}
		Send(sd2,sd3,&playernum,sizeof(char),MSG_NOSIGNAL);
		Send(sd2,sd3,&boardlen,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(sd2,sd3,&roundtime,sizeof(uint8_t),MSG_NOSIGNAL);
		playernum++;

		/* Accept second client's request */
		alen = sizeof(cad2);
		while ((sd3=accept(sd, (struct sockaddr *)&cad2, &alen)) < 0) {
			if (errno != EAGAIN) {
				fprintf(stderr, "Error: Accept failed\n");
				exit(EXIT_FAILURE);
			}
		}
		Send(sd3,sd2,&playernum,sizeof(char),MSG_NOSIGNAL);
		Send(sd3,sd2,&boardlen,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(sd3,sd2,&roundtime,sizeof(uint8_t),MSG_NOSIGNAL);
		playernum--;
		cpid = fork(); /* Start a new process to do the job. */
		if (cpid < 0) {
			/* Fork wasn't successful */
			perror ("fork");
			return -1;
		}
		/* We are the child! */
		if (cpid == 0) {
		 	srand(time(NULL));
			close(sd);
			/* game loop */
			roundnum = 1;
			while (p1score != 3 && p2score != 3) {
				Send(sd2,sd3,&p1score,sizeof(uint8_t),MSG_NOSIGNAL);
				Send(sd2,sd3,&p2score,sizeof(uint8_t),MSG_NOSIGNAL);
				Send(sd3,sd2,&p1score,sizeof(uint8_t),MSG_NOSIGNAL);
				Send(sd3,sd2,&p2score,sizeof(uint8_t),MSG_NOSIGNAL);
				Send(sd2,sd3,&roundnum,sizeof(uint8_t),MSG_NOSIGNAL);
				Send(sd3,sd2,&roundnum,sizeof(uint8_t),MSG_NOSIGNAL);
				generateboard(board, boardlen);
				Send(sd2,sd3,&board,sizeof(uint8_t)*boardlen,MSG_NOSIGNAL);
				Send(sd3,sd2,&board,sizeof(uint8_t)*boardlen,MSG_NOSIGNAL);
				roundloop(sd2, sd3, board);
			}
			/* One last round of sends to make sure client scores are updated */
			Send(sd2,sd3,&p1score,sizeof(uint8_t),MSG_NOSIGNAL);
			Send(sd2,sd3,&p2score,sizeof(uint8_t),MSG_NOSIGNAL);
			Send(sd3,sd2,&p1score,sizeof(uint8_t),MSG_NOSIGNAL);
			Send(sd3,sd2,&p2score,sizeof(uint8_t),MSG_NOSIGNAL);
			exit(EXIT_SUCCESS);
			/* End game loop */
		}

		/* We are the parent! */
		else {
			close(sd2);
			close(sd3);
		}
	}
}

void roundloop(int sd2, int sd3, uint8_t *board) {
	bool p1turn;
	bool valid = true;
	struct TrieNode *pastguesses = getNode();

	/* Determine if p1 or p2 should start */
	if (roundnum % 2 == 1) {
		p1turn = true;
	}
	else {
		p1turn = false;
	}
	while (valid) {

		/* p1's turn */
		if (p1turn) {
			valid = turnloop(sd2, sd3, board, pastguesses);
			if (!valid) { p2score++; }
		}

		/* p2's turn */
		else {
			valid = turnloop(sd3, sd2, board, pastguesses);
			if (!valid) { p1score++; }
		}
		p1turn = !p1turn;
	}

	clear(pastguesses);
	return;
}

/* Tell players whose turn it is, receive guesses */
bool turnloop(int p1, int p2, uint8_t *board, struct TrieNode *pastguesses) {
	uint8_t wordlen;
	Send(p1,p2,yes,sizeof(uint8_t),MSG_NOSIGNAL);
	Send(p2,p1,no,sizeof(uint8_t),MSG_NOSIGNAL);

	/* Receive word length and word, check for timeouts */
	bool timeout = Recv(p1, p2, &wordlen, sizeof(uint8_t), 0);
	if (timeout) {
		return false;
	}
	uint8_t word[wordlen];
	timeout = Recv(p1, p2, &word, sizeof(uint8_t)*wordlen, 0);
	if (timeout) {
		return false;
	}
	word[wordlen] = '\0';

	/* Check sent word for validity */
	int validGuess = checkguess(word, wordlen, board, boardlen, pastguesses);

	/* Correct guess */
	if (validGuess == 1) {
		insert(pastguesses, word);
		uint8_t sendval = 1;
		Send(p1,p2,&sendval,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(p2,p1,&wordlen,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(p2,p1,&word,sizeof(uint8_t)*wordlen,MSG_NOSIGNAL);
	}

	/* Incorrect guess */
	else {
		uint8_t sendval = 0;
		Send(p1,p2,&sendval,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(p2,p1,&sendval,sizeof(uint8_t),MSG_NOSIGNAL);
		roundnum++;
		return false;
	}
	insert(pastguesses, word);
	return true;
}

/* Creates a board of N characters where at least 1 is a vowel */
void generateboard(uint8_t *board, uint8_t boardlen) {
	 bool isvowel = false;
	 for (int i = 0; i < boardlen; i++) {
		 board[i] = 'a' + (rand() % 26);
	 }
	 if (strpbrk(board,vowels) != NULL) {
		 isvowel = true;
	 }
	 /* Force generate vowel if none in first N-1 chars */
	 while (isvowel == false) {
		 board[boardlen-1] = 'a' + (rand() % 26);
		 if (strpbrk(board,vowels) != NULL) {
			 isvowel = true;
		 }
	 }
	 return;
}

/* Checks whether the user's input could be made from the board,
	 exists in the dictionary, and hasn't been guessed already. */
int checkguess(uint8_t *word, uint8_t wordlen, uint8_t *board, uint8_t boardlen, struct TrieNode *pastguesses) {
	/* Word does not exist or has previously been guessed */
	if (search(pastguesses, word) || !search(dictionary, word)) {
		return -1;
	}

	/* Make sure word can be formed from board */
	uint8_t letters[256] = {0};
	for (int i = 0; i < boardlen; i++) {
		letters[board[i]]++;
	}
	for (int i = 0; i < wordlen; i++) {
		if (letters[word[i]] == 0) {
			return -1;
		}
		letters[word[i]]--;
	}
	return 1;
}

/* Wrapper for send to minimize repeat code */
void Send(int p1, int p2, void *msg, int msglen, int flag) {
	int ret = send(p1,msg,msglen,flag);

	/* Client disconnected */
	if (ret <= 0) {
		close(p1);
		close(p2);
		exit(EXIT_FAILURE);
	}
	return;
}

/* Wrapper for recv to minimize repeat code */
bool Recv(int p1, int p2, void *msg, int msglen, int flag) {
	int ret = recv(p1,msg,msglen,flag);

	/* Client disconnected */
	if (ret <= 0 && errno != EAGAIN) {
		close(p1);
		close(p2);
		exit(EXIT_FAILURE);
	}

	/* Active player timed out */
	else if (ret <= 0) {
		uint8_t sendval = 0;
		Send(p1,p2,&sendval,sizeof(uint8_t),MSG_NOSIGNAL);
		Send(p2,p1,&sendval,sizeof(uint8_t),MSG_NOSIGNAL);
		roundnum++;
		return true;
	}
	return false;
}
