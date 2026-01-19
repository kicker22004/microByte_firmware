file(GLOB SMS_SRCS "*.c")
file(GLOB SMSPLUS_SRCS "smsplus/*.c")
file(GLOB SMSPLUS_CPU_SRCS "smsplus/cpu/*.c")
# Don't include unzip - it requires full zlib
# file(GLOB SMSPLUS_UNZIP_SRCS "smsplus/unzip/*.c")

idf_component_register(
    SRCS ${SMS_SRCS} ${SMSPLUS_SRCS} ${SMSPLUS_CPU_SRCS}
    INCLUDE_DIRS "." "smsplus" "smsplus/cpu" "smsplus/unzip"
    REQUIRES display_HAL sound user_input
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LSB_FIRST=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=unused-const-variable)