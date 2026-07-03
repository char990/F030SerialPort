#include "SerialPort.h"

#include "string.h"

#include "Critical.h"

#define SERIAL_PORT_SEND_YIELD() \
	do                           \
	{                            \
	} while (0)

extern void Error_Handler();

#define SP_DEF(n)                                                                                                                                                                             \
	extern UART_HandleTypeDef CATAB(huart, n);                                                                                                                                                \
	static uint8_t CATAB(sp, CATAB(n, _tx_buf))[CATAB(SP, CATAB(n, _TX_BUF_SIZE))] __attribute__((aligned(4)));                                                                               \
	static uint8_t CATAB(sp, CATAB(n, _rx_buf))[CATAB(SP, CATAB(n, _RX_BUF_SIZE))] __attribute__((aligned(4)));                                                                               \
	static SerialPort_t CATAB(sp, n) = {                                                                                                                                                      \
		.MX_Init = CATAB(SP, CATAB(n, _MX_Init)),                                                                                                                                             \
		.huart = &CATAB(huart, n),                                                                                                                                                            \
		.tx_buf = {.size = CATAB(SP, CATAB(n, _TX_BUF_SIZE)), .dma_buf = CATAB(sp, CATAB(n, _tx_buf)), .head = CATAB(sp, CATAB(n, _tx_buf)), .tail = CATAB(sp, CATAB(n, _tx_buf)), .cnt = 0}, \
		.rx_buf = {.size = CATAB(SP, CATAB(n, _RX_BUF_SIZE)), .dma_buf = CATAB(sp, CATAB(n, _rx_buf)), .head = CATAB(sp, CATAB(n, _rx_buf)), .tail = CATAB(sp, CATAB(n, _rx_buf)), .cnt = 0}, \
	};

#define SP1_TX_BUF_SIZE 1024
#define SP1_RX_BUF_SIZE 1024
void SP1_MX_Init(uint32_t bps)
{
	(void)bps;
	// My_USART1_UART_Init(bps);
	extern void MX_USART1_UART_Init();
	MX_USART1_UART_Init();
}

SP_DEF(1)

SerialPort_t *const serialPort[TOTAL_SERIALPORT] = {&sp1};

void sp_buf_clr(sp_buf_t *buf)
{
	buf->cnt = buf->temp = 0;
	buf->head = buf->tail = buf->dma_buf;
}

void SerialPortClear(SerialPort_t *sp)
{
	ATOMIC_CODE()
	{
		sp->error_code = HAL_UART_ERROR_NONE;
		sp->flags = 0;
		sp_buf_clr(&sp->tx_buf);
		sp_buf_clr(&sp->rx_buf);
	}
}

void SerialPortStartRx(SerialPort_t *sp)
{
	HAL_StatusTypeDef st = HAL_UART_Receive_DMA(sp->huart, sp->rx_buf.dma_buf, sp->rx_buf.size);
	if (st == HAL_OK)
	{
		__HAL_DMA_DISABLE_IT(sp->huart->hdmarx, DMA_IT_HT | DMA_IT_TC);
		sp->error_code = 0;
	}
	else
	{
		sp->error_code = (SP_FLAGS_HAL_ERR | sp->huart->ErrorCode);
	}
}

SerialPort_t *GetSerialPort(UART_HandleTypeDef *huart)
{
	for (int i = 0; i < TOTAL_SERIALPORT; i++)
	{
		if (huart == serialPort[i]->huart)
		{
			return serialPort[i];
		}
	}
	return NULL;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	SerialPort_t *sp = GetSerialPort(huart);
	sp->error_code = huart->ErrorCode;
}

void SpErrorCheck(SerialPort_t *sp)
{
	if (sp->error_code != HAL_UART_ERROR_NONE)
	{
		sp->error_code_bak = sp->error_code;
		int rx = sp->error_code & HAL_UART_ERROR_DMA;
		if (sp->error_code & SP_FLAGS_HAL_ERR)
		{
			HAL_UART_Abort(sp->huart);
			HAL_UART_DeInit(sp->huart);
			SerialPortClear(sp);
			sp->MX_Init(sp->huart->Init.BaudRate);
			rx = HAL_UART_ERROR_DMA;
		}
		if (rx)
		{
			SerialPortStartRx(sp);
		}
	}
}

HAL_StatusTypeDef SP_Start_TX_DMA(SerialPort_t *sp)
{
	sp_buf_t *buf = &sp->tx_buf;
	if ((sp->flags & SP_FLAGS_TX_BUSY) || (buf->cnt <= 0))
		return HAL_OK;
	sp->flags |= SP_FLAGS_TX_BUSY;
	int available_len;
	if (buf->head >= buf->tail)
	{
		available_len = buf->head - buf->tail;
	}
	else
	{
		available_len = (buf->dma_buf + buf->size) - buf->tail;
	}
	buf->temp = (available_len > buf->cnt) ? buf->cnt : available_len;
	HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(sp->huart, buf->tail, buf->temp);
	if (st != HAL_OK)
	{
		sp->error_code = SP_FLAGS_HAL_ERR | sp->huart->ErrorCode;
	}
	return st;
}

