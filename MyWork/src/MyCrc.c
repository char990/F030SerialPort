#include "MyCrc.h"

#ifdef CRC32_ETHERNET
void Crc32Init()
{
    HAL_CRCEx_Input_Data_Reverse(&hcrc, CRC_INPUTDATA_INVERSION_BYTE);
    HAL_CRCEx_Output_Data_Reverse(&hcrc, CRC_OUTPUTDATA_INVERSION_ENABLE);
}
uint32_t CRC32Get(uint32_t v)
{
    return v ^ 0xFFFFFFFF;
}
#elifdef CRC32_BZIP2
void Crc32Init()
{
    HAL_CRCEx_Input_Data_Reverse(&hcrc, CRC_INPUTDATA_INVERSION_NONE);
    HAL_CRCEx_Output_Data_Reverse(&hcrc, CRC_INPUTDATA_INVERSION_NONE);
}
uint32_t CRC32Get(uint32_t v)
{
    return v ^ 0xFFFFFFFF;
}
#elifdef CRC32_MPEG2
void Crc32Init()
{
    HAL_CRCEx_Input_Data_Reverse(&hcrc, CRC_INPUTDATA_INVERSION_NONE);
    HAL_CRCEx_Output_Data_Reverse(&hcrc, CRC_INPUTDATA_INVERSION_NONE);
}
uint32_t CRC32Get(uint32_t v)
{
    return v;
}
#else
#error "No CRC mode defined"
#endif
