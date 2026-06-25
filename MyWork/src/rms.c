#include "rms.h"
#include "adc.h"

// RMS = sqrt((v[0]^ + v[1]^ + ... + V[127]^) / 128)

uint32_t rms_flag = 0;

// Note: adc is 12-bit, the value is 0 ~ 4095.
// When DC offset is at V33/2, AC input is -2047 ~ +2047, only 11-bits
// 2047*2047*128 = 536,346,752 = 0x1FF80080
// 11b + 11b + 7b = 29 bits
// can be fit in 32-bit without overflow

uint32_t fast_sqrt_32(uint32_t num)
{
    uint32_t res = 0;
    uint32_t bit = 1 << 30;

    while (bit != 0)
    {
        if (num >= res + bit)
        {
            num -= res + bit;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

void forward_low_pass_filter(uint16_t *buffer, uint8_t alpha256)
{
    uint32_t temp;
    uint32_t alpha_256 = alpha256;

    for (int i = 1; i < ADC_LENGTH; i++)
    {
        temp = alpha_256 * buffer[i] + (256 - alpha_256) * buffer[i - 1];
        buffer[i] = (uint16_t)(temp / 256);
    }
}

void backward_low_pass_filter(uint16_t *buffer, uint8_t alpha256)
{
    uint32_t temp;
    uint32_t alpha_256 = alpha256;

    for (int i = ADC_LENGTH - 2; i >= 0; i--)
    {
        temp = alpha_256 * buffer[i] + (256 - alpha_256) * buffer[i + 1];
        buffer[i] = (uint16_t)(temp / 256);
    }
}

void zero_phase_low_pass_filter(uint16_t *buffer, uint8_t alpha256)
{
    forward_low_pass_filter(buffer, alpha256);
    backward_low_pass_filter(buffer, alpha256);
}

uint32_t get_rms(uint16_t *buffer)
{
    uint32_t sum_squares = 0;
    int32_t sample_offset = 0;
    int32_t sum_raw = 0;

    // DC voltage offset
    for (int i = 0; i < RMS_LENGTH; i++)
    {
        sum_raw += buffer[i];
    }
    int32_t dc_offset = sum_raw / RMS_LENGTH;

    // sum of square of absolute value
    for (int i = 0; i < RMS_LENGTH; i++)
    {
        sample_offset = (int32_t)buffer[i] - dc_offset;
        sum_squares += sample_offset * sample_offset; // uint32_t: will not overflow
    }

    // sqrt of mean
    uint32_t rms_ad_val = fast_sqrt_32(sum_squares / RMS_LENGTH);

    return rms_ad_val;
}

/**
 * @brief  Conversion complete callback in non blocking mode
 * @param  hadc ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *p_hadc)
{
    HAL_ADC_Stop_DMA(p_hadc);
    rms_flag = 1;
}

void start_rms_adc(uint16_t *buffer)
{
    rms_flag = 0;
    HAL_ADCEx_Calibration_Start(&hadc);
    HAL_ADC_Start_DMA(&hadc, (uint32_t *)buffer, ADC_LENGTH);
}