HAL_StatusTypeDef SpWrite(SerialPort_t *sp, const uint8_t *data, int len)
{
	HAL_StatusTypeDef st;
	sp_buf_t *buf = &sp->tx_buf;
	for (int i = 0; i < len; i++)
	{
		while (buf->cnt >= buf->size)
		{
			if (!(sp->flags & SP_FLAGS_TX_BUSY))
			{
				ATOMIC_CODE() { st = SP_Start_TX_DMA(sp); }
				if (st != HAL_OK)
				{
					return st;
				}
			}
		}
		*buf->head = data[i];
		buf->head++;
		if (buf->head >= (buf->dma_buf + buf->size))
		{
			buf->head = buf->dma_buf;
		}
		ATOMIC_CODE() { buf->cnt++; }
	}
	ATOMIC_CODE() { st = SP_Start_TX_DMA(sp); }
	return st;
}

void SpRxUpdate(SerialPort_t *sp)
{
	sp_buf_t *rxb = &sp->rx_buf;
	int dma_pos = rxb->size - (uint16_t)__HAL_DMA_GET_COUNTER(sp->huart->hdmarx);
	if (dma_pos >= rxb->size || dma_pos < 0)
	{
		dma_pos = 0;
	}
	rxb->head = rxb->dma_buf + dma_pos;
	int bytes = rxb->head - rxb->tail;
	rxb->cnt = (bytes >= 0) ? bytes : (rxb->size + bytes);
}

int SpRead(SerialPort_t *sp, uint8_t *buf, int len)
{
	// SpErrorCheck(sp);
	// Do NOT run ErrorCheck
	// SpRead should be called after AnyChars
	sp_buf_t *rxb = &sp->rx_buf;
	int cnt;
	uint8_t *tail, *end;
	if (rxb->cnt <= 0 || len <= 0)
	{
		return 0;
	}
	end = rxb->dma_buf + rxb->size;
	tail = rxb->tail;
	cnt = rxb->cnt;
	if (cnt > len)
	{
		cnt = len;
	}
	rxb->cnt -= cnt;
	rxb->tail += cnt;
	if (rxb->tail > end)
	{
		rxb->tail -= rxb->size;
	}
	int size1 = end - tail;
	if (cnt >= size1)
	{
		memcpy(buf, tail, size1);
		int xlen = cnt - size1;
		if (xlen > 0)
		{
			memcpy(buf + size1, rxb->dma_buf, xlen);
		}
	}
	else
	{
		memcpy(buf, tail, cnt);
	}
	return cnt;
}

int SpAnyChars(SerialPort_t *sp)
{
	SpErrorCheck(sp);
	SpRxUpdate(sp);
	return sp->rx_buf.cnt;
}

char SpGetchar(SerialPort_t *sp)
{
	while (SpAnyChars(sp) == 0)
	{
		SERIAL_PORT_SEND_YIELD();
	}
	sp_buf_t *rxb = &sp->rx_buf;
	char c = *rxb->tail;
	if (++rxb->tail >= (rxb->dma_buf + rxb->size))
	{
		rxb->tail = rxb->dma_buf;
	}
	rxb->cnt--;
	return c;
}

int SpPutchar(SerialPort_t *sp, const char c)
{
	SpWrite(sp, (uint8_t *)&c, 1);
	return c;
}

int SpPuts(SerialPort_t *sp, const char *s)
{
	int len = strlen(s);
	if (len <= 0)
	{
		return -1;
	}
	SpWrite(sp, (uint8_t *)s, len);
	return len;
}

int SpFlush(SerialPort_t *sp)
{
	while (sp->huart->gState != HAL_UART_STATE_READY)
	{
		SERIAL_PORT_SEND_YIELD();
		SpErrorCheck(sp);
	}
	return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	SerialPort_t *sp = GetSerialPort(huart);
	sp_buf_t *buf = &sp->tx_buf;
	int sent = buf->temp;
	buf->tail += sent;
	if (buf->tail >= (buf->dma_buf + buf->size))
	{
		buf->tail -= buf->size;
	}
	buf->cnt -= sent;
	sp->flags &= ~SP_FLAGS_TX_BUSY;
	if (SP_Start_TX_DMA(sp) != HAL_OK)
	{
		sp->error_code = SP_FLAGS_HAL_ERR | sp->huart->ErrorCode;
	}
}
