/* CS 367 Chat Room
 * Authors: Michael Albert, Jim Riley
 * Created November 05, 2019
 * Modified Novemeber 26, 2019
 * prog3_server.c - code for server program that uses TCP to host a chat room
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
#include <stdbool.h>

#define QLEN 10 /* size of request queue */
#define USERNUM 255 /* total number of particpants/observers */
#define TIMEOUT 60 /* timeout value for clients */
#define PUBLIC 1 /* flag for public messages */
#define PRIVATE 0 /* flag for private messages */

typedef struct participant {
	int fd; // client file descriptor
	int obsfd; // fd of observer, -1 if none
	char *username; // NULL if no username
} participant;

int Send(int fd, void *msg, int len, int flag, participant *partilist);
int Recv(int fd, void *msg, int len, int flag, participant *partilist);
int findsmallest(int *arr, size_t size);
void addparti(int fd, participant *partilist);
void removeparti(int fd, participant *partilist);
void removeobsfd(int fd);
int searchpartifd(int fd, participant *partilist);
int searchpartiuser(char *username, participant *partilist);
void getmessageobs(int fd, participant *partilist);
int isvalidobs(char *username, participant *partilist, int fd);
char* getmessageuser(int fd, int pos, participant *partilist);
void getmessagechat(int fd, int pos, participant *partilist);
int checkvalid(char *message, participant *partilist, char *recipient);
void sendprivate(int pos, char *message, participant *partilist);
int isvalidusername(char *username, participant *partilist);
void broadcast(char *username, participant *partilist);
void prependmessage(int pos, char *prepend, participant *partilist, int flag);
void selectadd(fd_set *set, participant *partilist, int *obsfds);
int bindsocket(int sd, struct sockaddr_in *sad, int port, struct protoent *ptrp, int optval);

char yes = 'Y', no = 'N', timed = 'T', inval = 'I';
int valid = 0, invalid = 1, taken = 2;
int obsfds[USERNUM]; /* array to hold observers */
int timeouts[USERNUM*2]; /* array to hold timeout values for participants/observers */
int particonnect = 0, obsconnect = 0; /* number of participants/observers */

