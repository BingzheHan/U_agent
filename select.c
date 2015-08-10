/*
 * Event loop based on select() loop
 * Copyright (c) 2015-2020, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
//#include "trace.h"
#include "list.h"
#include "select.h"


static struct select_data uagent_select;


#define select_trace_sock_add_ref(table) do { } while (0)
#define select_trace_sock_remove_ref(table) do { } while (0)


int select_init(void)
{
	os_memset(&uagent_select, 0, sizeof(uagent_select));
	dl_list_init(&uagent_select.timeout);
	uagent_printf(MSG_INFO,"select init is okay.\n");
	return 0;
}

static int select_sock_table_add_sock(struct select_sock_table *table,
                                     int sock, select_sock_handler handler,
                                     void *select_data, void *user_data)
{
	struct select_sock *tmp;
	int new_max_sock;

	if (sock > uagent_select.max_sock)
		new_max_sock = sock;
	else
		new_max_sock = uagent_select.max_sock;

	if (table == NULL)
		return -1;

	select_trace_sock_remove_ref(table);
	tmp = os_realloc_array(table->table, table->count + 1,
			       sizeof(struct select_sock));
	if (tmp == NULL)
		return -1;

	tmp[table->count].sock = sock;
	tmp[table->count].select_data = select_data;
	tmp[table->count].user_data = user_data;
	tmp[table->count].handler = handler;
	/*wpa_trace_record(&tmp[table->count]);*/
	table->count++;
	table->table = tmp;
	uagent_select.max_sock = new_max_sock;
	uagent_select.count++;
	table->changed = 1;
	select_trace_sock_add_ref(table);

	return 0;
}


static void select_sock_table_remove_sock(struct select_sock_table *table,
                                         int sock)
{
	int i;

	if (table == NULL || table->table == NULL || table->count == 0)
		return;

	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock == sock)
			break;
	}
	if (i == table->count)
		return;
	select_trace_sock_remove_ref(table);
	if (i != table->count - 1) {
		os_memmove(&table->table[i], &table->table[i + 1],
			   (table->count - i - 1) *
			   sizeof(struct select_sock));
	}
	table->count--;
	uagent_select.count--;
	table->changed = 1;
	select_trace_sock_add_ref(table);
}



static void select_sock_table_set_fds(struct select_sock_table *table,
				     fd_set *fds)
{
	int i;

	FD_ZERO(fds);

	if (table->table == NULL)
		return;
      uagent_printf(MSG_INFO,"select sock table set fds\n");
	for (i = 0; i < table->count; i++)
		FD_SET(table->table[i].sock, fds);
}


static void select_sock_table_dispatch(struct select_sock_table *table,
				      fd_set *fds)
{
	int i;

	if (table == NULL || table->table == NULL)
		return;

	table->changed = 0;
	for (i = 0; i < table->count; i++) {
		if (FD_ISSET(table->table[i].sock, fds)) {
			table->table[i].handler(table->table[i].sock,
						table->table[i].select_data,
						table->table[i].user_data);
			if (table->changed)
				break;
		}
	}
}



static void select_sock_table_destroy(struct select_sock_table *table)
{
	if (table) {
		int i;
		for (i = 0; i < table->count && table->table; i++) {
			uagent_printf(MSG_INFO, "select: remaining socket: "
				   "sock=%d select_data=%p user_data=%p "
				   "handler=%p",
				   table->table[i].sock,
				   table->table[i].select_data,
				   table->table[i].user_data,
				   table->table[i].handler);
			/*uagent_trace_dump_funcname("select unregistered socket "
						"handler",
						table->table[i].handler);
			uagent_trace_dump("select sock", &table->table[i]);*/
		}
		os_free(table->table);
	}
}


int select_register_read_sock(int sock, select_sock_handler handler,
			     void *select_data, void *user_data)
{
	uagent_printf(MSG_INFO,"Begain to register read sock %d\n",sock);
	return select_register_sock(sock, EVENT_TYPE_READ, handler,
				   select_data, user_data);
}


