#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *portstr,
                         struct sockaddr_storage *storage);

// --- TRATAMENTO DE MENSAGENS --- //
enum MSG_TYPE {REQ_ID = 1, REQ_DEL, BROAD_ADD, BROAD_DEL, LIST_DEV};
unsigned parse_msg_type(const char *msg_type_in);