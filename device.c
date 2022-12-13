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
#define STR_MIN 8

// ----- ATRIBUTOS DE DISPOSITIVO (globais) -----//
int dev_id = ID_HOLD;
int dispositivos_id[MAX_DISPOSITIVOS];

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
	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	if(!strcmp(str_in, "close connection\n")){
		char *str_id = malloc(STR_MIN);
        sprintf(str_id, "%02d", dev_id); //parse int->string
		strcpy(buf, "REQ_DEL ");
		strcat(buf, str_id);
		strcpy(str_out, buf); //retorna o comando que foi recebido
	}
	else if(!strcmp(str_in, "list device\n")){
		strcpy(buf, "");
		char *str_id = malloc(STR_MIN);

		for(int i = 0; i < MAX_DISPOSITIVOS; i++){
			if(dispositivos_id[i] != ID_HOLD){
        		sprintf(str_id, "%02d ", i); //parse int->string
				strcat(buf, str_id);
			}
			strcpy(str_out, "nao");
		}
		printf("%s\n", buf);	
	}
}

void *get_command(void *data){
	struct server_data *dados = (struct server_data*) data;
	
	char comando[BUFSZ];
	char msg_out[BUFSZ];

	while(fgets(comando, BUFSZ, stdin)){
		//Processa comando e salva mensagem resultante em msg_out
		process_command(comando, (char *)msg_out);

		//Envia msg_out ao servidor
		if(strcmp(msg_out, "nao")){
			ssize_t count = sendto(dados->server_sock, msg_out, strlen(msg_out), 0, dados->server_addr, dados->server_addrlen);
			if (count != strlen(msg_out)) {
				logexit("erro ao enviar mensagem com sendto");
			}
		}
	}
	return NULL;
}

// argv[1] = IP do servidor
// argv[2] = porta
int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	//inicializa o vetor de dispositivos, ID_HOLD indica que o id nao estah na base de dados do dispositivo 
	for(int i = 0; i < MAX_DISPOSITIVOS; i++){
		dispositivos_id[i] = ID_HOLD;
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
	//send_data->aux_id = id;
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
		
		// printf("recebida> ");
		// puts(buf);

		char *token = strtok(buf, " "); //token = type
        unsigned msg_type = parse_msg_type(token); //salva o tipo da mensagem

		switch (msg_type){
			case BROAD_ADD:
				token = strtok(NULL, " "); //token = dev_id
            	if(dev_id == ID_HOLD){
					//se o id nao tiver sido inicializado
					dev_id = atoi(token);
					printf("New ID: %02d\n", dev_id);
				} 
				else{
					//se o id ja tiver sido inicializado
					dispositivos_id[atoi(token)] = atoi(token);
					printf("Device %s added\n", token);
				}
				break;
			
			case BROAD_DEL:
				token = strtok(NULL, " "); //token = dev_id a ser deletado
				int del_id = atoi(token);
				if(dev_id == del_id){
					printf("Successful removal\n");
					dev_id = ID_HOLD;

					//Fecha o socket
					close(s);
					exit(EXIT_SUCCESS);
				}
				else{
					dispositivos_id[del_id] = ID_HOLD;
					printf("Device %s removed\n", token);
				}
				break;
			
			case LIST_DEV:
				//recebe LIST_DEV <id1> <id2> ...  e deve adicionar todos os ids no vetor int dispositivos_id
				while((token = strtok(NULL, " ")) != NULL){
					//token = dev_id
					int dev_id = atoi(token);
					dispositivos_id[dev_id] = dev_id; //na posicao dev_id, o dispositivo de id dev_id eh instalado
				}
				break;

			default:
				break;
		}
		
	}
}