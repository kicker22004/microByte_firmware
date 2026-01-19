idf_component_register(
    SRCS "sd_storage.c"
    INCLUDE_DIRS "."
    REQUIRES fatfs sdmmc driver system_configuration
)