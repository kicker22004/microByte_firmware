file(GLOB NES_SRCS "*.c")
file(GLOB NOFRENDO_SRCS "nofrendo/*.c")
file(GLOB NOFRENDO_CPU_SRCS "nofrendo/cpu/*.c")
file(GLOB NOFRENDO_LIBSNSS_SRCS "nofrendo/libsnss/*.c")
file(GLOB NOFRENDO_MAPPERS_SRCS "nofrendo/mappers/*.c")
file(GLOB NOFRENDO_NES_SRCS "nofrendo/nes/*.c")
file(GLOB NOFRENDO_SNDHRDW_SRCS "nofrendo/sndhrdw/*.c")

idf_component_register(
    SRCS ${NES_SRCS} ${NOFRENDO_SRCS} ${NOFRENDO_CPU_SRCS} ${NOFRENDO_LIBSNSS_SRCS} ${NOFRENDO_MAPPERS_SRCS} ${NOFRENDO_NES_SRCS} ${NOFRENDO_SNDHRDW_SRCS}
    INCLUDE_DIRS "." "nofrendo" "nofrendo/cpu" "nofrendo/libsnss" "nofrendo/mappers" "nofrendo/nes" "nofrendo/sndhrdw"
    REQUIRES display_HAL sound user_input
)