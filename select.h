/*
 * Event select
 * Copyright (c) 2002-2006, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file defines an event loop interface that supports processing events
 * from registered timeouts (i.e., do something after N seconds), sockets
 * (e.g., a new packet available for reading), and signals. eloop.c is an
 * implementation of this interface using select() and sockets. This is
 * suitable for most UNIX/POSIX systems. When porting to other operating
 * systems, it may be necessary to replace that implementation with OS specific
 * mechanisms.
 */
#include "list.h"
#include "os.h"
#ifndef SELECT_H
#define SELECT_H
/**
 * ELOOP_ALL_CTX - eloop_cancel_timeout() magic number to match all timeouts
 */
#define SELECT_ALL_CTX (void *) -1

#define MAXLINE     1024
#define max(a,b) (a > b) ? a : b

/**
 * eloop_event_type - eloop socket event type for eloop_register_sock()
 * @EVENT_TYPE_READ: Socket has data available for reading
 * @EVENT_TYPE_WRITE: Socket has room for new data to be written
 * @EVENT_TYPE_EXCEPTION: An exception has been reported
 */
typedef enum {
	EVENT_TYPE_READ = 0,
	EVENT_TYPE_WRITE,
	EVENT_TYPE_EXCEPTION,
} select_event_type;

/*
 * select_sock_handler - eloop socket event callback type
 * @sock: File descriptor number for the socket
 * @server_ctx: Registered callback context data (eloop_data)
 * @uagent_ctx: Registered callback context data (user_data)
 */
typedef void (*select_sock_handler)(int sock, void *server_ctx, void *uagent_ctx);

/**
 * eloop_timeout_handler - eloop timeout event callback type
 * @server_ctx: Registered callback context data (eloop_data)
 * @uagent_ctx: Registered callback context data (user_data)
 */
typedef void (*select_timeout_handler)(void *server_data, void *uagent_ctx);

typedef void (*select_signal_handler)(void *server_data, void *uagent_ctx);
/**
 * select_init() - Initialize global select data
 * Returns: 0 on success, -1 on failure
 *
 * This function must be called before any other select_* function.
 */

int select_init(void);

/**
 * select_register_read_sock - Register handler for read events
 * @sock: File descriptor number for the socket
 * @handler: Callback function to be called when data is available for reading
 * @server_data: Callback context data (server_ctx)
 * @uagent_data: Callback context data (uagent_ctx)
 * Returns: 0 on success, -1 on failure
 *
 * Register a read socket notifier for the given file descriptor. The handler
 * function will be called whenever data is available for reading from the
 * socket. The handler function is responsible for clearing the event after
 * having processed it in order to avoid eloop from calling the handler again
 * for the same event.
 */
int select_register_read_sock(int sock, select_sock_handler handler,
			     void *select_data, void *user_data);

/**
 * eloop_register_timeout - Register timeout
 * @secs: Number of seconds to the timeout
 * @usecs: Number of microseconds to the timeout
 * @handler: Callback function to be called when timeout occurs
 * @eloop_data: Callback context data (select_ctx)
 * @user_data: Callback context data (select_ctx)
 * Returns: 0 on success, -1 on failure
 *
 * Register a timeout that will cause the handler function to be called after
 * given time.
 */
int select_register_timeout(unsigned int secs, unsigned int usecs,
			   select_timeout_handler handler,
			   void *uagent_data, void *server_data);

/**
 * eloop_cancel_timeout - Cancel timeouts
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data or %ELOOP_ALL_CTX to match all
 * @user_data: Matching user_data or %ELOOP_ALL_CTX to match all
 * Returns: Number of cancelled timeouts
 *
 * Cancel matching <handler,eloop_data,user_data> timeouts registered with
 * eloop_register_timeout(). ELOOP_ALL_CTX can be used as a wildcard for
 * cancelling all timeouts regardless of eloop_data/user_data.
 */
int select_cancel_timeout(select_timeout_handler handler,
			 void *uagent_data, void *server_data);

/**
 * eloop_cancel_timeout_one - Cancel a single timeout
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data
 * @user_data: Matching user_data
 * @remaining: Time left on the cancelled timer
 * Returns: Number of cancelled timeouts
 *
 * Cancel matching <handler,eloop_data,user_data> timeout registered with
 * eloop_register_timeout() and return the remaining time left.
 */
int select_cancel_timeout_one(select_timeout_handler handler,
			     void *server_data, void *uagent_data,
			     struct os_time *remaining);

/**
 * eloop_is_timeout_registered - Check if a timeout is already registered
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data
 * @user_data: Matching user_data
 * Returns: 1 if the timeout is registered, 0 if the timeout is not registered
 *
 * Determine if a matching <handler,eloop_data,user_data> timeout is registered
 * with eloop_register_timeout().
 */
int select_is_timeout_registered(select_timeout_handler handler,
				void *server_data, void *uagent_data);

/**
 * select_run - Start the select loop
 *
 * Start the select loop and continue running as long as there are any
 * registered event handlers. This function is run after event loop has been
 * initialized with event_init() and one or more events have been registered.
 */
void select_run(void);

/**
 * select_terminate - Terminate event loop
 *
 * Terminate event loop even if there are registered events. This can be used
 * to request the program to be terminated cleanly.
 */
void select_terminate(void);

/**
 * select_destroy - Free any resources allocated for the event loop
 *
 * After calling eloop_destroy(), other eloop_* functions must not be called
 * before re-running eloop_init().
 */
void select_destroy(void);

/**
 * select_terminated - Check whether event loop has been terminated
 * Returns: 1 = event loop terminate, 0 = event loop still running
 *
 * This function can be used to check whether eloop_terminate() has been called
 * to request termination of the event loop. This is normally used to abort
 * operations that may still be queued to be run when eloop_terminate() was
 * called.
 */
int select_terminated(void);

/**
 * eloop_wait_for_read_sock - Wait for a single reader
 * @sock: File descriptor number for the socket
 *
 * Do a blocking wait for a single read socket.
 */
void select_wait_for_read_sock(int sock);

struct select_sock {
	int sock;
	void *select_data;
	void *user_data;
	select_sock_handler handler;
};

struct select_timeout {
	struct dl_list list;
	struct os_time time;
	void *select_data;
	void *user_data;
	select_timeout_handler handler;
};

struct select_signal {
	int sig;
	void *user_data;
	select_signal_handler handler;
	int signaled;
};

struct select_sock_table {
	int count;
	struct select_sock *table;
	int changed;
};

struct select_data {
	void *user_data;
	int max_sock;

	int count; /* sum of all table counts */
	struct select_sock_table readers;
	struct select_sock_table writers;
	struct select_sock_table exceptions;

	struct dl_list timeout;

	int signal_count;
	struct select_signal *signals;
	int signaled;
	int pending_terminate;

	int terminate;
	int reader_table_changed;
};
#ifdef SEC_PRODUCT_FEATURE_WLAN_CHINA_WAPI
void * select_get_user_data(void);
#endif
#endif 