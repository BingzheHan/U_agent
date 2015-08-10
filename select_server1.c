/*
 * Select User Server Example 
 * Copyright (c) 2015-2020, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file defines the interface and data structure processing command from 
 * server and sending command to other application  
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include "select.h"
#include "uagent.h"
#include "os.h"
#include "uagent_debug.h"
#include "common.h"
#include "server_cmd.h"


#define IPADDRESS   "127.0.0.1"
#define PORT        8787
#define MAXLINE     1024
#define LISTENQ     5 
static struct select_data server1_select;

static int socket_bind(const char* ip,int port);
static void do_select(int listenfd);
static void handle_connection(int *connfds,int num,fd_set *prset,fd_set *pallset);


int main(int argc,char *argv[])
{	
	
	int  listenfd,connfd,sockfd;
	//os_memset(&server1_select, 0, sizeof(server1_select));
	//dl_list_init(&server1_select.timeout);
	//select_register_timeout(5,0,demon_server1_timeout,NULL,NULL);
	struct sockaddr_in cliaddr;
      socklen_t cliaddrlen;
      listenfd = socket_bind(IPADDRESS,PORT);
      listen(listenfd,LISTENQ);
      do_select(listenfd);
      return 0;
}

static int socket_bind(const char* ip,int port)
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    return listenfd;
}

static void do_select(int listenfd)
{
    int  connfd,sockfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    fd_set  rset,allset;
    int maxfd,maxi;
    int i;
    int clientfds[FD_SETSIZE]; 
    int nready;
    for (i = 0;i < FD_SETSIZE;i++)
        clientfds[i] = -1;
    maxi = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
    maxfd = listenfd;
    for ( ; ; )
    {
        rset = allset;
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);
        if (nready == -1)
        {
            perror("select error:");
            exit(1);
        }
        
        if (FD_ISSET(listenfd,&rset))
        {
            cliaddrlen = sizeof(cliaddr);
            
            if ((connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen)) == -1)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                   perror("accept error:");
                   exit(1);
                }
            }
            fprintf(stdout,"accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
            
            for (i = 0;i <FD_SETSIZE;i++)
            {
                if (clientfds[i] < 0)
                {
                    clientfds[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE)
            {
                fprintf(stderr,"too many clients.\n");
                exit(1);
            }
            
            FD_SET(connfd,&allset);
            
            maxfd = (connfd > maxfd ? connfd : maxfd);
          
            maxi = (i > maxi ? i : maxi);
            if (--nready <= 0)
                continue;
        }
        
        handle_connection(clientfds,maxi,&rset,&allset);
    }
}

static void handle_connection(int *connfds,int num,fd_set *prset,fd_set *pallset)
{
    int i,n;
    char buf[MAXLINE];
    memset(buf,0,MAXLINE);
	struct server_msg server1_msg;
	server1_msg.srv_cmd = STATUS;
	char buf_cmd[MAXLINE];
	memset(buf_cmd, 0 ,MAXLINE);
	//memcpy(buf_cmd, &server1_msg,sizeof(struct server_msg));
    for (i = 0;i <= num;i++)
    {
        if (connfds[i] < 0)
            continue;
        
        if (FD_ISSET(connfds[i],prset))
        {
           
            n = read(connfds[i],buf,MAXLINE);
            if (n == 0)
            {
                close(connfds[i]);
                FD_CLR(connfds[i],pallset);
                connfds[i] = -1;
                continue;
            }
            printf("read msg is:\n ");
            write(STDOUT_FILENO,buf,n);
            //write(connfds[i],buf,n);
			memset(buf,0,MAXLINE);
			memcpy(buf_cmd, &server1_msg,sizeof(struct server_msg));
			write(connfds[i],buf_cmd,sizeof(struct server_msg));
			memset(buf_cmd,0,MAXLINE);
			
        }
    }
}
/*void demon_server1_timeout(void *eloop_ctx, void *timeout_ctx)
{
	select_register_timeout(5,0,demon_server1_timeout,NULL,NULL);
	uagent_printf(MSG_INFO, "Demon server timemout is OKAY!\n");
	write(sockfd1,"I am back1\n",12);
	write(sockfd2,"I am back2\n",12);
}*/