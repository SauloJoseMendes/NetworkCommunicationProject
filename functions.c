#include "header.h"

int readTextFileConfig(UserList * list, char * filename){
    if(list!=NULL){
        FILE * file= fopen(filename, "r");
        if(file!=NULL){
            char buffer[BUF_SIZE];
            unsigned long index;
            while(fgets(buffer, BUF_SIZE, file)){
                if((index =strcspn(buffer, "\n"))){
                    buffer[index] = 0;
                }

                User * tempUser = createUser(buffer,";");
                insertNode(list,tempUser);
            }
            return 1;
        }
        else{
            return -1;
        }

    } else{
        return -1;
    }
}


User * createUser(char * buffer, const char * delim){
    char * token;
    User * tempUser= (User *) (malloc(sizeof(User)));
    //obter as informacoes do utilizador
    token= strtok(buffer,delim);
    if(token!=NULL)strcpy(tempUser->username, token);
    else return NULL;
    token= strtok(NULL,delim);
    if(token!=NULL) strcpy(tempUser->password, token);
    else return NULL;
    token= strtok(NULL,delim);
    if (pthread_mutex_init(&tempUser->myUserMutex, NULL) != 0)
    {
        erro("User mutex initialization failed");
    }
    if(token!=NULL&&(strcmp(token,"administrador")==0||strcmp(token,"leitor")==0||strcmp(token,"jornalista")==0)) {
        strcpy(tempUser->funcao, token);
        tempUser->topicListSubscribed = createTopicList();
        return tempUser;
    }
    else{
        free(tempUser);
        return NULL;
    }
}

int insertNode(UserList * list, User * user){
    if (list != NULL) {
        if (user != NULL) {
            pthread_mutex_lock(&userListMutex);
            UserListNode *temp = list->start;
            //criar novo no e adicionar o utilizador
            UserListNode *userNode = (UserListNode *) malloc(sizeof(UserListNode));
            userNode->user = user;
            userNode->ptr = NULL;
            //inserir o no na list
            if (temp == NULL) {
                //adicionar o novo no a lista principal caso a lista esteja vazia
                list->start = userNode;
                pthread_mutex_unlock(&userListMutex);
                return 1;
            } else {
                //caso contrario adiciona o utilizador no fim da lista
                //tambem verifica se ja existe algum utilizador com o mesmo username na lista
                while (temp->ptr != NULL && strcmp(temp->user->username,user->username) != 0) {
                    temp = temp->ptr;
                }
                if(temp->ptr == NULL && strcmp(temp->user->username,user->username) != 0){
                    temp->ptr = userNode;
                    pthread_mutex_unlock(&userListMutex);
                    return 1;
                }
                else {
                    pthread_mutex_unlock(&userListMutex);
                    return 0;
                }
            }
        }
        return 0;
    }
    return 0;
}


UserList * createUserList(){
    UserList * list= (UserList *) malloc(sizeof(UserList));
    list->start=NULL;
    return list;
}

void deleteUserList(UserList * list){
    UserListNode * tempNode;
    pthread_mutex_lock(&userListMutex);
    while (list->start!=NULL){
        tempNode=list->start;
        list->start=list->start->ptr;
        pthread_mutex_destroy(&tempNode->user->myUserMutex);
        if(tempNode->user->topicListSubscribed != NULL)
            deleteTopicList( tempNode->user->topicListSubscribed,0);
        free(tempNode->user);
        free(tempNode);
    }
    free(list);
    pthread_mutex_unlock(&userListMutex);
}
int deleteUser( UserList * list, char * username){
    if(list!=NULL) {
        pthread_mutex_lock(&userListMutex);
        UserListNode *temp = list->start;
        UserListNode *userListNode = NULL;
        if (temp != NULL) {
            if (username != NULL) {
                //verificar se o utilizador é o primeiro da lista
                if (strcmp(temp->user->username, username) == 0) {
                    userListNode = temp;
                    if (pthread_mutex_trylock(&userListNode->user->myUserMutex) != 0){
                        pthread_mutex_unlock(&userListMutex);
                        return 0;
                    }
                    list->start = temp->ptr;
                }
                    //caso contrario, procurar pela lista ate encontrar o utilizador ou ate chegar ao fim
                else {
                    while (temp->ptr != NULL && temp->ptr->ptr != NULL && strcmp(temp->ptr->ptr->user->username, username) != 0) {
                        temp = temp->ptr;
                    }
                    //verificar se o utilizador foi encontrado ou se apenas chegou ao fim da lista
                    if (temp->ptr != NULL && temp->ptr->ptr && (strcmp(temp->ptr->user->username, username) != 0)) {
                        userListNode = temp->ptr->ptr;
                        if (pthread_mutex_trylock(&userListNode->user->myUserMutex) != 0){
                            pthread_mutex_unlock(&userListMutex);
                            return 0;
                        }
                        temp->ptr = userListNode->ptr;
                    }
                }
                if (userListNode != NULL) {
                    pthread_mutex_destroy(&userListNode->user->myUserMutex);
                    free(userListNode->user);
                    free(userListNode);
                    pthread_mutex_unlock(&userListMutex);
                    return 1;
                } else{
                    pthread_mutex_unlock(&userListMutex);
                    return 0;
                }
            }
            pthread_mutex_unlock(&userListMutex);
            return 0;
        }
        pthread_mutex_unlock(&userListMutex);
        return 0;
    }
    return 0;
}

