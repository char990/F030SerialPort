#ifndef INC_MY_CRC_H_
#define INC_MY_CRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "crc.h"

#define CRC32_ETHERNET 0
#define CRC32_BZIP2 1
#define CRC32_MPEG2 2
#define CRC32_STANDARD CRC32_ETHERNET
#define CRC32_BRIGHTWAY CRC32_BZIP2

#define CRC32_USING CRC32_ETHERNET

    void Crc32Init(void);
    uint32_t CRC32Get(uint32_t v);

    /*
    Polynomial in STM32F0xx is 0x4C11DB7. It's hardware coded and cannot be changed.

    CRC32_STANDARD/ETHERNET:
        Initial Value: 0xFFFFFFFF
        Polynomial: 0x4C11DB7
        Input reflected/inversion: Y/Byte
        Result reflected/inversion: Y/Byte
        Final XOR Value: 0xFFFFFFFF

    CRC32_BRIGHTWAY/BZIP2:
        Initial Value: 0xFFFFFFFF
        Polynomial: 0x4C11DB7
        Input reflected/inversion: N
        Result reflected/inversion: N
        Final XOR Value: 0xFFFFFFFF

    CRC32_MPEG2:
        Initial Value: 0xFFFFFFFF
        Polynomial: 0x4C11DB7
        Input reflected/inversion: N
        Result reflected/inversion: N
        Final XOR Value: N/0

    Default HAL CRC settings:(same as MPEG2)
        Default Polynomial State: Enable (0x4C11DB7)
        Default Init ValueState: Enable (0xFFFFFFFF)
        Input Data Inversion Mode: None
        Output Data Inversion Mode: Disable
        Input Data Format: Bytes       

    How to:
    1. Call Crc32Init() to init CRC
    2. HAL API(from "stm32f0xx_hal_crc.h/.c")
     (+) Use HAL_CRC_Accumulate() function to compute the CRC value of the
         input data buffer starting with the previously computed CRC as
         initialization value
     (+) Use HAL_CRC_Calculate() function to compute the CRC value of the
         input data buffer starting with the defined initialization value
         (default or non-default) to initiate CRC calculation
     (*) HAL_CRC_Calculate() equals to
         {
            __HAL_CRC_DR_RESET(&hcrc) // hcrc->Instance->INIT is written in hcrc->Instance->DR
            HAL_CRC_Accumulate(...)
         }
    3. Call CRC32Get() to get final value
    */

#ifdef __cplusplus
}
#endif

#endif /* INC_MY_CRC_H_ */
