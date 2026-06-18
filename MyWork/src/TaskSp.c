/*
 * TaskSp.c
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */
#include <GetVer.h>
#include "TaskSp.h"
#include "stdlib.h"
#include "string.h"

#include "main.h"

#include "SerialPort.h"
#include "Pwm.h"
#include "MyTmr.h"
#include "Critical.h"
#include "Consts.h"
#include "MyPrintf.h"
#include "Tasks.h"
#include "OPT4001.h"
#include "AscHex.h"
#include "MyCrc.h"
#include "MyI2C.h"
#include "ScanAdc.h"
#include "Config.h"

/********************************************/
// #define TASK_CLI
#define TASK_SLV
/********************************************/

myPt_t ptSp;
#define this_pt (&ptSp)

msTmr_t tmrSp;
#define this_tmr (&tmrSp)

#define this_sp (&serialPort[0])

#ifdef TASK_CLI

#define SP1_RX_BUF_SIZE 32
static char sp1RxBuf[SP1_RX_BUF_SIZE]; /* The command string.*/
static uint8_t sp1RxLen;

void PrintVersion()
{
	char buf[64];
	GetVer(buf);
	MyPuts(buf);
	MyPutchar('\n');
}

typedef enum CMD_RET
{
	SUCCESS_WITHOUT_MSG = 0,
	UNKNOWN_CMD = 1,
	INVALID_PARAM = 2,
	CMD_RET_SIZE
} CMD_RET_t;

const char *CMD_RET_STR[CMD_RET_SIZE] =
	{
		"Success",
		"Unknown command",
		"Invalid parameters",
};

typedef struct command_t
{
	const char *cmd_str;
	CMD_RET_t (*cmd_func)(int argc, char **argv);
	const char *cmd_usage;
} command_t;

#define ARGUMENTS_MAX 16 // including command
char *argv[ARGUMENTS_MAX];
int MakeArg(char *cmdline)
{
	char *token;
	int argc = 0;
	for (int i = 0; i < ARGUMENTS_MAX; i++)
	{
		argv[i] = NULL;
	}
	token = strtok(cmdline, " ");
	while (token != NULL)
	{
		argv[argc++] = token;
		token = strtok(NULL, " ");
		if (argc == ARGUMENTS_MAX)
		{
			break;
		}
	}
	return argc;
}

CMD_RET_t TestPwm(int argc, char **argv)
{
	if (argc != 3)
	{
		return INVALID_PARAM;
	}
	else
	{
		int s = strtol(argv[1], NULL, 0);
		int duty = strtol(argv[2], NULL, 0);
		if (s > 1 || duty < 0 || duty > 255)
		{
			return INVALID_PARAM;
		}
		SetDuty(s, duty);
		return SUCCESS_WITHOUT_MSG;
	}
}

CMD_RET_t TestAd(int argc, char **argv)
{
	HAL_StatusTypeDef st;
	st = HAL_ADCEx_Calibration_Start(&hadc);
	if (st != HAL_OK)
	{
		MyPrintf("\nADCEx_Calibration Failed=%u\n", st);
		return SUCCESS_WITHOUT_MSG;
	}
	SetDuty(1, 255);
	SetDuty(0, 255);
	HAL_Delay(10);
	uint16_t adcv[3];
	memset(adcv, 0xFF, sizeof(adcv));
	st = HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcv, 3);
	while (hadc.State & HAL_ADC_STATE_REG_BUSY)
		;
	HAL_Delay(1);
	HAL_ADC_Stop_DMA(&hadc);
	SetDuty(1, 0);
	SetDuty(0, 0);
	if (hadc.State & (HAL_ADC_STATE_ERROR_DMA | HAL_ADC_STATE_ERROR_INTERNAL | HAL_ADC_STATE_ERROR_CONFIG))
	{
		MyPrintf("ADC error: ST = 0x%08X\n", hadc.State);
	}
	if (hadc.State & HAL_ADC_STATE_REG_OVR)
	{
		MyPrintf("ADC overrun occurred: ST = 0x%08X\n", hadc.State);
	}
	if (hadc.State & HAL_ADC_STATE_REG_EOC)
	{
		MyPrintf("ADC completed: ST = 0x%08X\n", hadc.State);
	}
	MyPrintf("ad[0]=%u, [1]=%u, [2]=%u\n", adcv[0], adcv[1], adcv[2]);

	MyPrintf("mv[0]=%u, [1]=%u, [2]=%u\n",
			 Get_mv(adcv[0], adcv[2]),
			 Get_mv(adcv[1], adcv[2]),
			 Get_mv(adcv[2], adcv[2]));

	return SUCCESS_WITHOUT_MSG;
}

