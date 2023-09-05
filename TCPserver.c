#include "header.h"

void listTopics(int client, TopicList * topicList){
    char * info;
    if (topicList!=NULL){
        TopicNode * temp = topicList->start;
        pthread_mutex_lock(&topicListMutex);
        while (temp!=NULL){
            if (temp->topic!=NULL){
                info= topicToString(temp->topic);
                send(client, info, strlen(info), 0);
                free(info);
            }
            temp=temp->ptr;
        }
        pthread_mutex_unlock(&topicListMutex);
    }
    sendServerMessage(client,"END\n\x04");
}

void subscribeToTopic(int client,int topicID, TopicList * topicList, User * user){
    if (user != NULL){
        char topicNotFound[] = "The topic with the requested ID was not found\n\x04";
        char commandSuccess[] = "Command processed with success\n\x04";
        if (findTopicByID(topicID,user->topicListSubscribed, 0) != NULL)
            sendServerMessage(client, "Already subscribed to topic \n\x04");
        else{
            Topic * myTopic= findTopicByID(topicID,topicList, 1);
            if (myTopic != NULL){
                char * resultString = ipAddressToString(myTopic);
                sendServerMessage(client, resultString);
                free(resultString);
                insertTopic(myTopic,user->topicListSubscribed, 0);
                sendServerMessage(client,commandSuccess);

            }
            else{
                sendServerMessage(client, topicNotFound);
            }
        }
    }
}

void leitorMenu(int client, TopicList*topicList, User * myUser){
    if (myUser != NULL){
        char goodbye[] = "Até logo!\n\x04";
        char invalidotCommand[] = "Unknown command\n\x04";
        char invalidID[] = "Invalid ID. The ID must be an int\n\x04";
        char buf[BUF_SIZE], bufCopy[BUF_SIZE], *token;
        while ((strcmp(buf, "SAIR") != 0) && ((receiveClientMessage(client, buf)) > 0)){
            strcpy(bufCopy,buf);
            token = strtok(bufCopy, " ");
            if (strcmp(token,"LIST_TOPICS") == 0)
                listTopics(client,topicList);
            else if (strcmp(token,"SUBSCRIBE_TOPIC") == 0){
                token = strtok(NULL, " ");
                if (isValidID(token)){
                    subscribeToTopic(client, strtol(token, NULL, 10),topicList, myUser);
                }
                else
                    sendServerMessage(client,invalidID);
            }
            else if (strcmp(token, "SAIR") == 0)
                sendServerMessage(client, goodbye);
            else
                sendServerMessage(client, invalidotCommand);
        }
        deleteTopicList(myUser->topicListSubscribed,0);
        myUser->topicListSubscribed= createTopicList();
    }
}

int sendMulticastMessage(Topic * myTopic, char * article){
    if (article == NULL)
        return 0;
    if (myTopic == NULL)
        return 0;
    int n_bytes;
    pthread_mutex_lock(&topicListMutex);
    n_bytes = snprintf(NULL, 0, "MESSAGE SENT FROM TOPIC: %s ID: %d\n",myTopic->topicName, myTopic->id);
    char * header = (char * ) malloc(n_bytes +1);
    snprintf(header, n_bytes +1,"MESSAGE SENT FROM TOPIC: %s ID: %d\n",myTopic->topicName, myTopic->id);
    pthread_mutex_unlock(&topicListMutex);
    if(sendto(myTopic->socket, header, strlen(header), 0,
           (struct sockaddr *) &myTopic->topicIPAddress, sizeof(myTopic->topicIPAddress)) < 0)
        return 0;
    free(header);
    if(sendto(myTopic->socket, article, strlen(article), 0,
           (struct sockaddr *) &myTopic->topicIPAddress, sizeof(myTopic->topicIPAddress)) < 0)
        return 0;
    return 1;
}

