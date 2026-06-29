#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f0xx_hal.h"

#define HAL_ERROR_FLAG (1 << 31)
#define SP_STATUS_BUSY (1<<0) 
#define UART_TX_BUF_SIZE (256)
#define UART_RX_BUF_SIZE (256)

typedef struct sp_buf_t
	{
		int size;
		uint8_t *dma_buf;
		volatile uint8_t *head;
		volatile uint8_t *tail;
		volatile int cnt;
		volatile int temp;
		volatile uint32_t status;
	} sp_buf_t;

typedef struct SerialPort_t
	{
		UART_HandleTypeDef *huart;
		DMA_HandleTypeDef *hdma_rx;
		sp_buf_t tx_buf;
		sp_buf_t rx_buf;
		volatile uint32_t error_code;
		volatile uint32_t flag;
	} SerialPort_t;


#define TOTAL_SERIALPORT 1

	extern SerialPort_t serialPort[TOTAL_SERIALPORT];

	void SerialPortInit(); // Init all serial ports
	void SpErrorCheck(SerialPort_t *sp);

	SerialPort_t *GetSerialPort(UART_HandleTypeDef *huart);
	int IsSerialPortRx(SerialPort_t *sp);

	void SerialPortSetBps(SerialPort_t *sp, uint32_t v);

	void SerialPortStartRx(SerialPort_t *sp);

	HAL_StatusTypeDef SpWrite(SerialPort_t *sp, const uint8_t *buf, int len);
	int SpRead(SerialPort_t *sp, uint8_t *buf, int len);

	int SpAnyChars(SerialPort_t *sp);
	char SpGetchar(SerialPort_t *sp);

	int SpPutchar(SerialPort_t *sp, const char c);
	int SpPuts(SerialPort_t *sp, const char *);
	int SpFlush(SerialPort_t *sp);

#ifdef __cplusplus
}
#endif

#endif // __SERIALPORT_H__
