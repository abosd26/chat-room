#include<stdio.h>
//for exit(1)
#include<stdlib.h>
//for close
#include<unistd.h>
//for bzero
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>

struct connection{
	pthread_t th;
	int *fd;
	struct connection *next;
};
struct user{
	char name[20];
	int fd;
	struct user *next;
};
struct room{
	char name[20];
	struct room *next;
	struct user *start;
};
struct MsgUnit{
	char src[20];
	char room[20];
	char msg[256];
};
typedef struct MsgUnit mu;
typedef struct connection *cPtr;
typedef struct user *uPtr;
typedef struct room *rPtr;
rPtr roomList = NULL;
cPtr threadList = NULL;
void ShowError(const char* errMsg);
void *ThreadStuff(void *param);
void printAll();
void printAll(){
	rPtr cR = roomList;
	uPtr cU;

	while(cR){
		cU = cR->start;
		while(cU){
			printf("room : %s, name : %s\n", cR->name, cU->name);
			cU = cU->next;
		}
		cR = cR->next;
	}
}
int main(int argc, char* argv[]){
	int sockfd, portno, n, pid, opt = 1;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int test = 0;
	//cPtr threadList = NULL;

	if(argc < 2){
		fprintf(stderr, "Error, no port provided!\n");
		exit(1);
	}
	else if(argc == 2){
		//socket system call, return a socket file descriptor, param : AF_INET for Internet domain, SOCK_STREAM for TCP
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		//if socket call fails, it returns -1
		if(sockfd < 0){
			ShowError("Error opening socket!");
		}
		//clear the buffer to 0s
		bzero((char *)&serv_addr, sizeof(serv_addr));
		portno = atoi(argv[1]);
		//code for address family, constant
		serv_addr.sin_family = AF_INET;
		//server ip, INADDR_ANY will get it automatically
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		//server port #, converted from host byte order to network byte order
		serv_addr.sin_port = htons(portno);
		//set the address to be bind on socket later can be reuse
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		//bind system call, bind the socket to the address
		if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
			ShowError("Error on binding!");
		}
		//listen to the socket for connections, 2nd param : # of connections that can be handled
		listen(sockfd, 5);
		clilen = sizeof(cli_addr);
		//
		printf("Waiting for clients on port %d...\n", portno);
		while(1){
			int *newsockfdptr = (int *)malloc(sizeof(int));
			//accept system call, return a new file descriptor, all communication should be done using this descriptor
			int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
			if(newsockfd < 0){
				ShowError("Error on accept!");
			}
			*newsockfdptr = newsockfd;
			cPtr newThread = (cPtr)malloc(sizeof(struct connection));
			pthread_create(&(newThread->th), NULL, ThreadStuff, newsockfdptr);
			newThread->next = threadList;
                        threadList = newThread;
			newThread->fd = newsockfdptr;
			pthread_detach(newThread->th);
		}
		//close the binding socket and release resources allocate to thread before exit
		close(sockfd);
	}
	else{
		ShowError("wrong");
	}
	return 0;
}
void ShowError(const char* errMsg){
	perror(errMsg);
	exit(1);
}
void *ThreadStuff(void *param){
	int n;
	char buffer[256], allUser[1000], temp1[100], temp2[100], myName[20], myRoom[20];
	int *sock = (int *)param;
	uPtr newUser = (uPtr)malloc(sizeof(struct user));
	rPtr currentRoom;
	uPtr currentUser;

	//new user attr initialize
	newUser->next = NULL;
	newUser->fd = *sock;
	//read name from user
	bzero(buffer, 256);
	n = read(*sock, buffer, sizeof(buffer));
	if(n < 0){
		ShowError("Error reading from socket!");
	}
	strcpy(newUser->name, buffer);
	//sent all users online to user
	if(!roomList){
		n = write(*sock, "\tNull\n", sizeof("\tNull\n"));
	}
	else{
		bzero(allUser, sizeof(allUser));
		currentRoom = roomList;
		while(currentRoom){
			currentUser = currentRoom->start;
			while(currentUser){
				bzero(buffer, sizeof(buffer));
				bzero(temp1, sizeof(temp1));
				bzero(temp2, sizeof(temp2));
				strcpy(temp1, "\t");
				strcat(temp1, currentUser->name);
				strcpy(temp2, currentRoom->name);
				//text form for output
				strcpy(buffer, strcat(strcat(strcat(temp1, "\t["), temp2), "]\n"));
				strcat(allUser, buffer);
				currentUser = currentUser->next;
			}
			currentRoom = currentRoom->next;
		}
		n = write(*sock, allUser, sizeof(allUser));
	}
	//read room from user
	bzero(buffer, sizeof(buffer));
	n = read(*sock, buffer, sizeof(buffer));
	currentRoom = roomList;
        while(currentRoom){
		if(strcmp(currentRoom->name, buffer) == 0){
			newUser->next = currentRoom->start;
			currentRoom->start = newUser;
			break;
		}
                currentRoom = currentRoom->next;
        }
        if(!currentRoom){
		rPtr newRoom = (rPtr)malloc(sizeof(struct room));
		newRoom->next = roomList;
		newRoom->start = newUser;
		strcpy(newRoom->name, buffer);
                roomList = newRoom;
		//for print
		currentRoom = roomList;
        }
        bzero(myName, sizeof(myName));
        bzero(myRoom, sizeof(myRoom));
	strcpy(myName, newUser->name);
	strcpy(myRoom, currentRoom->name);
	//print the log in information at server
	mu *newMsg = (mu *)malloc(sizeof(mu));
	bzero(newMsg->msg, sizeof(newMsg->msg));
	strcpy(newMsg->msg, "client [");
        strcat(strcat(strcat(strcat(newMsg->msg, myName), "] log in room ["), myRoom), "]");
	printf("%s\n", newMsg->msg);
	//inform all clients that a new client has log in
	bzero(newMsg->msg, sizeof(newMsg->msg));
        strcpy(newMsg->msg, "[note : client [");
        strcat(strcat(strcat(strcat(newMsg->msg, myName), "] log in room ["), myRoom), "]]");
	bzero(newMsg->src, sizeof(newMsg->src));
        strcpy(newMsg->src, "server");
        currentRoom = roomList;
        while(currentRoom){
        	currentUser = currentRoom->start;
                while(currentUser){
                	//inform all except the log out one
                        if(strcmp(currentUser->name, myName) != 0){
                        	n = write(currentUser->fd, newMsg, sizeof(mu));
                        }
                        currentUser = currentUser->next;
                }
                currentRoom = currentRoom->next;
        }
	free(newMsg);
	while(1){
		mu *newMsg = (mu *)malloc(sizeof(mu));
		n = read(*sock, newMsg, sizeof(mu));
		fflush(stdout);
		//server inform all clients that a client is log out from the chatroom
		if(strcmp(newMsg->msg, "Bye") == 0){
			//print leave message at server
			bzero(newMsg->msg, sizeof(newMsg->msg));
                        strcpy(newMsg->msg, "client [");
                        strcat(strcat(newMsg->msg, newMsg->src), "] leave.");
                        printf("%s\n", newMsg->msg);
			//prepare to send the leaving message
			bzero(newMsg->msg, sizeof(newMsg->msg));
			strcpy(newMsg->msg, "[note : client [");
			strcat(strcat(newMsg->msg, newMsg->src), "] leave.]");
			bzero(newMsg->src, sizeof(newMsg->src));
			strcpy(newMsg->src, "server");
			currentRoom = roomList;
			uPtr beforeUser = NULL;
			uPtr tempBefore;
			uPtr tempCurrent;
			rPtr beforeRoom = NULL;
			rPtr tempBeforeRoom;
			rPtr tempRoom;
                        while(currentRoom){
                        	currentUser = currentRoom->start;
                                while(currentUser){
					//inform all except the log out one
                                	if(strcmp(currentUser->name, myName) != 0){
                                        	n = write(currentUser->fd, newMsg, sizeof(mu));
                                	}
					//also acknowledge the log out client to leave
					else{
						mu *leaveMsg = (mu *)malloc(sizeof(mu));
						strcpy(leaveMsg->src, "server");
						strcpy(leaveMsg->msg, "ack");
						n = write(currentUser->fd, leaveMsg, sizeof(mu));
						free(leaveMsg);
						//remember the leaving client position in DS
						tempBefore = beforeUser;
						tempCurrent = currentUser;
						tempBeforeRoom = beforeRoom;
						tempRoom = currentRoom;
					}
					beforeUser = currentUser;
                                        currentUser = currentUser->next;
                                }
				beforeRoom = currentRoom;
                                currentRoom = currentRoom->next;
                        }
			//close the connection of the leaving client + release the related resources
                        if(tempBefore && tempCurrent != tempRoom->start){
                        	tempBefore->next = tempCurrent->next;
                        }
                        else{
                        	tempRoom->start = tempCurrent->next;
				//if there isn't any client exist in the room, then release resources of the room
				if(!tempRoom->start){
					if(tempBeforeRoom){
						tempBeforeRoom->next = tempRoom->next;
					}
					else{
						roomList = tempRoom->next;
					}
					free(tempRoom);
				}
                        }
                        //exit the thread after closing the connection and releasing resources of this client
                        close(*sock);
			free(tempCurrent);
			free(newMsg);
			cPtr currentThread = threadList;
			cPtr beforeThread = NULL;
			while(currentThread){
				if(sock == currentThread->fd){
					if(beforeThread){
						beforeThread->next = currentThread->next;
					}
					else{
						threadList = currentThread->next;
					}
					free(currentThread->fd);
					free(currentThread);
					break;
				}
				beforeThread = currentThread;
				currentThread = currentThread->next;
			}
			break;
		}
		//server transmit the message to the specified client(group)
		else if(newMsg->msg[0] == '/' && newMsg->msg[1] == 'W' && newMsg->msg[2] == ' '){
			fflush(stdout);
			bzero(buffer, sizeof(buffer));
			char buffer2[256];
			bzero(buffer2, sizeof(buffer2));
			strcpy(buffer2, newMsg->msg);
			int counter = 0;
			char *temp = strtok(buffer2, " ");
			while(temp){
				if(counter == 1){
					strcpy(buffer, temp);
				}
				else if(counter == 2){
					break;
				}
				temp = strtok(NULL, " ");
				counter++;
			}
			if(temp){
				if(strcmp(buffer, temp) == 0){
					strcpy(newMsg->msg, strstr(newMsg->msg, temp) + strlen(temp) + 1);
				}
				else{
					strcpy(newMsg->msg, strstr(newMsg->msg, temp));
				}
			}
			else{
				strcpy(newMsg->msg, "");
			}
			//find the client(group) that need to be transmitted
			int find = 0;
			currentRoom = roomList;
			while(currentRoom){
				//if the object to be transmitted is a group
				if(strcmp(currentRoom->name, buffer) == 0){
					currentUser = currentRoom->start;
                                        while(currentUser){
                                                n = write(currentUser->fd, newMsg, sizeof(mu));
                                                currentUser = currentUser->next;
                                        }
					find = 1;

				}
				//if the object to be transmitted is a specified client
				else{
					currentUser = currentRoom->start;
					while(currentUser){
						if(strcmp(currentUser->name, buffer) == 0){
							n = write(currentUser->fd, newMsg, sizeof(mu));
							find = 1;
						}
						currentUser = currentUser->next;
					}
				}
				currentRoom = currentRoom->next;
			}
			//the name/room which the client want to transmit is not exist
			if(find == 0){
				bzero(newMsg->src, sizeof(newMsg->src));
				bzero(newMsg->msg, sizeof(newMsg->msg));
				strcpy(newMsg->src, "server");
				strcpy(newMsg->msg, "[error : name/room not found!]");
				n = write(*sock, newMsg, sizeof(mu));
			}
		}
		else{
			currentRoom = roomList;
			while(currentRoom){
				if(strcmp(currentRoom->name, myRoom) == 0){
					currentUser = currentRoom->start;
					while(currentUser){
						n = write(currentUser->fd, newMsg, sizeof(mu));
						currentUser = currentUser->next;
					}
					break;
				}
				currentRoom = currentRoom->next;
			}
		}
		free(newMsg);
	}
}