/*------------------------------------------------------------------------
* Program: prog3_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for clients to connect
* (2) Receive valid username from client
* 	(2.2a) Add them as a participant to the chat room
*		(2.2b) Add them as an observer to the chat room
*		(2.2c) Disconnect them if the room is full
* (3) go back to step (1)
*
* Syntax: ./prog3_server port1 port2
*
* port1 - protocol port number for participants
* port2 - protocol port number for observers
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	fd_set read;
	struct sockaddr_in sad1, sad2; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd1, sd2; /* server socket descriptors */
	int client; /* client socket descriptor */
	int port1, port2; /* protocol port number */
	int alen; /* length of address */
	int retval; /* return for select */
	int optval = 1; /* boolean value when we set socket option */
  participant partilist[USERNUM]; /* array to hold participants */
	size_t size = sizeof(timeouts) / sizeof(timeouts[0]);
	struct timeval tv;

	if (argc != 3) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server particpant_port observer_port \n");
		exit(EXIT_FAILURE);
	}

	port1 = atoi(argv[1]); /* convert argument to binary */
	port2 = atoi(argv[2]); /* convert argument to binary */
	sd1 = bindsocket(sd1, &sad1, port1, ptrp, optval);
	sd2 = bindsocket(sd2, &sad2, port2, ptrp, optval);
	alen = sizeof(cad);

	/* initialize partilist and timeouts */
	for (int i = 0; i < USERNUM; i++) {
		partilist[i].fd = 0;
		partilist[i].obsfd = 0;
		partilist[i].username = NULL;
	}

	/* Main server loop - accept and handle requests */
	while (1) {
		FD_ZERO(&read);
		FD_SET(sd1, &read);
		FD_SET(sd2, &read);
		selectadd(&read, partilist, obsfds);
		int smallest = findsmallest(timeouts, size);

		/* Start clock */
		time_t start, end;
    start = time(&start);

		if (smallest == -1) {
			retval = select(USERNUM+5, &read, NULL, NULL, NULL);
		}
		else {
			tv.tv_sec = smallest;
			tv.tv_usec = 0;
			retval = select(USERNUM+5, &read, NULL, NULL, &tv);
		}
		/* End clock */
		end = time(&end);
		double time_taken = difftime(end, start); // in seconds

		for (int i = 0; i < USERNUM*2; i++) {
			if (timeouts[i] != 0 && timeouts[i]-time_taken <= 0) {
				removeparti(i+5, partilist);
				timeouts[i] = 0;
			}
			else if (timeouts[i] != 0) {
				timeouts[i] -= time_taken;
			}
		}

		/* Error in select */
		if (retval == -1) {
			perror("select()");
			break;
		}

		/* Select returned */
		for (int i = 3; i < 5+USERNUM*2; i++) {
			if (FD_ISSET(i, &read)) {
				/* i is participant */
				if (i == sd1) {
					client=accept(sd1, (struct sockaddr *)&cad, &alen);
					if (client < 0) {
						fprintf(stderr, "Error: Accept failed\n");
						exit(EXIT_FAILURE);
					}
					if (particonnect == 255) {
						Send(client, &no, sizeof(uint8_t), 0, partilist);
						close(client);
					}
					else {
						Send(client, &yes, sizeof(uint8_t), 0, partilist);
						particonnect++;
						addparti(client, partilist);
						timeouts[client-5] = 60;
					}
				}
				/* i is observer */
				else if (i == sd2) {
					client=accept(sd2, (struct sockaddr *)&cad, &alen);
					if (client < 0) {
						fprintf(stderr, "Error: Accept failed\n");
						exit(EXIT_FAILURE);
					}
					if (obsconnect == 255) {
						Send(client, &no, sizeof(uint8_t), 0, partilist);
						close(client);
					}
					else {
						Send(client, &yes, sizeof(uint8_t), 0, partilist);
						obsfds[obsconnect] = client;
						obsconnect++;
						timeouts[client-5] = 60;
					}
				}
				/* i is a client */
				else {
					int pos = searchpartifd(i, partilist);
					/* observer connecting to participant */
					if (pos == 255) {
						getmessageobs(i, partilist);
					}
					/* receiving message */
					else if (partilist[pos].username != NULL) {
						getmessagechat(i, pos, partilist);
					}
					/* negotiating username */
					else {
						char* test2 = getmessageuser(i, pos, partilist);
						if (test2 != NULL) {
	            partilist[pos].username = malloc(strlen(test2));
	            strcpy(partilist[pos].username, test2);
						}
					}
				}
			}
		}
	}
}



/* Wrapper for send to minimize repeat code */
int Send(int fd, void *msg, int len, int flag, participant *partilist) {
	int ret = send(fd,msg,len,flag);

	/* Client disconnected */
	if (ret <= 0) {
		removeparti(fd, partilist);
	}
	return ret;
}



/* Wrapper for recv to minimize repeat code */
int Recv(int fd, void *msg, int len, int flag, participant *partilist) {
	int ret = recv(fd,msg,len,flag);

	/* Client disconnected */
	if (ret <= 0 && errno != EAGAIN) {
		removeparti(fd, partilist);
	}
	return ret;
}



/* Finds the smallest value in an array */
int  findsmallest(int *arr, size_t size) {
	int smallest = TIMEOUT+1;
	for (int i = 0; i < size; i++) {
		if (arr[i] < smallest && arr[i] != 0) {
			smallest = arr[i];
		}
	}
	/* no current connections, select waits indefinitely */
	if (smallest == TIMEOUT+1) {
		smallest = -1;
	}
	return smallest;
}





