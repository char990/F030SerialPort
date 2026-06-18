#include "rms.h"

#define AD_2_VOLT 1 // adc value to volt
#define FILTER 64   // filter coefficient=64/256=0.25
#define ADC_LENGTH 128

// Note: adc is 12-bit, the value is 0 ~ 2047
// 2047*2047*128 = 536,346,752 = 0x1FF80080, can be fit in 32-bit without overflow
// RMS = sqrt((v[0]^ + v[1]^ + ... + V[127]^) / 128)

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

void zero_phase_low_pass_filter(uint16_t *buffer, uint8_t alpha256)
{
    uint32_t temp;
    uint32_t alpha_256 = alpha256;

    // 1. forward filter (0 -> length-1)
    for (int i = 1; i < ADC_LENGTH; i++)
    {
        temp = alpha_256 * buffer[i] + (256 - alpha_256) * buffer[i - 1];
        buffer[i] = (uint16_t)(temp >> 8);
    }

    // 2. backward filter (length-1 -> 0)
    for (int i = ADC_LENGTH - 2; i >= 0; i--)
    {
        temp = alpha_256 * buffer[i] + (256 - alpha_256) * buffer[i + 1];
        buffer[i] = (uint16_t)(temp >> 8);
    }
}

uint32_t rms_volt(uint16_t *buffer)
{
    zero_phase_low_pass_filter(buffer, FILTER);

    uint32_t sum_squares = 0;
    int32_t sample_offset = 0;
    int32_t sum_raw = 0;

    // DC voltage offset
    for (int i = 0; i < ADC_LENGTH; i++)
    {
        sum_raw += buffer[i];
    }
    int32_t dc_offset = sum_raw / ADC_LENGTH;

    // sum of square of absolute value
    for (int i = 0; i < ADC_LENGTH; i++)
    {
        sample_offset = (int32_t)buffer[i] - dc_offset;
        sum_squares += sample_offset * sample_offset; // uint32_t: will not overflow
    }

    // sqrt of mean
    uint32_t rms_ad_val = fast_sqrt_32(sum_squares / ADC_LENGTH);

    return rms_ad_val * AD_2_VOLT;
}