void select_unregister_read_sock(int sock)
{
	select_unregister_sock(sock, EVENT_TYPE_READ);
}


static struct select_sock_table *select_get_sock_table(select_event_type type)
{
	switch (type) {
	case EVENT_TYPE_READ:
		return &uagent_select.readers;
	case EVENT_TYPE_WRITE:
		return &uagent_select.writers;
	case EVENT_TYPE_EXCEPTION:
		return &uagent_select.exceptions;
	}

	return NULL;
}


int select_register_sock(int sock, select_event_type type,
			select_sock_handler handler,
			void *select_data, void *user_data)
{
	struct select_sock_table *table;

	table = select_get_sock_table(type);
	return select_sock_table_add_sock(table, sock, handler,
					 select_data, user_data);
}


void select_unregister_sock(int sock, select_event_type type)
{
	struct select_sock_table *table;

	table = select_get_sock_table(type);
	select_sock_table_remove_sock(table, sock);
}


int select_register_timeout(unsigned int secs, unsigned int usecs,
			   select_timeout_handler handler,
			   void *select_data, void *user_data)
{
	struct select_timeout *timeout, *tmp;
	os_time_t now_sec;

	timeout = os_zalloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;
	if (os_get_time(&timeout->time) < 0) {
		os_free(timeout);
		return -1;
	}
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) {
		/*
		 * Integer overflow - assume long enough timeout to be assumed
		 * to be infinite, i.e., the timeout would never happen.
		 */
		uagent_printf(MSG_INFO, "select: Too long timeout (secs=%u) to "
			   "ever happen - ignore it", secs);
		os_free(timeout);
		return 0;
	}
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->select_data = select_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	/*wpa_trace_add_ref(timeout, uagent_select, select_data);
	wpa_trace_add_ref(timeout, user, user_data);
	wpa_trace_record(timeout);*/

	/* Maintain timeouts in order of increasing time */
	dl_list_for_each(tmp, &uagent_select.timeout, struct select_timeout, list) {
		if (os_time_before(&timeout->time, &tmp->time)) {
			dl_list_add(tmp->list.prev, &timeout->list);
			return 0;
		}
	}
	dl_list_add_tail(&uagent_select.timeout, &timeout->list);

	return 0;
}


static void select_remove_timeout(struct select_timeout *timeout)
{
	dl_list_del(&timeout->list);
	/*wpa_trace_remove_ref(timeout, uagent_select, timeout->select_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);*/
	os_free(timeout);
}


int select_cancel_timeout(select_timeout_handler handler,
			 void *select_data, void *user_data)
{
	struct select_timeout *timeout, *prev;
	int removed = 0;

	dl_list_for_each_safe(timeout, prev, &uagent_select.timeout,
			      struct select_timeout, list) {
		if (timeout->handler == handler &&
		    (timeout->select_data == select_data ||
		     select_data == SELECT_ALL_CTX) &&
		    (timeout->user_data == user_data ||
		     user_data == SELECT_ALL_CTX)) {
			select_remove_timeout(timeout);
			removed++;
		}
	}

	return removed;
}


int select_cancel_timeout_one(select_timeout_handler handler,
			     void *select_data, void *user_data,
			     struct os_time *remaining)
{
	struct select_timeout *timeout, *prev;
	int removed = 0;
	struct os_time now;

	os_get_time(&now);
	remaining->sec = remaining->usec = 0;

	dl_list_for_each_safe(timeout, prev, &uagent_select.timeout,
			      struct select_timeout, list) {
		if (timeout->handler == handler &&
		    (timeout->select_data == select_data) &&
		    (timeout->user_data == user_data)) {
			removed = 1;
			if (os_time_before(&now, &timeout->time))
				os_time_sub(&timeout->time, &now, remaining);
			select_remove_timeout(timeout);
			break;
		}
	}
	return removed;
}


