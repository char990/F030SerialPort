#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f0xx_hal.h"

#include "RingBuffer.h"

	typedef struct SerialPort_t
	{
		UART_HandleTypeDef *huart;
		DMA_HandleTypeDef *hdma_rx;
		uint8_t *tx_dma_buf;
		uint8_t *rx_dma_buf;
		RingBuffer_t rx_ringbuf;
		volatile uint32_t error_code;
		uint32_t tx_timeout;
		uint32_t flag;
	} SerialPort_t;

#define TOTAL_SERIALPORT 1

	extern SerialPort_t serialPort[TOTAL_SERIALPORT];

	void SerialPortInit(); // Init all serial ports
	void SpErrorCheck(SerialPort_t *sp);

	SerialPort_t *GetSerialPort(UART_HandleTypeDef *huart);
	int IsSerialPortRx(SerialPort_t *sp);

	void SerialPortSetBps(SerialPort_t *sp, uint32_t v);

	void SerialPortStartRx(SerialPort_t *sp);

	void SpWrite(SerialPort_t *sp, const uint8_t *buf, int len);
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
