/*
 * User Agent
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

void stdin_fileno_receive(int sockfd, void *server1fd, void *server2fd)
{	
	char    sendline[MAXLINE];
	int n;
	int socketfd1 = *((int *)server1fd);
	int socketfd2 = *((int *)server2fd);
	uagent_printf(MSG_INFO, "STDIN is received \n");
	n = read(sockfd,sendline,MAXLINE);
	write(socketfd1,sendline,n);
	write(socketfd2,sendline,n);
}	

void sockfd_receive(int sockfd, void *server_ctx, void *uagent_ctx)
{	
	char   recvline[MAXLINE];
	char   buf[MAXLINE];
	int n,m;
	int cmd_type;
	int rsp_len;
	struct server_msg server_rev_msg;
	struct status_data dev_status;
	uagent_printf(MSG_INFO, "Sockfd%d server is received \n",sockfd);
	n = read(sockfd,recvline,MAXLINE);
	m = sizeof(struct server_msg);
	rsp_len = sizeof(struct resp_data);
	uagent_printf(MSG_INFO, "Size of server_msg is %d \n",m);
	uagent_hexdump(MSG_ERROR,"AZHE",recvline,n);
	if (m == n )
		{ 	
			memcpy(&server_rev_msg,recvline,sizeof(struct server_msg));
			cmd_type = server_rev_msg.srv_cmd;
			uagent_printf(MSG_ERROR, "sockfd %d server is received server_cmd %d.\n",
				sockfd,cmd_type);
			struct resp_data resp_server;
			resp_server = handle_server_msg( server_rev_msg, &dev_status);
			uagent_printf(MSG_ERROR, "The response cmd is %d \n", resp_server.srv_cmd);
			memcpy(buf,&resp_server,rsp_len);
			if(resp_server.srv_cmd == STATUS)
			{
				memcpy(buf+rsp_len,&dev_status,sizeof(struct status_data));
				rsp_len += sizeof(struct status_data);
			}
			uagent_hexdump(MSG_ERROR, "AZHE", buf, rsp_len);
			write(sockfd,buf,rsp_len);
		}
	
	//= sizeof(struct server_msg);
	//write(STDOUT_FILENO,recvline,n);
}	

void demon_learn_timeout(void *eloop_ctx, void *timeout_ctx)
{
	select_register_timeout(5,0,demon_learn_timeout,NULL,NULL);
	uagent_printf(MSG_INFO, "Demon learn timemout is OKAY!\n");
	write(sockfd1,"Start server cmd\n",18);
	struct status_data dev_status;
	char buf[MAXLINE];
	/*dev_status.ibeacon_status = 0;
	dev_status.wifi_collect_module = 1;
	dev_status.net_type = 0;
	dev_status.cpu_usage = 20;
	dev_status.mem_usage = 30;*/
	dev_status = dev_status_handle();
	uagent_printf(MSG_ERROR,"the dev status about wifi collect module is %d,"
		"the ibeacon status is %d, the net type is %d, the cpu_usage is %d\n",
		dev_status.wifi_collect_module,dev_status.ibeacon_status,dev_status.net_type,
		 dev_status.cpu_usage);
	memcpy(buf,&dev_status,sizeof(struct status_data));
	write(sockfd2,buf,sizeof(struct status_data));
}
struct sockaddr_in client_bind_address( char *ipaddress, int serv_port)
{
	struct sockaddr_in  servaddr;
	//socketfd = socket(AF_INET,SOCK_STREAM,0);
	//sockfd2 = socket(AF_INET,SOCK_STREAM,0);
	//bzero(&servaddr1,sizeof(servaddr1));
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(serv_port);
	//servaddr2.sin_family = AF_INET;
	//servaddr2.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,ipaddress,&servaddr.sin_addr);
	return servaddr;
}
