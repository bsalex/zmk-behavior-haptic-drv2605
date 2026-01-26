/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_haptic_drv2605

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/drv2605.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define DRV2605_REG_STATUS 0x00
#define DRV2605_REG_MODE 0x01
#define DRV2605_REG_GO 0x0C

#define DRV2605_MODE_AUTO_CAL 0x07
#define DRV2605_GO_BIT 0x01
#define DRV2605_DIAG_RESULT 0x08

LOG_MODULE_REGISTER(zmk_behavior_haptic_drv2605, CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605_LOG_LEVEL);

struct behavior_haptic_data {
    const struct device *haptic_dev;
};

struct behavior_haptic_config {
    int driver_index;
};

struct haptic_drv_info {
    const struct device *dev;
    struct i2c_dt_spec i2c;
};

#define EXTRACT_INFO(node_id) {.dev = DEVICE_DT_GET(node_id), .i2c = I2C_DT_SPEC_GET(node_id)},

/* Capture all drv2605 instances found in DTS */
static const struct haptic_drv_info haptics_available[] = {
    DT_FOREACH_STATUS_OKAY(ti_drv2605, EXTRACT_INFO)};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_haptic_data *data = (struct behavior_haptic_data *)dev->data;
    uint8_t effect_id = binding->param1;

    if (!data->haptic_dev || !device_is_ready(data->haptic_dev)) {
        LOG_WRN("Haptic device not ready");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    struct drv2605_rom_data rom_data = {
        .library = DRV2605_LIBRARY_LRA,
        .trigger = DRV2605_MODE_INTERNAL_TRIGGER,
        .seq_regs = {effect_id, 0}, // Play effect_id then stop
        .overdrive_time = 0,
        .sustain_pos_time = 0,
        .sustain_neg_time = 0,
        .brake_time = 0,
    };

    union drv2605_config_data config_data = {
        .rom_data = &rom_data,
    };

    int ret = drv2605_haptic_config(data->haptic_dev, DRV2605_HAPTICS_SOURCE_ROM, &config_data);
    if (ret < 0) {
        LOG_ERR("Failed to configure haptic device: %d", ret);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    ret = haptics_start_output(data->haptic_dev);
    if (ret < 0) {
        LOG_ERR("Failed to start haptic output: %d", ret);
    }

    LOG_DBG("Played effect %d", effect_id);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_haptic_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static int behavior_haptic_auto_cal(const struct i2c_dt_spec *i2c) {
    int ret;
    uint8_t val;
    int retries = 100;

    if (!i2c_is_ready_dt(i2c)) {
        LOG_WRN("I2C bus not ready for calibration");
        return -ENODEV;
    }

    LOG_DBG("Starting Auto-Calibration (Bypassing Driver)");

    /* Set Mode to Auto-Calibration */
    ret = i2c_reg_update_byte_dt(i2c, DRV2605_REG_MODE, 0x07, DRV2605_MODE_AUTO_CAL);
    if (ret < 0)
        return ret;

    /* Set GO bit */
    ret = i2c_reg_update_byte_dt(i2c, DRV2605_REG_GO, DRV2605_GO_BIT, DRV2605_GO_BIT);
    if (ret < 0)
        return ret;

    /* Poll GO bit */
    while (retries > 0) {
        k_msleep(10);
        ret = i2c_reg_read_byte_dt(i2c, DRV2605_REG_GO, &val);
        if (ret < 0)
            return ret;
        if ((val & DRV2605_GO_BIT) == 0)
            break;
        retries--;
    }

    if (retries == 0) {
        LOG_ERR("Auto-calibration timed out");
        return -ETIMEDOUT;
    }

    /* Check Result */
    ret = i2c_reg_read_byte_dt(i2c, DRV2605_REG_STATUS, &val);
    if (ret < 0)
        return ret;
    if (val & DRV2605_DIAG_RESULT) {
        LOG_WRN("Auto-calibration failed (Diag Result 0x%02x)", val);
        /* Continue anyway? */
    } else {
        LOG_DBG("Auto-Calibration Success");
    }

    /* Driver (at prio 35) or system expects Internal Trigger mode?
       The driver init sets it to Internal Trigger. We probably overwrote it.
       Restore it. */
    return i2c_reg_update_byte_dt(i2c, DRV2605_REG_MODE, 0x07, 0x00); /* 0x00 = Internal Trigger */
}

static int behavior_haptic_init(const struct device *dev) {
    struct behavior_haptic_data *data = (struct behavior_haptic_data *)dev->data;
    const struct behavior_haptic_config *cfg = dev->config;

    if (ARRAY_SIZE(haptics_available) == 0) {
        LOG_WRN("No DRV2605 devices found in DTS");
        return 0;
    }

    /* Use the configured driver index */
    int idx = cfg->driver_index;
    if (idx < 0 || idx >= ARRAY_SIZE(haptics_available)) {
        LOG_ERR("Driver index %d out of range (found %zu devices)", idx,
                ARRAY_SIZE(haptics_available));
        return -ENODEV;
    }

    const struct haptic_drv_info *info = &haptics_available[idx];
    data->haptic_dev = info->dev;

    if (device_is_ready(data->haptic_dev)) {
        LOG_DBG("Haptic device ready (Index %d)", idx);
    } else {
        LOG_WRN("Haptic device (Index %d) not ready during init", idx);
    }
    return 0;
}

#define BEHAVIOR_HAPTIC_DRV2605_DEFINE(inst)                                                       \
    static struct behavior_haptic_data behavior_haptic_data_##inst = {                             \
        .haptic_dev = NULL,                                                                        \
    };                                                                                             \
    static struct behavior_haptic_config behavior_haptic_config_##inst = {                         \
        .driver_index = DT_INST_PROP_OR(inst, driver_index, 0),                                    \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(inst, behavior_haptic_init, NULL, &behavior_haptic_data_##inst,        \
                            &behavior_haptic_config_##inst, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_haptic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_HAPTIC_DRV2605_DEFINE)

static void calibrate_inst(const struct device *dev) {
    const struct behavior_haptic_config *cfg = dev->config;
    struct behavior_haptic_data *data = (struct behavior_haptic_data *)dev->data;

    /* Re-check/acquire device if init failed to get it?
       Actually init does selection. Data->haptic_dev should be set. */

    if (data->haptic_dev && device_is_ready(data->haptic_dev)) {
        LOG_DBG("Late Calibration: Haptic device ready (Index %d)", cfg->driver_index);

        /* We need the i2c spec. It's in haptics_available using index */
        int idx = cfg->driver_index;
        if (idx >= 0 && idx < ARRAY_SIZE(haptics_available)) {
            behavior_haptic_auto_cal(&haptics_available[idx].i2c);
        }
    }
}

static int behavior_haptic_late_init(void) {
/* Iterate all behavior instances and run calibration */
#define CALIBRATE_INST(inst) calibrate_inst(DEVICE_DT_INST_GET(inst));
    DT_INST_FOREACH_STATUS_OKAY(CALIBRATE_INST)
    return 0;
}

SYS_INIT(behavior_haptic_late_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
