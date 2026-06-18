#include "RingBuffer.h"
#include "string.h"

void RB_Init(RingBuffer_t *prb, uint8_t *buf, int size)
{
	prb->size = size;
	prb->pPop = prb->pPush = prb->buf = buf;
}

uint8_t *RB_End(RingBuffer_t *prb)
{
	return prb->buf + prb->size;
}

int RB_Is_Full(RingBuffer_t *prb)
{
	return ((prb->pPop - 1 == prb->pPush) || (prb->pPop + prb->size - 1 == prb->pPush));
}

int RB_Push_c(RingBuffer_t *prb, uint8_t c)
{
	if (RB_Is_Full(prb))
	{
		return 0;
	}
	*prb->pPush++ = c;
	if (prb->pPush == RB_End(prb))
	{
		prb->pPush = prb->buf;
	}
	return 1;
}

int RB_Push(RingBuffer_t *prb, const uint8_t *inbuf, int len)
{
	int v = RB_Space_Free(prb);
	if (len <= 0 || v == 0)
	{
		return 0;
	}
	if (len > v)
	{
		len = v;
	}
	int size1 = RB_End(prb) - prb->pPush;
	if (size1 > len)
	{
		memcpy(prb->pPush, inbuf, len);
		prb->pPush += len;
	}
	else
	{
		memcpy(prb->pPush, inbuf, size1);
		len -= size1;
		if(len > 0)
		{
			memcpy(prb->buf, inbuf + size1, len);
		}
		prb->pPush = prb->buf + len;
	}
	return len;
}

uint8_t RB_Pop_c(RingBuffer_t *prb)
{
	while (prb->pPop == prb->pPush)
		;
	uint8_t c = *prb->pPop++;
	if (prb->pPop == RB_End(prb))
	{
		prb->pPop = prb->buf;
	}
	return c;
}

int RB_Pop(RingBuffer_t *prb, uint8_t *outbuf, int len)
{
	int used = RB_Space_Used(prb);
	if (used == 0 || len == 0)
	{
		return 0;
	}
	if (len > used)
	{
		len = used;
	}
	int size1 = RB_End(prb) - prb->pPop;
	if (len >= size1)
	{
		memcpy(outbuf, prb->pPop, size1);
		len -= size1;
		if (len > 0)
		{
			memcpy(outbuf + size1, prb->buf, len);
		}
		prb->pPop = prb->buf + len;
	}
	else
	{
		memcpy(outbuf, prb->pPop, len);
		prb->pPop += len;
	}
	return len;
}

int RB_Space_Used(const RingBuffer_t *prb)
{
	return prb->pPush + ((prb->pPop <= prb->pPush) ? 0 : prb->size) - prb->pPop;
}

int RB_Space_Free(const RingBuffer_t *prb)
{
	return prb->size - RB_Space_Used(prb) - 1;
}

void RB_Drop(RingBuffer_t *prb, int len)
{
	int used = RB_Space_Used(prb);
	if (used <= len)
	{
		RB_Clear(prb);
		return;
	}
	while (1)
		;
}

void RB_Clear(RingBuffer_t *prb)
{
	prb->pPop = prb->pPush;
}
