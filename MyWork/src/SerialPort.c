#include "SerialPort.h"

// #include "stdio.h"
#include "string.h"

#include "Critical.h"

#define SERIAL_PORT_SEND_YIELD() \
	do                           \
	{                            \
	} while (0)

extern void Error_Handler();


#define SP_DEF(n) \
	extern UART_HandleTypeDef CATAB(huart, n); \
    static uint8_t CATAB(sp, CATAB(n, _tx_buf))[CATAB(SP, CATAB(n, _TX_BUF_SIZE))] __attribute__((aligned(4))); \
    static uint8_t CATAB(sp, CATAB(n, _rx_buf))[CATAB(SP, CATAB(n, _RX_BUF_SIZE))] __attribute__((aligned(4))); \
    static SerialPort_t CATAB(sp, n) = { \
        .huart = &CATAB(huart, n), \
        .tx_buf = { .size = CATAB(SP, CATAB(n, _TX_BUF_SIZE)), .dma_buf = CATAB(sp, CATAB(n, _tx_buf)), \
                    .head = CATAB(sp, CATAB(n, _tx_buf)), .tail = CATAB(sp, CATAB(n, _tx_buf)), .cnt = 0 }, \
        .rx_buf = { .size = CATAB(SP, CATAB(n, _RX_BUF_SIZE)), .dma_buf = CATAB(sp, CATAB(n, _rx_buf)), \
                    .head = CATAB(sp, CATAB(n, _rx_buf)), .tail = CATAB(sp, CATAB(n, _rx_buf)), .cnt = 0 } \
    };

#define SP1_TX_BUF_SIZE 256
#define SP1_RX_BUF_SIZE 256
SP_DEF(1)

SerialPort_t *const serialPort[TOTAL_SERIALPORT] = {&sp1};

/* USART1 init function */
#if 0
void My_USART1_UART_Init(uint32_t bps)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = bps;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_RS485Ex_Init(&huart1, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */
}
#endif

void SerialPortClear(SerialPort_t *sp)
{
	sp->error_code = HAL_UART_ERROR_NONE;
	sp->flag = 0;
	sp_buf_t *buf[2] = {&sp->tx_buf, &sp->tx_buf};
	ATOMIC_CODE()
	{
		for (int i = 0; i < 2; i++)
		{
			buf[i]->cnt = buf[i]->status = buf[i]->temp = 0;
			buf[i]->head = buf[i]->tail = buf[i]->dma_buf;
		}
	}
}

void SerialPortInit()
{
	for (int i = 0; i < TOTAL_SERIALPORT; i++)
	{
		SerialPortClear(serialPort[i]);
	}
}

void SerialPortSetBps(SerialPort_t *sp, uint32_t v)
{
	HAL_UART_Abort(sp->huart);
	HAL_UART_DeInit(sp->huart);
	SerialPortClear(sp);
	// My_USART1_UART_Init(v);
	extern void MX_USART1_UART_Init();
	MX_USART1_UART_Init();
}

void SerialPortStartRx(SerialPort_t *sp)
{
	HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(sp->huart, sp->rx_dma_buf, UART_RX_DMA_BUF_SIZE);
	sp->error_code = (st == HAL_OK) ? 0 : (HAL_ERROR_FLAG | sp->huart->ErrorCode);
}

int IsSerialPortRx(SerialPort_t *sp)
{
	if (sp->huart == NULL)
	{
		return 0;
	}
	return (sp->huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE);
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
		if ((sp->error_code & HAL_ERROR_FLAG) ||
			((sp->error_code & HAL_ERROR_FLAG) == 0 &&
			 (sp->error_code & (HAL_UART_ERROR_DMA | HAL_UART_ERROR_RTO))))
		{
			SerialPortSetBps(sp, sp->huart->Init.BaudRate);
		}
		SerialPortStartRx(sp);
	}
}

HAL_StatusTypeDef SP_Start_TX_DMA(SerialPort_t *sp)
{
	sp_buf_t *buf = &sp->tx_buf;
	if ((buf->status & SP_STATUS_BUSY) || (buf->cnt <= 0))
		return HAL_OK;
	buf->status |= SP_STATUS_BUSY;
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
		sp->error_code = HAL_ERROR_FLAG | sp->huart->ErrorCode;
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
			if (!(buf->status & SP_STATUS_BUSY))
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

int SpRead(SerialPort_t *sp, uint8_t *buf, int len)
{
	// SpErrorCheck(sp);
	// Do NOT run ErrorCheck
	// SpRead should be called after AnyChars
	return RB_Pop(&sp->rx_ringbuf, buf, len);
}

int SpAnyChars(SerialPort_t *sp)
{
	SpErrorCheck(sp);
	return RB_Count(&sp->rx_ringbuf);
}

char SpGetchar(SerialPort_t *sp)
{
	while (RB_Count(&sp->rx_ringbuf) == 0)
	{
		SERIAL_PORT_SEND_YIELD();
		SpErrorCheck(sp);
	}
	return RB_Pop_c(&sp->rx_ringbuf);
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

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	SerialPort_t *sp = GetSerialPort(huart);
	if (huart->RxEventType == HAL_UART_RXEVENT_HT) // Half full
	{
		RB_Push(&sp->rx_ringbuf, sp->rx_dma_buf, UART_RX_DMA_BUF_SIZE_HALF);
	}
	else if (huart->RxEventType == HAL_UART_RXEVENT_TC) // Complete
	{
		RB_Push(&sp->rx_ringbuf, sp->rx_dma_buf + UART_RX_DMA_BUF_SIZE_HALF, UART_RX_DMA_BUF_SIZE_HALF);
		SerialPortStartRx(sp);
	}
	else if (huart->RxEventType == HAL_UART_RXEVENT_IDLE) // Idle
	{
		if (Size > 0 && Size < UART_RX_DMA_BUF_SIZE_HALF)
		{
			RB_Push(&sp->rx_ringbuf, sp->rx_dma_buf, Size);
		}
		else if (Size > UART_RX_DMA_BUF_SIZE_HALF && Size < UART_RX_DMA_BUF_SIZE)
		{
			RB_Push(&sp->rx_ringbuf, sp->rx_dma_buf + UART_RX_DMA_BUF_SIZE_HALF, Size - UART_RX_DMA_BUF_SIZE_HALF);
		}
		sp->flag = 1;
		SerialPortStartRx(sp);
	}
	return;
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
	buf->status &= ~SP_STATUS_BUSY;
	if (SP_Start_TX_DMA(sp) != HAL_OK)
	{
		sp->error_code = HAL_ERROR_FLAG | sp->huart->ErrorCode;
	}
}
