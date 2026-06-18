/*
 * MyPrintf.c
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */
#include "MyPrintf.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"

#include "SerialPort.h"

#define MYPRINTF_BUF_SIZE 256
int MyPrintf(const char *fmt, ...)
{
	char buf[MYPRINTF_BUF_SIZE];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, MYPRINTF_BUF_SIZE, fmt, args);
	va_end(args);
	MyPuts(buf);
    return len;
}

int MyPutchar(const char c)
{
	return SpPutchar(&serialPort[0], c);
}

int MyPuts(const char *s)
{
	return SpPuts(&serialPort[0], s);
}

static void Print_abcd(const char *abcd)
{
	char _abcd[5];
	for (int i = 0; i < 4; i++)
	{
		if (*abcd != 0)
		{
			_abcd[i] = *abcd++;
		}
		else
		{
			_abcd[i] = ' ';
		}
	}
	_abcd[4] = '\0';
	MyPrintf("\r\n%s|", _abcd);
}

static void PrintBar(int n)
{
	n = n * 16 + 5;
	if (n > MYPRINTF_BUF_SIZE - 3)
	{
		n = MYPRINTF_BUF_SIZE - 3;
	}
	char bar[MYPRINTF_BUF_SIZE];
	bar[0] = '\r';
	bar[1] = '\n';
	memset(bar + 2, '-', n);
	bar[n + 2] = '\0';
	MyPrintf(bar);
}

void PrintUint8(const uint8_t *_buf, int len, const char *abcd)
{
	Print_abcd(abcd);
	for (int i = 0; i < 16; i++)
	{
		MyPrintf("%6X|", i);
	}
	PrintBar(7);
	int lines = (len + 15) / 16;
	for (int s = 0; s < lines; s++)
	{
		MyPrintf("\r\n%03X_|", s);
		for (int k = 0; k < 16; k++)
		{
			MyPrintf("%02X:%3u|", *_buf, *_buf);
			_buf++;
		}
	}
	MyPuts("\r\n");
}

void PrintUint16(const uint16_t *_buf, int len, const char *abcd)
{
	Print_abcd(abcd);
	for (int i = 0; i < 16; i++)
	{
		MyPrintf("%10X|", i * 2);
	}
	PrintBar(11);
	int lines = (len + 15) / 16;
	for (int s = 0; s < lines; s++)
	{
		MyPrintf("\r\n%03X_|", s * 2);
		for (int k = 0; k < 16; k++)
		{
			MyPrintf("%04X:%5u|", *_buf, *_buf);
			_buf++;
		}
	}
	MyPuts("\r\n");
}

void PrintUint32(const uint32_t *_buf, int len, const char *abcd)
{
	Print_abcd(abcd);
	for (int i = 0; i < 16; i++)
	{
		MyPrintf("%8X|", i * 4);
	}
	PrintBar(9);
	int lines = (len + 15) / 16;
	for (int s = 0; s < lines; s++)
	{
		MyPrintf("\r\n%03X_|", s * 4);
		for (int k = 0; k < 16; k++)
		{
			MyPrintf("%08X|", *_buf);
			_buf++;
		}
	}
	MyPuts("\r\n");
}
