#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct RingBuffer_t
{
	int size;
	uint8_t *buf;
	uint8_t *pPush;
	uint8_t *pPop;
} RingBuffer_t;

/* init ring buffer */
void RB_Init(RingBuffer_t *prb, uint8_t *buf, int size);

/* Clear ring buffer */
void RB_Clear(RingBuffer_t *prb);

/* push a uint8_t into rb if there is a space */
int RB_Push_c(RingBuffer_t *prb, uint8_t c);

/* push as many as possible uint8_t into rb
 * return how many were pushed in*/
int RB_Push(RingBuffer_t *prb, const uint8_t *inbuf, int len);

/* pop up a uint8_t from rb. If rb is empty, wait */
uint8_t RB_Pop_c(RingBuffer_t *prb);

/* pop up as many as possible uint8_t from rb
 * return how many were popped up*/
int RB_Pop(RingBuffer_t *prb, uint8_t *outbuf, int len);

/* free space in rb */
int RB_Space_Free(const RingBuffer_t *prb);

/* used space in rb */
int RB_Space_Used(const RingBuffer_t *prb);

#ifdef __cplusplus
}
#endif

#endif
