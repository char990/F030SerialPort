#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f0xx_hal.h"

#define TOTAL_SERIALPORT 1

#define SP_FLAGS_TX_BUSY (1 << 0)
#define SP_FLAGS_RX_IDLE (1 << 1)
#define SP_FLAGS_HAL_ERR (1 << 2)

	typedef struct sp_buf_t
	{
		int size;
		uint8_t *dma_buf;
		volatile uint8_t *head;
		volatile uint8_t *tail;
		volatile int cnt;
		volatile int temp;
	} sp_buf_t;

	typedef struct SerialPort_t
	{
		UART_HandleTypeDef *huart;
		sp_buf_t tx_buf;
		sp_buf_t rx_buf;
		volatile uint32_t error_code;
		volatile uint32_t error_code_bak;
		volatile uint32_t flags;
		void (*MX_Init)(uint32_t bps);
	} SerialPort_t;

	extern SerialPort_t *const serialPort[TOTAL_SERIALPORT];

	void SerialPortClear(SerialPort_t *sp);
	void SpErrorCheck(SerialPort_t *sp);

	SerialPort_t *GetSerialPort(UART_HandleTypeDef *huart);

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