void jornalistaMenu(int client, User * myUser, TopicList * topicList) {
    char buf[BUF_SIZE], bufCopy[BUF_SIZE], *token, *article;
    int topicID;
    Topic *myTopic;

    char invalidID[] = "Invalid ID. The ID must be an int\n\x04";
    char duplicateID[] = "ID already taken, choose another one\n\x04";
    char commandFailed[] = "Command failed, check command syntax\n\x04";
    char commandSuccess[] = "Command processed with success\n\x04";
    char topicNotFound[] = "The topic with the requested ID was not found\n\x04";
    char goodbye[] = "Até logo!\n\x04";
    char invalidotCommand[] = "Unknown command\n\x04";
    while ((strcmp(buf, "SAIR") != 0) &&((receiveClientMessage(client, buf)) > 0)) {
        strcpy(bufCopy, buf);
        token = strtok(bufCopy, " ");
        if (strcmp(token, "CREATE_TOPIC") == 0) {
            token = strtok(NULL, " ");
            if (isValidID(token)) {
                topicID = (int) strtol(token, NULL, 10);
                if (isUniqueID(topicID, topicList)) {
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        myTopic = createTopic(token, topicID, myUser->username);
                        if(insertTopic(myTopic, topicList, 0))
                            sendServerMessage(client, commandSuccess);
                        else
                            sendServerMessage(client, commandFailed);
                    } else
                        sendServerMessage(client, commandFailed);
                } else
                    sendServerMessage(client, duplicateID);
            } else
                sendServerMessage(client, invalidID);
        } else if (strcmp(token, "SEND_NEWS") == 0) {
            token = strtok(NULL, " ");
            if (isValidID(token)) {
                topicID = (int) strtol(token, NULL, 10);
                myTopic = findTopicByID(topicID, topicList,1);
                if (myTopic != NULL) {
                    article = strchr(buf, ' ');
                    article++;
                    article = strchr(article, ' ');
                    article++;
                    if (article != NULL) {
                        if(sendMulticastMessage(myTopic,article))
                            sendServerMessage(client, commandSuccess);
                        else
                            sendServerMessage(client, commandFailed);
                    } else
                        sendServerMessage(client, commandFailed);

                } else
                    sendServerMessage(client, topicNotFound);

            } else
                sendServerMessage(client, invalidID);
        } else if (strcmp(token, "SAIR") == 0)
            sendServerMessage(client, goodbye);
        else
            sendServerMessage(client, invalidotCommand);
    }
}


 void * myTCPServer(void * args) {
     TCPThreadCore *myCore = (TCPThreadCore *) args;
     char welcome[] = "Bem-vindo ao servidor PORTO_NOTICIAS. Autenticação realizada com sucesso\n\x04";
     char failed[] = "AUTHENTICATION FAILED\n\x04";
     User *myUser;
     char password[BUF_SIZE], username[BUF_SIZE];
     //encaminhar mensagem inicial welcome
     sendServerMessage(myCore->client, "Welcome to our news server\nPlease insert your username:\n\x04");
     if(receiveClientMessage(myCore->client, username) <= 0){
         close(myCore->client);
         free(args);
         pthread_exit(NULL);
     }
     sendServerMessage(myCore->client, "Now please insert your password:\n\x04");
     if(receiveClientMessage(myCore->client, password) <= 0){
         close(myCore->client);
         free(args);
         pthread_exit(NULL);
     }
     if (authenticateUser(username, password, myCore->userList)) {
         myUser = findUserByUsername(username, myCore->userList)->user;
         sendServerMessage(myCore->client, welcome);
         pthread_mutex_lock(&myUser->myUserMutex);
         if (strcmp(myUser->funcao, "leitor") == 0) {
             leitorMenu(myCore->client, myCore->topicList, myUser);
         } else if (strcmp(myUser->funcao, "jornalista") == 0) {
             jornalistaMenu(myCore->client, myUser, myCore->topicList);

         }
         close(myCore->client);
         pthread_mutex_unlock(&myUser->myUserMutex);
         free(args);
         pthread_exit(NULL);
     } else {
         sendServerMessage(myCore->client,failed);
         close(myCore->client);
         free(args);
         pthread_exit(NULL);
     }
 }

_Noreturn void * bootTCP(void *args) {
    UserList *userList = ((ProgramCore *) args)->userList;
    TopicList *topicList = ((ProgramCore *) args)->topicList;
    int client, client_addr_size, position;
    struct sockaddr_in addr, client_addr;
    TCPThreadCore * newCore;
    char serverFull[] = "Server is currently full\n\x04";

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    // inet_addr(SERVER_IP);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(((ProgramCore *) args)->PORTO_NOTICIAS);


    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 5) < 0)
        erro("na funcao listen");
    client_addr_size = sizeof(client_addr);
    while (1) {
        //clean finished child processes, avoiding zombies
        //must use WNOHANG or would block whenever a child process was working
        //while (waitpid(-1, NULL, WNOHANG) > 0);
        //wait for new connection
        client = accept(fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_size);
        if (client > 0) {
            if((position = findAvailableThread()) > -1){
                threadIsBusy(position);
                newCore= (TCPThreadCore *) malloc(sizeof(TCPThreadCore));
                newCore->client =client;
                newCore->topicList = topicList;
                newCore->userList = userList;
                newCore->threadPosition = position;
                pthread_create(&usersThreads[position],NULL,myTCPServer,(void *)newCore);
            }
            else{
                sendServerMessage(client,serverFull);
                close(client);
            }
        }
    }
}


