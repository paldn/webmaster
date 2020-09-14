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
#include <vector>
#include <dirent.h>
#include <json/json.h>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

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
 
int find_pid_by_name(vector<int> &list,char* const name)
{
        DIR             *dir;
        struct dirent   *d;
        int             pid, i;
        char            *s;
        int pnlen;

        i = 0;
        pnlen = strlen(name);

        /* Open the /proc directory. */
        dir = opendir("/proc");
        if (!dir)
        {
                printf("cannot open /proc");
                return -1;
        }

        /* Walk through the directory. */
        while ((d = readdir(dir)) != NULL) {

                char exe [PATH_MAX+1];
                char path[PATH_MAX+1];
                int len;
                int namelen;

                /* See if this is a process */
                if ((pid = atoi(d->d_name)) == 0)       continue;
                if(pid == getpid())continue;
                snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
                if ((len = readlink(exe, path, PATH_MAX)) < 0)
                        continue;
                path[len] = '\0';

                /* Find ProcName */
                s = strrchr(path, '/');
                if(s == NULL) continue;
                s++;

                /* we don't need small name len */
                namelen = strlen(s);
                if(namelen < pnlen)     continue;

                if(!strncmp(name, s, pnlen)) {
                        /* to avoid subname like search proc tao but proc taolinke matched */
                        if(s[pnlen] == ' ' || s[pnlen] == '\0') {
                                list.push_back(pid);
                                i++;
                        }
                }
        }
        closedir(dir);
        return 0;
}



int main(int argc,char* argv[])
{
	char* const proc_name = "master_proc";
	const char* config_path = "conf/master_proc.conf";
        int i;
	if(argc == 2)
	{
		if(strcmp(argv[1],"--help") == 0)
		{
			printf("\n");
		}
		else
		{
			printf("invaild paramter\n");
			return 0;
		}
	}
	else if(argc == 3)
        {
		if(strcmp(argv[1],"-c") == 0)
		{
			config_path = argv[2];
		}
		else if(strcmp(argv[1],"-s") == 0)
		{
			if(strcmp(argv[2],"stop") == 0 || strcmp(argv[2],"reload") == 0)
			{
				vector<int> list;
                        	find_pid_by_name(list,proc_name);

                        	for(i=0;i<list.size();i++)
                        	{
                                	kill(list[i],SIGHUP);
                        	}
			}
			else
			{
				printf("invaild paramter\n");
                        	return 0;
			}
			if(strcmp(argv[2],"stop") == 0)
			{
				return 0;
			}

		}
		else
		{
			printf("invaild paramter\n");
                        return 0;
		}

        }
        else
        {
                printf("invaild paramters\n");
                return 0;
        }

	int status = 0;
	int port = 80;
	int keepalive_timeout = 30;

	Json::Reader reader;
	Json::Value root;

	ifstream in(config_path,ios::binary);
	
	if(!in.is_open())
	{
		printf("cannot open config file\n");
		return 0;
	}

	if(reader.parse(in,root))
	{

		if(root.isMember("http_server"))
		{
			if(root["http_server"].isMember("port"))
			{
				port = root["http_server"]["port"].asInt();
			}
			if(root["http_server"].isMember("keepalive_timeout"))
			{
				keepalive_timeout = root["http_server"]["keepalive_timeout"].asInt();
			}
		}
		
	}


	in.close();





   	status = start_http_server(port);
   	if(0 == status)
   	{
		printf("start http server faild!\n");
		return 0;
   	}
   	if(status)
   	{
		signal(SIGTERM, handleterm);
   		while(1)
   		{

			sleep(1);

   		}
   	}
   	return 0;
}
