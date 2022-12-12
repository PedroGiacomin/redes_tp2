#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

#define BUFSZ 1024
#define ID_HOLD 9999
#define MAX_DISPOSITIVOS 3

// argv[1] = IP do servidor
// argv[2] = porta
int main(int argc, char **argv) {
	// ----- ATRIBUTOS DE DISPOSITIVO -----//
	//int id = ID_HOLD;
	//int dispositivos[MAX_DISPOSITIVOS];
	
	if (argc < 3) {
		usage(argc, argv);
	}

	// ----- DEFINICAO E CRIACAO DO SOCKET ----- // 
	char *server_ip = argv[1];
	char *port = argv[2];

	// Define enderecos para serem passados no socket, storage guarda as informacoes de addr
	struct sockaddr_storage storage;
	if (0 != addrparse(server_ip, port, &storage)) {
		usage(argc, argv);
	}

	// Cria o socket UDP
	int s = socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1) {
		logexit("erro ao criar socket");
	}

	struct sockaddr *addr = (struct sockaddr*)(&storage);	//faz o parse pro formato correto, que e sockaddr
	socklen_t addr_len = sizeof(storage);

	// ----- TROCA DE MENSAGENS ----- // 
	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);
	printf("connected to %s\n", addrstr);

	// PLUG SEND MSG - Pega a mensagem do teclado e envia para o endereco
	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	// fgets(buf, BUFSZ-1, stdin);

	//Mandar REQ_ID
	strcpy(buf, "REQ_ID");
	ssize_t count = sendto(s, buf, strlen(buf), 0, addr, addr_len);
	if (count != strlen(buf)) {
		logexit("erro ao enviar mensagem com sendto");
	}

	// Entra em loop para enviar/receber mensagens
	while (1){
		// PLUG RECV MSG - recebe mensagem do servidor e guarda em buf
		memset(buf, 0, BUFSZ);
		count = recvfrom(s, buf, BUFSZ, 0, addr, &addr_len);
		if(count < 0){
            logexit("erro ao receber mensagem do cliente");
        }
		printf("recebida> ");
		puts(buf);
	}
	
	//Fecha o socket
	close(s);



	exit(EXIT_SUCCESS);
}