CMD_RET_t TestI2C(int argc, char **argv)
{
	I2C_HandleTypeDef *hi2c = &hi2c1;
	if (argc != 1 && argc != 3)
	{
		return INVALID_PARAM;
	}
	uint8_t x[256];
	x[0] = 0;
	if (argc == 1)
	{
		for (int i = 1; i < 128; i++)
		{
			x[i] = (HAL_I2C_Master_Receive(hi2c, i << 1, &x[i], 1, 10) == HAL_OK) ? i : 0;
		}
		PrintUint8(x, 128, "I2C");
	}
	else // (argc == 3)
	{
		long addr = strtol(argv[1], NULL, 0);
		long len = strtol(argv[2], NULL, 0);
		if (addr <= 0 || addr > 127 || len <= 0 || len > 128)
		{
			return INVALID_PARAM;
		}
		if (HAL_I2C_Master_Transmit(hi2c, addr << 1, x, 1, 1000) == HAL_OK &&
			HAL_I2C_Master_Receive(hi2c, addr << 1, x, len, 1000) == HAL_OK)
		{
			PrintUint8(x, len, "I2C");
		}
		else
		{
			MyPrintf("\n%s Failed\n", "I2C");
		}
	}
	return SUCCESS_WITHOUT_MSG;
}

CMD_RET_t MCP9808(int argc, char **argv)
{
	I2C_HandleTypeDef *hi2c = &hi2c1;
	if (argc != 2)
	{
		return INVALID_PARAM;
	}
	long addr = strtol(argv[1], NULL, 0);
	if (addr < 0x18 || addr > 0x1F)
	{
		return INVALID_PARAM;
	}
	uint16_t x[9];
	for (uint8_t reg = 1; reg < 9; reg++)
	{
		if (HAL_I2C_Master_Transmit(hi2c, addr << 1, &reg, 1, 1000) == HAL_OK &&
			HAL_I2C_Master_Receive(hi2c, addr << 1, (uint8_t *)&x[reg], (reg == 8) ? 1 : 2, 1000) == HAL_OK)
		{
			if (reg == 8)
			{
				x[reg] >>= 8;
				MyPrintf("\n[%u]=0x%02X\n", reg, x[reg]);
			}
			else
			{
				x[reg] = (x[reg] >> 8) | (x[reg] << 8);
				MyPrintf("\n[%u]=0x%04X\n", reg, x[reg]);
			}
		}
		else
		{
			MyPrintf("\n%s Failed\n", "I2C");
			break;
		}
	}
	const char *dot_t[16] = {
		"0",
		"0625",
		"125",
		"1875",
		"25",
		"3125",
		"375",
		"4375",
		"5",
		"5625",
		"625",
		"6875",
		"75",
		"5125",
		"875",
		"9375",
	};
	x[5] &= 0x1FFF;
	if (x[5] & 0x1000)
	{ // T < 0'C
	}
	else
	{
		MyPrintf("\nT=%d.%s'C\n", x[5] / 16, dot_t[x[5] & 0x000F]);
	}
	return SUCCESS_WITHOUT_MSG;
}

CMD_RET_t OPT4001(int argc, char **argv)
{
	MyPrintf("\nst_lux = %u\n", st_lux);
	return SUCCESS_WITHOUT_MSG;
}

CMD_RET_t CRC32(int argc, char **argv)
{
	if (argc != 2)
	{
		return INVALID_PARAM;
	}
	uint32_t len = strlen(argv[1]);
	uint32_t crc = CRC_Calculate((uint8_t *)argv[1], len);
	MyPrintf("\ncrc=0x%08X\n", crc);
	return SUCCESS_WITHOUT_MSG;
}

