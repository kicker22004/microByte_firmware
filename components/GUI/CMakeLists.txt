idf_component_register(
    SRCS "GUI.c" "GUI_frontend.c"
    INCLUDE_DIRS "." "icons"
    REQUIRES lvgl display_HAL sd_storage battery user_input backlight_ctrl system_configuration GBC NES SMS OTA
)