UserListNode * findUserByUsername( char * username,UserList * list){
    if(username!=NULL) {
        if(list!=NULL) {
            if (list->start != NULL) {
                pthread_mutex_lock(&userListMutex);
                UserListNode *tempNode = list->start;
                while (tempNode != NULL && strcmp(tempNode->user->username, username) != 0) {
                    tempNode = tempNode->ptr;
                }
                pthread_mutex_unlock(&userListMutex);
                if (tempNode != NULL && !strcmp(tempNode->user->username, username)) {
                    return tempNode;
                }
                else return NULL;
            }
            else return NULL;
        }
        else return NULL;
    }
    else return NULL;
}

int authenticateUser(char * username, char* password, UserList * list){
    UserListNode * userListNode= findUserByUsername(username, list);
    if(userListNode != NULL){

        return strcmp(userListNode->user->password, password) == 0;
    }
    else return 0;
}

int authenticateAdmin(char * info, UserList * list){
    char username[30], password[30], *token;
    token=strtok(info, " ");
    if (token == NULL)
        return 0;
    strcpy(username,  token);
    UserListNode * userListNode= findUserByUsername(username, list);
    if(userListNode != NULL && (strcmp(userListNode->user->funcao, "administrador") == 0)){
        token=strtok(NULL, " ");
        if (token == NULL)
            return 0;
        strcpy(password , token);
        return strcmp(userListNode->user->password, password) == 0;
    }
    else return 0;
}

char * userToString(User * user){
    char * string;
    int nbytes = snprintf(NULL, 0 ,"\nUsername:\t%s\nPassword:\t%s\nFuncao:\t%s\n",user->username,user->password,user->funcao);
    string = (char * ) malloc(nbytes + 1);
    snprintf(string, nbytes + 1 ,"\nUsername:\t%s\nPassword:\t%s\nFuncao:\t%s\n",user->username,user->password,user->funcao);
    return string;
}

void erro(char *string) {
    perror(string);
    exit(-1);
}

int writeTextFileConfig( UserList * list, char * filename){
    if(list!=NULL){
        FILE * file= fopen(filename, "w");
        if(file!=NULL){
            pthread_mutex_lock(&userListMutex);
            UserListNode * temp= list->start;
            while (temp!=NULL) {
                fprintf(file, "%s;%s;%s\n", temp->user->username,temp->user->password,temp->user->funcao);
                temp=temp->ptr;
            }
            pthread_mutex_unlock(&userListMutex);
            return 1;
        }
        else{
            return -1;
        }

    } else{
        return -1;
    }
}

char * getUserFuncao(char * username, UserList * userList){
    UserListNode * myUser= findUserByUsername(username,userList);
    if (myUser!=NULL){

        return myUser->user->funcao;
    }
    return NULL;
}

TopicList * createTopicList(){
    TopicList * topicList = (TopicList *) malloc(sizeof (TopicList));
    topicList->start=NULL;
    return topicList;
}

int isValidID(char *string){
    if (string != NULL){
        if (strlen(string)<=11){
            int i=0;
            while (string[i] != '\0'){
                if (!isdigit(string[i]))
                    return 0;
                i++;
            }

            return 1;
        }
        return 0;
    }
    return 0;
}

int isUniqueID(int id, TopicList * topicList){
    if (topicList != NULL){
        TopicNode * temp= topicList->start;
        while (temp != NULL){
            if (temp->topic->id == id)
                return 0;
            temp = temp->ptr;
        }
        return 1;
    }
    return 0;
}

int createTopicSocket(Topic * myTopic) {
    if (myTopic == NULL)
        return 0;
    myTopic->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (myTopic->socket < 0) {
        perror("socket");
        return 0;
    }
    memset(&myTopic->topicIPAddress, 0, sizeof(struct sockaddr_in));
    myTopic->topicIPAddress.sin_family = AF_INET;
    myTopic->topicIPAddress.sin_addr.s_addr = inet_addr(TOPIC_IP);
    myTopic->topicIPAddress.sin_port = htons(lastTopicPort++);

    int ttl = 64;
    if (setsockopt(myTopic->socket, IPPROTO_IP, IP_MULTICAST_TTL,
                   (char *)&ttl, sizeof(ttl)) < 0) {
        printf("setsockopt");
        close(myTopic->socket);
        return 0;
    }


    return 1;
}

