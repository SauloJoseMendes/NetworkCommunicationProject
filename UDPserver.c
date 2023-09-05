#include "header.h"
void * bootUDP(void *args) {
    UserList * userList= ((ProgramCore *) args)->userList;
    TopicList * topicList =((ProgramCore *) args)->topicList;
    short port = ((ProgramCore *) args)->PORTO_CONFIG;
    char * filename= ((ProgramCore *) args)->filename;
    struct sockaddr_in si_minha, si_outra;

    long recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUF_SIZE],bufCopy[BUF_SIZE], *token, *info;
    UserListNode *temp;
    User * tempUser;

    char welcome[] = "PORTO_CONFIG AUTHENTICATE YOURSELF {username} {password}\n";
    char successful[] = "SUCCESSFUL AUTHENTICATION\n";
    char failed[] = "AUTHENTICATION FAILED\n";
    char endServer[] = "SERVER ENDED\n";
    char quitConsole[] = "QUITTING THE CONSOLE\n";
    char sucessUser[]= "USER ADDED WITH SUCESS\n";
    char failedUser[]="CREATION FAILED: INVALID INPUTS\n";
    char failedInsertion[]="CREATION FAILED: USER ALREADY EXISTS\n";
    char userDeleted[]= "USER DELETED WITH SUCCESS\n";
    char userNotDeleted[]= "USER DELETION FAILED\n";
    char savedData[]="DATA SAVED WITH SUCCESS\n";


    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(port);
    //inet_addr(SERVER_IP);
    si_minha.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *) &si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }
    while (1) {
        sendto(s, welcome, strlen(welcome), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
        if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t * ) & slen)) == -1) {
            erro("Erro no recvfrom");
        }
        //remover \n e adicionar \0
        buf[recv_len-1] = '\0';
        if (strlen(buf)>0) {
            if (authenticateAdmin(buf, userList)) {
                sendto(s, successful, strlen(successful), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                do {
                    // Espera recepção de mensagem (a chamada é bloqueante)
                    if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra,
                                             (socklen_t *) &slen)) ==
                        -1) {
                        erro("Erro no recvfrom");
                    }
                    // Para ignorar o restante conteúdo (anterior do buffer)
                    buf[recv_len-1] = '\0';
                    strcpy(bufCopy,buf);
                    token = strtok(buf, " ");

                    if (!strcmp(token, "ADD_USER")) {
                        info = strchr(bufCopy, ' ');
                        //deslocar o ponteiro um byte para ignorar o primeiro char (espaco)
                        info++;

                        if((tempUser = createUser(info, " "))==NULL){
                            sendto(s, failedUser, strlen(failedUser), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                        }
                        else{
                            if(insertNode(userList, tempUser)){
                                sendto(s, sucessUser, strlen(sucessUser), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                            }
                            else{
                                sendto(s, failedInsertion, strlen(failedInsertion), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                            }

                        }
                    } else if (!strcmp(token, "DEL")) {
                        info = strchr(bufCopy, ' ');
                        //deslocar o ponteiro um byte para ignorar o primeiro char (espaco)
                        info++;
                        if(deleteUser(userList, info) == 1){
                            sendto(s, userDeleted, strlen(userDeleted), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                        } else{
                            sendto(s, userNotDeleted, strlen(userNotDeleted), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                        }

                    } else if (!strcmp(token, "LIST")) {
                        temp = userList->start;
                        if (temp != NULL) {
                            while (temp != NULL) {
                                if (temp->user!=NULL){
                                    info = userToString(temp->user);
                                    sendto(s, info, strlen(info), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                                    free(info);
                                }
                                temp = temp->ptr;
                            }
                        }
                    } else if (!strcmp(token, "QUIT_SERVER")) {
                        //guardar informacoes
                        if(writeTextFileConfig(userList,filename)>0){
                            sendto(s, savedData, strlen(savedData), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                        }
                        sendto(s, endServer, strlen(endServer), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
                        close(s);
                        close(fd);
                        //apagar lista de utilizadores
                        deleteUserList(userList);
                        //apagar lista de topicos
                        deleteTopicList(topicList, 1);
                        //destruir mutexes
                        pthread_mutex_destroy(&userListMutex);
                        pthread_mutex_destroy(&topicListMutex);
                        //enviar mensagem
                        pthread_exit(NULL);
                    }
                } while (strcmp(buf, "QUIT") != 0);
                sendto(s, quitConsole, strlen(quitConsole), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
            } else {
                sendto(s, failed, strlen(failed), 0, (struct sockaddr *) &si_outra, (socklen_t) slen);
            }
        }
    }
}