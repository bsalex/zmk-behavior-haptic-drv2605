/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_haptic_output_proxy

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct haptic_output_proxy_config {
    /* Kept alive solely as a structural pointer link for the Zephyr RTOS split relay */
};

static int haptic_output_proxy_init(const struct device *dev) {
    return 0;
}

#define HAPTIC_OUTPUT_PROXY_DEFINE(inst)                                                           \
    static const struct haptic_output_proxy_config config_##inst;                                  \
    DEVICE_DT_INST_DEFINE(inst, haptic_output_proxy_init, NULL, NULL, &config_##inst,              \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(HAPTIC_OUTPUT_PROXY_DEFINE)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

