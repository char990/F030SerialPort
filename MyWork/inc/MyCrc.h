#ifndef INC_MY_CRC_H_
#define INC_MY_CRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "crc.h"

#define CRC32_ETHERNET
    // #define CRC32_BZIP2
    // #define CRC32_MPEG2

    void Crc32Init(void);
    uint32_t CRC32Get(uint32_t v);

    /*
    1. Call Crc32Init() to init CRC
    2.
     (+) Use HAL_CRC_Accumulate() function to compute the CRC value of the
         input data buffer starting with the previously computed CRC as
         initialization value
     (+) Use HAL_CRC_Calculate() function to compute the CRC value of the
         input data buffer starting with the defined initialization value
         (default or non-default) to initiate CRC calculation
    3. Call CRC32Get() to get final value
    */

#ifdef __cplusplus
}
#endif

#endif /* INC_MY_CRC_H_ */
