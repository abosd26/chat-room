#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
//define hostent
#include<netdb.h>
#include<pthread.h>

struct MsgUnit{
	char src[20];
	char room[20];
	char msg[256];
};
typedef struct MsgUnit mu;
void ShowError(const char* errMsg);
void ChatRoom(int sock);
void *ForSend(void *param);
void *ForReceive(void *param);
char myName[20];
char myRoom[20];
int main(int argc, char* argv[]){
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	//hostent defines a host computer on the Internet
	struct hostent* server;
	char buffer[256];

	if(argc < 3){
		fprintf(stderr, "usage %s hostname port!\n", argv[0]);
		exit(0);
	}
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		ShowError("Error opening socket!");
	}
	server = gethostbyname(argv[1]);
	if(server == NULL){
		fprintf(stderr, "Error, no such host!\n");
		exit(0);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	//copy server->h_addr to &serv_addr.sin_addr.s_addr
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	//connect system call, to establish a connection to the server(need ip and port #), returns 0 on success and -1 if it fails
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		ShowError("Error connecting!");
	}
	ChatRoom(sockfd);
	return 0;
}
void ShowError(const char* errMsg){
	perror(errMsg);
	exit(1);
}
void ChatRoom(int sock){
	int n;
	char buffer[256], allUser[1000];

	printf("[ Welcome! ]\n");
	do{
		printf("\tPlease enter your name: ");
		bzero(buffer, sizeof(buffer));
		fgets(buffer, 255, stdin);
	}while(!strtok(buffer, "\n"));
	bzero(myName, sizeof(myName));
	strcpy(myName, buffer);
	n = write(sock, buffer, sizeof(buffer));
	//user list
	printf("[ Member list ]\n");
	bzero(allUser, sizeof(allUser));
	n = read(sock, allUser, sizeof(allUser));
	printf("%s", allUser);
	printf("-------------------------------\n");
	do{
		printf("\tPlease enter group name(if doesn't exist, then create): ");
		bzero(buffer, sizeof(buffer));
		fgets(buffer, 255, stdin);
	}while(!strtok(buffer, "\n"));
	bzero(myRoom, sizeof(myRoom));
	strcpy(myRoom, buffer);
	n = write(sock, buffer, sizeof(buffer));
	printf("\nUsage:\t<Message>\n\t/W <Name or room> <Message>\n\tBye\n");
	printf("-------------------------------\n");
	//create 2 threads responsible for send and receive
	pthread_t sendThread, receiveThread;
	pthread_create(&sendThread, NULL, ForSend, &sock);
	pthread_create(&receiveThread, NULL, ForReceive, &sock);
	//wait for threads finish their tasks
	pthread_join(sendThread, NULL);
	pthread_join(receiveThread, NULL);
}
void *ForSend(void *param){
	char buffer[256];
	int *sock = (int *)param, n;
	while(1){
		bzero(buffer, sizeof(buffer));
		//if client doesn't want to leave, keep printing header
		printf("%s > ", myName);
		//wait for client input
	        fgets(buffer, sizeof(buffer), stdin);
		mu newMsg;
		if(strtok(buffer, "\n")){
        		strcpy(newMsg.msg, buffer);
		}
		else{
			strcpy(newMsg.msg, "");
		}
		strcpy(newMsg.src, myName);
		strcpy(newMsg.room, myRoom);
        	n = write(*sock, &newMsg, sizeof(mu));
		//if client say Bye, then exit the thread
		if(strcmp(newMsg.msg, "Bye") == 0){
			break;
		}
	}
}
void *ForReceive(void *param){
	char buffer[256];
        int *sock = (int *)param, n;
	while(1){
		mu newMsg;
		//wait for message from server
		n = read(*sock, &newMsg, sizeof(mu));
		//the source of the message is from the other client
		if(strcmp(newMsg.src, "server") != 0){
			printf("\"%s : %s\"\n%s > ", newMsg.src, newMsg.msg, myName);
			fflush(stdout);
		}
		//the source of the message is from server
		else{
			//if client say Bye and receive ack from server, then close the socket and exit the thread
			if(strcmp(newMsg.msg, "ack") == 0){
				close(*sock);
				break;
			}
			else{
				printf("%s\n%s > ", newMsg.msg, myName);
				fflush(stdout);
			}
		}
	}
}
