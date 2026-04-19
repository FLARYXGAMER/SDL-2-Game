#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

void testServerFun(void);
void testClientFun(void);
void* serverThread(void* arg);

#endif