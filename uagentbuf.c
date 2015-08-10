/*
 * Dynamic data buffer
 * Copyright (c) 2015-2020, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "trace.h"
#include "uagentbuf.h"

#ifdef uagent_TRACE
#define uagentBUF_MAGIC 0x51a974e3

struct uagentbuf_trace {
	unsigned int magic;
};

static struct uagentbuf_trace * uagentbuf_get_trace(const struct uagentbuf *buf)
{
	return (struct uagentbuf_trace *)
		((const u8 *) buf - sizeof(struct uagentbuf_trace));
}
#endif /* uagent_TRACE */


static void uagentbuf_overflow(const struct uagentbuf *buf, size_t len)
{
#ifdef uagent_TRACE
	struct uagentbuf_trace *trace = uagentbuf_get_trace(buf);
	if (trace->magic != uagentBUF_MAGIC) {
		uagent_printf(MSG_ERROR, "uagentbuf: invalid magic %x",
			   trace->magic);
	}
#endif /* uagent_TRACE */
	uagent_printf(MSG_ERROR, "uagentbuf %p (size=%lu used=%lu) overflow len=%lu",
		   buf, (unsigned long) buf->size, (unsigned long) buf->used,
		   (unsigned long) len);
	uagent_trace_show("uagentbuf overflow");
	abort();
}


int uagentbuf_resize(struct uagentbuf **_buf, size_t add_len)
{
	struct uagentbuf *buf = *_buf;
#ifdef uagent_TRACE
	struct uagentbuf_trace *trace;
#endif /* uagent_TRACE */

	if (buf == NULL) {
		*_buf = uagentbuf_alloc(add_len);
		return *_buf == NULL ? -1 : 0;
	}

#ifdef uagent_TRACE
	trace = uagentbuf_get_trace(buf);
	if (trace->magic != uagentBUF_MAGIC) {
		uagent_printf(MSG_ERROR, "uagentbuf: invalid magic %x",
			   trace->magic);
		uagent_trace_show("uagentbuf_resize invalid magic");
		abort();
	}
#endif /* uagent_TRACE */

	if (buf->used + add_len > buf->size) {
		unsigned char *nbuf;
		if (buf->flags & uagentBUF_FLAG_EXT_DATA) {
			nbuf = os_realloc(buf->buf, buf->used + add_len);
			if (nbuf == NULL)
				return -1;
			os_memset(nbuf + buf->used, 0, add_len);
			buf->buf = nbuf;
		} else {
#ifdef uagent_TRACE
			nbuf = os_realloc(trace, sizeof(struct uagentbuf_trace) +
					  sizeof(struct uagentbuf) +
					  buf->used + add_len);
			if (nbuf == NULL)
				return -1;
			trace = (struct uagentbuf_trace *) nbuf;
			buf = (struct uagentbuf *) (trace + 1);
			os_memset(nbuf + sizeof(struct uagentbuf_trace) +
				  sizeof(struct uagentbuf) + buf->used, 0,
				  add_len);
#else /* uagent_TRACE */
			nbuf = os_realloc(buf, sizeof(struct uagentbuf) +
					  buf->used + add_len);
			if (nbuf == NULL)
				return -1;
			buf = (struct uagentbuf *) nbuf;
			os_memset(nbuf + sizeof(struct uagentbuf) + buf->used, 0,
				  add_len);
#endif /* uagent_TRACE */
			buf->buf = (u8 *) (buf + 1);
			*_buf = buf;
		}
		buf->size = buf->used + add_len;
	}

	return 0;
}


/**
 * uagentbuf_alloc - Allocate a uagentbuf of the given size
 * @len: Length for the allocated buffer
 * Returns: Buffer to the allocated uagentbuf or %NULL on failure
 */
struct uagentbuf * uagentbuf_alloc(size_t len)
{
#ifdef uagent_TRACE
	struct uagentbuf_trace *trace = os_zalloc(sizeof(struct uagentbuf_trace) +
					       sizeof(struct uagentbuf) + len);
	struct uagentbuf *buf;
	if (trace == NULL)
		return NULL;
	trace->magic = uagentBUF_MAGIC;
	buf = (struct uagentbuf *) (trace + 1);
#else /* uagent_TRACE */
	struct uagentbuf *buf = os_zalloc(sizeof(struct uagentbuf) + len);
	if (buf == NULL)
		return NULL;
#endif /* uagent_TRACE */

	buf->size = len;
	buf->buf = (u8 *) (buf + 1);
	return buf;
}


struct uagentbuf * uagentbuf_alloc_ext_data(u8 *data, size_t len)
{
#ifdef uagent_TRACE
	struct uagentbuf_trace *trace = os_zalloc(sizeof(struct uagentbuf_trace) +
					       sizeof(struct uagentbuf));
	struct uagentbuf *buf;
	if (trace == NULL)
		return NULL;
	trace->magic = uagentBUF_MAGIC;
	buf = (struct uagentbuf *) (trace + 1);
#else /* uagent_TRACE */
	struct uagentbuf *buf = os_zalloc(sizeof(struct uagentbuf));
	if (buf == NULL)
		return NULL;
#endif /* uagent_TRACE */

