
#include "internal.h"

/*
 * ring buffer code
 *
 * there's a "start" and an "end".  "start" is only moved by
 * the reader, "end" is only moved by the writer.
 * an exception is when writing to a ring buffer overflows it.
 * in this case, "start" is moved to keep up with "end".
 *
 * "start" always points to the first byte to be read out of 
 * the buffer; "end" always points to the first byte that can
 * be written.  when the ring buffer is empty, they point at
 * the same byte.  when the ring buffer is full, the end is
 * one byte behind the full.  both start and end wrap around
 * at the end of the buffer.
 */

ringbuf *
ringbuf_create(size)
	int size;
{
	ringbuf *ret;

	ret = (ringbuf *)malloc(sizeof(ringbuf));
	if (ret == NULL)
	{
	  fail:
		printf("ringbuf_create: can't allocate memory\n");
		return NULL;
	}

	ret->buf = (char*)malloc(size);
	if (ret->buf == NULL)
	{
		free(ret);
		goto fail;
	}

	ret->size = size;
	ret->start = 0;
	ret->end = 0;

	return ret;
}

void
ringbuf_destroy(buf)
	ringbuf *buf;
{
	free(buf->buf);
	free(buf);
}

/*
 * if there isn't enough room on the ring buffer,
 * just overwrite the oldest data on the ring.
 * it looses data ... but who cares?
 */

void
ringbuf_add(buf, dat, size)
	ringbuf *buf;
	char *dat;
	int size;
{
	int newend, endsize;

	if (size > buf->size)
	{
		printf("ringbuf_add: cannot add more bytes "
		       "than there are in the ringbuf.\n");
		return;
	}

	/*
	 * determine if we're going to have
	 * to wrap around the end.  if so, do
	 * the copy in two parts.
	 */

	if ((buf->end + size) >= buf->size)
	{
		endsize = buf->size - buf->end;
		newend = size - endsize;

		bcopy(dat, buf->buf + buf->end, endsize);
		bcopy(dat + endsize, buf->buf, newend);
	}
	else
	{
		newend = (buf->end + size) % buf->size;

		bcopy(dat, buf->buf + buf->end, size);
	}

	if ((buf->start > buf->end) && 
	    (buf->start <= newend))
	{
		buf->start = newend + 1;
		if (buf->start >= buf->size)
			buf->start -= buf->size;
	}

	buf->end = newend;
}

int
ringbuf_remove(buf, dat, size)
	ringbuf *buf;
	char *dat;
	int size;
{
	int retsize, endsize, startsize;

	if (ringbuf_empty(buf))
		return 0;

	/*
	 * determine if data is contiguous
	 */
	if (buf->end > buf->start)
	{
		if (size > (buf->end - buf->start))
			retsize = buf->end - buf->start;
		else
			retsize = size;

		bcopy(buf->buf + buf->start, dat, retsize);
		buf->start += retsize;
	}
	else
	{
		endsize = buf->size - buf->start;
		startsize = buf->end;

		if (size > (startsize + endsize))
			retsize = (startsize + endsize);
		else
			retsize = size;

		/*
		 * data is in two parts.
		 * determine if (a) we're getting part of the 
		 * end of the buffer, (b) we're getting exactly
		 * all of the end of the buffer, or (c) we're getting
		 * more than the end of the buffer.
		 */

		if (retsize < endsize)
		{
			bcopy(buf->buf + buf->start, dat, retsize);
			buf->start += retsize;
		}

		if (retsize == endsize)
		{
			bcopy(buf->buf + buf->start, dat, retsize);
			buf->start = 0;
		}

		if (retsize > endsize)
		{
			startsize = retsize - endsize;
			bcopy(buf->buf + buf->start, dat, endsize);
			bcopy(buf->buf, dat + endsize, startsize);
			buf->start = startsize;
		}
	}

	return retsize;
}

int
ringbuf_empty(buf)
	ringbuf *buf;
{
	if (buf->start == buf->end)
		return 1;

	return 0;
}

int
ringbuf_cursize(buf)
	ringbuf *buf;
{
	if (buf->end > buf->start)
	{
		return(buf->end - buf->start);
	}
	else
	{
		return((buf->size - buf->start) + buf->end);
	}
}