int select_is_timeout_registered(select_timeout_handler handler,
				void *select_data, void *user_data)
{
	struct select_timeout *tmp;

	dl_list_for_each(tmp, &uagent_select.timeout, struct select_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->select_data == select_data &&
		    tmp->user_data == user_data)
			return 1;
	}

	return 0;
}

#ifdef SIGNAL_HANDLE
#ifndef CONFIG_NATIVE_WINDOWS
#ifdef SEC_PRODUCT_FEATURE_WLAN_CHINA_WAPI
void select_handle_alarm(int sig)
#else
static void select_handle_alarm(int sig)
#endif
{
	wpa_printf(MSG_ERROR, "select: could not process SIGINT or SIGTERM in "
		   "two seconds. Looks like there\n"
		   "is a bug that ends up in a busy loop that "
		   "prevents clean shutdown.\n"
		   "Killing program forcefully.\n");
	exit(1);
}
#endif /* CONFIG_NATIVE_WINDOWS */


static void select_handle_signal(int sig)
{
	int i;

#ifndef CONFIG_NATIVE_WINDOWS
	if ((sig == SIGINT || sig == SIGTERM) && !select.pending_terminate) {
		/* Use SIGALRM to break out from potential busy loops that
		 * would not allow the program to be killed. */
		select.pending_terminate = 1;
		signal(SIGALRM, select_handle_alarm);
		alarm(2);
	}
#endif /* CONFIG_NATIVE_WINDOWS */

	select.signaled++;
	for (i = 0; i < select.signal_count; i++) {
		if (select.signals[i].sig == sig) {
			select.signals[i].signaled++;
			break;
		}
	}
}


static void select_process_pending_signals(void)
{
	int i;

	if (select.signaled == 0)
		return;
	select.signaled = 0;

	if (select.pending_terminate) {
#ifndef CONFIG_NATIVE_WINDOWS
		alarm(0);
#endif /* CONFIG_NATIVE_WINDOWS */
		select.pending_terminate = 0;
	}

	for (i = 0; i < select.signal_count; i++) {
		if (select.signals[i].signaled) {
			select.signals[i].signaled = 0;
			select.signals[i].handler(select.signals[i].sig,
						 select.signals[i].user_data);
		}
	}
}


int select_register_signal(int sig, select_signal_handler handler,
			  void *user_data)
{
	struct select_signal *tmp;

	tmp = os_realloc_array(select.signals, select.signal_count + 1,
			       sizeof(struct select_signal));
	if (tmp == NULL)
		return -1;

	tmp[select.signal_count].sig = sig;
	tmp[select.signal_count].user_data = user_data;
	tmp[select.signal_count].handler = handler;
	tmp[select.signal_count].signaled = 0;
	select.signal_count++;
	select.signals = tmp;
	signal(sig, select_handle_signal);

	return 0;
}


int select_register_signal_terminate(select_signal_handler handler,
				    void *user_data)
{
	int ret = select_register_signal(SIGINT, handler, user_data);
	if (ret == 0)
		ret = select_register_signal(SIGTERM, handler, user_data);
	return ret;
}


int select_register_signal_reconfig(select_signal_handler handler,
				   void *user_data)
{
#ifdef CONFIG_NATIVE_WINDOWS
	return 0;
#else /* CONFIG_NATIVE_WINDOWS */
	return select_register_signal(SIGHUP, handler, user_data);
#endif /* CONFIG_NATIVE_WINDOWS */
}
#endif

