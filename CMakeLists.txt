target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605 app PRIVATE src/behaviors/behavior_haptic_drv2605.c src/output_haptic_feedback.c src/haptic_output_proxy.c)
target_sources_ifdef(CONFIG_EMUL app PRIVATE src/emul/emul_drv2605.c)
