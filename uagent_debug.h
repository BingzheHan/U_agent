/*
 * Debug prints
 * Copyright (c) 2015-2020, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef uagent_DEBUG_H
#define uagent_DEBUG_H

#include "uagentbuf.h"

/* Debugging function - conditional printf and hex dump. Driver wrappers can
 * use these for debugging purposes. */
extern uagent_debug_level;
enum {
	MSG_INFO, MSG_WARNING, MSG_ERROR
};

#ifdef CONFIG_NO_STDOUT_DEBUG

#define uagent_debug_print_timestamp() do { } while (0)
#define uagent_printf(args...) do { } while (0)
#define uagent_hexdump(l,t,b,le) do { } while (0)
#define uagent_hexdump_buf(l,t,b) do { } while (0)
#define uagent_hexdump_key(l,t,b,le) do { } while (0)
#define uagent_hexdump_buf_key(l,t,b) do { } while (0)
#define uagent_hexdump_ascii(l,t,b,le) do { } while (0)
#define uagent_hexdump_ascii_key(l,t,b,le) do { } while (0)
#define uagent_debug_open_file(p) do { } while (0)
#define uagent_debug_close_file() do { } while (0)
#define uagent_dbg(args...) do { } while (0)

static inline int uagent_debug_reopen_file(void)
{
	return 0;
}

#else /* CONFIG_NO_STDOUT_DEBUG */

int uagent_debug_open_file(const char *path);
int uagent_debug_reopen_file(void);
void uagent_debug_close_file(void);

/**
 * uagent_debug_printf_timestamp - Print timestamp for debug output
 *
 * This function prints a timestamp in seconds_from_1970.microsoconds
 * format if debug output has been configured to include timestamps in debug
 * messages.
 */
void uagent_debug_print_timestamp(void);

/**
 * uagent_printf - conditional printf
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration.
 *
 * Note: New line '\n' is added to the end of the text when printing to stdout.
 */
void uagent_printf(int level, const char *fmt, ...)
PRINTF_FORMAT(2, 3);

/**
 * uagent_hexdump - conditional hex dump
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump.
 */
void uagent_hexdump(int level, const char *title, const u8 *buf, size_t len);

static inline void uagent_hexdump_buf(int level, const char *title,
				   const struct uagentbuf *buf)
{
	uagent_hexdump(level, title, buf ? uagentbuf_head(buf) : NULL,
		    buf ? uagentbuf_len(buf) : 0);
}

/**
 * uagent_hexdump_key - conditional hex dump, hide keys
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump. This works
 * like uagent_hexdump(), but by default, does not include secret keys (passwords,
 * etc.) in debug output.
 */
void uagent_hexdump_key(int level, const char *title, const u8 *buf, size_t len);

static inline void uagent_hexdump_buf_key(int level, const char *title,
				       const struct uagentbuf *buf)
{
	uagent_hexdump_key(level, title, buf ? uagentbuf_head(buf) : NULL,
			buf ? uagentbuf_len(buf) : 0);
}

/**
 * uagent_hexdump_ascii - conditional hex dump
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump with both
 * the hex numbers and ASCII characters (for printable range) are shown. 16
 * bytes per line will be shown.
 */
void uagent_hexdump_ascii(int level, const char *title, const u8 *buf,
		       size_t len);

/**
 * uagent_hexdump_ascii_key - conditional hex dump, hide keys
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump with both
 * the hex numbers and ASCII characters (for printable range) are shown. 16
 * bytes per line will be shown. This works like uagent_hexdump_ascii(), but by
 * default, does not include secret keys (passwords, etc.) in debug output.
 */
void uagent_hexdump_ascii_key(int level, const char *title, const u8 *buf,
			   size_t len);

/*
 * uagent_dbg() behaves like uagent_msg(), but it can be removed from build to reduce
 * binary size. As such, it should be used with debugging messages that are not
 * needed in the control interface while uagent_msg() has to be used for anything
 * that needs to shown to control interface monitors.
 */
#define uagent_dbg(args...) uagent_msg(args)

#endif /* CONFIG_NO_STDOUT_DEBUG */


#ifdef CONFIG_NO_uagent_MSG
#define uagent_msg(args...) do { } while (0)
#define uagent_msg_sec(args...) do { } while (0)
#define uagent_msg_ctrl(args...) do { } while (0)
#define uagent_msg_global(args...) do { } while (0)
#define uagent_msg_global_sec(args...) do { } while (0)
#define uagent_msg_no_global(args...) do { } while (0)
#define uagent_msg_register_cb(f) do { } while (0)
#define uagent_msg_register_ifname_cb(f) do { } while (0)
#else /* CONFIG_NO_uagent_MSG */
/**
 * uagent_msg - Conditional printf for default target and ctrl_iface monitors
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct uagent_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. This function is like uagent_printf(), but it also sends the
 * same message to all attached ctrl_iface monitors.
 *
 * Note: New line '\n' is added to the end of the text when printing to stdout.
 */
