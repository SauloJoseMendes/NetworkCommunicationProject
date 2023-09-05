#include "header.h"


int main(int argc, char * argv[]) {
    if (argc != 4){
        printf("news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n");
        return -1;
    }
    lastTopicPort = 1025;
    ProgramCore programCore;
    programCore.PORTO_NOTICIAS = (short) strtol(argv[1], NULL, 10);
    programCore.PORTO_CONFIG = (short) strtol(argv[2], NULL, 10);
    programCore.userList = createUserList();
    programCore.topicList = createTopicList();
    programCore.filename = argv[3];
    //Carregar informacoes do ficheiro de configuracao
    if(readTextFileConfig( programCore.userList,argv[3])==-1) {
        erro("Erro na leitura do ficheiro de configuracao");
    }
    //inicializar semaforos
    if (pthread_mutex_init(&userListMutex, NULL) != 0)
    {
        erro("User mutex initialization failed");
    }
    if (pthread_mutex_init(&topicListMutex, NULL) != 0)
    {
        erro("Topic mutex initialization failed");
    }
    if (pthread_mutex_init(&idThreadMutex, NULL) != 0)
    {
        erro("Topic mutex initialization failed");
    }
    //Inicializar array de threads disponíveis
    for (int i = 0; i <MAX_USERS; ++i) {
        availableThreads[i]= 1;
    }
    //incializar thread udp
    pthread_create(&myThreads[0],NULL,bootUDP,(void *)&programCore);

    //incializar thread tcp
    pthread_create(&myThreads[1],NULL,bootTCP,(void *)&programCore);

    pthread_join(myThreads[0],NULL);

    return 0;
}
