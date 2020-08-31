#include "httpserver.h"
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

#define WORKER_MAX        1024
#define EVENT_LIST_MAX    128
#define EVENT_MAX         12
#define WORK_REAL         4
#define POLL_SIZE         1024
#define BUFFER_LENGTH     4096

#define HTTP_SERVER_PORT	8080


extern int workers[WORKER_MAX];

static void handleterm(int sig)
{
    int i;
 
    for (i = 0; i < WORK_REAL; i++)
    {
        /* 杀掉子进程 */
        kill(workers[i], SIGTERM);
    }
    
    return;
}
 

int main()
{
   int status = 0;
   status = start_http_server(HTTP_SERVER_PORT);
   if(0 == status)
   {
	printf("start http server faild!\n");
	return 0;
   }
   if(status)
   {
   	while(1)
   	{
		signal(SIGTERM, handleterm);
        	pause();
   	}
   }
   return 0;
}
