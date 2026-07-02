#include "audio.h"
#include <math.h>

#define TABLE_BITS 8
#define TABLE_SIZE (1u << TABLE_BITS)

static TIM_HandleTypeDef *pwm_tim;
static uint32_t pwm_ch;
static uint16_t pwm_arr;
static uint32_t Fs;

static uint16_t table[WAVE_COUNT][TABLE_SIZE];

static volatile uint32_t phase_step = 0;
static uint32_t phase_acc = 0;
static volatile Waveform_t wave = WAVE_SINE;

static void build_tables(void)
{
    for (uint32_t i = 0; i < TABLE_SIZE; i++)
    {
        // 0..255 ramp
        uint32_t saw = i;

        // 0..255..0
        uint32_t tri = (i < 128) ? (i * 2) : ((255 - i) * 2);

        // 0 or 255
        uint32_t sq  = (i < 128) ? 255 : 0;

        // sine: 0..1..0 mapped to 0..ARR
        float x = (2.0f * 3.14159265f * (float)i) / (float)TABLE_SIZE;
        float s = (sinf(x) + 1.0f) * 0.5f;

        table[WAVE_SAW][i]    = (uint16_t)((saw * (uint32_t)pwm_arr) / 255u);
        table[WAVE_TRI][i]    = (uint16_t)((tri * (uint32_t)pwm_arr) / 255u);
        table[WAVE_SQUARE][i] = (uint16_t)((sq  * (uint32_t)pwm_arr) / 255u);
        table[WAVE_SINE][i]   = (uint16_t)(s * (float)pwm_arr);
    }
}

void Audio_Init(TIM_HandleTypeDef *htim_pwm, uint32_t pwm_channel,
                uint16_t arr, uint32_t Fs_hz)
{
    pwm_tim = htim_pwm;
    pwm_ch  = pwm_channel;
    pwm_arr = arr;
    Fs      = Fs_hz;

    build_tables();
    Audio_SetWaveform(WAVE_SINE);
    Audio_SetFreqHz(440.0f);
}

void Audio_SetWaveform(Waveform_t w)
{
    if (w < WAVE_COUNT) wave = w;
}

void Audio_SetFreqHz(float f_hz)
{
    if (f_hz < 1.0f) f_hz = 1.0f;
    if (f_hz > 5000.0f) f_hz = 5000.0f;

    // phase_step = f * 2^32 / Fs
    double step = (double)f_hz * 4294967296.0 / (double)Fs;
    if (step < 0) step = 0;
    if (step > 4294967295.0) step = 4294967295.0;
    phase_step = (uint32_t)step;
}

void Audio_Tick(void)
{
    phase_acc += phase_step;
    uint32_t idx = phase_acc >> (32 - TABLE_BITS); // 0..255
    uint16_t ccr = table[wave][idx];

    __HAL_TIM_SET_COMPARE(pwm_tim, pwm_ch, ccr);
}
