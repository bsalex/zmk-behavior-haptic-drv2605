/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT ti_drv2605

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(emul_drv2605, CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605_LOG_LEVEL);

struct emul_drv2605_data {};

struct emul_drv2605_config {};

static int emul_drv2605_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
                                 int addr) {
    LOG_DBG("I2C Emulator Transfer: Addr 0x%02x, Num Msgs %d", addr, num_msgs);

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

static int emul_drv2605_init(const struct emul *target, const struct device *parent) {
    LOG_INF("Initializing DRV2605 Emulator");
    return 0;
}

static const struct i2c_emul_api emul_drv2605_api = {
    .transfer = emul_drv2605_transfer,
};

#define EMUL_DRV2605_DEFINE(inst)                                                                  \
    static struct emul_drv2605_data emul_drv2605_data_##inst;                                      \
    static const struct emul_drv2605_config emul_drv2605_config_##inst;                            \
    EMUL_DT_INST_DEFINE(inst, emul_drv2605_init, &emul_drv2605_data_##inst,                        \
                        &emul_drv2605_config_##inst, &emul_drv2605_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(EMUL_DRV2605_DEFINE)
