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

//Funcoes pra construir mensagens de controle ERROR e OK ja em formato de string, fornecendo apenas o codigo
void build_error_msg(char *msg_out, unsigned codigo){
    
    //parse int->str
    char *code_aux = malloc(sizeof(STR_MIN));
    sprintf(code_aux, "%02u", codigo);

    strcpy(msg_out, "ERROR ");
    strcat(msg_out, code_aux);

    free(code_aux);
}
void build_ok_msg(char *msg_out, unsigned codigo){
    
    //parse int->str
    char *code_aux = malloc(sizeof(STR_MIN));
    sprintf(code_aux, "%02u", codigo);

    strcpy(msg_out, "OK ");
    strcat(msg_out, code_aux);

    free(code_aux);
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
    //printf("bound to %s, waiting connections\n", addrstr);

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

        //Tratamento da mensagem recebida
        char *token = strtok(buf, " "); //token = type
        unsigned msg_type = parse_msg_type(token); //salva o tipo da mensagem

        int disp_id;
        int num_disp = 0;
        switch (msg_type){
            case REQ_ID:
                //Encontra a primeira posicao vazia no vetor de dispositivos e atribui o dispositivo que acabou de chegar a ela
                for(int i = 0; i < MAX_DISPOSITIVOS; i++){
                    if(dispositivos[i] == NULL){
                        dispositivos[i] = malloc(sizeof(struct sockaddr*));
                        *dispositivos[i] = *client_addr;
                        disp_id = i;
                        break;
                    }
                    num_disp++;
                }
                
                //Checa se existe ERROR 01 e envia msg se sim
                if(num_disp == MAX_DISPOSITIVOS){
                    memset(buf, 0, BUFSZ);
                    build_error_msg(buf, 1);
                    int count = sendto(s, buf, strlen(buf), 0, client_addr, client_addrlen);
                    if (count != strlen(buf)) {
                        logexit("erro ao enviar mensagem de volta com sendto");
                    }
                    break;
                }

                //Manda mensagem BROAD_ADD <id> para todos os clientes cadastrados (!= NULL), por meio da funcao brodcast()
                memset(buf, 0, BUFSZ);
                char *str_id = malloc(STR_MIN);
                sprintf(str_id, "%02d", disp_id); //parse int->string
                strcpy(buf, "BROAD_ADD ");
                strcat(buf, str_id);

                broadcast(dispositivos, s, buf);
                printf("Device %s added\n", str_id);

                //Manda mensagem LIST_DEV <id1> <id2> para o cliente que acabou de ser cadastrado
                memset(buf, 0, BUFSZ);
                strcpy(buf, "LIST_DEV");

                str_id = malloc(STR_MIN);
                for(int i = 0; i < MAX_DISPOSITIVOS; i++){
                    //verifica quais dispositivos estao instalados e adiciona seu id na msg
                    if(dispositivos[i] != NULL){
                        sprintf(str_id, " %02d", i); //parse int->string
                        strcat(buf, str_id);
                    }
                }

                int count = sendto(s, buf, strlen(buf), 0, client_addr, client_addrlen);
                if (count != strlen(buf)) {
                    logexit("erro ao enviar mensagem de volta com sendto");
                }

                break;
            
            case REQ_DEL:
                token = strtok(NULL, " "); //token = dev_id a ser deletado
                disp_id = atoi(token);

                //Checa se existe ERROR 02
                if(dispositivos[disp_id] == NULL){
                    memset(buf, 0, BUFSZ);
                    build_error_msg(buf, 2);
                    int count = sendto(s, buf, strlen(buf), 0, client_addr, client_addrlen);
                    if (count != strlen(buf)) {
                        logexit("erro ao enviar mensagem de volta com sendto");
                    }
                    break;
                }
                
                //Manda mensagem BROAD_DEL <id> para todos os clientes cadastrados (!= NULL), por meio da funcao brodcast()
                memset(buf, 0, BUFSZ);
                str_id = malloc(STR_MIN);
                sprintf(str_id, "%02d", disp_id); //parse int->string
                strcpy(buf, "BROAD_DEL ");
                strcat(buf, str_id);
                
                broadcast(dispositivos, s, buf);
                printf("Device %s removed\n", str_id);

                dispositivos[disp_id] = NULL;  //tira o registro do dispositivo do vetor dispositivos[]
                num_disp--;
                break;

            default:
                break;
        }

        // printf("Enderecos gravados:\n");
        // for(int i = 0; i < MAX_DISPOSITIVOS; i++){
        //     printf("dispositivos[i]: %p\n", dispositivos[i]);
        // }
    }
        
    exit(EXIT_SUCCESS);
}