void uagent_msg(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);
void uagent_msg_sec(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * uagent_msg_ctrl - Conditional printf for ctrl_iface monitors
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct uagent_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like uagent_msg(), but it sends the output only to the
 * attached ctrl_iface monitors. In other words, it can be used for frequent
 * events that do not need to be sent to syslog.
 */
void uagent_msg_ctrl(void *ctx, int level, const char *fmt, ...)
PRINTF_FORMAT(3, 4);

/**
 * uagent_msg_global - Global printf for ctrl_iface monitors
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct uagent_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like uagent_msg(), but it sends the output as a global event,
 * i.e., without being specific to an interface. For backwards compatibility,
 * an old style event is also delivered on one of the interfaces (the one
 * specified by the context data).
 */
void uagent_msg_global(void *ctx, int level, const char *fmt, ...)
PRINTF_FORMAT(3, 4);
void uagent_msg_global_sec(void *ctx, int level, const char *fmt, ...)
PRINTF_FORMAT(3, 4);

/**
 * uagent_msg_no_global - Conditional printf for ctrl_iface monitors
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct uagent_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like uagent_msg(), but it does not send the output as a global
 * event.
 */
void uagent_msg_no_global(void *ctx, int level, const char *fmt, ...)
PRINTF_FORMAT(3, 4);

typedef void (*uagent_msg_cb_func)(void *ctx, int level, int global,
				const char *txt, size_t len);

/**
 * uagent_msg_register_cb - Register callback function for uagent_msg() messages
 * @func: Callback function (%NULL to unregister)
 */
void uagent_msg_register_cb(uagent_msg_cb_func func);

typedef const char * (*uagent_msg_get_ifname_func)(void *ctx);
void uagent_msg_register_ifname_cb(uagent_msg_get_ifname_func func);

#endif /* CONFIG_NO_uagent_MSG */

#ifdef CONFIG_NO_HOSTAPD_LOGGER
#define hostapd_logger(args...) do { } while (0)
#define hostapd_logger_register_cb(f) do { } while (0)
#else /* CONFIG_NO_HOSTAPD_LOGGER */
void hostapd_logger(void *ctx, const u8 *addr, unsigned int module, int level,
		    const char *fmt, ...) PRINTF_FORMAT(5, 6);

typedef void (*hostapd_logger_cb_func)(void *ctx, const u8 *addr,
				       unsigned int module, int level,
				       const char *txt, size_t len);

/**
 * hostapd_logger_register_cb - Register callback function for hostapd_logger()
 * @func: Callback function (%NULL to unregister)
 */
void hostapd_logger_register_cb(hostapd_logger_cb_func func);
#endif /* CONFIG_NO_HOSTAPD_LOGGER */

#define HOSTAPD_MODULE_IEEE80211	0x00000001
#define HOSTAPD_MODULE_IEEE8021X	0x00000002
#define HOSTAPD_MODULE_RADIUS		0x00000004
#define HOSTAPD_MODULE_uagent		0x00000008
#define HOSTAPD_MODULE_DRIVER		0x00000010
#define HOSTAPD_MODULE_IAPP		0x00000020
#define HOSTAPD_MODULE_MLME		0x00000040

enum hostapd_logger_level {
	HOSTAPD_LEVEL_DEBUG_VERBOSE = 0,
	HOSTAPD_LEVEL_DEBUG = 1,
	HOSTAPD_LEVEL_INFO = 2,
	HOSTAPD_LEVEL_NOTICE = 3,
	HOSTAPD_LEVEL_WARNING = 4
};


#ifdef CONFIG_DEBUG_SYSLOG

void uagent_debug_open_syslog(void);
void uagent_debug_close_syslog(void);

#else /* CONFIG_DEBUG_SYSLOG */

static inline void uagent_debug_open_syslog(void)
{
}

static inline void uagent_debug_close_syslog(void)
{
}

#endif /* CONFIG_DEBUG_SYSLOG */

#ifdef CONFIG_DEBUG_LINUX_TRACING

int uagent_debug_open_linux_tracing(void);
void uagent_debug_close_linux_tracing(void);

#else /* CONFIG_DEBUG_LINUX_TRACING */

static inline int uagent_debug_open_linux_tracing(void)
{
	return 0;
}

static inline void uagent_debug_close_linux_tracing(void)
{
}

#endif /* CONFIG_DEBUG_LINUX_TRACING */


#ifdef EAPOL_TEST
#define uagent_ASSERT(a)						       \
	do {							       \
		if (!(a)) {					       \
			printf("uagent_ASSERT FAILED '" #a "' "	       \
			       "%s %s:%d\n",			       \
			       __FUNCTION__, __FILE__, __LINE__);      \
			exit(1);				       \
		}						       \
	} while (0)
#else
#define uagent_ASSERT(a) do { } while (0)
#endif

#endif /* uagent_DEBUG_H */
