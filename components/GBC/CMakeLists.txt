file(GLOB GBC_SRCS "*.c")
file(GLOB GNUBOY_SRCS "gnuboy/*.c")

idf_component_register(
    SRCS ${GBC_SRCS} ${GNUBOY_SRCS}
    INCLUDE_DIRS "." "gnuboy"
    REQUIRES display_HAL sound sd_storage user_input
)