CMD_RET_t Help(int argc, char **argv);
command_t CLI_CMD[] =
	{
		/*********************** info ***********************/
		{"help", Help,
		 "\r\nhelp"
		 "\r\n  This help"},

		/*********************** test ***********************/
		{"testpwm", TestPwm,
		 "\r\ntestpwm HC duty"
		 "\r\n  HC=0|1, duty=0-255"},
		{"testad", TestAd,
		 "\r\ntestad"
		 "\r\n  test all ad channels"},
		{"testi2c", TestI2C,
		 "\r\ntesti2c [addr len]"
		 "\r\n  testi2c : List all slaves on I2C"
		 "\r\n  testi2c 0x45 32: Print regs[0-31](max=256) of slave[0x45] on I2C"},
		{"MCP9808", MCP9808,
		 "\r\nMCP9808 address"
		 "\r\n  List all register in MCP9808 and print temperature"},
		{"OPT4001", OPT4001,
		 "\r\nOPT4001"
		 "\r\n  Print st_lux"},
		{"CRC32", CRC32,
		 "\r\nCRC32 12sdlfkjsdf"
		 "\r\n  Calculate crc32"},
};

CMD_RET_t Help(int argc, char **argv)
{
	PrintVersion();
	CMD_RET_t ret = INVALID_PARAM;
	for (int i = 1; i < sizeof(CLI_CMD) / sizeof(CLI_CMD[0]); i++)
	{
		if (argc == 1 || strcmp(CLI_CMD[i].cmd_str, argv[1]) == 0)
		{
			ret = SUCCESS_WITHOUT_MSG;
			MyPuts(CLI_CMD[i].cmd_usage);
			MyPuts("\r\n");
		}
	}
	return ret;
}

uint8_t TaskSp()
{
	PT_BEGIN(this_pt);
	PrintVersion();
	for (;;)
	{
		PT_WAIT_UNTIL(this_pt, SpAnyChars(this_sp));
		{
			char sp1rx = SpGetchar(this_sp);
#if 0
			SpPutchar(this_sp, sp1rx);
			continue;
#else
			if (sp1rx == KEY_LF || sp1rx == KEY_CR)
			{
				sp1RxBuf[sp1RxLen] = '\0';
				MyPuts("\r\n");
				if (sp1RxLen > 0)
				{
					CMD_RET_t cmd_ret = UNKNOWN_CMD;
					int argc = MakeArg(sp1RxBuf);
					for (int i = 0; i < sizeof(CLI_CMD) / sizeof(CLI_CMD[0]); i++)
					{
						if (strcmp(CLI_CMD[i].cmd_str, argv[0]) == 0)
						{
							cmd_ret = (*CLI_CMD[i].cmd_func)(argc, argv);
							break;
						}
					}
					if (cmd_ret > 0 && cmd_ret < CMD_RET_SIZE)
					{
						MyPuts(CMD_RET_STR[cmd_ret]);
					}
					sp1RxLen = 0;
					sp1RxBuf[sp1RxLen] = '\0';
				}
				MyPuts("\r\n=>");
			}
			else
			{
				/* The if() clause performs the processing after a newline character
				 is received.  This else clause performs the processing if any other
				 character is received. */

				if (sp1rx == '\b')
				{
					/* Backspace was pressed.  Erase the last character in the input
					 buffer - if there are any. */
					if (sp1RxLen > 0)
					{
						sp1RxLen--;
						sp1RxBuf[sp1RxLen] = '\0';
						MyPuts("\b \b");
					}
				}
				else
				{
					/* A character was entered.  It was not a new line, backspace
					 or carriage return, so it is accepted as part of the input and
					 placed into the input buffer.  When a n is entered the complete
					 string will be passed to the command interpreter. */
					if (sp1RxLen < SP1_RX_BUF_SIZE - 1)
					{
						sp1RxBuf[sp1RxLen++] = sp1rx;
						sp1RxBuf[sp1RxLen] = '\0';
						SpPutchar(this_sp, sp1rx);
					}
					else
					{
						SpPutchar(this_sp, 0x07); // beep
					}
				}
			}
#endif
		}
	}
	PT_END(this_pt);
}

static void this_Init()
{
	sp1RxLen = 0;
	sp1RxBuf[0] = '\0';
}

#endif

#ifdef TASK_SLV

#define CMD_INDEX_SLVID 0
#define CMD_INDEX_CODE 1

// All commands
#define READ_HOLDING_REGISTERS 0x03
#define READ_INPUT_REGISTERS 0x04
#define WRITE_SINGLE_REGISTER 0x06

int CMD_ReadHoldingRegisters(int len);
int CMD_ReadInputRegisters(int len);
int CMD_WriteSingleRegister(int len);

typedef struct cmd_t
{
	/*
	 *	@brief	Command function pointer
	 *	@param	len:	data length
	 *	@retval	length of reply data
	 */
	int (*func)(int len);
	/*
	 *	@brief	Command ID / MI code
	 */
	uint8_t cmd_id;
	/*
	 *	@brief	Command length after decode. Stripped STX, CRC and ETX.
	 *	@note 0 means variable length
	 */
	uint16_t cmd_len;
} cmd_t;

