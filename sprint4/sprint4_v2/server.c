#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>

/**
* This version allows a server to recieve a message and print it
* Run the program : gcc -o server server.c && ./server PORT
*/

#define MAX_SOCKETS 100
#define MAX_NAME_LENGTH 20
#define MAX_BUFFER_LENGTH 256
#define MAX_LIST_LENGTH 1200
#define JOINER_LENGTH 4
#define JOINER " - "
#define FILE_PROTOCOL_LENGTH 10
#define FILE_PROTOCOL "-FILE-"
#define MAX_CHANNELS 10

/* Global sockets array initialize with zeros */
int sockets[MAX_SOCKETS] = {0};
int choiceChannel[MAX_SOCKETS] = {-1};
int nbChannels = 0;
int continueMenu = 1;

/* Create struct to share socket with username to threads */
struct sockets_struct
{
    char clientUsername[MAX_NAME_LENGTH];
    int numConnectedChannel;
    int socket;
};
struct sockets_struct clientStruct;

struct channel_struct{
    int numChannel;
    int maxClients;
	char name[MAX_NAME_LENGTH];
	char description[MAX_BUFFER_LENGTH];
	int nbClientConnected;
};
struct channel_struct channels[MAX_CHANNELS];

/* Print socket state */
void psockets(){
    printf("SOCKETS : ");
    for(int i = 0; i < MAX_SOCKETS; i++){
        printf("%d ",sockets[i]);
    }
    printf("\n");
}

int get_index_in_array (int chosenCh){

    if(chosenCh == 0){
        return 0;
    } else {
        int i;
        int indexArray=0;
        for (i=0;i<chosenCh;i++){
            indexArray += channels[i].maxClients;
        }
        return indexArray;
    }

}

/* Append new socket to sockets array */
int add_socket(int sockets[], int socket, int chosenCh){
    int i = get_index_in_array(chosenCh);
    int index = i + channels[chosenCh].maxClients;
    printf("max : %d\n",index);
    while(i < index  && sockets[i] != 0){
        i++;
    }
    if(i == index){
        /* End of array, no more space */
        perror("! Channel array full !\n");
        return -1;
    }
    else{
        printf("insert at : %d\n",i);
        /* There is space for new socket */
        sockets[i] = socket;
        channels[chosenCh].nbClientConnected += 1;
        return i; //return index of the socket;
    }
}

/* Remove socket to sockets array */
int remove_socket(int sockets[], int socket, int chosenCh){
        int i=get_index_in_array(chosenCh);
        int index = i + channels[chosenCh].maxClients;

        while(i < index && sockets[i] != socket){
            i++;
        }
        if(i == index){
        /* End of array, socket not found */
        perror("! Can't find socket !\n");
        return -1;
    }
    else{
        /* Socket found, need to remove it */
        sockets[i] = 0;
        channels[chosenCh].nbClientConnected -= 1;
        return 1;
    }
}


void init_channels(){
    for (int i = 0; i < 5; i++){
        channels[i].numChannel=i;
        channels[i].nbClientConnected = 0;
        switch (i) {
            case 0:
                strcpy(channels[i].name,"Channel 1");
                strcpy(channels[i].description,"Join to speak about Java Project");
                channels[i].maxClients = 5;
              break;

            case 1:
                strcpy(channels[i].name,"Channel 2");
                strcpy(channels[i].description,"Join to speak about FAR Project");
                channels[i].maxClients = 4;
              break;

            case 2:
                strcpy(channels[i].name,"Channel 3");
                strcpy(channels[i].description,"Join to speak about Swift Project");
                channels[i].maxClients = 6;
              break;

            case 3:
                strcpy(channels[i].name,"Channel 4");
                strcpy(channels[i].description,"Join to speak about JS Project");
                channels[i].maxClients = 8;
              break;

            case 4:
              strcpy(channels[i].name,"Random");
              strcpy(channels[i].description,"Join to speak about everything");
              channels[i].maxClients = 9;
              break;

        }
    }
    nbChannels = 5;
}

/* Check if the given channel exists or isn't full
    Return its number if all it's ok, -1 otherwise */