/* Adds a participant to the array of current participants */
void addparti(int fd, participant *partilist) {
	int i = 0;
	while (partilist[i].fd != 0) {
		i++;
	}
	partilist[i].fd = fd;
	return;
}



/* Removes a given fd from an array of participants if the fd is a participant
	 with a username, it returns the username, otherwise it returns NULL. */
void removeparti(int fd, participant *partilist) {
	int i = 0, pos = 0;
	while (i < USERNUM && partilist[i].fd != 0) {
		/* participant disconnect */
		if (partilist[i].fd == fd) {
			char *username = partilist[i].username;
			pos = i;
			if (username != NULL) {
				char *message = malloc(strlen(username)+strlen(" has left"));
				strcpy(message, username);
				strcat(message, " has left");
				broadcast(message, partilist);
				free(message);
			}
			if (partilist[i].obsfd != 0) {
				close(partilist[i].obsfd);
				removeobsfd(partilist[i].obsfd);
			}
			/* swap disconnected partilist with last in line to keep array contiguous */
			while (i < USERNUM && partilist[i].fd !=0) { i++; }
			if (pos != i-1) {
				partilist[pos].fd = partilist[i-1].fd;
				partilist[pos].obsfd = partilist[i-1].obsfd;
				if (partilist[i-1].username != NULL) {
					free(partilist[pos].username);
					partilist[pos].username = malloc(strlen(partilist[i-1].username));
					strcpy(partilist[pos].username, partilist[i-1].username);
				}
				else {
					partilist[pos].username = NULL;
				}
				partilist[i-1].fd = 0;
				partilist[i-1].obsfd = 0;
				free(partilist[i-1].username);
				partilist[i-1].username = NULL;
			}
			else {
				partilist[pos].fd = 0;
				partilist[pos].obsfd = 0;
				partilist[pos].username = NULL;
			}
			particonnect--;
			close(fd);
		}
		/* observer disconnect, make corresponding partipant open again */
		else if (partilist[i].obsfd == fd) {
			close(fd);
			removeobsfd(fd);
			partilist[i].obsfd = 0;
			return;
		}
		i++;
	}
	if (close(fd) == 0) {
		removeobsfd(fd);
	}
	return;
}



/* Removes a given fd from the global array of observers */
void removeobsfd(int fd) {
	int i = 0, pos = 0;
	while (i < USERNUM && obsfds[i] != 0) {
		if (obsfds[i] == fd) {
			pos = i;
		}
		i++;
	}
	if (i == 0) {
		obsfds[0] = 0;
	}
	else {
		obsfds[pos] = obsfds[i-1];
		obsfds[i-1] = 0;
	}
	obsconnect--;
	return;
}



/* Search for a given fd in the array of current participants */
int searchpartifd(int fd, participant *partilist) {
	int i = 0;
	while (i < USERNUM && partilist[i].fd != fd) {
		i++;
	}
	return i;
}



/* Search for a given username in the array of current participants */
int searchpartiuser(char *username, participant *partilist) {
	int i = 0;
	while (i < USERNUM && partilist[i].username != NULL && strcmp(partilist[i].username, username)) {
		i++;
	}
	if (partilist[i].username == NULL) {
		i = USERNUM;
	}
	return i;
}



/* Receive a username from an observer */
void getmessageobs(int fd, participant *partilist) {
	/* receiving message length */
	uint8_t msglen;
	int ret = Recv(fd, &msglen, sizeof(uint8_t), 0, partilist);
	ntohs(msglen);
	/* receiving message */
	char username[msglen];
	ret = Recv(fd, username, sizeof(username), MSG_WAITALL, partilist);
	if (ret > 0) {
		username[msglen] = '\0';
		int validity = isvalidobs(username, partilist, fd);
		if (validity == valid) {
			timeouts[fd-5] = 0;
			Send(fd, &yes, sizeof(uint8_t), 0, partilist);
			int i = 0;
			char *send = "A new observer has joined";
			uint16_t len = strlen(send);
			htons(len);
			while (i < USERNUM && partilist[i].fd != 0) {
				if (partilist[i].obsfd != 0) {
					Send(partilist[i].obsfd, &len, sizeof(uint16_t), 0, partilist);
					Send(partilist[i].obsfd, send, sizeof(uint8_t)*len, 0, partilist);
				}
				i++;
			}
		}
		/* invalid username, close observer */
		else if (validity == invalid) {
			removeobsfd(fd);
			Send(fd, &no, sizeof(uint8_t), 0, partilist);
			close(fd);
		}
		/* username is valid, but taken; reset timer */
		else if (validity == taken) {
			timeouts[fd-5] = TIMEOUT;
			Send(fd, &timed, sizeof(uint8_t), 0, partilist);
		}
	}
	return;
}



