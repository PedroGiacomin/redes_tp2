#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define ID_HOLD 9999
#define MAX_DISPOSITIVOS 3

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

struct server_data{
	struct sockaddr *server_addr;
	int server_sock;
	socklen_t server_addrlen;
};

void process_command(char *str_in, char *str_out){
	printf("comando recebido> %s\n", str_in);
	strcpy(str_out, str_in); //retorna o comando que foi recebido

}

void *get_command(void *data){
	struct server_data *dados = (struct server_data*) data;
	
	char comando[BUFSZ];
	char msg_out[BUFSZ];

	while(fgets(comando, BUFSZ, stdin)){
		//Processa comando e salva mensagem resultante em msg_out
		process_command(comando, (char *)msg_out);

		//Envia msg_out ao servidor
		ssize_t count = sendto(dados->server_sock, msg_out, strlen(msg_out), 0, dados->server_addr, dados->server_addrlen);
		if (count != strlen(msg_out)) {
			logexit("erro ao enviar mensagem com sendto");
		}
	}
	return NULL;
}

// argv[1] = IP do servidor
// argv[2] = porta
int main(int argc, char **argv) {
	// ----- ATRIBUTOS DE DISPOSITIVO -----//
	int id = ID_HOLD;
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

	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);

	//Mandar REQ_ID ao inicializar device
	strcpy(buf, "REQ_ID");
	ssize_t count = sendto(s, buf, strlen(buf), 0, addr, addr_len);
	if (count != strlen(buf)) {
		logexit("erro ao enviar mensagem com sendto");
	}

	//Cria thread com loop para pegar comando do teclado e enviar mensagem resultante ao servidor 
	pthread_t thread_comando;
	struct server_data *send_data = malloc(sizeof(send_data)); //passa socket do servidor e dados do endereco
	send_data->server_sock = s;
	send_data->server_addr = addr;
	send_data->server_addrlen = addr_len;
	if(0 != pthread_create(&thread_comando, NULL, get_command, send_data)){
		logexit("erro ao fazer thread");
	}

	// Entra em loop para receber mensagens
	while (1){
		// PLUG RECV MSG - recebe mensagem do servidor e guarda em buf
		memset(buf, 0, BUFSZ);
		count = recvfrom(s, buf, BUFSZ, 0, addr, &addr_len);
		if(count < 0){
            logexit("erro ao receber mensagem do cliente");
        }
		
		printf("recebida> ");
		puts(buf);

		char *token = strtok(buf, " "); //token = type
        unsigned msg_type = parse_msg_type(token); //salva o tipo da mensagem

		switch (msg_type){
			case BROAD_ADD:
				token = strtok(NULL, " "); //token = dev_id
            	if(id == ID_HOLD){
					//se o id nao tiver sido inicializado
					id = atoi(token);
					printf("New ID: %s\n", token);
				} 
				else{
					//se o id ja tiver sido inicializado
					printf("Device %s added\n", token);
				}
				break;
			
			default:
				break;
		}
		
	}
	
	//Fecha o socket
	close(s);



	exit(EXIT_SUCCESS);
}