void select_run(void)
{

	fd_set *rfds, *wfds, *efds;
	struct timeval _tv;
	int res;
	struct os_time tv, now;
	rfds = os_malloc(sizeof(*rfds));
	wfds = os_malloc(sizeof(*wfds));
	efds = os_malloc(sizeof(*efds));
	if (rfds == NULL || wfds == NULL || efds == NULL)
		goto out;
	while (!uagent_select.terminate &&
	       (!dl_list_empty(&uagent_select.timeout) || uagent_select.readers.count > 0 ||
		uagent_select.writers.count > 0 || uagent_select.exceptions.count > 0)) {
		struct select_timeout *timeout;
		timeout = dl_list_first(&uagent_select.timeout, struct select_timeout,
					list);
		if (timeout) {
			os_get_time(&now);
			if (os_time_before(&now, &timeout->time))
				os_time_sub(&timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
			_tv.tv_sec = tv.sec;
			_tv.tv_usec = tv.usec;
		}

		select_sock_table_set_fds(&uagent_select.readers, rfds);
		select_sock_table_set_fds(&uagent_select.writers, wfds);
		select_sock_table_set_fds(&uagent_select.exceptions, efds);
		res = select(uagent_select.max_sock + 1, rfds, wfds, efds,
			     timeout ? &_tv : NULL);
		if (res < 0 && errno != EINTR && errno != 0) {
			perror("select");
			goto out;
		}
		/*select_process_pending_signals();*/

		/* check if some registered timeouts have occurred */
		timeout = dl_list_first(&uagent_select.timeout, struct select_timeout,
					list);
		if (timeout) {
			os_get_time(&now);
			if (!os_time_before(&now, &timeout->time)) {
				void *select_data = timeout->select_data;
				void *user_data = timeout->user_data;
				select_timeout_handler handler =
					timeout->handler;
				select_remove_timeout(timeout);
				handler(select_data, user_data);
			}

		}

		if (res <= 0)
			continue;

		select_sock_table_dispatch(&uagent_select.readers, rfds);
		select_sock_table_dispatch(&uagent_select.writers, wfds);
		select_sock_table_dispatch(&uagent_select.exceptions, efds);
	}

	uagent_select.terminate = 0;
out:
	os_free(rfds);
	os_free(wfds);
	os_free(efds);
	return;
}


void select_terminate(void)
{
	uagent_select.terminate = 1;
}


void select_destroy(void)
{
	struct select_timeout *timeout, *prev;
	struct os_time now;

	os_get_time(&now);
	dl_list_for_each_safe(timeout, prev, &uagent_select.timeout,
			      struct select_timeout, list) {
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) {
			sec--;
			usec += 1000000;
		}
		uagent_printf(MSG_INFO, "select: remaining timeout: %d.%06d "
			   "select_data=%p user_data=%p handler=%p",
			   sec, usec, timeout->select_data, timeout->user_data,
			   timeout->handler);
		/*wpa_trace_dump_funcname("select unregistered timeout handler",
					timeout->handler);
		wpa_trace_dump("select timeout", timeout);*/
		select_remove_timeout(timeout);
	}
	select_sock_table_destroy(&uagent_select.readers);
	select_sock_table_destroy(&uagent_select.writers);
	select_sock_table_destroy(&uagent_select.exceptions);
	os_free(uagent_select.signals);

#ifdef CONFIG_select_POLL
	os_free(uagent_select.pollfds);
	os_free(uagent_select.pollfds_map);
#endif /* CONFIG_select_POLL */
}


int select_terminated(void)
{
	return uagent_select.terminate;
}


void select_wait_for_read_sock(int sock)
{
#ifdef CONFIG_select_POLL
	struct pollfd pfd;

	if (sock < 0)
		return;

	os_memset(&pfd, 0, sizeof(pfd));
	pfd.fd = sock;
	pfd.events = POLLIN;

	poll(&pfd, 1, -1);
#else /* CONFIG_select_POLL */
	fd_set rfds;

	if (sock < 0)
		return;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	select(sock + 1, &rfds, NULL, NULL, NULL);
#endif /* CONFIG_select_POLL */
}
#ifdef SEC_PRODUCT_FEATURE_WLAN_CHINA_WAPI
void * select_get_user_data(void)
{
	return uagent_select.user_data;
}
#endif
