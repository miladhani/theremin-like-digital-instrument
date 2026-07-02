#include "vl6180x.h"

#define VL6180X_REG_IDENTIFICATION_MODEL_ID          0x000
#define VL6180X_REG_SYSTEM_HISTORY_CTRL              0x012
#define VL6180X_REG_SYSTEM_INTERRUPT_CLEAR           0x015
#define VL6180X_REG_SYSTEM_FRESH_OUT_OF_RESET        0x016
#define VL6180X_REG_SYSRANGE_START                   0x018
#define VL6180X_REG_RESULT_RANGE_STATUS              0x04D
#define VL6180X_REG_RESULT_INTERRUPT_STATUS_GPIO     0x04F
#define VL6180X_REG_RESULT_RANGE_VAL                 0x062

static HAL_StatusTypeDef wr8(VL6180X_t *d, uint16_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(d->hi2c, (uint16_t)(d->addr7 << 1), reg,
                            I2C_MEMADD_SIZE_16BIT, &val, 1, 100);
}

static HAL_StatusTypeDef rd8(VL6180X_t *d, uint16_t reg, uint8_t *val)
{
    return HAL_I2C_Mem_Read(d->hi2c, (uint16_t)(d->addr7 << 1), reg,
                           I2C_MEMADD_SIZE_16BIT, val, 1, 100);
}

static HAL_StatusTypeDef rd16(VL6180X_t *d, uint16_t reg, uint16_t *val)
{
    uint8_t b[2];
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(d->hi2c, (uint16_t)(d->addr7 << 1), reg,
                                           I2C_MEMADD_SIZE_16BIT, b, 2, 100);
    if (st != HAL_OK) return st;
    *val = (uint16_t)((b[0] << 8) | b[1]); // big-endian
    return HAL_OK;
}

// sourced from the VL6180X app note (their comment: “page 24”).
static bool load_settings(VL6180X_t *d)
{
    if (wr8(d, 0x0207, 0x01) != HAL_OK) return false;
    if (wr8(d, 0x0208, 0x01) != HAL_OK) return false;
    if (wr8(d, 0x0096, 0x00) != HAL_OK) return false;
    if (wr8(d, 0x0097, 0xFD) != HAL_OK) return false;
    if (wr8(d, 0x00E3, 0x00) != HAL_OK) return false;
    if (wr8(d, 0x00E4, 0x04) != HAL_OK) return false;
    if (wr8(d, 0x00E5, 0x02) != HAL_OK) return false;
    if (wr8(d, 0x00E6, 0x01) != HAL_OK) return false;
    if (wr8(d, 0x00E7, 0x03) != HAL_OK) return false;
    if (wr8(d, 0x00F5, 0x02) != HAL_OK) return false;
    if (wr8(d, 0x00D9, 0x05) != HAL_OK) return false;
    if (wr8(d, 0x00DB, 0xCE) != HAL_OK) return false;
    if (wr8(d, 0x00DC, 0x03) != HAL_OK) return false;
    if (wr8(d, 0x00DD, 0xF8) != HAL_OK) return false;
    if (wr8(d, 0x009F, 0x00) != HAL_OK) return false;
    if (wr8(d, 0x00A3, 0x3C) != HAL_OK) return false;
    if (wr8(d, 0x00B7, 0x00) != HAL_OK) return false;
    if (wr8(d, 0x00BB, 0x3C) != HAL_OK) return false;
    if (wr8(d, 0x00B2, 0x09) != HAL_OK) return false;
    if (wr8(d, 0x00CA, 0x09) != HAL_OK) return false;
    if (wr8(d, 0x0198, 0x01) != HAL_OK) return false;
    if (wr8(d, 0x01B0, 0x17) != HAL_OK) return false;
    if (wr8(d, 0x01AD, 0x00) != HAL_OK) return false;
    if (wr8(d, 0x00FF, 0x05) != HAL_OK) return false;
    if (wr8(d, 0x0100, 0x05) != HAL_OK) return false;
    if (wr8(d, 0x0199, 0x05) != HAL_OK) return false;
    if (wr8(d, 0x01A6, 0x1B) != HAL_OK) return false;
    if (wr8(d, 0x01AC, 0x3E) != HAL_OK) return false;
    if (wr8(d, 0x01A7, 0x1F) != HAL_OK) return false;
    if (wr8(d, 0x0030, 0x00) != HAL_OK) return false;

    // Public registers
    if (wr8(d, 0x0011, 0x10) != HAL_OK) return false;
    if (wr8(d, 0x010A, 0x30) != HAL_OK) return false;
    if (wr8(d, 0x003F, 0x46) != HAL_OK) return false;
    if (wr8(d, 0x0031, 0xFF) != HAL_OK) return false;
    if (wr8(d, 0x0040, 0x63) != HAL_OK) return false;
    if (wr8(d, 0x002E, 0x01) != HAL_OK) return false;
    if (wr8(d, 0x001B, 0x09) != HAL_OK) return false;
    if (wr8(d, 0x003E, 0x31) != HAL_OK) return false;
    if (wr8(d, 0x0014, 0x24) != HAL_OK) return false;

    return true;
}

bool VL6180X_Init(VL6180X_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7)
{
    dev->hi2c = hi2c;
    dev->addr7 = addr7;

    uint8_t model = 0;
    if (rd8(dev, VL6180X_REG_IDENTIFICATION_MODEL_ID, &model) != HAL_OK) return false;

    // VL6180X model id should be 0xB4 (used by Adafruit in github as detection check)
    if (model != 0xB4) return false;

    if (!load_settings(dev)) return false;

    // Mark "fresh out of reset" as 0
    if (wr8(dev, VL6180X_REG_SYSTEM_FRESH_OUT_OF_RESET, 0x00) != HAL_OK) return false;

    // Enable range history buffer
    if (wr8(dev, VL6180X_REG_SYSTEM_HISTORY_CTRL, 0x01) != HAL_OK) return false;

    return true;
}

uint8_t VL6180X_ReadRangeStatus(VL6180X_t *dev)
{
    uint8_t st = 0;
    if (rd8(dev, VL6180X_REG_RESULT_RANGE_STATUS, &st) != HAL_OK) return 0xFF;
    return (uint8_t)(st >> 4);
}

bool VL6180X_ReadRangeSingle(VL6180X_t *dev, uint8_t *range_mm, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    uint8_t v = 0;

    // Wait until “device ready” bit is set
    while (1) {
        if (rd8(dev, VL6180X_REG_RESULT_RANGE_STATUS, &v) != HAL_OK) return false;
        if (v & 0x01) break;
        if ((HAL_GetTick() - t0) > timeout_ms) return false;
    }

    // Start single-shot
    if (wr8(dev, VL6180X_REG_SYSRANGE_START, 0x01) != HAL_OK) return false;

    // Wait for “new sample ready”
    t0 = HAL_GetTick();
    while (1) {
        if (rd8(dev, VL6180X_REG_RESULT_INTERRUPT_STATUS_GPIO, &v) != HAL_OK) return false;
        if (v & 0x04) break;
        if ((HAL_GetTick() - t0) > timeout_ms) return false;
    }

    // Read range in mm
    if (rd8(dev, VL6180X_REG_RESULT_RANGE_VAL, range_mm) != HAL_OK) return false;

    // Clear interrupt
    if (wr8(dev, VL6180X_REG_SYSTEM_INTERRUPT_CLEAR, 0x07) != HAL_OK) return false;

    return true;
}
