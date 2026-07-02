#pragma once
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr7; // 7-bit address. the default is 0x29
} VL6180X_t;

bool VL6180X_Init(VL6180X_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7);
bool VL6180X_ReadRangeSingle(VL6180X_t *dev, uint8_t *range_mm, uint32_t timeout_ms);
uint8_t VL6180X_ReadRangeStatus(VL6180X_t *dev);
