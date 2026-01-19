idf_component_register(
    SRCS "display_HAL.c"
    INCLUDE_DIRS "."
    REQUIRES ST7789 lvgl system_configuration
)