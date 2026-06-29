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
	return RB_Free(prb) == 0;
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
	int v = RB_Free(prb);
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
		int xlen = len - size1;
		if (xlen > 0)
		{
			memcpy(prb->buf, inbuf + size1, xlen);
		}
		prb->pPush = prb->buf + xlen;
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
	int used = RB_Count(prb);
	if (used == 0 || len <= 0)
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
		int xlen = len - size1;
		if (xlen > 0)
		{
			memcpy(outbuf + size1, prb->buf, xlen);
		}
		prb->pPop = prb->buf + xlen;
	}
	else
	{
		memcpy(outbuf, prb->pPop, len);
		prb->pPop += len;
	}
	return len;
}

int RB_Count(const RingBuffer_t *prb)
{
	return (prb->pPop <= prb->pPush) ? (prb->pPush - prb->pPop) : (prb->pPush + prb->size - prb->pPop);
}

int RB_Free(const RingBuffer_t *prb)
{
	return prb->size - RB_Count(prb) - 1;
}

void RB_Drop(RingBuffer_t *prb, int len)
{
	if (len <= 0)
	{
		return;
	}
	int cnt = RB_Count(prb);
	if (cnt <= len)
	{
		RB_Clear(prb);
	}
	else
	{
		prb->pPop += len;
		if (prb->pPop >= RB_End(prb))
		{
			prb->pPop -= prb->size;
		}
	}
}

void RB_Clear(RingBuffer_t *prb)
{
	prb->pPop = prb->pPush;
}