Topic * createTopic(char * topicName,int id, char * creatorUsername){
    if (lastTopicPort < 49152){
        if (topicName!=NULL && strlen(topicName) <= TOPIC_NAME_LENGTH){
            if (creatorUsername != NULL && strlen(creatorUsername) <= USERNAME_LENGTH){
                Topic * myTopic= (Topic *) malloc(sizeof(Topic));
                strcpy(myTopic->creatorUsername,creatorUsername);
                strcpy(myTopic->topicName,topicName);
                myTopic->id=id;
                if (createTopicSocket(myTopic))
                    return myTopic;
                else{
                    free(myTopic);
                    return NULL;
                }
            }
            return NULL;
        }
        return NULL;
    }
    return NULL;
}

int insertTopic(Topic * topic, TopicList * topicList,int mainList){
    if (topic!=NULL){
        if (topicList!=NULL){

            TopicNode * newTopicNode = (TopicNode * )(malloc(sizeof(TopicNode)));
            newTopicNode->topic = topic;
            newTopicNode->ptr=NULL;
            if (mainList == 1)
                pthread_mutex_lock(&topicListMutex);
            if (topicList->start == NULL){
                topicList->start = newTopicNode;
            } else{
                TopicNode * temp = topicList->start;
                while (temp->ptr != NULL)
                    temp = temp->ptr;
                temp->ptr=newTopicNode;
            }
            if (mainList == 1)
                pthread_mutex_unlock(&topicListMutex);
            return 1;
        }
        return 0;
    }
    return 0;
}

char * ipAddressToString(Topic * topic){
    if (topic != NULL){
        //determine string size
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(topic->topicIPAddress.sin_addr), ipAddress, INET_ADDRSTRLEN);
        int n_bytes = snprintf(NULL, 0,"@%s:%u\n",inet_ntoa(topic->topicIPAddress.sin_addr),
                               ntohs(topic->topicIPAddress.sin_port));
        //create final string
        char * finalString = (char *) malloc(n_bytes +1);
        snprintf(finalString, n_bytes +1,"@%s:%u\n",inet_ntoa(topic->topicIPAddress.sin_addr),
                 ntohs(topic->topicIPAddress.sin_port));
        return finalString;
    }
    return NULL;
}

Topic * findTopicByID(int id, TopicList * topicList, int mainList){
    if (topicList != NULL){
        TopicNode * temp = topicList->start;
        if (temp != NULL){
            if (mainList == 1)
                pthread_mutex_lock(&topicListMutex);
            while (temp != NULL && temp->topic->id != id)
                temp = temp->ptr;
            if (mainList == 1)
                pthread_mutex_unlock(&topicListMutex);
            if (temp != NULL && temp->topic->id == id)
                return temp->topic;
            else
                return NULL;
        }
        return NULL;
    }
    return NULL;
}



char * topicToString(Topic * topic){
    int n_byte=snprintf(NULL,0,"\nTítulo do tópico:\t%s\nID:\t%d\nCriador:\t%s\n",
                        topic->topicName,topic->id,topic->creatorUsername);
    char * string = (char *) malloc(n_byte +1);
    snprintf(string,n_byte + 1,"\nTítulo do tópico:\t%s\nID:\t%d\nCriador:\t%s\n",
             topic->topicName,topic->id,topic->creatorUsername);
    return string;
}

void deleteTopicList( TopicList * topicList, int mainList){
    if (topicList != NULL) {
        if (mainList == 1)
            pthread_mutex_lock(&topicListMutex);
        TopicNode *temp = topicList->start, *delete;
        while (temp != NULL) {
            delete = temp;
            temp = temp->ptr;
            free(delete->topic);
            free(delete);

        }
        free(topicList);
        if(mainList == 1)
            pthread_mutex_unlock(&topicListMutex);
    }

}

int findAvailableThread(){
    pthread_mutex_lock(&idThreadMutex);
    for (int i = 0; i < MAX_USERS; ++i) {
        if(availableThreads[i]){
            pthread_mutex_unlock(&idThreadMutex);
            return i;
        }
    }
    pthread_mutex_unlock(&idThreadMutex);
    return -1;
}

void threadIsBusy(int n){
    pthread_mutex_lock(&idThreadMutex);
    availableThreads[n] = 0;
    pthread_mutex_unlock(&idThreadMutex);
}

void threadIsFree(int n){
    pthread_mutex_lock(&idThreadMutex);
    availableThreads[n] = 1;
    pthread_mutex_unlock(&idThreadMutex);
}

void sendServerMessage(int client, char * msg){
    send(client, msg, strlen(msg), 0);
}

int receiveClientMessage(int client, char buffer[]){
    int nread = recv(client, buffer, BUF_SIZE - 1, 0), pos;
    buffer[nread] = '\0';
    if ((pos = strcspn(buffer, "\n")) > 0)
        buffer[pos] = '\0';
    printf("%s\n",buffer);
    return nread;
}
