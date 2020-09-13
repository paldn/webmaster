#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h> 
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
using namespace std;

struct webconnect;
typedef void (*callback_t)(webconnect*);

struct webconnect
{
    char* querybuf;
    int last_query_time;
    callback_t handler;
    void *ptr;
    int sockfd;
};

void init_webconnect(webconnect* &conn);

void web_accept(webconnect* conn);

void read_request(webconnect* conn);

void send_response(webconnect* conn);
 
void empty_callback(webconnect* conn);

void close_webconnect(webconnect* &conn);

/* 创建共享的mutex */
void initMutex(void);
 
int startup(unsigned short port);
 
int make_socket_non_blocking(int fd);
 
int start_http_server(unsigned int port);

void timer_func(int);

int set_http_keepalive_timeout(int sec,int usec);

#endif
