#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
  WAVE_SINE = 0,
  WAVE_TRI,
  WAVE_SAW,
  WAVE_SQUARE,
  WAVE_COUNT
} Waveform_t;

void Audio_Init(TIM_HandleTypeDef *htim_pwm, uint32_t pwm_channel,
                uint16_t pwm_arr, uint32_t Fs_hz);

void Audio_SetWaveform(Waveform_t w);
void Audio_SetFreqHz(float f_hz);

void Audio_Tick(void);