/// @brief  cmd_len includes address+code+data+crc=all bytes
const cmd_t SLV_CMD[] = {
	{CMD_ReadHoldingRegisters, READ_HOLDING_REGISTERS, 8},
	{CMD_ReadInputRegisters, READ_INPUT_REGISTERS, 8},
	{CMD_WriteSingleRegister, WRITE_SINGLE_REGISTER, 8},
};

#define INVALID_FUNCTION 1
#define INVALID_ADDRESS 2
#define INVALID_VALUE 3

#define SP1_TX_BUF_SIZE 256
static uint8_t sp1TxBuf[SP1_TX_BUF_SIZE];

#define SP1_RX_BUF_SIZE 256
static uint8_t sp1RxBuf[SP1_RX_BUF_SIZE];

static int ModbusException(uint8_t exception)
{
	uint8_t *p = sp1TxBuf;
	*p++ = SLV_ID;
	*p++ = sp1RxBuf[CMD_INDEX_CODE] | 0x80;
	*p++ = exception;
	return p - sp1TxBuf;
}

#define SLV_TIMEOUT 10000
static void this_Init()
{
	SetMsTmr(this_tmr, SLV_TIMEOUT);
	SerialPortStartRx(this_sp);
}
#define RTU_FRAME_SLOT 38 // Modbus char = 8-N-2(11-bit) => 11*3.5=38.5
#if 0
char *tx_array = "abcdefghijklmnopqrstuvwxyz";
PT_BEGIN(this_pt);
for (;;)
{
	SetMsTmr(this_tmr, 2000);
	PT_WAIT_UNTIL(this_pt, IsMsTmrExpired(this_tmr));
	SpErrorCheck(this_sp);
	wdt |= WDT_TASK_SP;
	SpWrite(this_sp, tx_array, 26);
}
PT_END(this_pt);
#endif
uint8_t TaskSp()
{
	SpErrorCheck(this_sp);
	wdt |= WDT_TASK_SP;
	// uint32_t cnt = GetBitCnt();
	if (this_sp->flag)
	{
		this_sp->flag = 0;
		int rxlen = 0;
		while (SpAnyChars(this_sp))
		{
			sp1RxBuf[rxlen++] = SpGetchar(this_sp);
		}

		if (rxlen >= 4)
		{
			// if (rxlen <= SP1_RX_BUF_SIZE)
			{
				if ((sp1RxBuf[CMD_INDEX_SLVID] == SLV_ID || sp1RxBuf[CMD_INDEX_SLVID] == BROADCAST_ID) &&
					MODBUS_CRC16(sp1RxBuf, rxlen - 2) == Cnvt_GetU16lh(sp1RxBuf + rxlen - 2))
				{
					int txlen = 0;
					for (int i = 0; i < sizeof(SLV_CMD) / sizeof(SLV_CMD[0]); i++)
					{
						if (sp1RxBuf[CMD_INDEX_CODE] == SLV_CMD[i].cmd_id &&
							(SLV_CMD[i].cmd_len == 0 || SLV_CMD[i].cmd_len == rxlen))
						{
							txlen = (*SLV_CMD[i].func)(rxlen);
						}
						else if (i == (sizeof(SLV_CMD) / sizeof(SLV_CMD[0]) - 1))
						{
							txlen = ModbusException(INVALID_FUNCTION);
						}
						if (txlen > 0)
						{
							if (sp1RxBuf[CMD_INDEX_SLVID] == SLV_ID)
							{
								Cnvt_PutU16lh(MODBUS_CRC16(sp1TxBuf, txlen), sp1TxBuf + txlen);
								SpWrite(this_sp, sp1TxBuf, txlen + 2);
							}
							break;
						}
					} // end of for (int i = 0; i < sizeof(SLV_CMD) / sizeof(SLV_CMD[0]); i++)
				}
			} // end of if (rxlen <= SP1_RX_BUF_SIZE)
			// SerialPortStartRx(this_sp);
		}
	}
	return 1;
}

