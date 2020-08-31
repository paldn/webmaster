#include "httpserver.h"
#define WORKER_MAX        1024
#define EVENT_LIST_MAX    128
#define EVENT_MAX         12
#define WORK_REAL         4
#define POLL_SIZE         1024
#define BUFFER_LENGTH     4096 



int epfd = -1;

int workers[WORKER_MAX];
 
/* 互斥量 */
pthread_mutex_t *mutex;

































void init_webconnect(webconnect* &conn)
{
	conn = (webconnect*)malloc(sizeof(webconnect));
	conn->querybuf = (char*)malloc(1024*1024);
	conn->handler = empty_callback;
}

void web_accept(webconnect* conn)
{
	struct sockaddr_in client_addr;
        memset(&client_addr,0,sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(client_addr);
        int sockfd = accept(conn->sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(-1 == sockfd)return;
	
	printf("connect....\n");


	make_socket_non_blocking(sockfd);

	struct epoll_event event = {0};
        event.events = EPOLLIN | EPOLLET;

	webconnect* new_conn = NULL;
	
	init_webconnect(new_conn);

	new_conn->handler = read_request;
	new_conn->sockfd = sockfd;

        event.data.ptr = (void*)new_conn;
        epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

}

void read_request(webconnect* conn)
{	
	memset(conn->querybuf,0,sizeof(conn->querybuf));

	int len,fd = conn->sockfd;
	int read_len = 1024;
	int _index = 0;

	while(1)
	{
		len = recv(fd,conn->querybuf+_index,read_len,0);
		
		if(0 == len)
	        {
			printf("******************\n");
        	        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
               		close_webconnect(conn);
                	return;
        	}
		else if(0 < len)
		{
			_index += read_len;
		}
		else
		{
			break;
		}

	}

	printf("\n\n%s\n\n",conn->querybuf);	
}

void send_response(webconnect* conn)
{
	char response_header[4096] = {0};
	char response_body[1024*1024] = {0};
	char response[1024*1024] = {0};

	char mime[64] = "text/html";
	int fd = open("/http/index.html",O_RDONLY);
	int contentLength = read(fd,response_body,sizeof(response_body));
	sprintf(response_header,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",mime,contentLength);
	sprintf(response,"%s%s",response_header,response_body);
	int ret = send(conn->sockfd,response,strlen(response),0);
	if(0 == ret)
        {
                epoll_ctl(epfd, EPOLL_CTL_DEL, conn->sockfd, 0);
                close_webconnect(conn);
                return;

        }

	printf("\n\n%s\n\n",response);
}

void empty_callback(webconnect* conn)
{
	printf("this is empty function\n");
}

void close_webconnect(webconnect* &conn)
{
	printf("connect closed...\n");
	close(conn->sockfd);
	free(conn->querybuf);
	free(conn);
}

/* 创建共享的mutex */
void initMutex(void)
{
    pthread_mutexattr_t attr;
    int ret;
    
    //设置互斥量为进程间共享
    mutex=(pthread_mutex_t*)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
    if( MAP_FAILED==mutex) {
        perror("mutex mmap failed");
        return;
    }
    
    //设置attr的属性
    pthread_mutexattr_init(&attr);
    ret = pthread_mutexattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
    if(ret != 0) {
        fprintf(stderr, "mutex set shared failed");
        return;
    }
    pthread_mutex_init(mutex, &attr);
 
    return;
}
 
int startup(unsigned short port)
{
    struct sockaddr_in servAddr;
    unsigned value = 1;
    int listenFd;
 
    memset(&servAddr, 0, sizeof(servAddr));
    
    //协议域（ip地址和端口）
    servAddr.sin_family = AF_INET;
    
    //绑定默认网卡
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //端口
    servAddr.sin_port = htons(port);
    
    //创建套接字
    if ((listenFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    
    //绑定套接字
    if (bind(listenFd, (struct sockaddr *)&servAddr, sizeof(servAddr))) {
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    //开始监听，设置最大连接请求
    if (listen(listenFd, 10) == -1) {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    
    return listenFd;
}
 
int make_socket_non_blocking(int fd)
{
  int flags, s;
 
  flags = fcntl (fd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }
 
  flags |= O_NONBLOCK;
  s = fcntl (fd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }
 
  return 0;
}
 
int start_http_server(unsigned int port)
{
    int listenfd = -1;
    unsigned int i;
    int pid;
    /* 初始化互斥量 */
    initMutex();

    /* 初始化，创建监听端口 */
    listenfd = startup(port);
    if (-1 == listenfd)
    {
        return 0;
    }
    make_socket_non_blocking(listenfd);

    // 创建子进程
    for(i=0;i<WORK_REAL;i++)
    {
        pid = fork();
        if(pid==0)
                break;
        else if(pid>0)
                workers[i] = pid;
	else
		return 0;
    }

    // child process
    if (pid == 0)
    {
	
	webconnect* srvconn = NULL;
	init_webconnect(srvconn);
	srvconn->handler = web_accept;
	
	int nfds;
    	struct epoll_event ev,events[POLL_SIZE] = {0};
	epfd = epoll_create(POLL_SIZE);
	ev.events = EPOLLIN | EPOLLET;
	srvconn->sockfd = listenfd;
    	ev.data.ptr = (void*)srvconn;
	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
	while(1)
	{
		int n;
    		nfds = epoll_wait(epfd, events, POLL_SIZE, -1);
        	for (n = 0; n < nfds; ++n) 
		{
			/*
			 * 如果是主socket的事件的话，则表示
			 * 有新连接进入了，进行新连接的处理。
			 */

			if(events[n].events & EPOLLIN)
	                {
                          	//      

                          	webconnect *conn = (webconnect *)events[n].data.ptr;

                          	if(conn->sockfd == listenfd)
                          	{
					if (pthread_mutex_trylock(mutex)==0)
					{
						conn->handler(conn);
						pthread_mutex_unlock(mutex);
					}
                		}
                		else
                		{

					conn->handler(conn);
					conn->handler = send_response;
	                        	events[n].events = EPOLLOUT | EPOLLET;

            		        	epoll_ctl(epfd, EPOLL_CTL_MOD, conn->sockfd, &events[n]);

               		 	}
			}
			else if(events[n].events & EPOLLOUT)
	               {
                  	     webconnect *conn = (webconnect*)(events[n].data.ptr);
                	     conn->handler(conn);
			     conn->handler = read_request;
                	     events[n].events = EPOLLIN | EPOLLET;
                	     epoll_ctl(epfd, EPOLL_CTL_MOD, conn->sockfd, &events[n]);
            		}
        	}
    	}	
    }
    return 1;
}
