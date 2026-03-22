/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_output_haptic

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/drv2605.h>
#include <zephyr/logging/log.h>

typedef int (*output_set_value_t)(const struct device *dev, uint8_t value);
typedef int (*output_get_ready_t)(const struct device *dev);

struct output_generic_api {
    output_set_value_t set_value;
    output_get_ready_t get_ready;
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

LOG_MODULE_REGISTER(zmk_output_haptic_feedback, CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605_LOG_LEVEL);

struct output_haptic_feedback_config {
    const struct device *haptic_dev;
};

static int output_haptic_feedback_set_value(const struct device *dev, uint8_t value) {
    const struct output_haptic_feedback_config *config = dev->config;

    LOG_DBG("Trigger haptic feedback val %d", value);

    if (!config->haptic_dev || !device_is_ready(config->haptic_dev)) {
        LOG_WRN("Haptic device not ready");
        return -ENODEV;
    }

    struct drv2605_rom_data rom_data = {
        .library = DRV2605_LIBRARY_LRA,
        .trigger = DRV2605_MODE_INTERNAL_TRIGGER,
        .seq_regs = {value, 0}, // Play effect_id then stop
        .overdrive_time = 0,
        .sustain_pos_time = 0,
        .sustain_neg_time = 0,
        .brake_time = 0,
    };

    union drv2605_config_data config_data = {
        .rom_data = &rom_data,
    };

    int ret = drv2605_haptic_config(config->haptic_dev, DRV2605_HAPTICS_SOURCE_ROM, &config_data);
    if (ret < 0) {
        LOG_ERR("Failed to configure haptic device: %d", ret);
        return ret;
    }

    ret = haptics_start_output(config->haptic_dev);
    if (ret < 0) {
        LOG_ERR("Failed to start haptic output: %d", ret);
    }

    return ret;
}

static int output_haptic_feedback_get_ready(const struct device *dev) {
    const struct output_haptic_feedback_config *config = dev->config;
    if (!config->haptic_dev || !device_is_ready(config->haptic_dev)) {
        return -ENODEV;
    }
    return 0;
}

static const struct output_generic_api output_haptic_feedback_api = {
    .set_value = output_haptic_feedback_set_value,
    .get_ready = output_haptic_feedback_get_ready,
};

static int output_haptic_feedback_init(const struct device *dev) {
    const struct output_haptic_feedback_config *config = dev->config;
    if (!config->haptic_dev) {
        LOG_ERR("No haptic device assigned");
        return -ENODEV;
    }
    if (!device_is_ready(config->haptic_dev)) {
        LOG_WRN("Haptic device not ready at boot, will check at runtime");
    }
    return 0;
}

#define OUTPUT_HAPTIC_FEEDBACK_DEFINE(inst)                                                        \
    static const struct output_haptic_feedback_config config_##inst = {                            \
        .haptic_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, device)),                                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(inst, output_haptic_feedback_init, NULL, NULL, &config_##inst,           \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,                           \
                          &output_haptic_feedback_api);

DT_INST_FOREACH_STATUS_OKAY(OUTPUT_HAPTIC_FEEDBACK_DEFINE)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