int CMD_ReadHoldingRegisters(int len)
{
#define CMD_03_ADDR 2
#define CMD_03_LEN 4
	uint16_t addr = Cnvt_GetU16hl(sp1RxBuf + CMD_03_ADDR);
	uint16_t len03 = Cnvt_GetU16hl(sp1RxBuf + CMD_03_LEN);
	int txlen;
	if (addr > 1)
	{
		txlen = ModbusException(INVALID_ADDRESS);
	}
	else if (len03 == 0 || (len03 + addr) > 2)
	{
		txlen = ModbusException(INVALID_VALUE);
	}
	else
	{
		uint16_t holding_reg[2];
		holding_reg[0] = st_conspicuity; // flashing status
		holding_reg[1] = st_pwm;		 // brightness

		uint8_t *p = sp1TxBuf;
		*p++ = SLV_ID;
		*p++ = sp1RxBuf[CMD_INDEX_CODE];
		*p++ = len03 * 2;
		for (int i = 0; i < len03; i++)
		{
			p = Cnvt_PutU16hl(holding_reg[i + addr], p);
		}
		txlen = p - sp1TxBuf;
	}
	return txlen;
}

int CMD_ReadInputRegisters(int len)
{
#define CMD_04_ADDR 2
#define CMD_04_LEN 4
#define CMD_04_REGS_SIZE 11
	uint16_t addr = Cnvt_GetU16hl(sp1RxBuf + CMD_04_ADDR);
	uint16_t len04 = Cnvt_GetU16hl(sp1RxBuf + CMD_04_LEN);
	int txlen;
	if (addr >= CMD_04_REGS_SIZE)
	{
		txlen = ModbusException(INVALID_ADDRESS);
	}
	else if (len04 == 0 || (len04 + addr) > CMD_04_REGS_SIZE)
	{
		txlen = ModbusException(INVALID_VALUE);
	}
	else
	{
		extern const uint16_t PCB_NUMBER;
		extern const uint16_t FW_BUILD_HEX[2];
		extern const uint16_t FW_VERSION;
		uint16_t in_reg[CMD_04_REGS_SIZE];
		uint16_t *p_reg = in_reg;
		*p_reg++ = PCB_NUMBER;						 // PCB number
		*p_reg++ = FW_BUILD_HEX[0];					 // firmware build date
		*p_reg++ = FW_BUILD_HEX[1];					 // firmware build time
		*p_reg++ = FW_VERSION;						 // firmware version
		*p_reg++ = st_conspicuity;					 // flashing status
		*p_reg++ = st_flasherCurrent[FLASHER_UPPER]; // mA of flasher[0]
		*p_reg++ = 0;								 // N/A
		*p_reg++ = st_flasherCurrent[FLASHER_LOWER]; // mA of flasher[2]
		*p_reg++ = 0;								 // N/A
		*p_reg++ = st_lux[LUX_FRONT];				 // lightsenor
		*p_reg++ = st_lux[LUX_BACK];

		uint8_t *p = sp1TxBuf;
		*p++ = SLV_ID;
		*p++ = sp1RxBuf[CMD_INDEX_CODE];
		*p++ = len04 * 2;
		for (int i = 0; i < len04; i++)
		{
			p = Cnvt_PutU16hl(in_reg[i + addr], p);
		}
		txlen = p - sp1TxBuf;
	}
	return txlen;
}

int Response06()
{
	memcpy(sp1TxBuf, sp1RxBuf, 6);
	return 6;
}

int CMD_WriteSingleRegister(int len)
{
#define CMD_06_ADDR 2
#define CMD_06_VALUE 4
	uint16_t addr = Cnvt_GetU16hl(sp1RxBuf + CMD_06_ADDR);
	uint16_t value = Cnvt_GetU16hl(sp1RxBuf + CMD_06_VALUE);
	int txlen;
	switch (addr)
	{
	case 0:
		if (value == FLASHER_ALL_OFF || value == FLASHER_LEFT_RIGHT || value == FLASHER_ALL_FLASH || value == FLASHER_ALL_ON || value == FLASHER_1ON_2FL)
		{
			conspicuity_changed = 1;
			st_conspicuity = value;
			txlen = Response06();
		}
		else
		{
			txlen = ModbusException(INVALID_VALUE);
		}
		break;
	case 1:
		if (value >= 0 && value <= 255)
		{
			pwm_changed = 1;
			st_pwm = value;
			txlen = Response06();
		}
		else
		{
			txlen = ModbusException(INVALID_VALUE);
		}
		break;
	default:
		txlen = ModbusException(INVALID_ADDRESS);
		break;
	}
	return txlen;
}
#endif

void TaskSpInit()
{
	SerialPortInit();
	PT_Reset(this_pt);
	this_Init();
}
