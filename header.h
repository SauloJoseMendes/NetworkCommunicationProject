//
// Created by Saulo José Mendes on 21/03/2023.
//
#ifndef PROJETO_HEADER_H
#define PROJETO_HEADER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

//MACROS

#define TOPIC_IP "239.1.1.1"
#define SERVER_IP "193.137.100.1"
#define BUF_SIZE    5120
#define PASSWORD_LENGTH 30
#define USERNAME_LENGTH 30
#define TOPIC_NAME_LENGTH 100
#define MAX_USERS 5
#define TEXT_LENGTH 2000
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y)

//GLOBAL VARIABLES
pthread_mutex_t userListMutex,topicListMutex, idThreadMutex;
pthread_t myThreads[2];
pthread_t usersThreads[MAX_USERS];
int lastTopicPort, availableThreads[MAX_USERS];
//UDP socket
int s;
//TCP socket
int fd;

//USER LIST
typedef struct User{
    pthread_mutex_t myUserMutex;
    char username[USERNAME_LENGTH];
    char password[PASSWORD_LENGTH];
    char funcao[30];
    struct TopicList  * topicListSubscribed;
}User;

typedef struct UserListNode{
    User * user;
    struct UserListNode * ptr;
}UserListNode;

typedef struct UserList{
    UserListNode * start;
}UserList;


//TOPIC LIST
typedef struct Topic{
    char topicName[TOPIC_NAME_LENGTH];
    int id;
    char creatorUsername[USERNAME_LENGTH];
    struct sockaddr_in topicIPAddress;
    int socket;
}Topic;

typedef struct TopicNode{
    Topic * topic;
    struct TopicNode * ptr;
}TopicNode;

typedef struct TopicList{
    TopicNode * start;
}TopicList;

//STRUCT PARA AS THREADS
typedef struct ProgramCore{
    struct UserList * userList;
    struct TopicList * topicList;
    short PORTO_CONFIG;
    short PORTO_NOTICIAS;
    char * filename;

}ProgramCore;

typedef struct TCPThreadCore{
    int client;
    UserList * userList;
    TopicList * topicList;
    int threadPosition;
} TCPThreadCore;

int readTextFileConfig( UserList * list, char * filename);
int writeTextFileConfig( UserList * list, char * filename);


//User list functions
User * createUser(char * buffer, const char * delim);
int insertNode(UserList * list, User * user);
UserList * createUserList();
UserListNode * findUserByUsername( char * username,UserList * list);
int authenticateUser(char * username, char* password, UserList * list);
int authenticateAdmin(char * info, UserList * list);
void erro(char *string);
int deleteUser( UserList * list, char * username);
char * userToString(User * user);
void deleteUserList(UserList * list);
char * getUserFuncao(char * username, UserList * userList);

//Topic list functions
TopicList * createTopicList();
int insertTopic(Topic * topic, TopicList * topicList, int mainlist);
char * topicToString(Topic * topic);
Topic * findTopicByID(int id, TopicList * topicList, int mainList);
char * ipAddressToString(Topic * topic);
void deleteTopicList( TopicList * topicList, int mainList);
Topic * createTopic(char * topicName,int id, char * creatorUsername);

//Communication functions
void sendServerMessage(int client, char * msg);
int receiveClientMessage(int client, char buffer[]);

//Other functions
int isValidID(char *string);
int isUniqueID(int id, TopicList * topicList);
int findAvailableThread();
void threadIsBusy(int n);
void threadIsFree(int n);

//Servers
void * bootUDP(void *args);
void * bootTCP(void *args);




#endif //PROJETO_HEADER_H
