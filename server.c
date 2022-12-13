#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024
#define STR_MIN 8
#define MAX_DISPOSITIVOS 3

void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void broadcast(struct sockaddr *dispositivos[], int s, char *msg){
    for(int i = 0; i < MAX_DISPOSITIVOS; i++){
        if(dispositivos[i] != NULL){
            socklen_t disp_len = sizeof(struct sockaddr_storage);
            int count = sendto(s, msg, strlen(msg), 0, dispositivos[i], disp_len);
            if (count != strlen(msg)) {
                logexit("erro ao enviar mensagem de volta com sendto");
            }
        }
    } 
}

// argv[1] = port
int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argc, argv);
    }

    // ----- DEFINICAO E CRIACAO DO SOCKET ----- // 
	char *port = argv[1];

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(port, &storage)) {
        usage(argc, argv);
    }

    // Cria o socket UDP
    int s = socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (s == -1) {
        logexit("erro ao criar socket");
    }

    // Opcoes do socket
    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("erro ao definir opcoes de socket com setsockopt");
    }
 
    struct sockaddr *addr = (struct sockaddr*)(&storage);	//faz o parse pro formato correto, que e sockaddr
	socklen_t addr_len = sizeof(storage);

    //Faz o bind
    if (0 != bind(s, addr, addr_len)) {
        logexit("erro no bind");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    // ----- LOGICA DE IDS -----//
    //Todos os dispositivo que se conectam tem seu endereco (no formato sockaddr*) salvo num vetor
    //O vetor eh inicializado todo com NULL quando o servidor eh inicializado
    //A posicao do vetor eh o id do dispositivo
    struct sockaddr *dispositivos[MAX_DISPOSITIVOS];
    for (int i = 0; i < MAX_DISPOSITIVOS; i++){
        dispositivos[i] = NULL;
    }

    // ----- TROCA DE MENSAGENS -----//
    while (1) {
        //Define socket do cliente
        struct sockaddr_storage client_storage;
        struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);
        socklen_t client_addrlen = sizeof(client_storage);

        //Espera receber mensagem do cliente e salva em buf. Fica bloqueado ate receber msg.
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        
        //O endereco do cliente que enviou a msg eh salvo em client_addr
        ssize_t count = recvfrom(s, buf, sizeof(buf), 0, client_addr, &client_addrlen);

        if(count < 0){
            logexit("erro ao receber mensagem do cliente");
        }
        char client_addrstr[BUFSZ];
        addrtostr(client_addr, client_addrstr, BUFSZ);
        
        //PLUG RECV MSG - so imprime a mensagem recebida
        printf("recebida de %s> %s\n", client_addrstr, buf); 
        char *token = strtok(buf, " "); //token = type
        unsigned msg_type = parse_msg_type(token); //salva o tipo da mensagem

        if(msg_type == REQ_ID){
            //Encontra a primeira posicao vazia no vetor de dispositivos e atribui o dispositivo que acabou de chegar a ela
            int disp_id;
            for(int i = 0; i < MAX_DISPOSITIVOS; i++){
                if(dispositivos[i] == NULL){
                    dispositivos[i] = malloc(sizeof(struct sockaddr*));
                    *dispositivos[i] = *client_addr;
                    disp_id = i;
                    break;
                }
            }

            char disp_addrstr[BUFSZ];
            addrtostr(dispositivos[disp_id], disp_addrstr, BUFSZ);
            printf("cadastrado %s\n", disp_addrstr); 

            printf("Enderecos gravados:\n");
            for(int i = 0; i < MAX_DISPOSITIVOS; i++){
                printf("dispositivos[i]: %p\n", dispositivos[i]);
            }

            //Manda mensagem RES_ID <id> para todos os clientes cadastrados (!= NULL)
            memset(buf, 0, BUFSZ);
            char *str_id = malloc(STR_MIN);
            sprintf(str_id, "%02d", disp_id); //parse int->string
            strcpy(buf, "BROAD_ADD ");
            strcat(buf, str_id);

            //faz o broadcast da mensagem de BROAD_ADD para todos os dispositivos conectados
            broadcast(dispositivos, s, buf);
       
        }

        // PLUG RECV MSG - so imprime a mensagem recebida    
        // Nao e necessario estabelecer um socket para o cliente, passa-se o endereco direto que esta salvo em client_addr
        // Envia mensagem de volta para o cliente a partir de seu endereco client_addr
        // count = sendto(s, buf, strlen(buf), 0, client_addr, client_addrlen);
        // if (count != strlen(buf)) {
        // logexit("erro ao enviar mensagem de volta com sendto");
        // }
    }
        
    exit(EXIT_SUCCESS);
}