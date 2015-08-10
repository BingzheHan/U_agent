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

#ifndef _U_AGENT_FUNCTION_H
#define _U_AGENT_FUNCTION_H

#include "list.h"
#include "os.h"
#include "select.h"

extern int sockfd1,sockfd2;
/**
 * struct u_agent_params - Parameters for u_agent_init()
 */
struct uagent_params {
	/**
	 * daemonize - Run %wpa_supplicant in the background
	 */
	int daemonize;

	/**
	 * uagent_debug_level - Debugging verbosity level (e.g., MSG_INFO)
	 */
	int uagent_debug_level;

	/**
	 * wpa_debug_timestamp - Whether to include timestamp in debug messages
	 */
	int uagent_debug_timestamp;

	/**
	 * wpa_debug_file_path - Path of debug file or %NULL to use stdout
	 */
	const char *uagent_debug_file_path;

	/**
	 * wpa_debug_syslog - Enable log output through syslog
	 */
	int uagent_debug_syslog;
};
void sockfd_receive(int sockfd, void *server_ctx, void *uagent_ctx);
void stdin_fileno_receive(int sockfd, void *server1fd, void *server2fd);

 void demon_learn_timeout(void *eloop_ctx, void *timeout_ctx);
 /*void stdin_fileno_receive(void *eloop_ctx, void *timeout_ctx);
 void sockfd1_receive(void *eloop_ctx, void *timeout_ctx);
 void sockfd2_receive(void *eloop_ctx, void *timeout_ctx);*/
 struct sockaddr_in client_bind_address( char *ipaddress, int serv_port);

#endif /*_U_AGENT_FUNCTION_*/
