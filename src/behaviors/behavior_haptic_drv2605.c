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

LOG_MODULE_REGISTER(zmk_behavior_haptic_drv2605, CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605_LOG_LEVEL);

struct behavior_haptic_config {
    const struct device *haptic_dev;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_haptic_config *cfg = dev->config;
    uint8_t effect_id = binding->param1;

    if (!device_is_ready(cfg->haptic_dev)) {
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

    int ret = drv2605_haptic_config(cfg->haptic_dev, DRV2605_HAPTICS_SOURCE_ROM, &config_data);
    if (ret < 0) {
        LOG_ERR("Failed to configure haptic device: %d", ret);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    ret = haptics_start_output(cfg->haptic_dev);
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

static int behavior_haptic_init(const struct device *dev) {
    const struct behavior_haptic_config *cfg = dev->config;

    if (device_is_ready(cfg->haptic_dev)) {
        LOG_DBG("Haptic device ready");
    } else {
        LOG_WRN("Haptic device not ready during init");
    }
    return 0;
}

#define BEHAVIOR_HAPTIC_DRV2605_DEFINE(inst)                                                       \
    static const struct behavior_haptic_config behavior_haptic_config_##inst = {                   \
        .haptic_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, haptic_device)),                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(inst, behavior_haptic_init, NULL, NULL,                                \
                            &behavior_haptic_config_##inst, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_haptic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEHAVIOR_HAPTIC_DRV2605_DEFINE)
