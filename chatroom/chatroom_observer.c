/* CS 367 Chat Room Observer
* Authors: Michael Albert, Jim Riley
* Created November 04, 2019
* Modified Novemeber 26, 2019
* prog3_observer.c - code for client program to observe a chat room participant
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
#include <stdbool.h>


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

	char servreturn;
	Recv(sd, &servreturn, sizeof(uint8_t), 0);
  /* valid connection */
	if (servreturn == 'Y') {
		char username[1000];
		bool isvalidname = false;
		while (!isvalidname) {
			/* get username */
			fprintf(stderr, "Please input a username between 1-10 characters: ");
			fgets(username, 1000, stdin);
      if (!strchr(username, '\n')) {     //newline does not exist
        while (fgetc(stdin)!='\n');//discard until newline
      }
			uint8_t namelen = strlen(username);
			htons(namelen);
			namelen--; /* ignore null terminator */
			if (namelen <= 10 && namelen >= 1) {
				isvalidname = true;
			}
			fprintf(stderr, "\n");
			if (isvalidname) {
				Send(sd, &namelen, sizeof(uint8_t), 0);
				Send(sd, username, sizeof(uint8_t)*namelen, 0);
				Recv(sd, &servreturn, sizeof(uint8_t), 0);
				/* name valid */
				if (servreturn == 'Y') {
				}
				/* no active participant found */
				else if (servreturn == 'N') {
					close(sd);
          exit(EXIT_FAILURE);
				}
        /* name already has observer, time reset */
        else if (servreturn == 'T'){
					isvalidname = false;
        }
			}
		}
    /* receive messages from server until disconnected */
		while (1) {
			uint16_t msglen;
			Recv(sd, &msglen, sizeof(uint16_t), 0);
			ntohs(msglen);
      char message[msglen];
			Recv(sd, message, sizeof(uint8_t)*msglen, 0);
      /* append newline if none exists */
      if (message[msglen-1] != '\n') {
        message[msglen] = '\n';
        msglen++;
      }
      message[msglen] = '\0';
      fprintf(stdout, "%s", message);
		}
	}
/* invalid connection */
	else {
		close(sd);
		exit(EXIT_FAILURE);
	}
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