int check_channel(char name[]){
    /* remove the last character */
    char *chname = strchr(name, '\n');
    *chname = '\0';

    for(int i = 0; i < MAX_CHANNELS; i++){
        if(strcmp(channels[i].name,name) == 0 && channels[i].nbClientConnected < channels[i].maxClients){
            return channels[i].numChannel;
        }
    }
    return -1;
}

/* Create thread which wait for client message and send it to all others clients */
void *thread_func(void *arg){

    /* Get client username and socket */
    struct sockets_struct *args = (void *)arg;
    char username[MAX_NAME_LENGTH];
    strcpy(username,args->clientUsername);
    int socketCli = args->socket;

    continueMenu=1;

    /* The number of the channel the client is connected in */
    int numChannel = args->numConnectedChannel;
    int index = get_index_in_array(numChannel);

    /* Create buffer for messages */
    char buffer[MAX_BUFFER_LENGTH];
    char joiner[JOINER_LENGTH];
    strcpy(joiner,JOINER);
    char message[MAX_BUFFER_LENGTH + 4 + MAX_NAME_LENGTH];
    char file_protocol[FILE_PROTOCOL_LENGTH] = FILE_PROTOCOL;

    /* Define some int */
    int rv;
    int sd;
    int i;
    int is_file = 0;
    int flip_flop = 0;

    while(1){
        /* Clean the buffer */
        memset(buffer, 0, sizeof(buffer));
        flip_flop = 0;

        /* Waiting for message from client */
        rv = recv(socketCli, &buffer, sizeof(buffer), 0);

        if(rv < 0){
            /* Connexion lost with client, need to remove his socket */
            remove_socket(sockets, socketCli,numChannel);
            break;
        }
        else if(rv == 0){

        }
        else{

            if(strcmp(buffer,"fin") == 0){
                //Disconnect client.
                remove_socket(sockets, socketCli,numChannel);
                continueMenu=0;
                break;
            }

            /* Switcher */
            if(strcmp(buffer, file_protocol) == 0){
                printf("File protocol detected !\n");
                if(is_file == 0){
                    is_file = 1;
                }
                else{
                    is_file = 0;
                    /* Flip flop */
                    flip_flop = 1;
                }
            }

            /* Check if it's a simple text message to format the sending message */
            if(flip_flop == 0 && is_file == 0){

                memset(message, 0, sizeof(message));
                strcat(message,username);
                strcat(message,joiner);
                strcat(message,buffer);

                /* Add here send messsage to only sockets in the channel */
                for(i = index ; i < index + channels[numChannel].maxClients; i++){
                    /* Only send on valid sockets and not to our client socket... */
                    if(sockets[i] != 0 && sockets[i] != socketCli){ //
                        /* Send message to client [i] */
                        while(sd = send(sockets[i], message, sizeof(message),0) <= 0){
                            /* Error sending message to client [i] */
                            if(sd < 0){
                                /* Because of connexion lost with client [i] need to remove his socket */
                                remove_socket(sockets, socketCli,numChannel);
                                break;
                            }
                        }
                    }
                }
            }
            /* Check if it's a file */
            else if(is_file == 1){
                for(i = index ; i < index + channels[numChannel].maxClients; i++){
                    /* Only send on valid sockets and not to our client socket... */
                    if(sockets[i] != 0 && sockets[i] != socketCli){
                        /* Send message to client [i] */
                        while(sd = send(sockets[i], buffer, sizeof(buffer),0) <= 0){
                            /* Error sending message to client [i] */
                            if(sd < 0){
                                /* Because of connexion lost with client [i] need to remove his socket */
                                remove_socket(sockets, socketCli,numChannel);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    pthread_exit(0);
}

void enter_channel(int socketCli){
    //Join a channel

    int sd,rc;
    /* Receive the chosen channel name */
    char chosenChannel[MAX_NAME_LENGTH];
    rc = recv(socketCli, &chosenChannel, sizeof(chosenChannel),0);
    if(rc <0){
        printf("! Error receiving chosen channel from client !\n");
    }

    int chosenCh;
    chosenCh = check_channel(chosenChannel);
    /* Check if the client can be connected to the chosen channel */
    while(chosenCh < 0){
        memset(chosenChannel, 0, MAX_NAME_LENGTH);
        /* Send the error message */
        sd = send(socketCli, &chosenCh, sizeof(chosenCh),0);
        if(sd < 0){
          printf("! Error sending the error channel message to client !\n");
        }

        rc = recv(socketCli, &chosenChannel, sizeof(chosenChannel), 0);
        if(rc < 0){
            printf("! Error receiving new chosen channel from client !\n");
        }

        chosenCh = check_channel(chosenChannel);

    }

    /* Send the num channel */
    sd = send(socketCli, &chosenCh, sizeof(chosenCh),0);
    if(sd < 0){
      printf("! Error sending the error channel message to client !\n");
    }

    /* The channel is available (id in chosenCh) */
    printf("Client enter in the channel\n");

    /* Add this new client to sockets tab */
    int indexNewSocket;
    if(indexNewSocket = add_socket(sockets,socketCli,chosenCh) != -1){
        psockets();

        /* Bind socket and username to structure */
        clientStruct.numConnectedChannel=chosenCh; //the num of the channel he's connected

        /* Create thread */
        pthread_t th;
        pthread_create(&th, NULL, thread_func, (void *)&clientStruct);

        /* Wait the son thread is terminated */
        pthread_join(th, NULL);
    }
}

void add_channel(int socketCli){
    //Add a channel

    int sd,rc;
    /* Receive the chosen channel name */
    char chosenName[MAX_NAME_LENGTH];
    rc = recv(socketCli, &chosenName, sizeof(chosenName),0);
    if(rc <0){
        printf("! Error receiving chosen channel from client !\n");
    }

    /* If the client don't want to abort */
    if(strcmp(chosenName,"/abort") != 0){
        /* Receive the new description */
        char chosenDescription[MAX_BUFFER_LENGTH];
        rc = recv(socketCli, &chosenDescription, sizeof(chosenDescription),0);
        if(rc <0){
            printf("! Error receiving chosen channel from client !\n");
        }

        /* Receive the max clients */
        int nbMaxCli;
        rc = recv(socketCli, &nbMaxCli, sizeof(nbMaxCli),0);
        if(rc <0){
            printf("! Error receiving chosen channel from client !\n");
        }

        /* Create a new channel */
        struct channel_struct newChan;
        newChan.numChannel=nbChannels;
        strcpy(newChan.name,chosenName);
        strcpy(newChan.description,chosenDescription);
        newChan.maxClients=nbMaxCli;
        newChan.nbClientConnected=0;

         /* Add it in the channels list */
        channels[nbChannels]=newChan;
        nbChannels+=1;

        printf("New channel created !\n");

    }
}

/* Method to delete a given numChannel to the channels list */
void delete_in_array(int numChannel){
    int i=numChannel;
    /* Replace the deleted channel by the next ones */
    while(i<nbChannels){
        channels[i]=channels[i+1];
        if(i!=nbChannels-1){
            channels[i].numChannel=i;
        }
        i++;
    }

}

void delete_channel(int socketCli){
    int sd,rc;
    char message[MAX_LIST_LENGTH]="";
    int i;

    /* Show the channel names */
    strcat(message,"\n\n");
    strcat(message,"------ Channel list ------");
    strcat(message,"\n");
    for(int i = 0; i < nbChannels; i++){
        strcat(message,channels[i].name);
        strcat(message,"\n");
    }
    strcat(message,"\n");

    /* Send the channel list */
    sd = send(socketCli, &message, sizeof(message),0);
    if(sd < 0){
        printf("! Error sending the channel list to client !\n");
    }

    /* Receive the chosen channel name */
    char chosenName[MAX_NAME_LENGTH];
    rc = recv(socketCli, &chosenName, sizeof(chosenName),0);
    if(rc <0){
        printf("! Error receiving chosen channel from client !\n");
    }

    /* Check if chosen channel exists */
    int numCh = check_channel(chosenName);
    char resp[MAX_BUFFER_LENGTH];
    if(numCh < 0){
        strcat(resp,"Channel doesn't exist");
    } else if(channels[numCh].nbClientConnected != 0){
        strcat(resp,"You can't delete a channel with clients connected in.");
    } else {
        delete_in_array(numCh);
        nbChannels-=1;
        strcat(resp,"Channel deleted");
    }

    /* Send the response to remove a channel */
    sd = send(socketCli, &resp, sizeof(message),0);
    if(sd < 0){
        printf("! Error sending the resp to remove a channel to client !\n");
    }

}

void update_attr_channel(char chosenName[], int numChannel, int socketCli){
    int rc, sd;

    /* Switch case of client choice */
    /* Change name */
    if(strcmp(chosenName,"/name")==0){
        char channelName[MAX_NAME_LENGTH];
        rc = recv(socketCli,&channelName,sizeof(channelName),0);
        if (rc < 0){
         perror("! Error with receiving the new name of the updated channel !");
        }
        char *chfin = strchr(channelName, '\n');
        *chfin = '\0';

        strcpy(channels[numChannel].name,channelName);

        printf("Channel name is updated!\n");

    }
    /* Change Description */
    else if(strcmp(chosenName,"/description")==0){
        char channelDescription[MAX_BUFFER_LENGTH];
        rc = recv(socketCli,&channelDescription,sizeof(channelDescription),0);
        if (rc < 0){
         perror("! Error with receiving the new description of the updated channel !");
        }

        char *chfin = strchr(channelDescription, '\n');
        *chfin = '\0';

        strcpy(channels[numChannel].description,channelDescription);

        printf("Channel description is updated!\n");

    }
    /* Change max clients */
    else if(strcmp(chosenName,"/maxClients")==0){
        int channelMaxCli;
        rc = recv(socketCli,&channelMaxCli,sizeof(channelMaxCli),0);
        if (rc < 0){
         perror("! Error with receiving the new max clients of the updated channel !");
        }

        channels[numChannel].maxClients=channelMaxCli;

        printf("Channel max clients is updated!\n");

    } else { /* Error */
        printf("! Error, invalid command !\n");
    }

}

void update_channel(int socketCli){
    int sd,rc;
    int i;

    /* Receive the chosen channel name */
    char chosenName[MAX_NAME_LENGTH];
    rc = recv(socketCli, &chosenName, sizeof(chosenName),0);
    if(rc <0){
        printf("! Error receiving chosen channel from client !\n");
    }

    /* Check if the channel exists */
    int numCh = check_channel(chosenName);
    char resp[MAX_BUFFER_LENGTH];
    int canUp;
    if(numCh < 0){ //if channel doesn't exist
        canUp = -1;
        strcat(resp,"Channel doesn't exist");
    } else if(channels[numCh].nbClientConnected != 0){ //if it contains clients
        canUp = 0;
        strcat(resp,"You can't update a channel with clients connected in.");
    } else {
        canUp=1;
        strcat(resp,"You can update the channel.");
    }

    /* Send the response to update a channel */
    sd = send(socketCli, &canUp, sizeof(canUp),0);
    if(sd < 0){
        printf("! Error sending the resp to update a channel to client !\n");
    }

    /* Update channel if it can be updated */
    if(canUp==1){
        memset(chosenName,0,MAX_NAME_LENGTH);
        rc = recv(socketCli, &chosenName, sizeof(chosenName),0);
        if(rc <0){
            printf("! Error receiving chosen attribute from client !\n");
        }

        update_attr_channel(chosenName,numCh,socketCli);
    }


}

void *thread_chan(void *arg){

        /* Get client username and socket */
        struct sockets_struct *args = (void *)arg;
        char username[MAX_NAME_LENGTH];
        strcpy(username,args->clientUsername);
        int socketCli = args->socket;

        int sd,rc;

    while(continueMenu==1){
        /* Creation of the channels list */
        char message[MAX_LIST_LENGTH]="";
        strcat(message,"\n\n");
        strcat(message,"------ Channel list ------");
        strcat(message,"\n");
        for(int i = 0; i < nbChannels; i++){
            char nbC[2];
            int nbCoInt = channels[i].nbClientConnected;
            sprintf(nbC,"%d",nbCoInt);
            char nbMax[2];
            sprintf(nbMax,"%d",channels[i].maxClients);

            strcat(message,channels[i].name);
            strcat(message," : ");

            if(nbCoInt < channels[i].maxClients-2){
            //green : the channel isn't full
                char nbCh[] = "\033[00;32m[";
                strcat(nbCh,nbC);
                strcat(nbCh,"/");
                strcat(nbCh,nbMax);
                strcat(nbCh,"]\033[0m");

                strcat(message,nbCh);

            } else if(nbCoInt >= channels[i].maxClients-2 && nbCoInt < channels[i].maxClients){
            //orange : only a few places (2)
                char nbCh[] = "\033[0;33m[";
                strcat(nbCh,nbC);
                strcat(nbCh,"/");
                strcat(nbCh,nbMax);
                strcat(nbCh,"]\033[0m");

                strcat(message,nbCh);

            } else {
            //red : the channel is full
                char nbCh[] = "\033[0;31m[";
                strcat(nbCh,nbC);
                strcat(nbCh,"/");
                strcat(nbCh,nbMax);
                strcat(nbCh,"]\033[0m");

                strcat(message,nbCh);

            }

            strcat(message,"\n");
            strcat(message,channels[i].description);
            strcat(message,"\n\n");
        }

        /* Send the channel list */
        sd = send(socketCli, &message, sizeof(message),0);
        if(sd < 0){
            printf("! Error sending the channel list to client !\n");
        }
        memset(message,0,MAX_LIST_LENGTH);

        /* Receive the chosen command number */
        int numChoice;
        rc = recv(socketCli, &numChoice, sizeof(numChoice),0);
        if(rc <0){
            printf("! Error receiving chosen channel from client !\n");
        }

        switch (numChoice){
            case 1 :
                enter_channel(socketCli);
                break;

            case 2 :
                add_channel(socketCli);
                break;

            case 3 :
                delete_channel(socketCli);
                break;

            case 4 :
                update_channel(socketCli);
                break;

            default :
                printf("! Error, invalid command enter by the client !\n");

        }
    }

    pthread_exit(0);
}


/* MAIN */
int main(int argc, char *argv[]){

    /* Checking args */
    if(argc != 2){
		perror("! I need to be call like -> :program PORT !\n");
		exit(1);
	}

    /* Define some variables */
    int tmp;
    char username[MAX_NAME_LENGTH];
    int rc,sd;

    /* Define target (ip:port) with calling program parameters */
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int dS, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[MAX_BUFFER_LENGTH];
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AF_WANPIPE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully bind(2).
    If socket() (or bind()) fails, we (close the socket
    and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        dS = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (dS == -1){
            continue;
        }
        if (bind(dS, rp->ai_addr, rp->ai_addrlen) == 0){
            /* Success */
            break;
        }
        close(dS);
    }

    if (rp == NULL) {
        /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    /* Starting the server */
    listen(dS,MAX_SOCKETS);
    printf("Start server on port : %s\n", argv[1]);

    init_channels();

    while(1){
        /* Clean username buffer */
        memset(username, 0, sizeof(username));

        /* Create addr for a clients */
        struct sockaddr_in addrCli;
        socklen_t lg = sizeof(struct sockaddr_in);

        /* Accept the connexion from client */
        int socketCli = accept(dS, (struct sockaddr*)&addrCli,&lg);

        if(socketCli > 0){
            printf("\033[0;32mConnexion established with client : %s:%d \033[0m\n",inet_ntoa(addrCli.sin_addr),ntohs(addrCli.sin_port));

            /* Waiting for the username */
            rc = recv(socketCli, &username, sizeof(username), 0);
            if (rc< 0){
                printf("! Error receiving username from client !\n");
            }

            /* Bind socket and username to structure */
            strcpy(clientStruct.clientUsername,username); //:username (but username doesn't work...)
            clientStruct.socket = socketCli;

            /* Create thread */
            pthread_t thread;
            pthread_create(&thread, NULL, thread_chan, (void *)&clientStruct);

        }
    }

    /* Close connexion */
    close(dS);
    printf("Stop server\n");
    return 1;
}