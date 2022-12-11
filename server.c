#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
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

    char *id_str = "ID";

    // ----- TROCA DE MENSAGENS -----//
    while (1) {

        //Define socket do cliente
        struct sockaddr_storage client_storage;
        struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);
        socklen_t client_addrlen = sizeof(client_storage);

        //Espera receber mensagem do cliente e salva em buf. Fica bloqueado ate receber msg.
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);

        ssize_t count = recvfrom(s, buf, sizeof(buf), 0, client_addr, &client_addrlen);
        if(count < 0){
            logexit("erro ao receber mensagem do cliente");
        }
        char client_addrstr[BUFSZ];
        addrtostr(client_addr, client_addrstr, BUFSZ);
        
        //PLUG RECV MSG - so imprime a mensagem recebida
        printf("recebida> de %s: %s\n", client_addrstr, buf); 
    
        
        // Nao e necessario estabelecer um socket para o cliente, passa-se o endereco direto
        // Envia mensagem de volta para o cliente a partir de seu endereco client_addr

        //PLUG SEND MSG - So envia a mensagem recebida de volta
        if (strcmp(buf,"REQ_ID") == 0){
            count = sendto(s, id_str, strlen(id_str), 0, client_addr, client_addrlen);
            if (count != strlen(id_str)) {
            logexit("erro ao enviar mensagem de volta com sendto");
            }
        }
        else{
            count = sendto(s, buf, strlen(buf), 0, client_addr, client_addrlen);
            if (count != strlen(buf)) {
            logexit("erro ao enviar mensagem de volta com sendto");
            }
        }
    }
        
    exit(EXIT_SUCCESS);
}