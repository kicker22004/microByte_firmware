idf_component_register(
    SRCS "system_manager.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_system spi_flash app_update
)