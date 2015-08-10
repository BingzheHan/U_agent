/*
 * Dynamic data buffer
 * Copyright (c) 2015-2020, Brad Han <bingzhehan@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#include "common.h"
#ifndef uagentBUF_H
#define uagentBUF_H

/* uagentbuf::buf is a pointer to external data */
#define uagentBUF_FLAG_EXT_DATA BIT(0)

/*
 * Internal data structure for uagentbuf. Please do not touch this directly from
 * elsewhere. This is only defined in header file to allow inline functions
 * from this file to access data.
 */
struct uagentbuf {
	size_t size; /* total size of the allocated buffer */
	size_t used; /* length of data in the buffer */
	u8 *buf; /* pointer to the head of the buffer */
	unsigned int flags;
	/* optionally followed by the allocated buffer */
};


int uagentbuf_resize(struct uagentbuf **buf, size_t add_len);
struct uagentbuf * uagentbuf_alloc(size_t len);
struct uagentbuf * uagentbuf_alloc_ext_data(u8 *data, size_t len);
struct uagentbuf * uagentbuf_alloc_copy(const void *data, size_t len);
struct uagentbuf * uagentbuf_dup(const struct uagentbuf *src);
void uagentbuf_free(struct uagentbuf *buf);
void * uagentbuf_put(struct uagentbuf *buf, size_t len);
struct uagentbuf * uagentbuf_concat(struct uagentbuf *a, struct uagentbuf *b);
struct uagentbuf * uagentbuf_zeropad(struct uagentbuf *buf, size_t len);
void uagentbuf_printf(struct uagentbuf *buf, char *fmt, ...) PRINTF_FORMAT(2, 3);


/**
 * uagentbuf_size - Get the currently allocated size of a uagentbuf buffer
 * @buf: uagentbuf buffer
 * Returns: Currently allocated size of the buffer
 */
static inline size_t uagentbuf_size(const struct uagentbuf *buf)
{
	return buf->size;
}

/**
 * uagentbuf_len - Get the current length of a uagentbuf buffer data
 * @buf: uagentbuf buffer
 * Returns: Currently used length of the buffer
 */
static inline size_t uagentbuf_len(const struct uagentbuf *buf)
{
	return buf->used;
}

/**
 * uagentbuf_tailroom - Get size of available tail room in the end of the buffer
 * @buf: uagentbuf buffer
 * Returns: Tail room (in bytes) of available space in the end of the buffer
 */
static inline size_t uagentbuf_tailroom(const struct uagentbuf *buf)
{
	return buf->size - buf->used;
}

/**
 * uagentbuf_head - Get pointer to the head of the buffer data
 * @buf: uagentbuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline const void * uagentbuf_head(const struct uagentbuf *buf)
{
	return buf->buf;
}

static inline const u8 * uagentbuf_head_u8(const struct uagentbuf *buf)
{
	return uagentbuf_head(buf);
}

/**
 * uagentbuf_mhead - Get modifiable pointer to the head of the buffer data
 * @buf: uagentbuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline void * uagentbuf_mhead(struct uagentbuf *buf)
{
	return buf->buf;
}

static inline u8 * uagentbuf_mhead_u8(struct uagentbuf *buf)
{
	return uagentbuf_mhead(buf);
}

static inline void uagentbuf_put_u8(struct uagentbuf *buf, u8 data)
{
	u8 *pos = uagentbuf_put(buf, 1);
	*pos = data;
}

static inline void uagentbuf_put_le16(struct uagentbuf *buf, u16 data)
{
	u8 *pos = uagentbuf_put(buf, 2);
	uagent_PUT_LE16(pos, data);
}

static inline void uagentbuf_put_le32(struct uagentbuf *buf, u32 data)
{
	u8 *pos = uagentbuf_put(buf, 4);
	uagent_PUT_LE32(pos, data);
}

static inline void uagentbuf_put_be16(struct uagentbuf *buf, u16 data)
{
	u8 *pos = uagentbuf_put(buf, 2);
	uagent_PUT_BE16(pos, data);
}

static inline void uagentbuf_put_be24(struct uagentbuf *buf, u32 data)
{
	u8 *pos = uagentbuf_put(buf, 3);
	uagent_PUT_BE24(pos, data);
}

static inline void uagentbuf_put_be32(struct uagentbuf *buf, u32 data)
{
	u8 *pos = uagentbuf_put(buf, 4);
	uagent_PUT_BE32(pos, data);
}

static inline void uagentbuf_put_data(struct uagentbuf *buf, const void *data,
				   size_t len)
{
	if (data)
		os_memcpy(uagentbuf_put(buf, len), data, len);
}

static inline void uagentbuf_put_buf(struct uagentbuf *dst,
				  const struct uagentbuf *src)
{
	uagentbuf_put_data(dst, uagentbuf_head(src), uagentbuf_len(src));
}

static inline void uagentbuf_set(struct uagentbuf *buf, const void *data, size_t len)
{
	buf->buf = (u8 *) data;
	buf->flags = uagentBUF_FLAG_EXT_DATA;
	buf->size = buf->used = len;
}

static inline void uagentbuf_put_str(struct uagentbuf *dst, const char *str)
{
	uagentbuf_put_data(dst, str, os_strlen(str));
}

#endif /* uagentBUF_H */
