/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_i2c_mock

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_mock, CONFIG_I2C_LOG_LEVEL);

struct i2c_mock_config {};

struct i2c_mock_data {};

static int i2c_mock_configure(const struct device *dev, uint32_t dev_config) { return 0; }

static int i2c_mock_get_config(const struct device *dev, uint32_t *dev_config) {
    *dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);
    return 0;
}

static int i2c_mock_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
                             uint16_t addr) {
    LOG_DBG("I2C Mock Transfer: Addr 0x%02x, Num Msgs %d", addr, num_msgs);

    /* Emulate valid Device ID for DRV2605 (Reg 0x00) */
    if (num_msgs == 2 && (msgs[0].flags & I2C_MSG_READ) == 0 && (msgs[1].flags & I2C_MSG_READ)) {
        uint8_t reg = msgs[0].buf[0];
        if (reg == 0x00) {
            /* DRV2605_REG_STATUS: Device ID 3 (shifted 5) = 0x60 */
            msgs[1].buf[0] = 0x60;
        } else {
            /* Return 0 for others (safe for Reset polling) */
            msgs[1].buf[0] = 0x00;
        }
    }

    return 0;
}

static const struct i2c_driver_api i2c_mock_driver_api = {
    .configure = i2c_mock_configure,
    .get_config = i2c_mock_get_config,
    .transfer = i2c_mock_transfer,
};

static int i2c_mock_init(const struct device *dev) {
    LOG_INF("Initializing I2C Mock");
    return 0;
}

#define I2C_MOCK_DEFINE(inst)                                                                      \
    static const struct i2c_mock_config i2c_mock_config_##inst = {};                               \
    static struct i2c_mock_data i2c_mock_data_##inst = {};                                         \
    DEVICE_DT_INST_DEFINE(inst, i2c_mock_init, NULL, &i2c_mock_data_##inst,                        \
                          &i2c_mock_config_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,          \
                          &i2c_mock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_MOCK_DEFINE)
