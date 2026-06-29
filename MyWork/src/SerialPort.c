#include "SerialPort.h"

// #include "stdio.h"
#include "string.h"

#include "Critical.h"

#include "RingBuffer.h"

#define SERIAL_PORT_SEND_YIELD() \
	do                           \
	{                            \
	} while (0)

static volatile uint8_t sp_tx_dma_buf[TOTAL_SERIALPORT][UART_TX_DMA_BUF_SIZE];
static volatile uint8_t sp_tx_buf[TOTAL_SERIALPORT][UART_TX_BUF_SIZE];

// double buffer for rx
static volatile uint8_t sp_rx_dma_buf[TOTAL_SERIALPORT][UART_RX_DMA_BUF_SIZE];

static volatile uint8_t sp_rx_buf[TOTAL_SERIALPORT][UART_RX_BUF_SIZE];

#define TX_TIMEOUT_Factor 5
#define TX_TIMEOUT_BPS(v) (UART_TX_DMA_BUF_SIZE * TX_TIMEOUT_Factor * 10 * 1000 / v)

// for 9600, timeout is 666ms
#define TX_TIMEOUT_BPS9600 TX_TIMEOUT_BPS(9600)

// for 115200, timeout is 55ms
#define TX_TIMEOUT_BPS115200 TX_TIMEOUT_BPS(115200)

#define HAL_ERROR_FLAG (1 << 31)

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern void Error_Handler();

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
SerialPort_t serialPort[TOTAL_SERIALPORT];

UART_HandleTypeDef *HUARTS[TOTAL_SERIALPORT] =
	{
		&huart1,

};
DMA_HandleTypeDef *HDMA_UART_RXS[TOTAL_SERIALPORT] =
	{
		&hdma_usart1_rx,
};

void SerialPortInit()
{
	for (int i = 0; i < TOTAL_SERIALPORT; i++)
	{
		serialPort[i].huart = HUARTS[i];
		serialPort[i].hdma_rx = HDMA_UART_RXS[i];
		serialPort[i].tx_dma_buf = &sp_tx_dma_buf[i][0];
		serialPort[i].rx_dma_buf = &sp_rx_dma_buf[i][0];
		RB_Init(&serialPort[i].rx_ringbuf, &sp_rx_buf[i][0], UART_RX_BUF_SIZE);
		RB_Init(&serialPort[i].tx_ringbuf, &sp_tx_buf[i][0], UART_TX_BUF_SIZE);
		serialPort[i].error_code = HAL_UART_ERROR_NONE;
		serialPort[i].flag = 0;
	}
}

void SerialPortSetBps(SerialPort_t *sp, uint32_t v)
{
	HAL_UART_Abort(sp->huart);
	RB_Clear(&sp->rx_ringbuf);
	RB_Clear(&sp->tx_ringbuf);
	HAL_UART_DeInit(sp->huart);
	// My_USART1_UART_Init(v);
	extern void MX_USART1_UART_Init();
	MX_USART1_UART_Init();
	sp->error_code = HAL_UART_ERROR_NONE;
	sp->tx_timeout = TX_TIMEOUT_BPS(v);
	sp->flag = 0;
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
		if (huart == serialPort[i].huart)
		{
			return &serialPort[i];
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

void SpWrite(SerialPort_t *sp, const uint8_t *buf, int len)
{
#if 0
	uint32_t time_start;
	HAL_StatusTypeDef st;
	SpErrorCheck(sp);
	while (len > 0)
	{
		time_start = HAL_GetTick();
		while (sp->huart->gState != HAL_UART_STATE_READY)
		{
			SERIAL_PORT_SEND_YIELD();
			if ((HAL_GetTick() - time_start) > sp->tx_timeout)
			{
				sp->error_code = HAL_ERROR_FLAG;
				return;
			}
		}
		int xlen = (len < UART_TX_DMA_BUF_SIZE) ? len : UART_TX_DMA_BUF_SIZE;
		memcpy(sp->tx_dma_buf, buf, xlen);
		ENTER_CRITICAL();
		st = HAL_UART_Transmit_DMA(sp->huart, sp->tx_dma_buf, xlen);
		EXIT_CRITICAL();
		if (st != HAL_OK)
		{
			sp->error_code = HAL_ERROR_FLAG | sp->huart->ErrorCode;
			return;
		}
		len -= xlen;
		buf += xlen;
	};
	return;
#else
	while (RB_Free(&sp->tx_ringbuf) < len)
	{
		SERIAL_PORT_SEND_YIELD();
		SpErrorCheck(sp);
	}
	ATOMIC_CODE()
	{
		int xlen = 0;
		HAL_StatusTypeDef st = HAL_OK;
		if (sp->huart->gState == HAL_UART_STATE_READY)
		{
			xlen = (len < UART_TX_DMA_BUF_SIZE) ? len : UART_TX_DMA_BUF_SIZE;
			memcpy(sp->tx_dma_buf, buf, xlen);
			buf += xlen;
			len -= xlen;
			st = HAL_UART_Transmit_DMA(sp->huart, sp->tx_dma_buf, xlen);
			if (st != HAL_OK)
			{
				sp->error_code = HAL_ERROR_FLAG | sp->huart->ErrorCode;
			}
		}
		if (st == HAL_OK)
		{
			if (len > 0)
			{
				RB_Push(&sp->tx_ringbuf, buf, len);
			}
		}
	}
#endif
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
	int len = RB_Count(&sp->tx_ringbuf);
	if (len > 0)
	{
		if (sp->huart->gState == HAL_UART_STATE_READY)
		{
			int xlen = (len < UART_TX_DMA_BUF_SIZE) ? len : UART_TX_DMA_BUF_SIZE;
			RB_Pop(&sp->tx_ringbuf, sp->tx_dma_buf, xlen);
			if (HAL_UART_Transmit_DMA(sp->huart, sp->tx_dma_buf, xlen) != HAL_OK)
			{
				sp->error_code = HAL_ERROR_FLAG | sp->huart->ErrorCode;
			}
		}
	}
}