	buf->size = len;
	buf->used = len;
	buf->buf = data;
	buf->flags |= uagentBUF_FLAG_EXT_DATA;

	return buf;
}


struct uagentbuf * uagentbuf_alloc_copy(const void *data, size_t len)
{
	struct uagentbuf *buf = uagentbuf_alloc(len);
	if (buf)
		uagentbuf_put_data(buf, data, len);
	return buf;
}


struct uagentbuf * uagentbuf_dup(const struct uagentbuf *src)
{
	struct uagentbuf *buf = uagentbuf_alloc(uagentbuf_len(src));
	if (buf)
		uagentbuf_put_data(buf, uagentbuf_head(src), uagentbuf_len(src));
	return buf;
}


/**
 * uagentbuf_free - Free a uagentbuf
 * @buf: uagentbuf buffer
 */
void uagentbuf_free(struct uagentbuf *buf)
{
#ifdef uagent_TRACE
	struct uagentbuf_trace *trace;
	if (buf == NULL)
		return;
	trace = uagentbuf_get_trace(buf);
	if (trace->magic != uagentBUF_MAGIC) {
		uagent_printf(MSG_ERROR, "uagentbuf_free: invalid magic %x",
			   trace->magic);
		uagent_trace_show("uagentbuf_free magic mismatch");
		abort();
	}
	if (buf->flags & uagentBUF_FLAG_EXT_DATA)
		os_free(buf->buf);
	os_free(trace);
#else /* uagent_TRACE */
	if (buf == NULL)
		return;
	if (buf->flags & uagentBUF_FLAG_EXT_DATA)
		os_free(buf->buf);
	os_free(buf);
#endif /* uagent_TRACE */
}


void * uagentbuf_put(struct uagentbuf *buf, size_t len)
{
	void *tmp = uagentbuf_mhead_u8(buf) + uagentbuf_len(buf);
	buf->used += len;
	if (buf->used > buf->size) {
		uagentbuf_overflow(buf, len);
	}
	return tmp;
}


/**
 * uagentbuf_concat - Concatenate two buffers into a newly allocated one
 * @a: First buffer
 * @b: Second buffer
 * Returns: uagentbuf with concatenated a + b data or %NULL on failure
 *
 * Both buffers a and b will be freed regardless of the return value. Input
 * buffers can be %NULL which is interpreted as an empty buffer.
 */
struct uagentbuf * uagentbuf_concat(struct uagentbuf *a, struct uagentbuf *b)
{
	struct uagentbuf *n = NULL;
	size_t len = 0;

	if (b == NULL)
		return a;

	if (a)
		len += uagentbuf_len(a);
	if (b)
		len += uagentbuf_len(b);

	n = uagentbuf_alloc(len);
	if (n) {
		if (a)
			uagentbuf_put_buf(n, a);
		if (b)
			uagentbuf_put_buf(n, b);
	}

	uagentbuf_free(a);
	uagentbuf_free(b);

	return n;
}


/**
 * uagentbuf_zeropad - Pad buffer with 0x00 octets (prefix) to specified length
 * @buf: Buffer to be padded
 * @len: Length for the padded buffer
 * Returns: uagentbuf padded to len octets or %NULL on failure
 *
 * If buf is longer than len octets or of same size, it will be returned as-is.
 * Otherwise a new buffer is allocated and prefixed with 0x00 octets followed
 * by the source data. The source buffer will be freed on error, i.e., caller
 * will only be responsible on freeing the returned buffer. If buf is %NULL,
 * %NULL will be returned.
 */
struct uagentbuf * uagentbuf_zeropad(struct uagentbuf *buf, size_t len)
{
	struct uagentbuf *ret;
	size_t blen;

	if (buf == NULL)
		return NULL;

	blen = uagentbuf_len(buf);
	if (blen >= len)
		return buf;

	ret = uagentbuf_alloc(len);
	if (ret) {
		os_memset(uagentbuf_put(ret, len - blen), 0, len - blen);
		uagentbuf_put_buf(ret, buf);
	}
	uagentbuf_free(buf);

	return ret;
}


void uagentbuf_printf(struct uagentbuf *buf, char *fmt, ...)
{
	va_list ap;
	void *tmp = uagentbuf_mhead_u8(buf) + uagentbuf_len(buf);
	int res;

	va_start(ap, fmt);
	res = vsnprintf(tmp, buf->size - buf->used, fmt, ap);
	va_end(ap);
	if (res < 0 || (size_t) res >= buf->size - buf->used)
		uagentbuf_overflow(buf, res);
	buf->used += res;
}
