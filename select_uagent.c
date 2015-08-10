/*
 * Select User Agent
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
#include <errno.h> 
#include "select.h"
#include "uagent.h"
#include "os.h"
#include "uagent_debug.h"
#include "common.h"

const char *u_agent_version =
"u_agent v\n"
"Copyright (c) 2015-2020, Brad Han <Bingzhehan@gmail.com> and contributors";

const char *u_agent_license =
"This software may be distributed under the terms of the BSD license.\n"
"See README for more details.\n";

#define IPADDRESS1   "192.168.0.105"
#define IPADDRESS2   "192.168.0.105"
#define SERV_PORT1        8000
#define SERV_PORT2        8001
#define MAXLINE     1024
#define LISTENQ     5 
#define max(a,b) (a > b) ? a : b

static void usage(void)
{
	int i;
	printf("%s\n\n%s\n",
	       u_agent_version, u_agent_license);
}

int sockfd1, sockfd2;
int main(int argc,char *argv[])
{	
    	int c;
	struct uagent_params params;
	os_memset(&params, 0, sizeof(params));
	uagent_debug_level = MSG_INFO;
	
	for (;;) {
		c = getopt(argc, argv,
			   "p:BIEW");
		if (c < 0)
			break;
		switch (c) {
		case 'I':
			uagent_debug_level = MSG_INFO;
			uagent_printf(MSG_WARNING, "Uagent debug level is MSG_INFO !\n");
			break;
		case 'B':
			params.daemonize++;
			break;
		case 'E':
			uagent_debug_level = MSG_ERROR;
			uagent_printf(MSG_WARNING, "Uagent debug level is MSG_ERROR !\n");
			break;
		case 'W':
			uagent_debug_level = MSG_WARNING;
			uagent_printf(MSG_WARNING, "Uagent debug level is MSG_WARNING !\n");
			break;
		case 'p':
			params.uagent_debug_file_path = optarg;
			uagent_printf(MSG_WARNING, "Uagent debug file path is %s.\n", optarg);
			break;
		default:
			usage();
		}
	}	
	
	uagent_printf(MSG_INFO, "This is INFO msg.\n");
	uagent_printf(MSG_WARNING, "This is WARNING msg.\n");
	uagent_printf(MSG_ERROR, "This is ERROR msg.\n");
	int error1;
	int error2;
	select_init();
	select_register_timeout(5,0,demon_learn_timeout,NULL,NULL);
	struct sockaddr_in  servaddr1, servaddr2;
	sockfd1 = socket(AF_INET,SOCK_STREAM,0);
	sockfd2 = socket(AF_INET,SOCK_STREAM,0);
	servaddr1 = client_bind_address(IPADDRESS1, SERV_PORT1);
	servaddr2 = client_bind_address(IPADDRESS2, SERV_PORT2);
	error1 = connect(sockfd1,(struct sockaddr*)&servaddr1,sizeof(servaddr1));
	if (0 != error1)
		{
			uagent_printf(MSG_ERROR,"The connect error is %d\n",error1);
			uagent_printf(MSG_ERROR,"errno=%d\n",errno); 
			char * mesg = strerror(errno);
			uagent_printf("Mesg:%s\n",mesg); 
			return 0;
		}
	error2 = connect(sockfd2,(struct sockaddr*)&servaddr2,sizeof(servaddr2));
	if (0 != error2)
		{
			uagent_printf(MSG_ERROR,"The connect error is %d\n",error1);
			uagent_printf(MSG_ERROR,"errno=%d\n",errno); 
			char * mesg = strerror(errno);
			uagent_printf("Mesg:%s\n",mesg); 
			return 0;
		}
	//select_register_read_sock(STDIN_FILENO,stdin_fileno_receive,NULL,NULL);
	select_register_read_sock(sockfd1,sockfd_receive,NULL,NULL);
	select_register_read_sock(sockfd2,sockfd_receive,NULL,NULL);
	select_run();
      return 0;
}