/* Check if username given by an observer is valid */
int isvalidobs(char *username, participant *partilist, int fd) {
	int i = 0;
	while (partilist[i].fd != 0) {
		if (partilist[i].username != NULL) {
			char *name = partilist[i].username;
			if (!strcmp(name, username) && partilist[i].obsfd == 0) {
				partilist[i].obsfd = fd;
				return valid;
			}
			else if (!strcmp(name, username)) {
				return taken;
			}
		}
		i++;
	}
	return invalid;
}



/* Get a username from a participant */
char* getmessageuser(int fd, int pos, participant *partilist) {
	/* receiving username length */
	uint8_t msglen;
	int ret = Recv(fd, &msglen, sizeof(uint8_t), 0, partilist);
	ntohs(msglen);
	/* receiving username */
	char username[msglen];
	ret = Recv(fd, username, sizeof(username), MSG_WAITALL, partilist);
	if (ret > 0) {
		username[msglen] = '\0';
		/* valid username */
		int validity = isvalidusername(username, partilist);
		if (validity == valid) {
			timeouts[fd-5] = 0;
			static char ret[11];
			strcpy(ret, username);
			Send(fd, &yes, sizeof(uint8_t), 0, partilist);
			char *message = malloc(strlen(username)+strlen(" has joined"));
			strcpy(message, username);
			strcat(message, " has joined");
			broadcast(message, partilist);
			free(message);
			return ret;
		}
		/* invalid username */
		else if (validity == invalid) {
			Send(fd, &inval, sizeof(uint8_t), 0, partilist);
		}
		/* client timed out */
		else if (validity == taken) {
			timeouts[fd-5] = TIMEOUT;
			Send(fd, &timed, sizeof(uint8_t), 0, partilist);
		}
	}
	return NULL;
}



/* Get a message from a participant */
void getmessagechat(int fd, int pos, participant *partilist) {
	/* receiving message length */
	uint16_t msglen;
	int ret = Recv(fd, &msglen, sizeof(uint16_t), 0, partilist);
	ntohs(msglen);
	/* disconnect if larger than 1000 */
	if (msglen > 1000) {
		removeparti(fd, partilist);
		return;
	}
	else {
		/* receiving message */
		char message[msglen];
		ret = Recv(fd, message, sizeof(message), MSG_WAITALL, partilist);
		/* check if there is a message to send */
		if (ret > 0) {
			char *result;
			message[msglen] = '\0';
			char prepend[14];
			/* private message */
			if (message[0] == '@') {
				char recipient[11];
				int recippos = checkvalid(message, partilist, recipient);
				/* recipient exists */
				if (recippos != 255) {
					prependmessage(pos, prepend, partilist, PRIVATE);
					result = malloc(strlen(prepend)+strlen(message));
					strcpy(result, prepend);
					strcat(result, message);
					sendprivate(pos, result, partilist);
					sendprivate(recippos, result, partilist);
					free(result);
				}
				/* recipient doesn't exist */
				else {
					result = malloc(strlen(recipient)+strlen("user  doesn't exist..."));
					strcpy(result, "user ");
					strcat(result, recipient);
					strcat(result, " doesn't exist...");
					sendprivate(pos, result, partilist);
					free(result);
				}
			}
			/* public message */
			else {
				prependmessage(pos, prepend, partilist, PUBLIC);
				result = malloc(strlen(prepend)+strlen(message));
				strcpy(result, prepend);
				strcat(result, message);
				broadcast(result, partilist);
				free(result);
			}
		}
	}
	return;
}



