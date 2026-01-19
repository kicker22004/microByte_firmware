idf_component_register(
    SRCS "external_app.c" "update_firmware.c"
    INCLUDE_DIRS "."
    REQUIRES app_update spi_flash
)