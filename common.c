#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

void logexit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    } 
    else {
        logexit("unknown protocol family.");
    }
    if (str) {
        snprintf(str, strsize, "IPv4 %s %hu", addrstr, port);
    }
}

int server_sockaddr_init(const char *portstr,
                         struct sockaddr_storage *storage) {

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET; //apenas IPv4
    addr4->sin_addr.s_addr = INADDR_ANY; 
    addr4->sin_port = port;
    return 0;

}

enum MSG_TYPE {REQ_ID = 1, REQ_DEL, BROAD_ADD, BROAD_DEL, LIST_DEV, ERROR, REQ_DEV, REQ_INFO, RES_INFO, RES_DEV};
unsigned parse_msg_type(const char *msg_type_in){
    if(!strcmp(msg_type_in, "REQ_ID"))
        return REQ_ID; 
    else if(!strcmp(msg_type_in, "REQ_DEL"))
        return REQ_DEL; 
    else if(!strcmp(msg_type_in, "BROAD_ADD"))
        return BROAD_ADD; 
    else if(!strcmp(msg_type_in, "BROAD_DEL"))
        return BROAD_DEL; 
    else if(!strcmp(msg_type_in, "LIST_DEV"))
        return LIST_DEV; 
    else if(!strcmp(msg_type_in, "ERROR"))
        return ERROR;
    else if(!strcmp(msg_type_in, "REQ_DEV"))
        return REQ_DEV;
    else if(!strcmp(msg_type_in, "REQ_INFO"))
        return REQ_INFO;
    else if(!strcmp(msg_type_in, "RES_INFO"))
        return RES_INFO;
    else if(!strcmp(msg_type_in, "RES_DEV"))
        return RES_DEV;
    else 
        return 0;
}