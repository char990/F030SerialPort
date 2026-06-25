#ifndef RMS_H
#define RMS_H

#include <stdint.h>

#define RMS_LENGTH 128
#define FLT_MORE 16
#define ADC_LENGTH (RMS_LENGTH + FLT_MORE * 2)

// filter coefficient: Smooth 1~255 Sharp
//#define FILTER 80   // 80/256 = 0.3125, good to filter high frequency noises
#define FILTER 64   // 64/256 = 0.25, better to filter high frequency noises

void start_rms_adc(uint16_t *buffer);

uint32_t get_rms(uint16_t *buffer);

void forward_low_pass_filter(uint16_t *buffer, uint8_t alpha256);
void backward_low_pass_filter(uint16_t *buffer, uint8_t alpha256);

void zero_phase_low_pass_filter(uint16_t *buffer, uint8_t alpha256);

/*
//#define USE_FORWARD

#ifdef USE_FORWARD // forward

#define RMS_FLT_OFFSET (FLT_MORE*2)
#define run_filter forward_low_pass_filter

#else // forward + backward

#define RMS_FLT_OFFSET (FLT_MORE)
#define run_filter zero_phase_low_pass_filter

#endif
*/
extern uint32_t rms_flag;

#endif // RMS_H
