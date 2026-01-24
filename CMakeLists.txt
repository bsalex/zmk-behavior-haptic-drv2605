if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_HAPTIC_DRV2605 app PRIVATE src/behaviors/behavior_haptic_drv2605.c)

  zephyr_include_directories(include)
endif()