/* determine if a recipient is vaid */
int checkvalid(char *message, participant *partilist, char *recipient) {
	int i = 1;
	/* copy over recipient */
	while (message[i] != ' ' && i < 11) {
		recipient[i-1] = message[i];
		i++;
	}
	recipient[i-1] = '\0';
	int pos = searchpartiuser(recipient, partilist);
	return pos;
}


/* send a private message */
void sendprivate(int pos, char *message, participant *partilist) {
	uint16_t len = strlen(message);
	htons(len);
	if (partilist[pos].obsfd != 0) {
		Send(partilist[pos].obsfd, &len, sizeof(uint16_t), 0, partilist);
		Send(partilist[pos].obsfd, message, sizeof(uint8_t)*len, 0, partilist);
	}
	return;
}



/* Check if a username is valid */
int isvalidusername(char *name, participant *partilist) {
	int i = 0;
	char *username;
	while (partilist[i].username != NULL) {
		username = partilist[i].username;
		if (!strcmp(name, username)) {
			return taken;
		}
		i++;
	}
	for (int j = 0; j < strlen(name); j++) {
		// invalid characters
		if (name[j] != 95 && (name[j] < 48 || (name[j] > 57 && name[j] < 65) || (name[j] > 90 && name[j] < 97) || name[j] > 122)) {
			return invalid;
		}
	}
	return valid;
}



/* Send message to all observers */
void broadcast(char *message, participant *partilist) {
	uint16_t len = strlen(message);
	htons(len);
	int i = 0;
	while (i < USERNUM && partilist[i].fd != 0) {
		if (partilist[i].obsfd != 0) {
			Send(partilist[i].obsfd, &len, sizeof(uint16_t), 0, partilist);
			Send(partilist[i].obsfd, message, sizeof(uint8_t)*len, 0, partilist);
		}
		i++;
	}
	return;
}



/* Add 14 long prepend to a broadcast message */
void prependmessage(int pos, char *prepend, participant *partilist, int flag) {
	char *username = partilist[pos].username;
	int userlen = strlen(username);
	int i = 1;
	if (flag == PUBLIC) {
		prepend[0] = '>';
	}
	else if (flag == PRIVATE) {
		prepend[0] = '-';
	}
	prepend[12] = ':';
	prepend[13] = ' ';
	while (i <= 11-userlen) {
		prepend[i] = ' ';
		i++;
	}
	int tmp = i;
	while (i < 12) {
		prepend[i] = username[i-tmp];
		i++;
	}
	return;
}



/* Re-add all currently being monitored file descriptors to a set */
void selectadd(fd_set *set, participant *partilist, int *obsfds) {
	int i = 0;
	/* Only works if you make sure partilist is contiguous */
	while (i < USERNUM && partilist[i].fd != 0) {
			FD_SET(partilist[i].fd, set);
			i++;
	}
	i = 0;
	while (i < USERNUM && obsfds[i] != 0) {
		FD_SET(obsfds[i], set);
		i++;
	}
	return;
}



/* Set up the initial connection for a socket */
int bindsocket(int sd, struct sockaddr_in *sad, int port, struct protoent *ptrp, int optval) {
	memset((char *)sad,0,sizeof(*sad)); /* clear sockaddr structure */
	sad->sin_family = AF_INET; /* set family to Internet */
	sad->sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	if (port > 0) { /* test for illegal value */
		sad->sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %d\n",port);
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
	if (bind(sd, (struct sockaddr *)sad, sizeof(*sad)) < 0) {
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
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sd, SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(struct timeval));

	return sd;
}
